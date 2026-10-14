// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <dirent.h>
#include <inttypes.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <libxnvme.h>
#include <xnvme_vcs.h>

int
enumerate_cb(struct xnvme_dev *dev, void *cb_args)
{
	uint32_t *ns_count_ref = cb_args;
	const struct xnvme_ident *ident;

	if (*ns_count_ref == 0) {
		fprintf(stdout, "\n");
	}

	ident = xnvme_dev_get_ident(dev);
	fprintf(stdout, "  - {");
	xnvme_ident_yaml(stdout, ident, 0, ", ", 0);
	fprintf(stdout, "}\n");

	*ns_count_ref = *ns_count_ref + 1;

	return XNVME_ENUMERATE_DEV_CLOSE;
}

int
listing_cb(struct xnvme_dev *dev, void *cb_args)
{
	struct xnvme_cli_enumeration *list = cb_args;
	const struct xnvme_ident *ident;

	ident = xnvme_dev_get_ident(dev);
	if (xnvme_cli_enumeration_append(list, ident)) {
		XNVME_DEBUG("FAILED: adding ident");
	}

	return XNVME_ENUMERATE_DEV_CLOSE;
}

static int
sub_listing(struct xnvme_cli *cli)
{
	struct xnvme_cli_enumeration *listing = NULL;
	struct xnvme_opts opts = {0};
	int err;

	err = xnvme_cli_to_opts(cli, &opts);
	if (err) {
		xnvme_cli_perr("xnvme_cli_to_opts()", err);
		return err;
	}

	err = xnvme_cli_enumeration_alloc(&listing, 100);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_cli_enumeration_alloc()");
		return err;
	}

	err = xnvme_enumerate(cli->args.sys_uri, &opts, *listing_cb, listing);
	if (err) {
		xnvme_cli_perr("xnvme_enumerate()", err);
		goto exit;
	}

	xnvme_cli_enumeration_pp(listing, XNVME_PR_DEF);

exit:
	xnvme_buf_virt_free(listing);

	return err;
}

static int
sub_enumerate(struct xnvme_cli *cli)
{
	struct xnvme_opts opts = {0};
	uint32_t ns_count = 0;
	int err = 0;

	err = xnvme_cli_to_opts(cli, &opts);
	if (err) {
		xnvme_cli_perr("xnvme_cli_to_opts()", err);
		return err;
	}

	fprintf(stdout, "xnvme_cli_enumeration:");

	err = xnvme_enumerate(cli->args.sys_uri, &opts, *enumerate_cb, &ns_count);
	if (err) {
		xnvme_cli_perr("xnvme_enumerate()", err);
		return err;
	}

	if (ns_count == 0) {
		fprintf(stdout, " ~\n");
	}

	return 0;
}

static int
scan_cb(const struct xnvme_ident *ident, void *cb_args)
{
	uint32_t *count_ref = cb_args;

	if (*count_ref == 0) {
		fprintf(stdout, "\n");
	}

	fprintf(stdout, "  - {");
	xnvme_ident_yaml(stdout, ident, 0, ", ", 0);
	fprintf(stdout, "}\n");

	*count_ref = *count_ref + 1;

	return 0;
}

static int
sub_scan(struct xnvme_cli *cli)
{
	struct xnvme_opts opts = {0};
	uint32_t count = 0;
	int err = 0;

	err = xnvme_cli_to_opts(cli, &opts);
	if (err) {
		xnvme_cli_perr("xnvme_cli_to_opts()", err);
		return err;
	}

	fprintf(stdout, "xnvme_scan:");

	err = xnvme_scan(cli->args.sys_uri, &opts, scan_cb, &count);
	if (err) {
		xnvme_cli_perr("xnvme_scan()", err);
		return err;
	}

	if (count == 0) {
		fprintf(stdout, " ~\n");
	}

	return 0;
}

static int
sub_info(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	int err;

	err = xnvme_dev_derive_geo(dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_dev_derive_geo(), err: %d", err);
		return err;
	}

	xnvme_dev_pr(dev, XNVME_PR_DEF);

	return 0;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{
		"list",
		"List devices on the system",
		"List devices on the system",
		sub_listing,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SYS_URI, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_FLAGS, XNVME_CLI_LOPT},

			XNVME_CLI_CORE_OPTS,
		},
	},
	{
		"enum",
		"Enumerate devices on the system",
		"Enumerate devices on the system",
		sub_enumerate,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SYS_URI, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_FLAGS, XNVME_CLI_LOPT},

			XNVME_CLI_CORE_OPTS,
		},
	},
	{
		"scan",
		"Scan for devices without opening them",
		"Scan for devices without opening them",
		sub_scan,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SYS_URI, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_FLAGS, XNVME_CLI_LOPT},

			XNVME_CLI_CORE_OPTS,
		},
	},
	{
		"info",
		"Retrieve derived information for given device",
		"Retrieve derived information for given device",
		sub_info,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_ASYNC_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "xNVMe - Cross-platform NVMe utility",
	.vcs = XNVME_VCS_TAG,
	.descr_short = "Construct and execute NVMe Commands",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

/**
 * Front-end: bare 'xnvme' and a group name print the command tree one level at a time, and a
 * command that lives in another program is looked up on the exec path and run.
 *
 * The programs are named 'xnvme-<command>' and installed off PATH, in XNVME_LIBEXECDIR. The
 * exec path is XNVME_EXEC_PATH when set, a list of directories, else that directory.
 */
#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
#include <process.h>
#define CLI_PATH_SEP ';'
#define CLI_EXE_SUFFIX ".exe"
#else
#define CLI_PATH_SEP ':'
#define CLI_EXE_SUFFIX ""
#endif

#define CLI_PROG_PREFIX "xnvme-"
#define CLI_TEST_PREFIX "xnvme-test-"
#define CLI_EXAMPLE_PREFIX "xnvme-example-"
#define CLI_NAMES_MAX 256

/**
 * The programs the overview lists at the top level; any 'xnvme-<command>' on the exec path runs
 * as 'xnvme <command>', listed or not
 */
struct cli_listed {
	const char *name;  ///< Command name under 'xnvme'
	const char *descr; ///< Short description, shown by the overview
};

static const struct cli_listed g_cli_listed[] = {
	{"perf", "Run IO benchmarks against xNVMe devices"},
};

/// The servers, each listed in the overview when it is installed
static const struct cli_listed g_cli_served[] = {
	{"homi", "Share NVMe controllers between processes"},
	{"qublk", "Serve an xNVMe device as a Linux block device"},
};

static const char *
cli_exec_path(void)
{
	const char *env = getenv("XNVME_EXEC_PATH");

	return (env && env[0]) ? env : XNVME_LIBEXECDIR;
}

/**
 * Find the program 'prefix' followed by 'name' in the directories of the exec path, writing its
 * path to 'path'; a '_' in 'name' is taken as '-'
 *
 * @return On success, 0 is returned. When no directory holds it, -ENOENT is returned.
 */
static int
cli_resolve(const char *prefix, const char *name, char *path, size_t path_len)
{
	const char *seg = cli_exec_path();
	char prog[256];

	snprintf(prog, sizeof(prog), "%s%s", prefix, name);
	for (char *c = prog; *c; ++c) {
		*c = (*c == '_') ? '-' : *c;
	}

	while (*seg) {
		const char *end = strchr(seg, CLI_PATH_SEP);
		int seg_len = end ? (int)(end - seg) : (int)strlen(seg);
		struct stat st;

		if (seg_len) {
			snprintf(path, path_len, "%.*s/%s%s", seg_len, seg, prog, CLI_EXE_SUFFIX);
			if (!stat(path, &st) && S_ISREG(st.st_mode)) {
				return 0;
			}
		}
		if (!end) {
			break;
		}
		seg = end + 1;
	}

	return -ENOENT;
}

/**
 * Run the program at 'path' with 'argv0' followed by the 'argc' arguments in 'argv'
 *
 * @return Only returns on failure, with 1, except on Windows, where the exit status of the
 * program is returned
 */
static int
cli_run(const char *path, char *argv0, int argc, char **argv)
{
	char **nargv = calloc(argc + 2, sizeof(*nargv));
	int err;

	if (!nargv) {
		fprintf(stderr, "xnvme: out of memory\n");
		return 1;
	}
	nargv[0] = argv0;
	for (int i = 0; i < argc; ++i) {
		nargv[i + 1] = argv[i];
	}

#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
	// Windows has no exec that replaces the process, and joins argv into one command line
	// without quoting, so the program keeps its own name rather than 'xnvme <command>'
	nargv[0] = (char *)path;
	err = (int)_spawnv(_P_WAIT, path, (const char *const *)nargv);
	if (err >= 0) {
		free(nargv);
		return err;
	}
#else
	execv(path, nargv);
#endif
	err = errno;
	fprintf(stderr, "xnvme: cannot run '%s': %s\n", path, strerror(err));
	free(nargv);

	return 1;
}

/**
 * Run 'xnvme <words> <name>' as the program 'prefix' 'name', with the 'argc' arguments in
 * 'argv'
 */
static int
cli_dispatch(const char *prefix, const char *words, const char *name, int argc, char **argv)
{
	char path[4096];
	char argv0[512];

	if (cli_resolve(prefix, name, path, sizeof(path))) {
		fprintf(stderr, "xnvme: no command '%s%s'; see 'xnvme'\n", words, name);
		return 1;
	}
	snprintf(argv0, sizeof(argv0), "xnvme %s%s", words, name);

	return cli_run(path, argv0, argc, argv);
}

static const struct xnvme_cli_sub *
cli_builtin(struct xnvme_cli *cli, const char *name)
{
	for (int i = 0; i < cli->nsubs; ++i) {
		if (!strcmp(cli->subs[i].name, name)) {
			return &cli->subs[i];
		}
	}

	return NULL;
}

static int
cli_cmp_names(const void *a, const void *b)
{
	return strcmp(*(char *const *)a, *(char *const *)b);
}

/**
 * Collect the names of the programs on the exec path that start with 'prefix', without it
 *
 * @return The number of names stored in 'names', sorted and without duplicates
 */
static int
cli_names(const char *prefix, char **names, int names_max)
{
	const char *seg = cli_exec_path();
	size_t prefix_len = strlen(prefix);
	size_t suffix_len = strlen(CLI_EXE_SUFFIX);
	int count = 0;

	while (*seg) {
		const char *end = strchr(seg, CLI_PATH_SEP);
		int seg_len = end ? (int)(end - seg) : (int)strlen(seg);
		char dpath[4096];
		struct dirent *ent;
		DIR *dir;

		snprintf(dpath, sizeof(dpath), "%.*s", seg_len, seg);
		dir = seg_len ? opendir(dpath) : NULL;
		while (dir && (ent = readdir(dir)) && count < names_max) {
			size_t len = strlen(ent->d_name);
			char fpath[4096 + 256];
			struct stat st;

			if (len <= prefix_len + suffix_len ||
			    strncmp(ent->d_name, prefix, prefix_len)) {
				continue;
			}
			if (suffix_len && strcmp(ent->d_name + len - suffix_len, CLI_EXE_SUFFIX)) {
				continue;
			}
			snprintf(fpath, sizeof(fpath), "%s/%s", dpath, ent->d_name);
			if (stat(fpath, &st) || !S_ISREG(st.st_mode)) {
				continue;
			}
			len -= prefix_len + suffix_len;
			names[count] = malloc(len + 1);
			if (names[count]) {
				memcpy(names[count], ent->d_name + prefix_len, len);
				names[count][len] = '\0';
				count += 1;
			}
		}
		if (dir) {
			closedir(dir);
		}
		if (!end) {
			break;
		}
		seg = end + 1;
	}

	qsort(names, count, sizeof(*names), cli_cmp_names);
	for (int i = 1; i < count; ++i) {
		if (!strcmp(names[i - 1], names[i])) {
			free(names[i]);
			memmove(&names[i], &names[i + 1], (count - i - 1) * sizeof(*names));
			count -= 1;
			i -= 1;
		}
	}

	return count;
}

static void
cli_names_free(char **names, int count)
{
	for (int i = 0; i < count; ++i) {
		free(names[i]);
	}
}

/**
 * Return 's' as the inside of a single-quoted YAML scalar, its quotes doubled. The result
 * lives in one of a few static buffers, so up to four can be used in one printf()
 */
static const char *
cli_yq(const char *s)
{
	static char bufs[4][1024];
	static int next;
	char *buf = bufs[next++ % 4];
	size_t len = 0;

	for (; s && *s && len < sizeof(bufs[0]) - 2; ++s) {
		buf[len++] = *s;
		if (*s == '\'') {
			buf[len++] = '\'';
		}
	}
	buf[len] = '\0';

	return buf;
}

static void
cli_row(const char *cmd, const char *descr)
{
	printf("  %-24s %s\n", cmd, descr);
}

/**
 * Print 'heading' and a row for each program in 'listed' that is installed; nothing when none is
 *
 * @return The number of rows printed
 */
static int
cli_listed_pr(const char *heading, const struct cli_listed *listed, size_t nlisted)
{
	char path[4096];
	int nrows = 0;

	for (size_t i = 0; i < nlisted; ++i) {
		char cmd[64];

		if (cli_resolve(CLI_PROG_PREFIX, listed[i].name, path, sizeof(path))) {
			continue;
		}
		if (!nrows++ && heading) {
			printf("%s\n", heading);
		}
		snprintf(cmd, sizeof(cmd), "xnvme %s", listed[i].name);
		cli_row(cmd, listed[i].descr);
	}

	return nrows;
}

static int
cli_overview(struct xnvme_cli *cli)
{
	printf("Usage: xnvme <command> [<args>] [--help]\n");
	printf("\n");
	printf("%s\n", cli->descr_short);
	printf("\n");
	printf("Start here:\n");
#ifdef XNVME_PLATFORM_LINUX_ENABLED
	cli_row("xnvme list", "NVMe controller functions and their driver");
#else
	cli_row("xnvme list", "List devices on the system");
#endif
	cli_row("xnvme info <uri>", cli_builtin(cli, "info")->descr_short);
	cli_row("xnvme enum", cli_builtin(cli, "enum")->descr_short);
	cli_listed_pr(NULL, g_cli_listed, sizeof g_cli_listed / sizeof(*g_cli_listed));
	printf("\n");
	printf("Set up the host:\n");
	cli_row("xnvme env", "Relevant system config and library introspection");
	cli_row("xnvme driver", "Bind the driver a backend needs, or show them");
	printf("\n");
	if (cli_listed_pr("Serve devices:", g_cli_served,
			  sizeof g_cli_served / sizeof(*g_cli_served))) {
		printf("\n");
	}
	printf("Command passthru:\n");
	cli_row("xnvme cmd-admin", "An admin command, as 'nvme admin-passthru' takes it");
	cli_row("xnvme cmd-io", "An I/O command, as 'nvme io-passthru' takes it");
	cli_row("xnvme cmd-iov", "An I/O command with its data in --vec-cnt segments");
	printf("\n");
	printf("Groups, run one to list its commands:\n");
	cli_row("xnvme test", "installed test programs");
	cli_row("xnvme example", "installed example programs");
	cli_row("xnvme --help", "the device commands list, enum, scan and info");

	return 0;
}

/**
 * List the programs of the 'test' or 'example' group
 */
static int
cli_group_programs(const char *group, const char *prefix)
{
	char *names[CLI_NAMES_MAX];
	int count = cli_names(prefix, names, CLI_NAMES_MAX);

	if (!count) {
		printf("No %s programs found on the exec path: %s\n", group, cli_exec_path());
		return 0;
	}

	printf("The %s programs, run one to list its subcommands:\n", group);
	printf("\n");
	for (int i = 0; i < count; ++i) {
		printf("  xnvme %s %s\n", group, names[i]);
	}
	printf("\n");
	printf("See 'xnvme %s <name> <subcommand> --help' for the description of [<args>]\n",
	       group);
	cli_names_free(names, count);

	return 0;
}

/**
 * 'xnvme env': whether this host can run xNVMe, and what this xNVMe is, as YAML. Each fact of
 * the host is a mapping of what was found, with a 'fix' where something is missing. It reads,
 * never changes.
 */
#ifdef XNVME_PLATFORM_LINUX_ENABLED
#include <sys/resource.h>
#include <sys/utsname.h>

static bool
cli_path_exists(const char *path)
{
	struct stat st;

	return !stat(path, &st);
}

static int
cli_dir_count(const char *path)
{
	DIR *dir = opendir(path);
	struct dirent *ent;
	int count = 0;

	while (dir && (ent = readdir(dir))) {
		count += ent->d_name[0] != '.';
	}
	if (dir) {
		closedir(dir);
	}

	return dir ? count : -1;
}

/**
 * Whether the file at 'path' has a line containing 'needle'
 */
static bool
cli_file_has(const char *path, const char *needle)
{
	FILE *fp = fopen(path, "r");
	char line[4096];
	bool found = false;

	while (fp && !found && fgets(line, sizeof(line), fp)) {
		found = strstr(line, needle) != NULL;
	}
	if (fp) {
		fclose(fp);
	}

	return found;
}

static long
cli_file_long(const char *path)
{
	FILE *fp = fopen(path, "r");
	long val = -1;

	if (fp) {
		if (fscanf(fp, "%ld", &val) != 1) {
			val = -1;
		}
		fclose(fp);
	}

	return val;
}

/**
 * Print the kernel module 'name': available, built in, and loaded
 */
static void
cli_env_kmod(const char *name, const char *fix)
{
	struct utsname uts;
	char path[512];
	char needle[128];
	char sysname[128];
	bool builtin = false, loaded, available = false;

	snprintf(sysname, sizeof(sysname), "%s", name);
	for (char *c = sysname; *c; ++c) {
		*c = (*c == '-') ? '_' : *c;
	}
	snprintf(needle, sizeof(needle), "%s ", sysname);
	loaded = false;
	{
		FILE *fp = fopen("/proc/modules", "r");
		char line[512];

		while (fp && !loaded && fgets(line, sizeof(line), fp)) {
			loaded = !strncmp(line, needle, strlen(needle));
		}
		if (fp) {
			fclose(fp);
		}
	}
	if (!uname(&uts)) {
		snprintf(needle, sizeof(needle), "/%s.ko", name);
		snprintf(path, sizeof(path), "/lib/modules/%s/modules.builtin", uts.release);
		builtin = cli_file_has(path, needle);
		snprintf(path, sizeof(path), "/lib/modules/%s/modules.dep", uts.release);
		available = builtin || loaded || cli_file_has(path, needle);
	}

	printf("  %s: {available: %s, builtin: %s, loaded: %s", name, available ? "true" : "false",
	       builtin ? "true" : "false", loaded ? "true" : "false");
	if (!builtin && !loaded) {
		printf(", fix: '%s'", cli_yq(fix));
	}
	printf("}\n");
}

/**
 * Print whether the device node 'path' exists, as the fact 'name'
 */
static void
cli_env_node(const char *name, const char *path, const char *fix)
{
	bool exists = cli_path_exists(path);

	printf("  %s: {path: '%s', exists: %s", name, path, exists ? "true" : "false");
	if (!exists) {
		printf(", fix: '%s'", cli_yq(fix));
	}
	printf("}\n");
}

static int
cli_cmp_long(const void *a, const void *b)
{
	long x = *(const long *)a, y = *(const long *)b;

	return (x > y) - (x < y);
}

/**
 * Print the hugepages: the default size, and for each size the pages reserved and free, and
 * those reserved on each NUMA node
 */
static void
cli_env_hugepages(void)
{
	DIR *dir = opendir("/sys/kernel/mm/hugepages");
	struct dirent *ent;
	long sizes[16];
	int nsizes = 0;
	long dflt = 0, reserved = 0;
	FILE *fp = fopen("/proc/meminfo", "r");
	char line[256];

	while (fp && fgets(line, sizeof(line), fp)) {
		sscanf(line, "Hugepagesize: %ld kB", &dflt);
	}
	if (fp) {
		fclose(fp);
	}
	while (dir && (ent = readdir(dir)) && nsizes < 16) {
		long kib;

		if (sscanf(ent->d_name, "hugepages-%ldkB", &kib) == 1) {
			sizes[nsizes++] = kib;
		}
	}
	if (dir) {
		closedir(dir);
	}
	qsort(sizes, nsizes, sizeof(*sizes), cli_cmp_long);

	printf("  hugepages: {default_kib: %ld, sizes: {", dflt);
	for (int i = 0; i < nsizes; ++i) {
		char path[256];
		long total, free_;
		int nnodes = 0;

		snprintf(path, sizeof(path),
			 "/sys/kernel/mm/hugepages/hugepages-%ldkB/nr_hugepages", sizes[i]);
		total = cli_file_long(path);
		snprintf(path, sizeof(path),
			 "/sys/kernel/mm/hugepages/hugepages-%ldkB/free_hugepages", sizes[i]);
		free_ = cli_file_long(path);
		reserved += total > 0 ? total : 0;
		printf("%s%ld: {total: %ld, free: %ld, nodes: {", i ? ", " : "", sizes[i], total,
		       free_);
		for (int node = 0; node < 1024; ++node) {
			long n;

			snprintf(path, sizeof(path),
				 "/sys/devices/system/node/node%d/hugepages/hugepages-%ldkB/"
				 "nr_hugepages",
				 node, sizes[i]);
			n = cli_file_long(path);
			if (n < 0) {
				if (node > 64) {
					break;
				}
				continue;
			}
			printf("%s%d: %ld", nnodes++ ? ", " : "", node, n);
		}
		printf("}}");
	}
	printf("}");
	if (!reserved) {
		printf(", fix: '%s'",
		       cli_yq("hugepages setup --count <pages>, and vm.nr_hugepages in "
			      "/etc/sysctl.d/ to keep them across a reboot"));
	}
	printf("}\n");
}

/**
 * Print where hugetlbfs is mounted
 */
static void
cli_env_hugetlbfs(void)
{
	FILE *fp = fopen("/proc/self/mountinfo", "r");
	char line[4096];
	int count = 0;

	printf("  hugetlbfs: {paths: [");
	while (fp && fgets(line, sizeof(line), fp)) {
		char mnt[1024];

		if (!strstr(line, " - hugetlbfs ") ||
		    sscanf(line, "%*s %*s %*s %*s %1023s", mnt) != 1) {
			continue;
		}
		printf("%s'%s'", count++ ? ", " : "", cli_yq(mnt));
	}
	if (fp) {
		fclose(fp);
	}
	printf("], mounted: %s", count ? "true" : "false");
	if (!count) {
		printf(", fix: '%s'", cli_yq("hugepages mount; SPDK needs it"));
	}
	printf("}\n");
}

static void
cli_env_ctrlrs(void);

/// The host setup 'xnvme driver' leaves to other tools
static const char *g_cli_env_tools[] = {"hugepages", "iommu"};

static bool
cli_on_path(const char *name)
{
	const char *seg = getenv("PATH");

	while (seg && *seg) {
		const char *end = strchr(seg, ':');
		int seg_len = end ? (int)(end - seg) : (int)strlen(seg);
		char path[4096];

		snprintf(path, sizeof(path), "%.*s/%s", seg_len, seg, name);
		if (seg_len && !access(path, X_OK)) {
			return true;
		}
		seg = end ? end + 1 : NULL;
	}

	return false;
}

/// What polled I/O with io_uring needs from the kernel nvme driver
static const char *g_cli_poll_hint =
	"polled I/O with io_uring needs poll queues: add \"options nvme poll_queues=4\" to "
	"/etc/modprobe.d/nvme.conf, then reload nvme or reboot; a reload drops every NVMe device "
	"on the host";

/**
 * The value of the module parameter 'param' of 'mod', or -1 when it is not there
 */
static long
cli_mod_param(const char *mod, const char *param)
{
	char path[512], line[64];
	FILE *fp;
	long val = -1;

	snprintf(path, sizeof(path), "/sys/module/%s/parameters/%s", mod, param);
	fp = fopen(path, "r");
	if (fp && fgets(line, sizeof(line), fp)) {
		val = strtol(line, NULL, 10);
	}
	if (fp) {
		fclose(fp);
	}

	return val;
}

static void
cli_env_system(void)
{
	struct utsname uts;
	struct rlimit rlim;
	bool missing = false;
	int count;

	count = cli_dir_count("/sys/kernel/iommu_groups");
	printf("  iommu: {groups: %d", count > 0 ? count : 0);
	if (count <= 0) {
		printf(", fix: '%s'",
		       cli_yq("enable VT-d or AMD-Vi in firmware; 'iommu show' prints the mode "
			      "the "
			      "kernel runs it in, and 'iommu pt' sets passthrough"));
	}
	printf("}\n");

	cli_env_kmod("vfio-pci",
		     "modprobe vfio-pci, and list it in /etc/modules-load.d/ to load at boot");
	cli_env_node("iommufd", "/dev/iommu",
		     "a kernel with CONFIG_IOMMUFD; uPCIe falls back to VFIO_TYPE1 without it");
	cli_env_node("vfio-cdev", "/dev/vfio/devices",
		     "a kernel with CONFIG_VFIO_DEVICE_CDEV; uPCIe falls back to VFIO_TYPE1 "
		     "without it");
	cli_env_kmod("uio_pci_generic",
		     "modprobe uio_pci_generic, for userspace drivers without an IOMMU");
	cli_env_hugepages();
	cli_env_hugetlbfs();

	if (!getrlimit(RLIMIT_MEMLOCK, &rlim)) {
		if (rlim.rlim_cur == RLIM_INFINITY) {
			printf("  memlock: {unlimited: true}\n");
		} else {
			printf("  memlock: {unlimited: false, limit_kib: %llu, fix: '%s'}\n",
			       (unsigned long long)rlim.rlim_cur / 1024,
			       cli_yq("ulimit -l unlimited, or a memlock entry in "
				      "/etc/security/limits.d/"));
		}
	}

	cli_env_kmod("ublk_drv", "modprobe ublk_drv; qublk needs it");
	cli_env_node("cuse", "/dev/cuse",
		     "modprobe cuse; for the NVMe ioctl mimic of homi and qublk");
	cli_env_ctrlrs();
	if (cli_mod_param("nvme", "io_queue_depth") >= 0) {
		long poll = cli_mod_param("nvme", "poll_queues");

		printf("  nvme_driver: {poll_queues: %ld, write_queues: %ld, io_queue_depth: %ld",
		       poll, cli_mod_param("nvme", "write_queues"),
		       cli_mod_param("nvme", "io_queue_depth"));
		if (!poll) {
			printf(", fix: '%s'", cli_yq(g_cli_poll_hint));
		}
		printf("}\n");
	}

	printf("  tools: {");
	for (size_t i = 0; i < sizeof g_cli_env_tools / sizeof(*g_cli_env_tools); ++i) {
		bool found = cli_on_path(g_cli_env_tools[i]);

		printf("%s%s: %s", i ? ", " : "", g_cli_env_tools[i], found ? "true" : "false");
		missing |= !found;
	}
	if (missing) {
		printf(", fix: 'pipx install hugepages iommu'");
	}
	printf("}\n");

	printf("  systemd: {running: %s}\n",
	       cli_path_exists("/run/systemd/system") ? "true" : "false");
	if (!uname(&uts)) {
		printf("  kernel: {release: '%s'}\n", cli_yq(uts.release));
	}
}
#else
static void
cli_env_system(void)
{
	printf("  platform: {checked: false, why: 'the system checks are written for Linux'}\n");
}
#endif

/**
 * 'xnvme list', 'attach' and 'detach': the NVMe controller functions on the PCI bus, and moving
 * them between the kernel nvme driver and a userspace one, through libxnvme_driver.h. Nothing is
 * recorded: 'list' reads sysfs, and 'detach' returns a controller to nvme, the kernel driver of
 * every NVMe controller function.
 */
#define CLI_CTRLRS_MAX 256
#define CLI_DEVS_MAX 64

#if defined(XNVME_PLATFORM_LINUX_ENABLED) || defined(XNVME_PLATFORM_FREEBSD_ENABLED)
struct cli_ctrlr {
	char bdf[32];
	char driver[64]; ///< Bound driver, empty when none
	char nsdev[64];  ///< A namespace device node of the kernel driver, empty when none
	char iommu[8];   ///< IOMMU group, empty when the function has none
	char state[512]; ///< What uses it, from a dry run, when 'in_use'
	bool userspace;  ///< On one of the platform's userspace drivers
	bool in_use;     ///< Refuses an attach, or for a userspace driver, a detach
};

struct cli_scan {
	struct cli_ctrlr *ctrlrs;
	int max;
	int count;
};

/**
 * Collect the controllers xnvme_scan() reports; a controller is reported
 * by its BDF, a namespace by its device node. A controller's namespaces follow it, and of those
 * its 'nsdev' is the first that has a node, a block device over a
 * generic one; the kernel registers a namespace of a command set it does not support without a
 * block node.
 */
static int
cli_scan_cb(const struct xnvme_ident *ident, void *cb_args)
{
	struct cli_scan *scan = cb_args;
	struct cli_ctrlr *c;
	struct stat st, cur;

	if (ident->uri[0] != '/') {
		if (scan->count == scan->max) {
			return 1;
		}
		c = &scan->ctrlrs[scan->count++];
		memset(c, 0, sizeof(*c));
		snprintf(c->bdf, sizeof(c->bdf), "%s", ident->uri);
		snprintf(c->driver, sizeof(c->driver), "%s", ident->kernel_driver);
		snprintf(c->iommu, sizeof(c->iommu), "%s", ident->iommu_group);
		return 0;
	}
	if (!scan->count || stat(ident->uri, &st)) {
		return 0;
	}
	c = &scan->ctrlrs[scan->count - 1];
	if (!c->nsdev[0] ||
	    (S_ISBLK(st.st_mode) && !stat(c->nsdev, &cur) && !S_ISBLK(cur.st_mode))) {
		snprintf(c->nsdev, sizeof(c->nsdev), "%s", ident->uri);
	}

	return 0;
}

static int
cli_cmp_ctrlrs(const void *a, const void *b)
{
	return strcmp(((const struct cli_ctrlr *)a)->bdf, ((const struct cli_ctrlr *)b)->bdf);
}

/**
 * Find the NVMe controller functions with xnvme_scan(), sorted by address, without working out
 * what holds them
 *
 * @return The number found, or a negative errno
 */
static int
cli_scan(struct cli_ctrlr *ctrlrs, int max)
{
	struct cli_scan scan = {.ctrlrs = ctrlrs, .max = max};
	int err = xnvme_scan(NULL, NULL, cli_scan_cb, &scan);

	if (err) {
		return err;
	}
	qsort(ctrlrs, scan.count, sizeof(*ctrlrs), cli_cmp_ctrlrs);

	return scan.count;
}

/**
 * Keep what uses 'c', the refusal 'why' of a dry run, when it is in use
 */
static void
cli_ctrlr_state(struct cli_ctrlr *c, const char *why)
{
	snprintf(c->state, sizeof(c->state), "%s", c->in_use ? why : "");
}

/**
 * Find the NVMe controller functions, and work out the state of each: a dry-run attach of those
 * on the kernel nvme driver, and a dry-run detach of the others, say what holds each
 *
 * @return The number found, or a negative errno
 */
static int
cli_ctrlrs(struct cli_ctrlr *ctrlrs, int max)
{
	static struct xnvme_driver_result results[CLI_CTRLRS_MAX];
	const char *bdfs[CLI_CTRLRS_MAX];
	int idx[CLI_CTRLRS_MAX];
	int count = cli_scan(ctrlrs, max < CLI_CTRLRS_MAX ? max : CLI_CTRLRS_MAX);

	for (int detach = 0; count > 0 && detach < 2; ++detach) {
		int n = 0, err;

		for (int i = 0; i < count; ++i) {
			if ((strcmp(ctrlrs[i].driver, "nvme") != 0) == (detach != 0)) {
				idx[n] = i;
				bdfs[n++] = ctrlrs[i].bdf;
			}
		}
		err = detach ? xnvme_driver_detach(bdfs, n, NULL, XNVME_DRIVER_DRY_RUN, results)
			     : xnvme_driver_attach(bdfs, n, NULL,
						   XNVME_DRIVER_DRY_RUN | XNVME_DRIVER_FORCE,
						   results);
		if (err < 0) {
			return err;
		}
		for (int j = 0; j < n; ++j) {
			struct cli_ctrlr *c = &ctrlrs[idx[j]];

			c->in_use = results[j].err == -EBUSY;
			c->userspace = detach && results[j].err != -EALREADY;
			cli_ctrlr_state(c, results[j].why);
		}
	}

	return count;
}

/**
 * Accept "03:00.0" for "0000:03:00.0"
 */
static void
cli_bdf(const char *arg, char *bdf, size_t len)
{
	int colons = 0;

	for (const char *c = arg; *c; ++c) {
		colons += *c == ':';
	}
	snprintf(bdf, len, "%s%s", colons == 1 ? "0000:" : "", arg);
}

static struct cli_ctrlr *
cli_ctrlr_find(struct cli_ctrlr *ctrlrs, int count, const char *bdf)
{
	for (int i = 0; i < count; ++i) {
		if (!strcmp(ctrlrs[i].bdf, bdf)) {
			return &ctrlrs[i];
		}
	}

	return NULL;
}

static int
cli_list(int argc, char **argv)
{
	static struct cli_ctrlr ctrlrs[CLI_CTRLRS_MAX];
	int count;

	if (argc) {
		printf("Usage: xnvme list\n\n");
		printf("The NVMe controller functions on the PCI bus, as YAML: the driver each "
		       "is\n");
		printf("on and its IOMMU group, read from sysfs without opening anything\n");
		return strcmp(argv[0], "--help") ? 1 : 0;
	}

	count = cli_scan(ctrlrs, CLI_CTRLRS_MAX);
	if (count < 0) {
		fprintf(stderr, "xnvme list: %s\n", strerror(-count));
		return 1;
	}
	printf("devices:%s\n", count ? "" : " {}");
	for (int i = 0; i < count; ++i) {
		char driver[80] = "null";

		if (ctrlrs[i].driver[0]) {
			snprintf(driver, sizeof(driver), "'%s'", cli_yq(ctrlrs[i].driver));
		}
		printf("  '%s': {driver: %s, iommu_group: %s}\n", cli_yq(ctrlrs[i].bdf), driver,
		       ctrlrs[i].iommu[0] ? ctrlrs[i].iommu : "null");
	}

	return 0;
}

/// One controller in the YAML that 'xnvme driver --be' prints
struct cli_drv_row {
	char bdf[32];
	const char *now;  ///< The driver it is on afterwards; NULL when it is no controller
	const char *from; ///< The driver it was on
	const char *to;   ///< The driver it was to go to
	char note[640]; ///< Why it did not go: "noop", "dry-run", or a reason; empty when it went
};

static struct cli_drv_row *
cli_drv_row_add(struct cli_drv_row *rows, int *nrows, const char *bdf, const char *driver,
		const char *to)
{
	struct cli_drv_row *row = &rows[(*nrows)++];

	snprintf(row->bdf, sizeof(row->bdf), "%s", bdf);
	row->now = row->from = (driver && driver[0]) ? driver : "-";
	row->to = to;
	row->note[0] = '\0';

	return row;
}

/**
 * Print 'rows' as YAML: the driver each controller is on, whether that changed, and why not
 * when it was to change and did not
 */
static void
cli_drv_rows_pr(const struct cli_drv_row *rows, int nrows)
{
	printf("devices:%s\n", nrows ? "" : " {}");
	for (int i = 0; i < nrows; ++i) {
		const struct cli_drv_row *r = &rows[i];
		bool changed = r->now && strcmp(r->now, r->from);

		if (!r->now || !strcmp(r->now, "-")) {
			printf("  '%s': {driver: null, changed: false", cli_yq(r->bdf));
		} else {
			printf("  '%s': {driver: '%s', changed: %s", cli_yq(r->bdf),
			       cli_yq(r->now), changed ? "true" : "false");
		}
		if (!strcmp(r->note, "dry-run")) {
			printf(", why: 'dry-run, would go to %s'", cli_yq(r->to ? r->to : "-"));
		} else if (r->note[0] && strcmp(r->note, "noop")) {
			printf(", why: '%s'", cli_yq(r->note));
		}
		printf("}\n");
	}
}

/**
 * Put the named functions, or with --all every one on the kernel nvme driver and not in use, on
 * the userspace driver 'driver'
 */
static int
cli_driver_attach(int argc, char **argv, const char *driver)
{
	static struct cli_ctrlr ctrlrs[CLI_CTRLRS_MAX];
	static struct xnvme_driver_result results[CLI_CTRLRS_MAX];
	static struct cli_drv_row rows[2 * CLI_CTRLRS_MAX];
	struct cli_ctrlr *targets[CLI_CTRLRS_MAX];
	int trow[CLI_CTRLRS_MAX];
	const char *bdfs[CLI_CTRLRS_MAX];
	bool force = false, all = false, dry_run = false;
	int count, ntargets = 0, nrefused = 0, nrows = 0, err;

	count = cli_ctrlrs(ctrlrs, CLI_CTRLRS_MAX);
	for (int i = 0; i < argc; ++i) {
		char bdf[32];

		if (!strcmp(argv[i], "--force")) {
			force = true;
		} else if (!strcmp(argv[i], "--all")) {
			all = true;
		} else if (!strcmp(argv[i], "--dry-run")) {
			dry_run = true;
		} else {
			struct cli_ctrlr *c;

			cli_bdf(argv[i], bdf, sizeof(bdf));
			c = count < 0 ? NULL : cli_ctrlr_find(ctrlrs, count, bdf);
			if (!c && count >= 0) {
				struct cli_drv_row *row =
					cli_drv_row_add(rows, &nrows, bdf, "", NULL);

				row->now = NULL;
				snprintf(row->note, sizeof(row->note),
					 "not an NVMe controller function");
				nrefused += 1;
				continue;
			}
			if (c) {
				cli_drv_row_add(rows, &nrows, c->bdf, c->driver, NULL);
				trow[ntargets] = nrows - 1;
			}
			targets[ntargets++] = c;
		}
	}
	if (count < 0) {
		fprintf(stderr, "xnvme driver: %s\n",
			count == -ENOSYS ? "not supported on this platform" : strerror(-count));
		return 1;
	}
	// --all takes every function on nvme that is not in use, and reports the others as they
	// are
	for (int i = 0; all && i < count; ++i) {
		struct cli_ctrlr *c = &ctrlrs[i];
		struct cli_drv_row *row;
		bool named = false;

		for (int j = 0; j < ntargets; ++j) {
			named |= targets[j] == c;
		}
		if (named) {
			continue;
		}
		row = cli_drv_row_add(rows, &nrows, c->bdf, c->driver, driver);
		if (!strcmp(c->driver, "nvme") && !c->in_use) {
			trow[ntargets] = nrows - 1;
			targets[ntargets++] = c;
		} else if (c->userspace) {
			row->to = row->from;
			snprintf(row->note, sizeof(row->note), "noop");
		} else {
			snprintf(row->note, sizeof(row->note), "skipped, %s",
				 c->in_use ? c->state : "on another driver");
		}
	}
	if (!ntargets) {
		if (!nrows && !all) {
			fprintf(stderr, "xnvme driver: name the controllers; see 'xnvme list'\n");
			return 1;
		}
		cli_drv_rows_pr(rows, nrows);
		return nrefused ? 1 : 0;
	}
	if (!dry_run && geteuid()) {
		fprintf(stderr, "xnvme driver: needs root, it rebinds drivers\n");
		return 1;
	}

	for (int i = 0; i < ntargets; ++i) {
		bdfs[i] = targets[i]->bdf;
	}
	err = xnvme_driver_attach(
		bdfs, ntargets, driver,
		(force ? XNVME_DRIVER_FORCE : 0) | (dry_run ? XNVME_DRIVER_DRY_RUN : 0), results);
	if (err < 0) {
		fprintf(stderr, "xnvme driver: %s\n", strerror(-err));
		return 1;
	}

	for (int i = 0; i < ntargets; ++i) {
		struct cli_ctrlr *c = targets[i];
		struct xnvme_driver_result *r = &results[i];
		struct cli_drv_row *row = &rows[trow[i]];

		row->to = driver;
		switch (r->err) {
		case 0:
			if (dry_run) {
				snprintf(row->note, sizeof(row->note), "dry-run");
				break;
			}
			row->now = driver;
			break;
		case -EALREADY:
			row->to = row->from;
			snprintf(row->note, sizeof(row->note), "noop");
			break;
		case -EISCONN:
			if (dry_run) {
				snprintf(row->note, sizeof(row->note), "dry-run");
				break;
			}
			snprintf(row->note, sizeof(row->note), "refused, on %s", c->driver);
			nrefused += 1;
			break;
		case -EEXIST:
			snprintf(row->note, sizeof(row->note),
				 "refused, %s; --force attaches it anyway", r->why);
			nrefused += 1;
			break;
		default:
			snprintf(row->note, sizeof(row->note), "refused, %s", r->why);
			nrefused += 1;
		}
	}
	cli_drv_rows_pr(rows, nrows);

	return nrefused ? 1 : 0;
}

/**
 * Return the named functions, or with --all every one on a userspace driver, to the kernel nvme
 * driver
 */
static int
cli_driver_detach(int argc, char **argv)
{
	static struct cli_ctrlr ctrlrs[CLI_CTRLRS_MAX];
	static struct cli_drv_row rows[2 * CLI_CTRLRS_MAX];
	struct cli_ctrlr *targets[CLI_CTRLRS_MAX];
	int trow[CLI_CTRLRS_MAX];
	bool all = false, dry_run = false, pick;
	int count, ntargets = 0, nrefused = 0, nrows = 0;

	count = cli_ctrlrs(ctrlrs, CLI_CTRLRS_MAX);
	for (int i = 0; i < argc; ++i) {
		char bdf[32];

		if (!strcmp(argv[i], "--all")) {
			all = true;
		} else if (!strcmp(argv[i], "--force")) {
			continue;
		} else if (!strcmp(argv[i], "--dry-run")) {
			dry_run = true;
		} else {
			struct cli_ctrlr *c;

			cli_bdf(argv[i], bdf, sizeof(bdf));
			c = count < 0 ? NULL : cli_ctrlr_find(ctrlrs, count, bdf);
			if (!c && count >= 0) {
				struct cli_drv_row *row =
					cli_drv_row_add(rows, &nrows, bdf, "", NULL);

				row->now = NULL;
				snprintf(row->note, sizeof(row->note),
					 "not an NVMe controller function");
				nrefused += 1;
				continue;
			}
			if (c) {
				cli_drv_row_add(rows, &nrows, c->bdf, c->driver, "nvme");
				trow[ntargets] = nrows - 1;
			}
			targets[ntargets++] = c;
		}
	}
	if (count < 0) {
		fprintf(stderr, "xnvme driver: %s\n",
			count == -ENOSYS ? "not supported on this platform" : strerror(-count));
		return 1;
	}
	// With no controller named, --all takes those on a userspace driver and reports the others
	pick = all && !ntargets && !nrefused;
	for (int i = 0; pick && i < count; ++i) {
		struct cli_ctrlr *c = &ctrlrs[i];
		struct cli_drv_row *row = cli_drv_row_add(rows, &nrows, c->bdf, c->driver, "nvme");

		if (c->userspace) {
			trow[ntargets] = nrows - 1;
			targets[ntargets++] = c;
		} else if (!strcmp(c->driver, "nvme")) {
			row->to = row->from;
			snprintf(row->note, sizeof(row->note), "noop");
		} else {
			snprintf(row->note, sizeof(row->note), "skipped, on another driver");
		}
	}
	if (!ntargets) {
		if (!nrows && !all) {
			fprintf(stderr, "xnvme driver: name the controllers; see 'xnvme list'\n");
			return 1;
		}
		cli_drv_rows_pr(rows, nrows);
		return nrefused ? 1 : 0;
	}
	if (!dry_run && geteuid()) {
		fprintf(stderr, "xnvme driver: needs root, it rebinds drivers\n");
		return 1;
	}

	for (int i = 0; i < ntargets; ++i) {
		struct cli_ctrlr *c = targets[i];
		struct cli_drv_row *row = &rows[trow[i]];
		const char *bdf = c->bdf;
		struct xnvme_driver_result r;
		int err;

		err = xnvme_driver_detach(&bdf, 1, row->to, dry_run ? XNVME_DRIVER_DRY_RUN : 0,
					  &r);
		if (err < 0) {
			snprintf(row->note, sizeof(row->note), "refused, %s", strerror(-err));
			nrefused += 1;
			continue;
		}
		if (r.err == -EALREADY) {
			row->to = row->from;
			snprintf(row->note, sizeof(row->note), "noop");
			continue;
		}
		if (r.err) {
			snprintf(row->note, sizeof(row->note), "refused, %s", r.why);
			nrefused += 1;
			continue;
		}
		if (dry_run) {
			snprintf(row->note, sizeof(row->note), "dry-run");
			continue;
		}
		row->now = row->to;
	}
	cli_drv_rows_pr(rows, nrows);

	return nrefused ? 1 : 0;
}

static int
cli_env(struct xnvme_cli *cli, int argc, char **argv);

static int
cli_driver(struct xnvme_cli *cli, int argc, char **argv)
{
	char *rest[CLI_CTRLRS_MAX + 8];
	const char *be = NULL, *driver;
	int nrest = 0, err;

	for (int i = 0; i < argc; ++i) {
		if (!strcmp(argv[i], "--be") && i + 1 < argc) {
			be = argv[++i];
		} else if (!strcmp(argv[i], "--all") || !strcmp(argv[i], "--force") ||
			   !strcmp(argv[i], "--dry-run") || argv[i][0] != '-') {
			if (nrest < (int)(sizeof rest / sizeof(*rest))) {
				rest[nrest++] = argv[i];
			}
		} else {
			printf("Usage: xnvme driver [--be <backend>] [<bdf>... | --all] [--force] "
			       "[--dry-run]\n\n");
			printf("Without --be, print 'xnvme list' and 'xnvme env': each function "
			       "with "
			       "its\n");
			printf("driver, and the host setup the drivers depend on.\n\n");
			printf("With --be, put each named function, or with --all every function "
			       "not in use,\n");
			printf("on the driver the backend needs: the kernel nvme driver for the "
			       "kernel\n");
			printf("backends, a userspace driver for spdk, libvfn and upcie. A "
			       "function "
			       "whose\n");
			printf("namespaces are in use is refused; --force overrides only the "
			       "refusal "
			       "of a\n");
			printf("namespace carrying a filesystem or partition table. --dry-run "
			       "changes nothing.\n");
			return strcmp(argv[i], "--help") ? 1 : 0;
		}
	}

	if (!be) {
		if (nrest) {
			fprintf(stderr, "xnvme driver: name the backend with --be\n");
			return 1;
		}
		cli_list(0, NULL);
		printf("\n");
		return cli_env(cli, 0, NULL);
	}

	driver = xnvme_driver_for_be(be);
	if (!driver) {
		fprintf(stderr, "xnvme driver: %s\n",
			errno == ENOENT   ? "no such backend; 'xnvme env library' lists them"
			: errno == EINVAL ? "the backend uses no NVMe controller function"
			: errno == ENOSYS ? "not supported on this platform"
					  : strerror(errno));
		return 1;
	}

	if (!strcmp(driver, "nvme")) {
		err = cli_driver_detach(nrest, rest);
#ifdef XNVME_PLATFORM_LINUX_ENABLED
		if (!strncmp(be, "io_uring", strlen("io_uring")) &&
		    cli_mod_param("nvme", "poll_queues") == 0) {
			printf("hint: '%s'\n", cli_yq(g_cli_poll_hint));
		}
#endif
		return err;
	}

	// A function on another userspace driver goes back to nvme on its way to 'driver'
	{
		static struct cli_ctrlr ctrlrs[CLI_CTRLRS_MAX];
		int count = cli_ctrlrs(ctrlrs, CLI_CTRLRS_MAX);
		bool all = false, dry_run = false;

		for (int i = 0; i < nrest; ++i) {
			all |= !strcmp(rest[i], "--all");
			dry_run |= !strcmp(rest[i], "--dry-run");
		}
		for (int i = 0; !dry_run && i < count; ++i) {
			struct cli_ctrlr *c = &ctrlrs[i];
			struct xnvme_driver_result r;
			const char *bdf = c->bdf;
			bool named = all;

			for (int j = 0; !named && j < nrest; ++j) {
				char want[32];

				cli_bdf(rest[j], want, sizeof(want));
				named = !strcmp(want, c->bdf);
			}
			if (named && c->userspace && strcmp(c->driver, driver)) {
				xnvme_driver_detach(&bdf, 1, NULL, 0, &r);
			}
		}
	}

	return cli_driver_attach(nrest, rest, driver);
}

#ifdef XNVME_PLATFORM_LINUX_ENABLED
/**
 * Print the NVMe controller functions, and how many are on each driver
 */
static void
cli_env_ctrlrs(void)
{
	static struct cli_ctrlr ctrlrs[CLI_CTRLRS_MAX];
	const char *drivers[16];
	int counts[16] = {0};
	int ndrivers = 0;
	int count = cli_scan(ctrlrs, CLI_CTRLRS_MAX);

	for (int i = 0; i < count; ++i) {
		const char *driver = ctrlrs[i].driver[0] ? ctrlrs[i].driver : "null";
		int d;

		for (d = 0; d < ndrivers && strcmp(drivers[d], driver); ++d) {
			;
		}
		if (d == ndrivers && ndrivers < 16) {
			drivers[ndrivers++] = driver;
		}
		counts[d < 16 ? d : 15] += 1;
	}
	printf("  nvme: {controllers: %d, drivers: {", count > 0 ? count : 0);
	for (int d = 0; d < ndrivers; ++d) {
		printf("%s'%s': %d", d ? ", " : "", cli_yq(drivers[d]), counts[d]);
	}
	printf("}}\n");
}
#endif
#else
static int
cli_list(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	return -1;
}

static int
cli_env(struct xnvme_cli *cli, int argc, char **argv);

static int
cli_driver(struct xnvme_cli *cli, int argc, char **argv)
{
	int err = xnvme_driver_attach(NULL, 0, NULL, 0, NULL);

	(void)argv;
	if (!argc) {
		return cli_env(cli, 0, NULL);
	}
	fprintf(stderr, "xnvme driver: %s\n",
		err == -ENOSYS ? "not supported on this platform" : strerror(-err));

	return 1;
}
#endif

static void
cli_env_library(struct xnvme_cli *cli)
{
	xnvme_ver_pr(XNVME_PR_DEF);
	printf("\nexec_path: '%s'\n", cli_exec_path());
	printf("\n");
	xnvme_libconf_pr(xnvme_libconf_get(), XNVME_PR_DEF);
	printf("\n");
	xnvme_be_attr_list_pr(XNVME_PR_DEF);
	if (cli->vcs && strcmp(cli->vcs, xnvme_ver_vcs())) {
		fprintf(stderr, "xnvme: WARNING: built from '%s', libxnvme from '%s'\n", cli->vcs,
			xnvme_ver_vcs());
	}
}

static int
cli_env(struct xnvme_cli *cli, int argc, char **argv)
{
	bool system = true, library = true;

	for (int i = 0; i < argc; ++i) {
		if (!strcmp(argv[i], "system")) {
			library = false;
		} else if (!strcmp(argv[i], "library")) {
			system = false;
		} else {
			FILE *out = strcmp(argv[i], "--help") ? stderr : stdout;

			fprintf(out, "Usage: xnvme env [system|library]\n\n");
			fprintf(out, "As YAML, what the drivers depend on, such as the IOMMU,\n");
			fprintf(out, "the kernel modules, hugepages and memlock, the NVMe\n");
			fprintf(out, "controllers and the queues of the kernel nvme driver,\n");
			fprintf(out, "each with a fix where something is missing; then the\n");
			fprintf(out, "library version, build configuration and backends.\n");
			fprintf(out, "'system' or 'library' prints one of the two.\n");
			return out == stdout ? 0 : 1;
		}
	}

	if (system) {
		printf("system:\n");
		cli_env_system();
	}
	if (library) {
		if (system) {
			printf("\n");
		}
		cli_env_library(cli);
	}

	return 0;
}

/**
 * 'xnvme cmd-admin', 'cmd-io' and 'cmd-iov': a raw command, spelled as nvme-cli's
 * admin-passthru and io-passthru spell it, so a command line moves between the two by
 * changing the program and the subcommand. The device is an xNVMe URI, so it reaches every
 * path xNVMe has; the options choosing the path are long options only, and take no letter
 * nvme-cli uses.
 */
#include <getopt.h>

enum cli_cmd_kind {
	CLI_CMD_ADMIN,
	CLI_CMD_IO,
	CLI_CMD_IOV,
};

enum {
	CLI_OPT_BE = 0x100,
	CLI_OPT_ADMIN,
	CLI_OPT_SYNC,
	CLI_OPT_ASYNC,
	CLI_OPT_DEV_NSID,
	CLI_OPT_HOMI_ID,
	CLI_OPT_VEC_CNT,
	CLI_OPT_NO_RETRIES,
	CLI_OPT_OFV,
	CLI_OPT_HELP,
};

static const struct option g_cli_cmd_opts[] = {
	// nvme-cli 2.16 admin-passthru and io-passthru, which share one set
	{"opcode", required_argument, NULL, 'O'},
	{"flags", required_argument, NULL, 'f'},
	{"prefill", required_argument, NULL, 'p'},
	{"rsvd", required_argument, NULL, 'R'},
	{"namespace-id", required_argument, NULL, 'n'},
	{"data-len", required_argument, NULL, 'l'},
	{"metadata-len", required_argument, NULL, 'm'},
	{"cdw2", required_argument, NULL, '2'},
	{"cdw3", required_argument, NULL, '3'},
	{"cdw10", required_argument, NULL, '4'},
	{"cdw11", required_argument, NULL, '5'},
	{"cdw12", required_argument, NULL, '6'},
	{"cdw13", required_argument, NULL, '7'},
	{"cdw14", required_argument, NULL, '8'},
	{"cdw15", required_argument, NULL, '9'},
	{"input-file", required_argument, NULL, 'i'},
	{"metadata", required_argument, NULL, 'M'},
	{"raw-binary", no_argument, NULL, 'b'},
	{"show-command", no_argument, NULL, 's'},
	{"dry-run", no_argument, NULL, 'd'},
	{"read", no_argument, NULL, 'r'},
	{"write", no_argument, NULL, 'w'},
	{"latency", no_argument, NULL, 'T'},
	{"timeout", required_argument, NULL, 't'},
	{"output-format", required_argument, NULL, 'o'},
	{"verbose", no_argument, NULL, 'v'},
	{"no-retries", no_argument, NULL, CLI_OPT_NO_RETRIES},
	{"output-format-version", required_argument, NULL, CLI_OPT_OFV},
	// xNVMe: the path, long options only
	{"be", required_argument, NULL, CLI_OPT_BE},
	{"admin", required_argument, NULL, CLI_OPT_ADMIN},
	{"sync", required_argument, NULL, CLI_OPT_SYNC},
	{"async", required_argument, NULL, CLI_OPT_ASYNC},
	{"dev-nsid", required_argument, NULL, CLI_OPT_DEV_NSID},
	{"homi-id", required_argument, NULL, CLI_OPT_HOMI_ID},
	{"shm_id", required_argument, NULL, CLI_OPT_HOMI_ID}, ///< One id, as in xnvme_cli
	{"vec-cnt", required_argument, NULL, CLI_OPT_VEC_CNT},
	{"help", no_argument, NULL, CLI_OPT_HELP},
	{NULL, 0, NULL, 0},
};

static void
cli_cmd_usage(const char *name, const char *counterpart)
{
	printf("Usage: xnvme %s <uri> [OPTIONS]\n\n", name);
	printf("Send a user-defined command to the device at <uri>, any xNVMe URI: a\n");
	printf("character or block device, a BDF, or a fabrics address. The options are\n");
	printf("those of %s, with the same names and letters:\n\n", counterpart);
	printf("  -O, --opcode=NUM       -f, --flags=NUM        -p, --prefill=NUM\n");
	printf("  -R, --rsvd=NUM         -n, --namespace-id=NUM -l, --data-len=NUM\n");
	printf("  -m, --metadata-len=NUM -2, --cdw2=NUM         -3, --cdw3=NUM\n");
	printf("  -4 .. -9, --cdw10=NUM .. --cdw15=NUM          -i, --input-file=FILE\n");
	printf("  -M, --metadata=FILE    -b, --raw-binary       -s, --show-command\n");
	printf("  -d, --dry-run          -r, --read             -w, --write\n");
	printf("  -T, --latency          -t, --timeout=NUM      -o, --output-format=FMT\n");
	printf("  -v, --verbose          --no-retries\n\n");
	printf("and, choosing the path: --be, --admin, --sync, --async, --dev-nsid and\n");
	printf("--homi-id or --shm_id%s.\n", strcmp(name, "cmd-iov")
						     ? ""
						     : ", and --vec-cnt=NUM, the number of equal "
						       "segments the data is sent in");
}

static uint64_t
cli_num(const char *arg, bool *ok)
{
	char *end = NULL;
	unsigned long long val;

	errno = 0;
	val = strtoull(arg, &end, 0);
	*ok = *ok && !errno && end && end != arg && !*end;

	return val;
}

static void
cli_hexdump(const uint8_t *buf, size_t nbytes)
{
	printf("       0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	for (size_t ofs = 0; ofs < nbytes; ofs += 16) {
		printf("%04zx:", ofs);
		for (size_t i = 0; i < 16; ++i) {
			if (ofs + i < nbytes) {
				printf(" %02x", buf[ofs + i]);
			} else {
				printf("   ");
			}
		}
		printf(" \"");
		for (size_t i = 0; i < 16 && ofs + i < nbytes; ++i) {
			uint8_t c = buf[ofs + i];

			putchar(c >= 0x20 && c < 0x7f ? c : '.');
		}
		printf("\"\n");
	}
}

static int
cli_file_load(const char *path, void *buf, size_t nbytes)
{
	FILE *fp = path ? fopen(path, "rb") : stdin;
	size_t nread;

	if (!fp) {
		return -errno;
	}
	nread = fread(buf, 1, nbytes, fp);
	if (path) {
		fclose(fp);
	}

	return nread == nbytes ? 0 : -EIO;
}

static int
cli_file_save(const char *path, const void *buf, size_t nbytes)
{
	FILE *fp = fopen(path, "wb");
	size_t nwritten;

	if (!fp) {
		return -errno;
	}
	nwritten = fwrite(buf, 1, nbytes, fp);
	fclose(fp);

	return nwritten == nbytes ? 0 : -EIO;
}

static int
cli_cmd(enum cli_cmd_kind kind, int argc, char **argv)
{
	const char *name =
		kind == CLI_CMD_ADMIN ? "cmd-admin" : (kind == CLI_CMD_IO ? "cmd-io" : "cmd-iov");
	const char *counterpart =
		kind == CLI_CMD_ADMIN ? "nvme admin-passthru" : "nvme io-passthru";
	struct xnvme_opts opts = xnvme_opts_default();
	uint64_t cdw[16] = {0}, opcode = 0, flags = 0, prefill = 0, rsvd = 0, data_len = 0,
		 meta_len = 0, vec_cnt = 1;
	const char *input = NULL, *metafile = NULL, *ofmt = "normal", *uri;
	bool have_opcode = false, raw = false, show = false, dry = false, rd = false, wr = false,
	     latency = false, ok = true;
	struct xnvme_dev *dev = NULL;
	struct xnvme_cmd_ctx ctx;
	uint8_t *dbuf = NULL, *mbuf = NULL;
	struct iovec *iov = NULL;
	struct xnvme_timer timer = {0};
	uint32_t *dwords;
	int opt, err = 0;

	optind = 1;
	opterr = 1;
	while ((opt = getopt_long(argc, argv, "O:f:p:R:n:l:m:2:3:4:5:6:7:8:9:i:M:bsdrwTt:o:v",
				  g_cli_cmd_opts, NULL)) != -1) {
		switch (opt) {
		case 'O':
			opcode = cli_num(optarg, &ok);
			have_opcode = true;
			break;
		case 'f':
			flags = cli_num(optarg, &ok);
			break;
		case 'p':
			prefill = cli_num(optarg, &ok);
			break;
		case 'R':
			rsvd = cli_num(optarg, &ok);
			break;
		case 'n':
			cdw[1] = cli_num(optarg, &ok);
			break;
		case 'l':
			data_len = cli_num(optarg, &ok);
			break;
		case 'm':
			meta_len = cli_num(optarg, &ok);
			break;
		case '2':
		case '3':
			cdw[opt - '0'] = cli_num(optarg, &ok);
			break;
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			cdw[opt - '4' + 10] = cli_num(optarg, &ok);
			break;
		case 'i':
			input = optarg;
			break;
		case 'M':
			metafile = optarg;
			break;
		case 'b':
			raw = true;
			break;
		case 's':
			show = true;
			break;
		case 'd':
			dry = true;
			break;
		case 'r':
			rd = true;
			break;
		case 'w':
			wr = true;
			break;
		case 'T':
			latency = true;
			break;
		case 't':
			fprintf(stderr,
				"xnvme %s: --timeout is not supported: xNVMe's synchronous "
				"command path has no per-command timeout\n",
				name);
			return 1;
		case 'o':
			ofmt = optarg;
			break;
		case 'v':
		case CLI_OPT_NO_RETRIES:
		case CLI_OPT_OFV:
			break;
		case CLI_OPT_BE:
			opts.be = optarg;
			break;
		case CLI_OPT_ADMIN:
			opts.admin = optarg;
			break;
		case CLI_OPT_SYNC:
			opts.sync = optarg;
			break;
		case CLI_OPT_ASYNC:
			opts.async = optarg;
			break;
		case CLI_OPT_DEV_NSID:
			opts.nsid = (uint32_t)cli_num(optarg, &ok);
			break;
		case CLI_OPT_HOMI_ID:
			// As xnvme_cli fills both: SPDK and multi-process uPCIe read shm_id
			opts.homi_id = (uint32_t)cli_num(optarg, &ok);
			opts.shm_id = opts.homi_id;
			break;
		case CLI_OPT_VEC_CNT:
			vec_cnt = cli_num(optarg, &ok);
			break;
		case CLI_OPT_HELP:
			cli_cmd_usage(name, counterpart);
			return 0;
		default:
			cli_cmd_usage(name, counterpart);
			return 1;
		}
	}
	if (!ok || optind != argc - 1 || !have_opcode || opcode > 0xFF || flags > 0xFF ||
	    prefill > 0xFF) {
		fprintf(stderr,
			"xnvme %s: needs <uri> and --opcode, and numbers that fit; see "
			"'xnvme %s --help'\n",
			name, name);
		return 1;
	}
	if (rsvd) {
		fprintf(stderr,
			"xnvme %s: --rsvd is not supported: an xNVMe command has no field "
			"for it\n",
			name);
		return 1;
	}
	if (strcmp(ofmt, "normal") && strcmp(ofmt, "binary")) {
		fprintf(stderr,
			"xnvme %s: --output-format '%s' is not supported; normal or binary\n",
			name, ofmt);
		return 1;
	}
	if (kind == CLI_CMD_IOV && (!vec_cnt || !data_len || data_len % vec_cnt)) {
		fprintf(stderr, "xnvme %s: --data-len must split into --vec-cnt equal segments\n",
			name);
		return 1;
	}
	raw = raw || !strcmp(ofmt, "binary");
	uri = argv[optind];

	cdw[0] = opcode | (flags & 0x3) << 8;
	if (show || dry) {
		printf("opcode       : 0x%02" PRIx64 "\n", opcode);
		printf("flags        : 0x%02" PRIx64 "\n", flags);
		printf("nsid         : 0x%08" PRIx64 "\n", cdw[1]);
		printf("cdw2         : 0x%08" PRIx64 "\n", cdw[2]);
		printf("cdw3         : 0x%08" PRIx64 "\n", cdw[3]);
		printf("data_len     : 0x%08" PRIx64 "\n", data_len);
		printf("metadata_len : 0x%08" PRIx64 "\n", meta_len);
		for (int i = 10; i <= 15; ++i) {
			printf("cdw%-10d: 0x%08" PRIx64 "\n", i, cdw[i]);
		}
	}
	if (dry) {
		return 0;
	}

	dev = xnvme_dev_open(uri, &opts);
	if (!dev) {
		fprintf(stderr, "xnvme %s: opening '%s' failed: %s\n", name, uri, strerror(errno));
		return 1;
	}
	if (data_len) {
		dbuf = xnvme_buf_alloc(dev, data_len);
		if (!dbuf) {
			err = -ENOMEM;
			goto exit;
		}
		memset(dbuf, (int)prefill, data_len);
		if (wr) {
			err = cli_file_load(input, dbuf, data_len);
			if (err) {
				fprintf(stderr, "xnvme %s: reading the data failed: %s\n", name,
					strerror(-err));
				goto exit;
			}
		}
	}
	if (meta_len) {
		mbuf = xnvme_buf_alloc(dev, meta_len);
		if (!mbuf) {
			err = -ENOMEM;
			goto exit;
		}
		memset(mbuf, (int)prefill, meta_len);
		if (wr && metafile) {
			err = cli_file_load(metafile, mbuf, meta_len);
			if (err) {
				goto exit;
			}
		}
	}

	ctx = xnvme_cmd_ctx_from_dev(dev);
	dwords = (uint32_t *)&ctx.cmd;
	for (int i = 0; i < 16; ++i) {
		if (i < 4 || i >= 10) {
			dwords[i] = (uint32_t)cdw[i];
		}
	}

	xnvme_timer_start(&timer);
	switch (kind) {
	case CLI_CMD_ADMIN:
		err = xnvme_cmd_pass_admin(&ctx, dbuf, data_len, mbuf, meta_len);
		break;
	case CLI_CMD_IO:
		err = xnvme_cmd_pass(&ctx, dbuf, data_len, mbuf, meta_len);
		break;
	case CLI_CMD_IOV:
		iov = calloc(vec_cnt, sizeof(*iov));
		if (!iov) {
			err = -ENOMEM;
			break;
		}
		for (uint64_t i = 0; i < vec_cnt; ++i) {
			iov[i].iov_base = dbuf + i * (data_len / vec_cnt);
			iov[i].iov_len = data_len / vec_cnt;
		}
		err = xnvme_cmd_pass_iov(&ctx, iov, vec_cnt, data_len, mbuf, meta_len);
		break;
	}
	xnvme_timer_stop(&timer);

	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		fprintf(stderr, "xnvme %s: the command failed: err(%d), sct(0x%x), sc(0x%x)\n",
			name, err, ctx.cpl.status.sct, ctx.cpl.status.sc);
		err = err ? err : -EIO;
		goto exit;
	}
	if (!raw) {
		printf("%s Command is Success and result: 0x%08x\n",
		       kind == CLI_CMD_ADMIN ? "Admin" : "IO", ctx.cpl.cdw0);
	}
	if (latency) {
		printf("latency: %" PRIu64 " us\n", (uint64_t)xnvme_timer_elapsed_usecs(&timer));
	}
	if (rd && dbuf) {
		if (input) {
			err = cli_file_save(input, dbuf, data_len);
		} else if (raw) {
			fwrite(dbuf, 1, data_len, stdout);
		} else {
			cli_hexdump(dbuf, data_len);
		}
	}
	if (rd && mbuf && metafile) {
		err = err ? err : cli_file_save(metafile, mbuf, meta_len);
	}

exit:
	free(iov);
	xnvme_buf_free(dev, mbuf);
	xnvme_buf_free(dev, dbuf);
	xnvme_dev_close(dev);

	return err ? 1 : 0;
}

/**
 * Handle what the front-end owns: the overview, the groups, '--exec-path', and the
 * commands run by other programs
 *
 * @return The exit status of a command the front-end handled, or -1 for a command it leaves
 * to xnvme_cli_run(), a built-in subcommand or one that does not exist
 */
static int
cli_main(struct xnvme_cli *cli, int argc, char **argv)
{
	const char *cmd = argc > 1 ? argv[1] : NULL;
	char path[4096];

	if (!cmd) {
		return cli_overview(cli);
	}
	if (!strcmp(cmd, "list")) {
		// Off Linux, cli_list() returns -1 and the built-in 'list' enumerates instead
		return cli_list(argc - 2, argv + 2);
	}
	if (!strcmp(cmd, "driver")) {
		return cli_driver(cli, argc - 2, argv + 2);
	}
	if (!strcmp(cmd, "cmd-admin")) {
		return cli_cmd(CLI_CMD_ADMIN, argc - 1, argv + 1);
	}
	if (!strcmp(cmd, "cmd-io")) {
		return cli_cmd(CLI_CMD_IO, argc - 1, argv + 1);
	}
	if (!strcmp(cmd, "cmd-iov")) {
		return cli_cmd(CLI_CMD_IOV, argc - 1, argv + 1);
	}
	if (cli_builtin(cli, cmd)) {
		return -1;
	}
	if (!strcmp(cmd, "env")) {
		return cli_env(cli, argc - 2, argv + 2);
	}
	if (!strcmp(cmd, "--exec-path")) {
		printf("%s\n", cli_exec_path());
		return 0;
	}
	if (!strcmp(cmd, "cmd")) {
		printf("Commands in 'xnvme cmd':\n\n");
		cli_row("cmd-admin", "An admin command, as 'nvme admin-passthru' takes it");
		cli_row("cmd-io", "An I/O command, as 'nvme io-passthru' takes it");
		cli_row("cmd-iov", "An I/O command with its data in --vec-cnt segments");
		printf("\nSee 'xnvme <command> --help' for the description of [<args>]\n");
		return 0;
	}
	if (!strcmp(cmd, "test")) {
		return argc > 2 && strcmp(argv[2], "--help")
			       ? cli_dispatch(CLI_TEST_PREFIX, "test ", argv[2], argc - 3,
					      argv + 3)
			       : cli_group_programs("test", CLI_TEST_PREFIX);
	}
	if (!strcmp(cmd, "example")) {
		return argc > 2 && strcmp(argv[2], "--help")
			       ? cli_dispatch(CLI_EXAMPLE_PREFIX, "example ", argv[2], argc - 3,
					      argv + 3)
			       : cli_group_programs("example", CLI_EXAMPLE_PREFIX);
	}
	if (cmd[0] != '-' && !cli_resolve(CLI_PROG_PREFIX, cmd, path, sizeof(path))) {
		return cli_dispatch(CLI_PROG_PREFIX, "", cmd, argc - 2, argv + 2);
	}

	return -1;
}

int
main(int argc, char **argv)
{
	int err = cli_main(&g_cli, argc, argv);

	if (err >= 0) {
		return err;
	}

	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
