// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_platform.h>

#ifdef XNVME_PLATFORM_LINUX_ENABLED
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define PCI_DEVICES "/sys/bus/pci/devices"
#define DEVS_MAX 64

struct ctrlr {
	char bdf[32];
	char driver[64];
	char why[448];
	int iommu_group; ///< -1 when it has none
	bool userspace;
	bool held; ///< Refuses an attach, or for a userspace driver, a detach
	bool unchecked;
};

static bool
path_exists(const char *path)
{
	struct stat st;

	return !stat(path, &st);
}

static int
dir_count(const char *path)
{
	DIR *dir = opendir(path);
	struct dirent *ent;
	int count = 0;

	if (!dir) {
		return -1;
	}
	while ((ent = readdir(dir))) {
		count += ent->d_name[0] != '.';
	}
	closedir(dir);

	return count;
}

static bool
file_has(const char *path, const char *needle)
{
	char line[256];
	FILE *fp = fopen(path, "r");
	bool found = false;

	while (fp && !found && fgets(line, sizeof(line), fp)) {
		found = strstr(line, needle) != NULL;
	}
	if (fp) {
		fclose(fp);
	}

	return found;
}

static int
read_line(const char *path, char *buf, size_t len)
{
	FILE *fp = fopen(path, "r");

	buf[0] = '\0';
	if (!fp) {
		return -errno;
	}
	if (!fgets(buf, len, fp)) {
		buf[0] = '\0';
	}
	fclose(fp);
	buf[strcspn(buf, "\n")] = '\0';

	return 0;
}

/**
 * systemd and udev put the names of their workers in parentheses
 */
static void
read_comm(const char *pid, char *comm, size_t len)
{
	char path[128];
	size_t n;

	snprintf(path, sizeof(path), "/proc/%s/comm", pid);
	read_line(path, comm, len);
	n = strlen(comm);
	if (n > 2 && comm[0] == '(' && comm[n - 1] == ')') {
		memmove(comm, comm + 1, n - 2);
		comm[n - 2] = '\0';
	}
}

static void
link_base(const char *path, char *buf, size_t len)
{
	char link[512];
	ssize_t nbytes = readlink(path, link, sizeof(link) - 1);
	const char *base;

	buf[0] = '\0';
	if (nbytes <= 0) {
		return;
	}
	link[nbytes] = '\0';
	base = strrchr(link, '/');
	snprintf(buf, len, "%s", base ? base + 1 : link);
}

static int
sysfs_write(const char *path, const char *val)
{
	int fd = open(path, O_WRONLY);
	ssize_t nbytes;
	int err;

	if (fd < 0) {
		return -errno;
	}
	nbytes = write(fd, val, strlen(val));
	err = nbytes < 0 ? -errno : 0;
	close(fd);

	return err;
}

static void
modprobe(const char *driver)
{
	pid_t pid = fork();
	int status;

	if (pid < 0) {
		return;
	}
	if (!pid) {
		int null = open("/dev/null", O_WRONLY);

		if (null >= 0) {
			dup2(null, STDOUT_FILENO);
			dup2(null, STDERR_FILENO);
		}
		execlp("modprobe", "modprobe", driver, (char *)NULL);
		execl("/sbin/modprobe", "modprobe", driver, (char *)NULL);
		_exit(127);
	}
	waitpid(pid, &status, 0);
}

static bool
is_userspace(const char *driver)
{
	return !strcmp(driver, "vfio-pci") || !strcmp(driver, "uio_pci_generic");
}

static bool
is_nsdev(const char *name)
{
	const char *c = name;
	int ns = 0;

	if (!strncmp(c, "nvme", 4)) {
		c += 4;
	} else if (!strncmp(c, "ng", 2)) {
		c += 2;
	} else {
		return false;
	}
	if (!*c) {
		return false;
	}
	for (; *c; ++c) {
		if (*c == 'n') {
			ns += 1;
		} else if (*c != 'c' && (*c < '0' || *c > '9')) {
			return false;
		}
	}

	return ns == 1 && name[strlen(name) - 1] != 'n';
}

static void
devs_add(char devs[][64], int *ndevs, const char *name)
{
	for (int i = 0; i < *ndevs; ++i) {
		if (!strcmp(devs[i], name)) {
			return;
		}
	}
	if (*ndevs < DEVS_MAX) {
		snprintf(devs[*ndevs], 64, "%s", name);
		*ndevs += 1;
	}
}

/**
 * Block devices come first, 'nblock' of them
 */
static int
ctrlr_devs(const char *bdf, char devs[][64], int *nblock)
{
	char chars[DEVS_MAX][64];
	int nchars = 0;
	int ndevs = 0;
	char path[512];
	struct dirent *ent;
	DIR *dir;

	snprintf(path, sizeof(path), PCI_DEVICES "/%s/nvme", bdf);
	dir = opendir(path);
	while (dir && (ent = readdir(dir))) {
		struct dirent *nent;
		DIR *ndir;

		if (strncmp(ent->d_name, "nvme", 4)) {
			continue;
		}
		devs_add(chars, &nchars, ent->d_name);
		snprintf(path, sizeof(path), PCI_DEVICES "/%s/nvme/%s", bdf, ent->d_name);
		ndir = opendir(path);
		while (ndir && (nent = readdir(ndir))) {
			const char *name = nent->d_name;

			if (!is_nsdev(name)) {
				continue;
			}
			if (!strncmp(name, "ng", 2)) {
				devs_add(chars, &nchars, name);
			} else if (strchr(name + 4, 'c')) {
				const char *c = strchr(name + 4, 'c');
				const char *n = c + 1 + strspn(c + 1, "0123456789");
				struct dirent *bent;
				DIR *bdir = opendir("/sys/block");
				char head[64];

				// /dev has only the head nvme<S>n<N> of path nvme<S>c<C>n<N>;
				// the head's multipath/ links exist from Linux 6.15
				snprintf(head, sizeof(head), "%.*s%.32s", (int)(c - name), name,
					 n);
				snprintf(path, sizeof(path), "/sys/block/%s", head);
				if (path_exists(path)) {
					devs_add(devs, &ndevs, head);
				}
				while (bdir && (bent = readdir(bdir))) {
					snprintf(path, sizeof(path), "/sys/block/%s/multipath/%s",
						 bent->d_name, name);
					if (path_exists(path)) {
						devs_add(devs, &ndevs, bent->d_name);
					}
				}
				if (bdir) {
					closedir(bdir);
				}
			} else {
				devs_add(devs, &ndevs, name);
			}
		}
		if (ndir) {
			closedir(ndir);
		}
	}
	if (dir) {
		closedir(dir);
	}

	for (int i = 0, nheads = ndevs; i < nheads; ++i) {
		size_t len = strlen(devs[i]);

		snprintf(path, sizeof(path), "/sys/block/%s", devs[i]);
		dir = opendir(path);
		while (dir && (ent = readdir(dir))) {
			if (!strncmp(ent->d_name, devs[i], len) && ent->d_name[len] == 'p') {
				devs_add(devs, &ndevs, ent->d_name);
			}
		}
		if (dir) {
			closedir(dir);
		}
	}

	*nblock = ndevs;
	for (int i = 0; i < nchars; ++i) {
		devs_add(devs, &ndevs, chars[i]);
	}

	return ndevs;
}

static bool
blockdev_in_use(const char *dev, char *why, size_t len)
{
	char devno[32];
	char path[512];
	char line[4096];
	FILE *fp;
	bool found = false;

	snprintf(path, sizeof(path), "/sys/class/block/%s/dev", dev);
	if (read_line(path, devno, sizeof(devno)) || !devno[0]) {
		return false;
	}

	fp = fopen("/proc/self/mountinfo", "r");
	while (fp && !found && fgets(line, sizeof(line), fp)) {
		char majmin[32], root[2048], mnt[2048];

		if (sscanf(line, "%*s %*s %31s %2047s %2047s", majmin, root, mnt) == 3 &&
		    !strcmp(majmin, devno)) {
			snprintf(why, len, "/dev/%s mounted on %s", dev, mnt);
			found = true;
		}
	}
	if (fp) {
		fclose(fp);
	}
	if (found) {
		return true;
	}

	snprintf(path, sizeof(path), "/sys/class/block/%s/holders", dev);
	if (dir_count(path) > 0) {
		snprintf(why, len, "/dev/%s has holders (device-mapper, md or LVM)", dev);
		return true;
	}

	fp = fopen("/proc/swaps", "r");
	snprintf(path, sizeof(path), "/dev/%s", dev);
	while (fp && !found && fgets(line, sizeof(line), fp)) {
		char swap[512];

		if (sscanf(line, "%511s", swap) == 1 && !strcmp(swap, path)) {
			snprintf(why, len, "/dev/%s is used as swap", dev);
			found = true;
		}
	}
	if (fp) {
		fclose(fp);
	}

	return found;
}

/**
 * @return 1 when found, 0 when not, -1 when another user's processes could not be read
 */
static int
proc_holding(char paths[][80], int npaths, char *why, size_t len)
{
	DIR *proc = opendir("/proc");
	struct dirent *pent;
	bool unreadable = false;
	int found = 0;

	while (proc && !found && (pent = readdir(proc))) {
		char path[512];
		struct dirent *fent;
		DIR *fds;

		if (pent->d_name[0] < '0' || pent->d_name[0] > '9') {
			continue;
		}
		snprintf(path, sizeof(path), "/proc/%s/fd", pent->d_name);
		fds = opendir(path);
		if (!fds) {
			unreadable |= errno == EACCES;
			continue;
		}
		while (!found && (fent = readdir(fds))) {
			char link[512];
			ssize_t nbytes;

			snprintf(path, sizeof(path), "/proc/%s/fd/%s", pent->d_name, fent->d_name);
			nbytes = readlink(path, link, sizeof(link) - 1);
			if (nbytes <= 0) {
				continue;
			}
			link[nbytes] = '\0';
			for (int i = 0; i < npaths && !found; ++i) {
				char comm[64];

				if (strcmp(link, paths[i])) {
					continue;
				}
				read_comm(pent->d_name, comm, sizeof(comm));
				snprintf(why, len, "%s open by %s (pid %s)", paths[i], comm,
					 pent->d_name);
				found = 1;
			}
		}
		closedir(fds);
	}
	if (proc) {
		closedir(proc);
	}

	return found ? 1 : (unreadable ? -1 : 0);
}

/**
 * Userspace drivers on uio_pci_generic map the BARs through sysfs rather than open /dev/uioN
 *
 * @return 1 when found, 0 when not, -1 when another user's processes could not be read
 */
static int
proc_mapping(const char *bdf, char *why, size_t len)
{
	DIR *proc = opendir("/proc");
	struct dirent *pent;
	char needle[64];
	bool unreadable = false;
	int found = 0;

	snprintf(needle, sizeof(needle), "/%s/resource", bdf);
	while (proc && !found && (pent = readdir(proc))) {
		char path[512];
		char line[1024];
		FILE *fp;

		if (pent->d_name[0] < '0' || pent->d_name[0] > '9') {
			continue;
		}
		snprintf(path, sizeof(path), "/proc/%s/maps", pent->d_name);
		fp = fopen(path, "r");
		if (!fp) {
			unreadable |= errno == EACCES;
			continue;
		}
		while (!found && fgets(line, sizeof(line), fp)) {
			char comm[64];

			if (!strstr(line, needle)) {
				continue;
			}
			read_comm(pent->d_name, comm, sizeof(comm));
			snprintf(why, len, "BAR mapped by %s (pid %s)", comm, pent->d_name);
			found = 1;
		}
		fclose(fp);
	}
	if (proc) {
		closedir(proc);
	}

	return found ? 1 : (unreadable ? -1 : 0);
}

static int
userspace_nodes(const struct ctrlr *c, char paths[][80])
{
	const char *subdirs[] = {"vfio-dev", "uio"};
	const char *prefixes[] = {"/dev/vfio/devices/", "/dev/"};
	char path[256];
	int n = 0;

	if (c->iommu_group >= 0) {
		snprintf(paths[n++], 80, "/dev/vfio/%d", c->iommu_group);
		snprintf(paths[n++], 80, "/dev/vfio/noiommu-%d", c->iommu_group);
	}
	for (int s = 0; s < 2; ++s) {
		struct dirent *ent;
		DIR *dir;

		snprintf(path, sizeof(path), PCI_DEVICES "/%s/%s", c->bdf, subdirs[s]);
		dir = opendir(path);
		while (dir && (ent = readdir(dir))) {
			if (ent->d_name[0] != '.' && n < 4) {
				snprintf(paths[n++], 80, "%s%s", prefixes[s], ent->d_name);
			}
		}
		if (dir) {
			closedir(dir);
		}
	}

	return n;
}

static void
ctrlr_check(struct ctrlr *c)
{
	char devs[DEVS_MAX][64];
	char paths[DEVS_MAX][80];
	int nblock, ndevs, held;

	if (!strcmp(c->driver, "nvme")) {
		ndevs = ctrlr_devs(c->bdf, devs, &nblock);
		for (int i = 0; i < nblock && !c->held; ++i) {
			c->held = blockdev_in_use(devs[i], c->why, sizeof(c->why));
		}
		for (int i = 0; i < ndevs; ++i) {
			snprintf(paths[i], 80, "/dev/%s", devs[i]);
		}
		if (!c->held) {
			held = proc_holding(paths, ndevs, c->why, sizeof(c->why));
			c->held = held == 1;
			c->unchecked = held == -1;
		}
		return;
	}
	if (c->userspace) {
		ndevs = userspace_nodes(c, paths);
		held = proc_holding(paths, ndevs, c->why, sizeof(c->why));
		if (held != 1) {
			int mapped = proc_mapping(c->bdf, c->why, sizeof(c->why));

			held = mapped == 1 ? 1 : (held == -1 || mapped == -1 ? -1 : 0);
		}
		c->held = held == 1;
		c->unchecked = held == -1;
	}
}

const char *
xnvme_driver_linux_default(void)
{
	char noiommu[8] = {0};

	read_line("/sys/module/vfio/parameters/enable_unsafe_noiommu_mode", noiommu,
		  sizeof(noiommu));

	return dir_count("/sys/kernel/iommu_groups") > 0 || noiommu[0] == 'Y' ? "vfio-pci"
									      : "uio_pci_generic";
}

static bool
group_ok(const struct ctrlr *c, const char *const *bdfs, int nbdfs, char *why, size_t len)
{
	char path[256];
	struct dirent *ent;
	DIR *dir;
	bool ok = true;

	if (c->iommu_group < 0) {
		return true;
	}
	snprintf(path, sizeof(path), "/sys/kernel/iommu_groups/%d/devices", c->iommu_group);
	dir = opendir(path);
	while (dir && ok && (ent = readdir(dir))) {
		char driver[64];
		bool along = false;

		if (ent->d_name[0] == '.' || !strcmp(ent->d_name, c->bdf)) {
			continue;
		}
		for (int i = 0; i < nbdfs; ++i) {
			along |= !strcmp(bdfs[i], ent->d_name);
		}
		snprintf(path, sizeof(path), PCI_DEVICES "/%s/driver", ent->d_name);
		link_base(path, driver, sizeof(driver));
		snprintf(path, sizeof(path), PCI_DEVICES "/%s/class", ent->d_name);
		along |= !driver[0] || !strcmp(driver, "vfio-pci") ||
			 !strcmp(driver, "pcieport") || file_has(path, "0x0604");
		if (!along) {
			snprintf(why, len,
				 "shares IOMMU group %d with %s on %s; vfio-pci needs the whole "
				 "group",
				 c->iommu_group, ent->d_name, driver);
			ok = false;
		}
	}
	if (dir) {
		closedir(dir);
	}

	return ok;
}

static int
rebind(const char *bdf, const char *driver)
{
	char path[256];
	char bound[64];
	int err;

	snprintf(path, sizeof(path), PCI_DEVICES "/%s/driver_override", bdf);
	err = sysfs_write(path, driver[0] ? driver : "\n");
	if (err) {
		return err;
	}
	snprintf(path, sizeof(path), PCI_DEVICES "/%s/driver", bdf);
	if (path_exists(path)) {
		snprintf(path, sizeof(path), PCI_DEVICES "/%s/driver/unbind", bdf);
		err = sysfs_write(path, bdf);
		if (err) {
			return err;
		}
	}
	if (driver[0]) {
		err = sysfs_write("/sys/bus/pci/drivers_probe", bdf);
		if (err) {
			return err;
		}
	}
	snprintf(path, sizeof(path), PCI_DEVICES "/%s/driver", bdf);
	link_base(path, bound, sizeof(bound));

	return strcmp(bound, driver) ? -ENODEV : 0;
}

static void
restore(const struct ctrlr *c)
{
	char path[256];

	rebind(c->bdf, c->driver);
	snprintf(path, sizeof(path), PCI_DEVICES "/%s/driver_override", c->bdf);
	sysfs_write(path, "\n");
}

static int
ctrlr_get(const char *bdf, struct ctrlr *c)
{
	char path[512];
	char group[32];

	memset(c, 0, sizeof(*c));
	snprintf(path, sizeof(path), PCI_DEVICES "/%s/class", bdf);
	if (strchr(bdf, '/') || !file_has(path, "0x010802")) {
		return -ENODEV;
	}
	snprintf(c->bdf, sizeof(c->bdf), "%s", bdf);
	snprintf(path, sizeof(path), PCI_DEVICES "/%s/driver", c->bdf);
	link_base(path, c->driver, sizeof(c->driver));
	snprintf(path, sizeof(path), PCI_DEVICES "/%s/iommu_group", c->bdf);
	link_base(path, group, sizeof(group));
	c->iommu_group = group[0] ? atoi(group) : -1;
	c->userspace = is_userspace(c->driver);
	ctrlr_check(c);

	return 0;
}

static void
lookup(const char *const *bdfs, int nbdfs, struct ctrlr *ctrlrs, struct ctrlr **found,
       struct xnvme_driver_result *results)
{
	for (int i = 0; i < nbdfs; ++i) {
		memset(&results[i], 0, sizeof(results[i]));
		found[i] = ctrlr_get(bdfs[i], &ctrlrs[i]) ? NULL : &ctrlrs[i];
		if (!found[i]) {
			results[i].err = -ENODEV;
			snprintf(results[i].why, sizeof(results[i].why),
				 "not an NVMe controller function");
		}
	}
}

int
xnvme_driver_linux_attach(const char *const *bdfs, int nbdfs, const char *driver, int flags,
			  struct xnvme_driver_result *results)
{
	struct ctrlr **found;
	struct ctrlr *ctrlrs;
	const char **going;
	int nfailed = 0;

	found = calloc(nbdfs ? nbdfs : 1, sizeof(*found));
	ctrlrs = calloc(nbdfs ? nbdfs : 1, sizeof(*ctrlrs));
	going = calloc(nbdfs ? nbdfs : 1, sizeof(*going));
	if (!found || !ctrlrs || !going) {
		free(found);
		free(ctrlrs);
		free(going);
		return -ENOMEM;
	}
	lookup(bdfs, nbdfs, ctrlrs, found, results);
	driver = driver ? driver : xnvme_driver_linux_default();

	for (int i = 0; i < nbdfs; ++i) {
		struct ctrlr *c = found[i];
		struct xnvme_driver_result *r = &results[i];
		char devs[DEVS_MAX][64];
		char path[256];
		const char *sig = NULL;
		int nblock;

		if (!c) {
			continue;
		}
		if (!strcmp(c->driver, driver)) {
			r->err = -EALREADY;
			snprintf(r->why, sizeof(r->why), "already on %s", driver);
		} else if (c->userspace) {
			r->err = -EISCONN;
			snprintf(r->why, sizeof(r->why), "on %s; detach it first", c->driver);
		} else if (c->held) {
			r->err = -EBUSY;
			snprintf(r->why, sizeof(r->why), "in use: %s", c->why);
		} else if (c->unchecked) {
			r->err = -EACCES;
			snprintf(r->why, sizeof(r->why), "open files could not be checked");
		}
		if (r->err) {
			continue;
		}

		ctrlr_devs(c->bdf, devs, &nblock);
		for (int d = 0; d < nblock && !sig && !(flags & XNVME_DRIVER_FORCE); ++d) {
			if (!strchr(devs[d] + 4, 'p')) {
				snprintf(path, sizeof(path), "/dev/%s", devs[d]);
				sig = xnvme_driver_signature(path);
			}
			if (sig) {
				r->err = -EEXIST;
				snprintf(r->why, sizeof(r->why), "%s carries %s", path, sig);
			}
		}
	}

	// vfio-pci takes a group whole, and a refusal can strand a sibling: until none changes
	for (bool changed = !strcmp(driver, "vfio-pci"); changed;) {
		int ngoing = 0;

		changed = false;
		for (int i = 0; i < nbdfs; ++i) {
			if (found[i] && !results[i].err) {
				going[ngoing++] = bdfs[i];
			}
		}
		for (int i = 0; i < nbdfs; ++i) {
			if (found[i] && !results[i].err &&
			    !group_ok(found[i], going, ngoing, results[i].why,
				      sizeof(results[i].why))) {
				results[i].err = -EXDEV;
				changed = true;
			}
		}
	}

	for (int i = 0; i < nbdfs; ++i) {
		struct ctrlr *c = found[i];
		struct xnvme_driver_result *r = &results[i];
		char path[256];

		if (!c || r->err || (flags & XNVME_DRIVER_DRY_RUN)) {
			continue;
		}
		snprintf(path, sizeof(path), "/sys/bus/pci/drivers/%s", driver);
		if (!path_exists(path)) {
			modprobe(driver);
		}
		if (!path_exists(path)) {
			r->err = -ENOENT;
			snprintf(r->why, sizeof(r->why),
				 "no driver %s; 'modprobe %s' did not load it", driver, driver);
		} else {
			r->err = rebind(c->bdf, driver);
		}
		if (!r->err) {
			continue;
		}
		if (r->err != -ENOENT) {
			snprintf(r->why, sizeof(r->why), "binding %s failed: %s", driver,
				 strerror(-r->err));
			restore(c);
		}

		for (int j = 0; c->iommu_group >= 0 && j < nbdfs; ++j) {
			if (j == i || !found[j] || results[j].err ||
			    found[j]->iommu_group != c->iommu_group) {
				continue;
			}
			if (j < i) {
				restore(found[j]);
			}
			results[j].err = -ECANCELED;
			snprintf(results[j].why, sizeof(results[j].why),
				 "left on %s: %s in its IOMMU group failed to bind",
				 found[j]->driver, c->bdf);
		}
	}
	for (int i = 0; i < nbdfs; ++i) {
		nfailed += results[i].err && results[i].err != -EALREADY;
	}
	free(found);
	free(ctrlrs);
	free(going);

	return nfailed;
}

int
xnvme_driver_linux_detach(const char *const *bdfs, int nbdfs, const char *driver, int flags,
			  struct xnvme_driver_result *results)
{
	struct ctrlr **found;
	struct ctrlr *ctrlrs;
	int nfailed = 0;

	found = calloc(nbdfs ? nbdfs : 1, sizeof(*found));
	ctrlrs = calloc(nbdfs ? nbdfs : 1, sizeof(*ctrlrs));
	if (!found || !ctrlrs) {
		free(found);
		free(ctrlrs);
		return -ENOMEM;
	}
	lookup(bdfs, nbdfs, ctrlrs, found, results);
	driver = driver ? driver : "nvme";

	for (int i = 0; i < nbdfs; ++i) {
		struct ctrlr *c = found[i];
		struct xnvme_driver_result *r = &results[i];
		char path[256];

		if (!c) {
			nfailed += 1;
			continue;
		}
		if (!c->userspace) {
			r->err = -EALREADY;
			snprintf(r->why, sizeof(r->why), "not attached, on %s",
				 c->driver[0] ? c->driver : "-");
		} else if (c->held) {
			r->err = -EBUSY;
			snprintf(r->why, sizeof(r->why), "held: %s", c->why);
		} else if (c->unchecked) {
			r->err = -EACCES;
			snprintf(r->why, sizeof(r->why), "open files could not be checked");
		}
		if (r->err || (flags & XNVME_DRIVER_DRY_RUN)) {
			nfailed += r->err && r->err != -EALREADY;
			continue;
		}

		r->err = rebind(c->bdf, driver);
		if (!r->err) {
			// Clear the override again so later probes behave as without xnvme
			snprintf(path, sizeof(path), PCI_DEVICES "/%s/driver_override", c->bdf);
			r->err = sysfs_write(path, "\n");
		}
		if (r->err) {
			snprintf(r->why, sizeof(r->why), "binding %s failed: %s", driver,
				 strerror(-r->err));
			restore(c);
			nfailed += 1;
		}
	}
	free(found);
	free(ctrlrs);

	return nfailed;
}
#endif
