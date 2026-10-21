// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_platform.h>

#ifdef XNVME_PLATFORM_FREEBSD_ENABLED
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <kenv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/bus.h>
#include <sys/ioctl.h>
#include <sys/linker.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/pciio.h>
#include <sys/ucred.h>
#include <unistd.h>

#define USERSPACE_DRIVER "nic_uio"
#define NIC_UIO_BDFS "hw.nic_uio.bdfs"
#define PATHS_MAX 64

/**
 * An NVMe controller function, with its driver and what holds it
 */
struct ctrlr {
	char bdf[32];
	char driver[64]; ///< Bound driver, empty when none
	char why[448];   ///< What holds it, when 'held'
	bool userspace;  ///< On nic_uio
	bool held;       ///< Refuses an attach, or for a userspace driver, a detach
	bool unchecked;  ///< Open files could not be checked, for want of privileges
	struct pcisel sel;
	int unit; ///< Unit of the bound driver, e.g. 0 for nvme0
};

static void
selector(const struct pcisel *sel, char *buf, size_t len)
{
	snprintf(buf, len, "pci%d:%d:%d:%d", sel->pc_domain, sel->pc_bus, sel->pc_dev,
		 sel->pc_func);
}

/**
 * The CAM disks, nda<N>, attached to the controller nvme<unit>, from the bus headers in
 * 'camcontrol devlist -v' such as "scbus1 on nvme0 bus 0:"
 */
static int
cam_disks(int unit, char disks[][16], int max)
{
	char instance[32];
	char line[512];
	bool ours = false;
	int n = 0;
	FILE *fp;

	snprintf(instance, sizeof(instance), "nvme%d", unit);
	fp = popen("/sbin/camcontrol devlist -v 2>/dev/null", "r");
	while (fp && fgets(line, sizeof(line), fp)) {
		char bus[32], on[8], name[32];
		char *names;

		if (sscanf(line, " %31s %7s %31s", bus, on, name) == 3 &&
		    !strncmp(bus, "scbus", 5) && !strcmp(on, "on")) {
			ours = !strcmp(name, instance);
			continue;
		}
		names = strrchr(line, '(');
		if (!ours || !names) {
			continue;
		}
		for (char *tok = strtok(names + 1, ",)\n"); tok && n < max;
		     tok = strtok(NULL, ",)\n")) {
			if (!strncmp(tok, "nda", 3)) {
				snprintf(disks[n++], 16, "%s", tok);
			}
		}
	}
	if (fp) {
		pclose(fp);
	}

	return n;
}

/**
 * Add each /dev entry named 'stem' followed by nothing, a digit-free suffix or a partition, e.g.
 * "nda0", "nda0p1"; not "nda01"
 */
static void
dev_nodes(const char *stem, char paths[][80], int *npaths)
{
	size_t len = strlen(stem);
	struct dirent *ent;
	DIR *dir = opendir("/dev");

	while (dir && *npaths < PATHS_MAX && (ent = readdir(dir))) {
		char next = ent->d_name[len];

		if (strncmp(ent->d_name, stem, len) || (next >= '0' && next <= '9')) {
			continue;
		}
		snprintf(paths[(*npaths)++], 80, "/dev/%s", ent->d_name);
	}
	if (dir) {
		closedir(dir);
	}
}

/**
 * Find a process with one of the 'npaths' 'paths' open or mapped, via fstat(1)
 *
 * @return 1 with the first holder in 'why', 0 when none is
 */
static int
fstat_holding(char paths[][80], int npaths, char *why, size_t len)
{
	char cmd[PATHS_MAX * 81 + 64] = "/usr/bin/fstat -m";
	char line[512];
	int nlines = 0;
	FILE *fp;

	if (!npaths) {
		return 0;
	}
	for (int i = 0; i < npaths; ++i) {
		strcat(cmd, " ");
		strcat(cmd, paths[i]);
	}
	strcat(cmd, " 2>/dev/null");

	fp = popen(cmd, "r");
	while (fp && fgets(line, sizeof(line), fp)) {
		char user[64], comm[64], pid[32];

		// The first line is the header; any other is a process holding one of them
		nlines += 1;
		if (nlines == 2 && sscanf(line, "%63s %63s %31s", user, comm, pid) == 3) {
			snprintf(why, len, "open by %s (pid %s)", comm, pid);
		}
	}
	if (fp) {
		pclose(fp);
	}

	return nlines > 1;
}

static bool
mounted(char paths[][80], int npaths, char *why, size_t len)
{
	struct statfs *mnts;
	int nmnts = getmntinfo(&mnts, MNT_NOWAIT);

	for (int m = 0; m < nmnts; ++m) {
		for (int i = 0; i < npaths; ++i) {
			if (!strcmp(mnts[m].f_mntfromname, paths[i])) {
				snprintf(why, len, "%s mounted on %s", paths[i],
					 mnts[m].f_mntonname);
				return true;
			}
		}
	}

	return false;
}

static void
ctrlr_check(struct ctrlr *c)
{
	char paths[PATHS_MAX][80];
	char disks[8][16];
	char stem[32];
	int npaths = 0, ndisks;

	// Another user's open files are not visible to fstat without privileges
	c->unchecked = geteuid() != 0;

	if (!strcmp(c->driver, "nvme")) {
		ndisks = cam_disks(c->unit, disks, 8);
		for (int i = 0; i < ndisks; ++i) {
			dev_nodes(disks[i], paths, &npaths);
		}
		c->held = mounted(paths, npaths, c->why, sizeof(c->why));
		snprintf(stem, sizeof(stem), "nvme%d", c->unit);
		dev_nodes(stem, paths, &npaths);
		if (!c->held) {
			c->held = fstat_holding(paths, npaths, c->why, sizeof(c->why));
		}
		return;
	}
	if (c->userspace) {
		snprintf(paths[npaths++], 80, "/dev/uio@pci:%d:%d:%d", c->sel.pc_bus,
			 c->sel.pc_dev, c->sel.pc_func);
		c->held = fstat_holding(paths, npaths, c->why, sizeof(c->why));
	}
}

/**
 * The function at 'bdf', "DDDD:BB:DD.F", with what holds it when 'check'
 *
 * @return 0 on success, -ENODEV when it is not an NVMe controller function
 */
static int
ctrlr_get(const char *bdf, struct ctrlr *c, bool check)
{
	struct pci_match_conf pattern = {0};
	struct pci_conf_io pc = {0};
	struct pci_conf conf;
	unsigned int domain, bus, dev, func;
	int fd, err;

	memset(c, 0, sizeof(*c));
	if (sscanf(bdf, "%x:%x:%x.%x", &domain, &bus, &dev, &func) != 4) {
		return -ENODEV;
	}
	pattern.pc_sel.pc_domain = domain;
	pattern.pc_sel.pc_bus = bus;
	pattern.pc_sel.pc_dev = dev;
	pattern.pc_sel.pc_func = func;
	pattern.flags = PCI_GETCONF_MATCH_DOMAIN | PCI_GETCONF_MATCH_BUS | PCI_GETCONF_MATCH_DEV |
			PCI_GETCONF_MATCH_FUNC;
	pc.pat_buf_len = sizeof(pattern);
	pc.num_patterns = 1;
	pc.patterns = &pattern;
	pc.match_buf_len = sizeof(conf);
	pc.matches = &conf;

	fd = open("/dev/pci", O_RDONLY);
	if (fd < 0) {
		return -errno;
	}
	err = ioctl(fd, PCIOCGETCONF, &pc) < 0 ? -errno : 0;
	close(fd);
	if (err) {
		return err;
	}
	if (pc.status == PCI_GETCONF_ERROR || pc.num_matches != 1 || conf.pc_class != 0x01 ||
	    conf.pc_subclass != 0x08 || conf.pc_progif != 0x02) {
		return -ENODEV;
	}
	c->sel = conf.pc_sel;
	c->unit = conf.pd_unit;
	snprintf(c->bdf, sizeof(c->bdf), "%04x:%02x:%02x.%x", domain, bus, dev, func);
	snprintf(c->driver, sizeof(c->driver), "%s", conf.pd_name);
	c->userspace = !strcmp(conf.pd_name, USERSPACE_DRIVER);
	if (check) {
		ctrlr_check(c);
	}

	return 0;
}

const char *
xnvme_driver_freebsd_default(void)
{
	return USERSPACE_DRIVER;
}

/**
 * Issue a devctl(8) request, as libdevctl does, on the device at 'sel'
 */
static int
devctl(unsigned long cmd, const struct pcisel *sel, int flags, const char *driver)
{
	struct devreq req = {0};
	int fd, err;

	fd = open("/dev/devctl2", O_RDONLY);
	if (fd < 0) {
		return -errno;
	}
	selector(sel, req.dr_name, sizeof(req.dr_name));
	req.dr_flags = flags;
	req.dr_data = (void *)driver;
	err = ioctl(fd, cmd, &req) < 0 ? -errno : 0;
	close(fd);

	return err;
}

/**
 * The hw.nic_uio.bdfs entries, "bus:device:function" in decimal, comma separated
 */
static void
nic_uio_bdfs(char *buf, size_t len)
{
	if (kenv(KENV_GET, NIC_UIO_BDFS, buf, len) < 0) {
		buf[0] = '\0';
	}
}

static bool
nic_uio_listed(const char *bdfs, const char *entry)
{
	size_t len = strlen(entry);

	for (const char *c = bdfs; (c = strstr(c, entry)); c += len) {
		if ((c == bdfs || c[-1] == ',') && (c[len] == ',' || c[len] == '\0')) {
			return true;
		}
	}

	return false;
}

static int
nic_uio_set(const char *bdfs)
{
	if (!bdfs[0]) {
		return kenv(KENV_UNSET, NIC_UIO_BDFS, NULL, 0) < 0 && errno != ENOENT ? -errno : 0;
	}

	return kenv(KENV_SET, NIC_UIO_BDFS, (char *)bdfs, strlen(bdfs) + 1) < 0 ? -errno : 0;
}

static void
nic_uio_remove(const char *entry)
{
	char bdfs[1024], out[1024] = {0};

	nic_uio_bdfs(bdfs, sizeof(bdfs));
	for (char *tok = strtok(bdfs, ","); tok; tok = strtok(NULL, ",")) {
		if (strcmp(tok, entry)) {
			snprintf(out + strlen(out), sizeof(out) - strlen(out), "%s%s",
				 out[0] ? "," : "", tok);
		}
	}
	nic_uio_set(out);
}

/**
 * Enable memory space and bus mastering in the command register, as nic_uio leaves them off
 */
static void
bus_master(const struct pcisel *sel)
{
	struct pci_io io = {0};
	int fd = open("/dev/pci", O_RDWR);

	if (fd < 0) {
		return;
	}
	io.pi_sel = *sel;
	io.pi_reg = 0x4;
	io.pi_width = 2;
	if (!ioctl(fd, PCIOCREAD, &io)) {
		io.pi_data |= 0x6;
		ioctl(fd, PCIOCWRITE, &io);
	}
	close(fd);
}

int
xnvme_driver_freebsd_attach(const char *const *bdfs, int nbdfs, const char *driver, int flags,
			    struct xnvme_driver_result *results)
{
	char listed[1024];
	int nfailed = 0, nbind = 0, fileid, err;

	driver = driver ? driver : USERSPACE_DRIVER;
	nic_uio_bdfs(listed, sizeof(listed));

	for (int i = 0; i < nbdfs; ++i) {
		struct xnvme_driver_result *r = &results[i];
		struct ctrlr ctrlr, *c = &ctrlr;
		char path[80];
		const char *sig = NULL;

		memset(r, 0, sizeof(*r));
		if (ctrlr_get(bdfs[i], c, true)) {
			r->err = -ENODEV;
			snprintf(r->why, sizeof(r->why), "not an NVMe controller function");
		} else if (strcmp(driver, USERSPACE_DRIVER)) {
			r->err = -EINVAL;
			snprintf(r->why, sizeof(r->why),
				 "%s is not a userspace driver here; use %s", driver,
				 USERSPACE_DRIVER);
		} else if (c->userspace) {
			r->err = -EALREADY;
			snprintf(r->why, sizeof(r->why), "already on %s", driver);
		} else if (c->sel.pc_domain) {
			r->err = -EINVAL;
			snprintf(r->why, sizeof(r->why), "nic_uio supports PCI domain 0 only");
		} else if (c->held) {
			r->err = -EBUSY;
			snprintf(r->why, sizeof(r->why), "in use: %s", c->why);
		} else if (c->unchecked) {
			r->err = -EACCES;
			snprintf(r->why, sizeof(r->why), "open files could not be checked");
		} else if (!(flags & XNVME_DRIVER_FORCE) && !strcmp(c->driver, "nvme")) {
			char disks[8][16];
			int ndisks = cam_disks(c->unit, disks, 8);

			for (int d = 0; d < ndisks && !sig; ++d) {
				snprintf(path, sizeof(path), "/dev/%s", disks[d]);
				sig = xnvme_driver_signature(path);
			}
			if (sig) {
				r->err = -EEXIST;
				snprintf(r->why, sizeof(r->why), "%s carries %s", path, sig);
			}
		}
		if (r->err) {
			nfailed += r->err != -EALREADY;
			continue;
		}
		if (flags & XNVME_DRIVER_DRY_RUN) {
			continue;
		}

		// A driver pinned with 'devctl set driver' keeps nic_uio from claiming it
		devctl(DEV_CLEAR_DRIVER, &c->sel, DEVF_CLEAR_DRIVER_DETACH, NULL);
		snprintf(path, sizeof(path), "%d:%d:%d", c->sel.pc_bus, c->sel.pc_dev,
			 c->sel.pc_func);
		if (!nic_uio_listed(listed, path)) {
			snprintf(listed + strlen(listed), sizeof(listed) - strlen(listed), "%s%s",
				 listed[0] ? "," : "", path);
		}
		nbind += 1;
	}
	if (!nbind) {
		return nfailed;
	}

	// nic_uio claims the functions in hw.nic_uio.bdfs when it loads, and only then
	fileid = kldfind(USERSPACE_DRIVER ".ko");
	err = fileid >= 0 && kldunload(fileid) ? -errno : 0;
	if (!err) {
		err = nic_uio_set(listed);
	}
	if (!err && kldload(USERSPACE_DRIVER) < 0) {
		err = -errno;
	}

	for (int i = 0; i < nbdfs; ++i) {
		struct xnvme_driver_result *r = &results[i];
		struct ctrlr c;

		if (r->err || ctrlr_get(bdfs[i], &c, false)) {
			continue;
		}
		if (c.userspace) {
			bus_master(&c.sel);
			continue;
		}
		r->err = err ? err : -ENODEV;
		snprintf(r->why, sizeof(r->why), "binding %s failed: %s", driver,
			 strerror(-r->err));
		nfailed += 1;
	}

	return nfailed;
}

int
xnvme_driver_freebsd_detach(const char *const *bdfs, int nbdfs, const char *driver, int flags,
			    struct xnvme_driver_result *results)
{
	int nfailed = 0;

	driver = driver ? driver : "nvme";

	for (int i = 0; i < nbdfs; ++i) {
		struct xnvme_driver_result *r = &results[i];
		struct ctrlr ctrlr, *c = &ctrlr;
		char entry[32];

		memset(r, 0, sizeof(*r));
		if (ctrlr_get(bdfs[i], c, true)) {
			r->err = -ENODEV;
			snprintf(r->why, sizeof(r->why), "not an NVMe controller function");
		} else if (!c->userspace) {
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

		r->err = devctl(DEV_SET_DRIVER, &c->sel, DEVF_SET_DRIVER_DETACH, driver);
		if (r->err) {
			snprintf(r->why, sizeof(r->why), "binding %s failed: %s", driver,
				 strerror(-r->err));
			nfailed += 1;
			continue;
		}
		// A later load of nic_uio must not claim it back
		snprintf(entry, sizeof(entry), "%d:%d:%d", c->sel.pc_bus, c->sel.pc_dev,
			 c->sel.pc_func);
		nic_uio_remove(entry);
	}

	return nfailed;
}
#endif
