#define _XOPEN_SOURCE 700
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <libxnvmec.h>
#include <xnvme_be.h>
#include <libxnvme_file.h>

#define XNVMEC_SUB_MAXOPTS 64

const char *
xnvmec_opt_type_str(enum xnvmec_opt_type otype)
{
	switch (otype) {
	case XNVMEC_POSA:
		return "XNVMEC_POSA";
	case XNVMEC_LFLG:
		return "XNVMEC_LFLG";
	case XNVMEC_LOPT:
		return "XNVMEC_LOPT";
	case XNVMEC_LREQ:
		return "XNVMEC_LREQ";
	}

	return "XNVMEC_ENOSYS";
}

void *
xnvmec_buf_clear(void *buf, size_t nbytes)
{
	return memset(buf, 0, nbytes);
}

/**
 * Helper function writing/reading given buf to/from file. When mode has
 * O_WRONLY, the given buffer is written to file, otherwise, it is read.
 *
 * NOTE: this depends on POSIX syscalls
 */
static int
fdio_func(void *buf, size_t nbytes, const char *path, struct xnvme_opts *opts)
{
	struct xnvme_cmd_ctx ctx = {0};
	struct xnvme_dev *fh = NULL;
	size_t transferred = 0;
	int err = 0;

	fh = xnvme_file_open(path, opts);
	if (fh == NULL) {
		XNVME_DEBUG("FAILED: xnvme_file_open(), errno: %d", errno);
		return -errno;
	}

	ctx = xnvme_file_get_cmd_ctx(fh);

	while (transferred < nbytes) {
		const size_t remain = nbytes - transferred;
		size_t nbytes_call = remain < SSIZE_MAX ? remain : SSIZE_MAX;

		if (opts->wronly) {
			err = xnvme_file_pwrite(&ctx, buf + transferred, nbytes_call, transferred);
		} else {
			err = xnvme_file_pread(&ctx, buf + transferred, nbytes_call, transferred);
		}
		if (err) {
			XNVME_DEBUG("FAILED: opts->wronly: %d, cpl.result: %ld, errno: %d",
				    opts->wronly, ctx.cpl.result, errno);
			xnvme_file_close(fh);
			return -errno;
		}

		// when reading, break at end of file
		if (!opts->wronly && ctx.cpl.result == 0) {
			break;
		}

		transferred += ctx.cpl.result;
	}

	err = xnvme_file_close(fh);
	if (err) {
		XNVME_DEBUG("FAILED: opts->wronly: %d, err: %d, errno: %d", opts->wronly, err,
			    errno);
		return -errno;
	}

	return 0;
}

int
xnvmec_buf_from_file(void *buf, size_t nbytes, const char *path)
{
	struct xnvme_opts opts = {
		.rdonly = 1,
	};

	return fdio_func(buf, nbytes, path, &opts);
}

int
xnvmec_buf_to_file(void *buf, size_t nbytes, const char *path)
{
	struct xnvme_opts opts = {
		.create = 1,
		.wronly = 1,
		.create_mode = S_IWUSR | S_IRUSR,
	};

	return fdio_func(buf, nbytes, path, &opts);
}

int
xnvmec_buf_fill(void *buf, size_t nbytes, const char *content)
{
	uint8_t *cbuf = buf;

	if (!strncmp(content, "anum", 4)) {
		for (size_t i = 0; i < nbytes; ++i) {
			cbuf[i] = (i % 26) + 65;
		}

		return 0;
	}

	if (!strncmp(content, "rand-t", 6)) {
		srand(time(NULL));
		for (size_t i = 0; i < nbytes; ++i) {
			cbuf[i] = (rand() % 26) + 65;
		}

		return 0;
	}

	if (!strncmp(content, "rand-k", 6)) {
		srand(0);
		for (size_t i = 0; i < nbytes; ++i) {
			cbuf[i] = (rand() % 26) + 65;
		}

		return 0;
	}

	if (!strncmp(content, "ascii", 5)) {
		for (size_t i = 0; i < nbytes; ++i) {
			cbuf[i] = (i % 26) + 65;
		}

		return 0;
	}

	if (!strncmp(content, "zero", 4)) {
		xnvmec_buf_clear(buf, nbytes);

		return 0;
	}

	return xnvmec_buf_from_file(buf, nbytes, content);
}

size_t
xnvmec_buf_diff(const void *expected, const void *actual, size_t nbytes)
{
	const uint8_t *exp = expected;
	const uint8_t *act = actual;
	size_t diff = 0;

	for (size_t i = 0; i < nbytes; ++i) {
		if (exp[i] == act[i]) {
			continue;
		}

		++diff;
	}

	return diff;
}

void
xnvmec_buf_diff_pr(const void *expected, const void *actual, size_t nbytes, int XNVME_UNUSED(opts))
{
	const uint8_t *exp = expected;
	const uint8_t *act = actual;
	size_t diff = 0;

	printf("comparison:\n");
	printf("  diffs:\n");
	for (size_t i = 0; i < nbytes; ++i) {
		if (exp[i] == act[i]) {
			continue;
		}

		++diff;
		printf("    - {byte: '%06lu', expected: 0x%x, actual: 0x%x)\n", i, exp[i], act[i]);
	}
	printf("  nbytes: %zu\n", nbytes);
	printf("  nbytes_diff: %zu\n", diff);
}

/**
 * Returns the minimum width required for sub-name, should be in the range
 * [0, XNVMEC_SUB_NAME_LEN_MAX]
 */
static inline size_t
sub_name_wmin(struct xnvmec *cli)
{
	size_t wmin = 0;

	for (int i = 0; i < cli->nsubs; ++i) {
		size_t len = 0;

		len = strnlen(cli->subs[i].name, XNVMEC_SUB_NAME_LEN_MAX);
		wmin = len > wmin ? len : wmin;
	}

	return wmin;
}

struct xnvmec_sub *
sub_by_name(struct xnvmec *cli, const char *name)
{
	for (int i = 0; i < cli->nsubs; ++i) {
		if (!cli->subs[i].name) {
			break;
		}
		if (!strcmp(cli->subs[i].name, name)) {
			return &cli->subs[i];
		}
	}

	return NULL;
}

// TODO: finish this printer function
void
xnvmec_args_pr(struct xnvmec_args *args, int opts)
{
	xnvmec_pinf("opts: %d", opts);

	for (int i = 0; i < 16; ++i) {
		printf("cdw%i: 0x%x\n", i, args->cdw[i]);
	}

	printf("uri: '%s'\n", args->uri);
	printf("sys_uri: '%s'\n", args->sys_uri);

	printf("fid: 0x%x\n", args->fid);
	printf("feat: 0x%x\n", args->feat);

	printf("status: %d\n", args->status);
	printf("save: %d\n", args->save);
	printf("reset: %d\n", args->reset);
	printf("verbose: %d\n", args->verbose);
	printf("help: %d\n", args->help);
}

const char *
xnvmec_opt_value_type_str(int vtype)
{
	switch (vtype) {
	case XNVMEC_OPT_VTYPE_URI:
		return "uri";
	case XNVMEC_OPT_VTYPE_NUM:
		return "NUM";
	case XNVMEC_OPT_VTYPE_HEX:
		return "0xNUM";
	case XNVMEC_OPT_VTYPE_FILE:
		return "FILE";
	case XNVMEC_OPT_VTYPE_STR:
		return "STRING";
	}

	return "ENOSYS";
}

static struct xnvmec_opt_attr xnvmec_opts[] = {
	{
		.opt = XNVMEC_OPT_CDW00,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "cdw0",
		.descr = "Command Dword0",
	},
	{
		.opt = XNVMEC_OPT_CDW01,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "cdw1",
		.descr = "Command Dword1",
	},
	{
		.opt = XNVMEC_OPT_CDW02,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "cdw2",
		.descr = "Command Dword2",
	},
	{
		.opt = XNVMEC_OPT_CDW03,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "cdw3",
		.descr = "Command Dword3",
	},
	{
		.opt = XNVMEC_OPT_CDW04,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "cdw4",
		.descr = "Command Dword4",
	},
	{
		.opt = XNVMEC_OPT_CDW05,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "cdw5",
		.descr = "Command Dword5",
	},
	{
		.opt = XNVMEC_OPT_CDW06,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "cdw6",
		.descr = "Command Dword6",
	},
	{
		.opt = XNVMEC_OPT_CDW07,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "cdw7",
		.descr = "Command Dword7",
	},
	{
		.opt = XNVMEC_OPT_CDW08,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "cdw8",
		.descr = "Command Dword8",
	},
	{
		.opt = XNVMEC_OPT_CDW09,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "cdw9",
		.descr = "Command Dword9",
	},
	{
		.opt = XNVMEC_OPT_CDW10,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "cdw10",
		.descr = "Command Dword10",
	},
	{
		.opt = XNVMEC_OPT_CDW11,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "cdw11",
		.descr = "Command Dword11",
	},
	{
		.opt = XNVMEC_OPT_CDW12,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "cdw12",
		.descr = "Command Dword12",
	},
	{
		.opt = XNVMEC_OPT_CDW13,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "cdw13",
		.descr = "Command Dword13",
	},
	{
		.opt = XNVMEC_OPT_CDW14,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "cdw14",
		.descr = "Command Dword14",
	},
	{
		.opt = XNVMEC_OPT_CDW15,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "cdw15",
		.descr = "Command Dword15",
	},
	{
		.opt = XNVMEC_OPT_CMD_INPUT,
		.vtype = XNVMEC_OPT_VTYPE_FILE,
		.name = "cmd-input",
		.descr = "Path to command input-file",
	},
	{
		.opt = XNVMEC_OPT_CMD_OUTPUT,
		.vtype = XNVMEC_OPT_VTYPE_FILE,
		.name = "cmd-output",
		.descr = "Path to command output-file",
	},
	{
		.opt = XNVMEC_OPT_DATA_NBYTES,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "data-nbytes",
		.descr = "Data size in bytes",
	},
	{
		.opt = XNVMEC_OPT_DATA_INPUT,
		.vtype = XNVMEC_OPT_VTYPE_FILE,
		.name = "data-input",
		.descr = "Path to data input-file",
	},
	{
		.opt = XNVMEC_OPT_DATA_OUTPUT,
		.vtype = XNVMEC_OPT_VTYPE_FILE,
		.name = "data-output",
		.descr = "Path to data output-file",
	},
	{
		.opt = XNVMEC_OPT_META_NBYTES,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "meta-nbytes",
		.descr = "Meta size in bytes",
	},
	{
		.opt = XNVMEC_OPT_META_INPUT,
		.vtype = XNVMEC_OPT_VTYPE_FILE,
		.name = "meta-input",
		.descr = "Path to meta input-file",
	},
	{
		.opt = XNVMEC_OPT_META_OUTPUT,
		.vtype = XNVMEC_OPT_VTYPE_FILE,
		.name = "meta-output",
		.descr = "Path to meta output-file",
	},
	{
		.opt = XNVMEC_OPT_LBAF,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "lbaf",
		.descr = "LBA Format",
	},
	{
		.opt = XNVMEC_OPT_SLBA,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "slba",
		.descr = "Start Logical Block Address",
	},
	{
		.opt = XNVMEC_OPT_ELBA,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "elba",
		.descr = "End Logical Block Address",
	},
	{
		.opt = XNVMEC_OPT_LBA,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "lba",
		.descr = "Logical Block Address",
	},
	{
		.opt = XNVMEC_OPT_NLB,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "nlb",
		.descr = "Number of LBAs (NOTE: zero-based value)",
	},
	{
		.opt = XNVMEC_OPT_URI,
		.vtype = XNVMEC_OPT_VTYPE_URI,
		.name = "uri",
		.descr = "Device URI e.g. '/dev/nvme0n1', '0000:01:00.1', '10.9.8.1.8888', "
			 "'\\\\.\\PhysicalDrive1'",
	},
	{
		.opt = XNVMEC_OPT_SYS_URI,
		.vtype = XNVMEC_OPT_VTYPE_URI,
		.name = "uri",
		.descr = "System URI e.g. '10.9.8.1:8888'",
	},
	{
		.opt = XNVMEC_OPT_CNTID,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "cntid",
		.descr = "Controller Identifier",
	},
	{
		.opt = XNVMEC_OPT_NSID,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "nsid",
		.descr = "Namespace Identifier for Command Construction",
	},
	{
		.opt = XNVMEC_OPT_UUID,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "uuid",
		.descr = "Universally Unique Identifier",
	},
	{
		.opt = XNVMEC_OPT_CNS,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "cns",
		.descr = "Controller or Namespace Struct",
	},
	{
		.opt = XNVMEC_OPT_CSI,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "csi",
		.descr = "Command Set Identifier",
	},
	{
		.opt = XNVMEC_OPT_INDEX,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "index",
		.descr = "Index",
	},
	{
		.opt = XNVMEC_OPT_SETID,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "setid",
		.descr = "NVM Set Identifier",
	},
	{
		.opt = XNVMEC_OPT_LPO_NBYTES,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "lpo-nbytes",
		.descr = "Log-Page Offset (in bytes)",
	},
	{
		.opt = XNVMEC_OPT_LID,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "lid",
		.descr = "Log-page IDentifier",
	},
	{
		.opt = XNVMEC_OPT_LSP,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "lsp",
		.descr = "Log-SPecific parameters",
	},
	{
		.opt = XNVMEC_OPT_RAE,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "rae",
		.descr = "Reset Async. Events",
	},
	{
		.opt = XNVMEC_OPT_ZF,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "zf",
		.descr = "Zone Format",
	},
	{
		.opt = XNVMEC_OPT_SES,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "ses",
		.descr = "Secure Erase setting. No=0x0, UserData=0x1, Cryptographic=0x2",
	},
	{
		.opt = XNVMEC_OPT_SEL,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "sel",
		.descr = "current=0x0, default=0x1, saved=0x2, supported=0x3",
	},
	{
		.opt = XNVMEC_OPT_MSET,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "mset",
		.descr = "Metadata settings. Off=0x0, On=0x1",
	},
	{
		.opt = XNVMEC_OPT_AUSE,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "ause",
		.descr = "AUSE?",
	},
	{
		.opt = XNVMEC_OPT_OVRPAT,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "ovrpat",
		.descr = "Overwrite Pattern",
	},
	{
		.opt = XNVMEC_OPT_OWPASS,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "owpass",
		.descr = "Overwrite Passes",
	},
	{
		.opt = XNVMEC_OPT_OIPBP,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "oipbp",
		.descr = "Overwrite Inverse Bit Pattern",
	},
	{
		.opt = XNVMEC_OPT_NODAS,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "nodas",
		.descr = "Nodas?",
	},
	{
		.opt = XNVMEC_OPT_ACTION,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "action",
		.descr = "Command action",
	},
	{
		.opt = XNVMEC_OPT_ZRMS,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "zrms",
		.descr = "Zone Resource Management",
	},
	{
		.opt = XNVMEC_OPT_PI,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "pi",
		.descr = "Protection Information. Off=0x0, Type1/2/3=0x1/0x2/0x3",
	},
	{
		.opt = XNVMEC_OPT_PIL,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "pil",
		.descr = "Protection Information Location. Last=0x0, First=0x1",
	},
	{
		.opt = XNVMEC_OPT_FID,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "fid",
		.descr = "Feature Identifier",
	},
	{
		.opt = XNVMEC_OPT_FEAT,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "feat",
		.descr = "Feature e.g. cdw12 content",
	},
	{
		.opt = XNVMEC_OPT_OPCODE,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "opcode",
		.descr = "Command opcode",
	},
	{
		.opt = XNVMEC_OPT_FLAGS,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "flags",
		.descr = "Command flags",
	},
	{
		.opt = XNVMEC_OPT_ALL,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "all",
		.descr = "Select / Affect all",
	},
	{
		.opt = XNVMEC_OPT_SEED,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "seed",
		.descr = "Use given 'NUM' as random seed",
	},
	{
		.opt = XNVMEC_OPT_LIMIT,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "limit",
		.descr = "Restrict amount to 'NUM'",
	},
	{
		.opt = XNVMEC_OPT_IOSIZE,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "iosize",
		.descr = "Use given 'NUM' as bs/iosize",
	},
	{
		.opt = XNVMEC_OPT_QDEPTH,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "qdepth",
		.descr = "Use given 'NUM' as queue max capacity",
	},
	{
		.opt = XNVMEC_OPT_DIRECT,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "direct",
		.descr = "Bypass layers",
	},
	{
		.opt = XNVMEC_OPT_COUNT,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "count",
		.descr = "Use given 'NUM' as count",
	},
	{
		.opt = XNVMEC_OPT_OFFSET,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "offset",
		.descr = "Use given 'NUM' as offset",
	},
	{
		.opt = XNVMEC_OPT_CLEAR,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "clear",
		.descr = "Clear something...",
	},
	{
		.opt = XNVMEC_OPT_STATUS,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "status",
		.descr = "Provide command state",
	},
	{
		.opt = XNVMEC_OPT_SAVE,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "save",
		.descr = "Save",
	},
	{
		.opt = XNVMEC_OPT_RESET,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "reset",
		.descr = "Reset controller",
	},
	{
		.opt = XNVMEC_OPT_VERBOSE,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "verbose",
		.descr = "Increase output info",
	},
	{
		.opt = XNVMEC_OPT_HELP,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "help",
		.descr = "Show usage / help",
	},
	{
		.opt = XNVMEC_OPT_DEV_NSID,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "dev-nsid",
		.descr = "Namespace Identifier for Device Handle",
	},
	{
		.opt = XNVMEC_OPT_BE,
		.vtype = XNVMEC_OPT_VTYPE_STR,
		.name = "be",
		.descr =
			"xNVMe backend, e.g. 'linux', 'spdk', 'fbsd', 'macos', 'posix', 'windows'",
	},
	{
		.opt = XNVMEC_OPT_MEM,
		.vtype = XNVMEC_OPT_VTYPE_STR,
		.name = "mem",
		.descr = "xNVMe buffer/memory manager",
	},
	{
		.opt = XNVMEC_OPT_SYNC,
		.vtype = XNVMEC_OPT_VTYPE_STR,
		.name = "sync",
		.descr = "xNVMe sync. command-interface, e.g. 'nvme', 'block'",
	},
	{
		.opt = XNVMEC_OPT_ASYNC,
		.vtype = XNVMEC_OPT_VTYPE_STR,
		.name = "async",
		.descr = "xNVMe async. command-interface, e.g. 'io_uring', 'emu'",
	},
	{
		.opt = XNVMEC_OPT_ADMIN,
		.vtype = XNVMEC_OPT_VTYPE_STR,
		.name = "admin",
		.descr = "xNVMe admin. command-interface, e.g. 'nvme', 'block'",
	},
	{
		.opt = XNVMEC_OPT_SHM_ID,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "shm_id",
		.descr = "For be=spdk, multi-process shared-memory-id",
	},
	{
		.opt = XNVMEC_OPT_MAIN_CORE,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "main_core",
		.descr = "For be=spdk, multi-process main core",
	},
	{
		.opt = XNVMEC_OPT_CORE_MASK,
		.vtype = XNVMEC_OPT_VTYPE_STR,
		.name = "core_mask",
		.descr = "For be=spdk, multi-process core_mask/cpus",
	},
	{
		.opt = XNVMEC_OPT_USE_CMB_SQS,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "use_cmb_sqs",
		.descr = "For be=spdk, use controller-memory-buffer for sqs",
	},
	{
		.opt = XNVMEC_OPT_CSS,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "css",
		.descr = "For be=spdk, ctrl.config. command-set-selection",
	},
	{
		.opt = XNVMEC_OPT_ADRFAM,
		.vtype = XNVMEC_OPT_VTYPE_STR,
		.name = "adrfam",
		.descr = "For be=spdk, Fabrics Address-Family e.g. IPv4/IPv6",
	},

	{
		.opt = XNVMEC_OPT_POLL_IO,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "poll_io",
		.descr = "For async=io_uring, enable hipri/io-compl.polling",
	},
	{
		.opt = XNVMEC_OPT_POLL_SQ,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "poll_sq",
		.descr = "For async=io_uring, enable kernel-side sqthread-poll",
	},
	{
		.opt = XNVMEC_OPT_REGISTER_FILES,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "register_files",
		.descr = "For async=io_uring, register files",
	},
	{
		.opt = XNVMEC_OPT_REGISTER_BUFFERS,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "register_buffers",
		.descr = "For async=io_uring, register buffers",
	},
	{
		.opt = XNVMEC_OPT_TRUNCATE,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "truncate",
		.descr = "For files; on-open truncate contents",
	},
	{
		.opt = XNVMEC_OPT_RDONLY,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "rdonly",
		.descr = "For files; open read-only",
	},
	{
		.opt = XNVMEC_OPT_WRONLY,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "wronly",
		.descr = "For files; open write-only",
	},
	{
		.opt = XNVMEC_OPT_RDWR,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "rdwr",
		.descr = "For files; open for read and write",
	},
	{
		.opt = XNVMEC_OPT_CREATE,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "create",
		.descr = "For files; on-open create",
	},
	{
		.opt = XNVMEC_OPT_OFLAGS,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "oflags",
		.descr = "For files; combination of file-open-flags",
	},

	{
		.opt = XNVMEC_OPT_CREATE_MODE,
		.vtype = XNVMEC_OPT_VTYPE_HEX,
		.name = "create_mode",
		.descr = "For files; create-mode / umask / mode_t in HEX",
	},

	{
		.opt = XNVMEC_OPT_VEC_CNT,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "vec-cnt",
		.descr = "Number of elements in vectors when doing vectored IOs",
	},

	{
		.opt = XNVMEC_OPT_END,
		.vtype = XNVMEC_OPT_VTYPE_NUM,
		.name = "",
		.descr = "",
	},
};

struct xnvmec_opt_attr *
xnvmec_opt_attr_by_opt(enum xnvmec_opt opt, struct xnvmec_opt_attr *attrs)
{
	for (int idx = 0; attrs[idx].opt; ++idx) {
		if (attrs[idx].opt != opt) {
			continue;
		}

		return &attrs[idx];
	}

	return NULL;
}

const struct xnvmec_opt_attr *
xnvmec_get_opt_attr(enum xnvmec_opt opt)
{
	return xnvmec_opt_attr_by_opt(opt, xnvmec_opts);
}

struct xnvmec_opt_attr *
xnvmec_opt_attr_by_getoptval(int getoptval, struct xnvmec_opt_attr *attrs)
{
	for (int idx = 0; attrs[idx].getoptval; ++idx) {
		if (attrs[idx].getoptval != getoptval) {
			continue;
		}

		return &attrs[idx];
	}

	return NULL;
}

void
xnvmec_pinf(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	printf("# ");
	vprintf(format, args);
	printf("\n");

	va_end(args);

	fflush(stdout);
}

#ifdef WIN32
#include <windows.h>
void
xnvmec_perr(const char *msg, int err)
{
	if (err < 0) {
		err = err * -1;
	}
	char err_msg_buff[1024];
	int count;

	count = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, (LPTSTR)&err_msg_buff,
			      sizeof(err_msg_buff), NULL);

	if (count != 0) {
		fprintf(stderr, "# ERR: '%s': {error code: %d, msg: '%s'}\n", msg, err,
			err_msg_buff);
	} else {
		fprintf(stderr, "# ERR: Format message failed:  {error code: %d}\n",
			GetLastError());
	}
}
#else
void
xnvmec_perr(const char *msg, int err)
{
	fprintf(stderr, "# ERR: '%s': {errno: %d, msg: '%s'}\n", msg, err,
		strerror(err < 0 ? -err : err));

	fflush(stderr);
}
#endif

void
xnvmec_usage_sub_long(struct xnvmec *cli, struct xnvmec_sub *sub)
{
	int nopts = 0;

	printf("Usage: %s %s ", cli->argv[0], sub->name);
	for (int oi = 0; oi < XNVMEC_SUB_OPTS_LEN; ++oi) {
		struct xnvmec_sub_opt *opt = &sub->opts[oi];
		struct xnvmec_opt_attr *attr = NULL;

		if ((opt->opt == XNVMEC_OPT_END) || (opt->opt == XNVMEC_OPT_NONE)) {
			break;
		}

		attr = xnvmec_opt_attr_by_opt(opt->opt, xnvmec_opts);
		if (!attr) {
			break;
		}
		if (opt->type && (opt->type != XNVMEC_POSA)) {
			++nopts;
			break;
		}

		printf("<%s> ", attr->name);
	}

	printf("[<args>]\n");

	if (strnlen(sub->descr_long, 2) > 1) {
		printf("\n%s\n", sub->descr_long);
	}

	printf("\n");
	printf("Where <args> include:\n\n");

	for (int oi = 0; oi < XNVMEC_SUB_OPTS_LEN; ++oi) {
		struct xnvmec_sub_opt *opt = &sub->opts[oi];
		struct xnvmec_opt_attr *attr = NULL;
		int width = 0;

		if ((opt->opt == XNVMEC_OPT_END) || (opt->opt == XNVMEC_OPT_NONE)) {
			break;
		}

		attr = xnvmec_opt_attr_by_opt(opt->opt, xnvmec_opts);
		if (!attr) {
			break;
		}

		printf("  ");
		switch (opt->type) {
		case XNVMEC_POSA:
			width = printf("%s", attr->name);
			break;
		case XNVMEC_LREQ:
			width = printf("--%s %s", attr->name,
				       xnvmec_opt_value_type_str(attr->vtype));
			break;
		case XNVMEC_LOPT:
			width = printf("[ --%s %s ]", attr->name,
				       xnvmec_opt_value_type_str(attr->vtype));
			break;
		case XNVMEC_LFLG:
			width = printf("[ --%s ]", attr->name);
			break;
		}

		printf("%*s; %s", 30 - width, "", attr->descr);
		printf("\n");
	}

	printf("\n");
	printf("See '%s --help' for other commands\n", cli->argv[0]);

	if (cli->title) {
		printf("\n");
		printf("%s -- ", cli->title);
		cli->ver_pr(XNVME_PR_DEF);
		printf("\n");
	}
}

void
xnvmec_usage_sub_short(struct xnvmec_sub *sub, size_t name_width)
{
	int name_len = strnlen(sub->name, XNVMEC_SUB_NAME_LEN_MAX);
	int fill = name_width - name_len;

	printf("  %s%*s | ", sub->name, fill + 1, "");

	if (strnlen(sub->descr_short, 2) > 1) {
		printf("%s", sub->descr_short);
	} else {
		printf("Undocumented");
	}

	printf("\n");
}

void
xnvmec_usage(struct xnvmec *cli)
{
	if (!cli) {
		return;
	}

	printf("Usage: %s <command> [<args>]\n", cli->argv[0]);

	printf("\n");
	printf("Where <command> is one of:\n");
	printf("\n");
	{
		size_t name_width = sub_name_wmin(cli);

		if (name_width < 15) {
			name_width = 15;
		}

		for (int si = 0; si < cli->nsubs; ++si) {
			xnvmec_usage_sub_short(&cli->subs[si], name_width);
		}
	}

	printf("\n");
	printf("See '%s <command> --help' for the description of [<args>]\n", cli->argv[0]);

	if (cli->title) {
		printf("\n");
		printf("%s -- ", cli->title);
		cli->ver_pr(XNVME_PR_DEF);
		printf("\n");
	}
}

int
xnvmec_assign_arg(struct xnvmec *cli, struct xnvmec_opt_attr *opt_attr, char *arg,
		  enum xnvmec_opt_type opt_type)
{
	struct xnvmec_args *args = &cli->args;
	char *endptr = NULL;
	uint64_t num = 0;

	// Check numerical args
	if (arg && (opt_type != XNVMEC_LFLG)) {
		int num_base = opt_attr->vtype == XNVMEC_OPT_VTYPE_NUM ? 10 : 16;

		switch (opt_attr->vtype) {
		case XNVMEC_OPT_VTYPE_URI:
		case XNVMEC_OPT_VTYPE_FILE:
		case XNVMEC_OPT_VTYPE_STR:
			break;

		case XNVMEC_OPT_VTYPE_NUM:
		case XNVMEC_OPT_VTYPE_HEX:
			errno = 0;
			num = strtoll(arg, &endptr, num_base);
			if (errno) {
				XNVME_DEBUG("FAILED: strtoll(), errno: %d", errno);
				errno = EINVAL;
				return -1;
			}
			if (arg == endptr) {
				XNVME_DEBUG("FAILED: strtoll(), no num. !");
				errno = EINVAL;
				return -1;
			}
			break;
		}
	}

	switch (opt_attr->opt) {
	case XNVMEC_OPT_CDW00:
		args->cdw[0] = num;
		break;
	case XNVMEC_OPT_CDW01:
		args->cdw[1] = num;
		break;
	case XNVMEC_OPT_CDW02:
		args->cdw[2] = num;
		break;
	case XNVMEC_OPT_CDW03:
		args->cdw[3] = num;
		break;
	case XNVMEC_OPT_CDW04:
		args->cdw[4] = num;
		break;
	case XNVMEC_OPT_CDW05:
		args->cdw[5] = num;
		break;
	case XNVMEC_OPT_CDW06:
		args->cdw[6] = num;
		break;
	case XNVMEC_OPT_CDW07:
		args->cdw[7] = num;
		break;
	case XNVMEC_OPT_CDW08:
		args->cdw[8] = num;
		break;
	case XNVMEC_OPT_CDW09:
		args->cdw[9] = num;
		break;
	case XNVMEC_OPT_CDW10:
		args->cdw[10] = num;
		break;
	case XNVMEC_OPT_CDW11:
		args->cdw[11] = num;
		break;
	case XNVMEC_OPT_CDW12:
		args->cdw[12] = num;
		break;
	case XNVMEC_OPT_CDW13:
		args->cdw[13] = num;
		break;
	case XNVMEC_OPT_CDW14:
		args->cdw[14] = num;
		break;
	case XNVMEC_OPT_CDW15:
		args->cdw[15] = num;
		break;
	case XNVMEC_OPT_CMD_INPUT:
		args->cmd_input = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_CMD_OUTPUT:
		args->cmd_output = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_DATA_NBYTES:
		args->data_nbytes = num;
		break;
	case XNVMEC_OPT_DATA_INPUT:
		args->data_input = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_DATA_OUTPUT:
		args->data_output = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_META_NBYTES:
		args->meta_nbytes = num;
		break;
	case XNVMEC_OPT_META_INPUT:
		args->meta_input = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_META_OUTPUT:
		args->meta_output = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_LBAF:
		args->lbaf = num;
		break;
	case XNVMEC_OPT_SLBA:
		args->slba = num;
		break;
	case XNVMEC_OPT_ELBA:
		args->elba = num;
		break;
	case XNVMEC_OPT_LBA:
		args->lba = num;
		break;
	case XNVMEC_OPT_NLB:
		args->nlb = num;
		break;
	case XNVMEC_OPT_URI:
		args->uri = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_SYS_URI:
		args->sys_uri = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_UUID:
		args->uuid = num;
		break;
	case XNVMEC_OPT_NSID:
		args->nsid = num;
		break;
	case XNVMEC_OPT_DEV_NSID:
		args->dev_nsid = num;
		break;
	case XNVMEC_OPT_CNS:
		args->cns = num;
		break;
	case XNVMEC_OPT_CSI:
		args->csi = num;
		break;
	case XNVMEC_OPT_INDEX:
		args->index = num;
		break;
	case XNVMEC_OPT_SETID:
		args->setid = num;
		break;

	case XNVMEC_OPT_CNTID:
		args->cntid = num;
		break;
	case XNVMEC_OPT_LID:
		args->lid = num;
		break;
	case XNVMEC_OPT_LSP:
		args->lsp = num;
		break;
	case XNVMEC_OPT_LPO_NBYTES:
		args->action = num;
		break;
	case XNVMEC_OPT_RAE:
		args->rae = num;
		break;
	case XNVMEC_OPT_CLEAR:
		args->clear = arg ? num : 1;
		break;
	case XNVMEC_OPT_ZF:
		args->zf = num;
		break;
	case XNVMEC_OPT_SES:
		args->ses = num;
		break;
	case XNVMEC_OPT_SEL:
		args->sel = num;
		break;
	case XNVMEC_OPT_MSET:
		args->mset = num;
		break;
	case XNVMEC_OPT_AUSE:
		args->ause = num;
		break;
	case XNVMEC_OPT_OVRPAT:
		args->ovrpat = num;
		break;
	case XNVMEC_OPT_OWPASS:
		args->owpass = num;
		break;
	case XNVMEC_OPT_OIPBP:
		args->oipbp = num;
		break;
	case XNVMEC_OPT_NODAS:
		args->nodas = num;
		break;

	case XNVMEC_OPT_ACTION:
		args->action = num;
		break;
	case XNVMEC_OPT_ZRMS:
		args->action = num;
		break;
	case XNVMEC_OPT_PI:
		args->pi = num;
		break;
	case XNVMEC_OPT_PIL:
		args->pil = num;
		break;
	case XNVMEC_OPT_FID:
		args->fid = num;
		break;
	case XNVMEC_OPT_FEAT:
		args->feat = num;
		break;
	case XNVMEC_OPT_SEED:
		args->seed = num;
		break;
	case XNVMEC_OPT_LIMIT:
		args->limit = num;
		break;
	case XNVMEC_OPT_IOSIZE:
		args->iosize = num;
		break;
	case XNVMEC_OPT_QDEPTH:
		args->qdepth = num;
		break;
	case XNVMEC_OPT_DIRECT:
		args->direct = arg ? num : 1;
		break;

	case XNVMEC_OPT_OPCODE:
		args->opcode = num;
		break;
	case XNVMEC_OPT_FLAGS:
		args->flags = num;
		break;

	case XNVMEC_OPT_ALL:
		args->all = arg ? num : 1;
		break;
	case XNVMEC_OPT_STATUS:
		args->status = arg ? num : 1;
		break;
	case XNVMEC_OPT_SAVE:
		args->save = arg ? num : 1;
		break;
	case XNVMEC_OPT_RESET:
		args->reset = arg ? num : 1;
		break;
	case XNVMEC_OPT_VERBOSE:
		args->verbose = arg ? num : 1;
		break;
	case XNVMEC_OPT_HELP:
		args->help = arg ? num : 1;
		break;

	case XNVMEC_OPT_COUNT:
		args->count = arg ? num : 1;
		break;
	case XNVMEC_OPT_OFFSET:
		args->offset = arg ? num : 1;
		break;

	case XNVMEC_OPT_BE:
		args->be = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_MEM:
		args->mem = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_SYNC:
		args->sync = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_ASYNC:
		args->async = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_ADMIN:
		args->admin = arg ? arg : "INVALID_INPUT";
		break;

	case XNVMEC_OPT_SHM_ID:
		args->shm_id = num;
		break;
	case XNVMEC_OPT_MAIN_CORE:
		args->main_core = arg ? num : 0;
		break;
	case XNVMEC_OPT_CORE_MASK:
		args->core_mask = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_USE_CMB_SQS:
		args->use_cmb_sqs = arg ? num : 0;
		break;
	case XNVMEC_OPT_CSS:
		args->css.value = arg ? num : 0;
		args->css.given = arg ? 1 : 0;
		break;

	case XNVMEC_OPT_POLL_IO:
		args->poll_io = arg ? num : 0;
		break;
	case XNVMEC_OPT_POLL_SQ:
		args->poll_sq = arg ? num : 0;
		break;
	case XNVMEC_OPT_REGISTER_FILES:
		args->register_files = arg ? num : 0;
		break;
	case XNVMEC_OPT_REGISTER_BUFFERS:
		args->register_buffers = arg ? num : 0;
		break;
	case XNVMEC_OPT_TRUNCATE:
		args->truncate = arg ? num : 0;
		break;
	case XNVMEC_OPT_RDONLY:
		args->rdonly = arg ? num : 0;
		break;
	case XNVMEC_OPT_WRONLY:
		args->wronly = arg ? num : 0;
		break;
	case XNVMEC_OPT_RDWR:
		args->rdwr = arg ? num : 0;
		break;
	case XNVMEC_OPT_CREATE:
		args->create = arg ? num : 0;
		break;
	case XNVMEC_OPT_CREATE_MODE:
		args->create = arg ? num : 0;
		break;
	case XNVMEC_OPT_OFLAGS:
		args->oflags = arg ? num : 0;
		break;
	case XNVMEC_OPT_ADRFAM:
		args->adrfam = arg ? arg : "IPv4";
		break;
	case XNVMEC_OPT_VEC_CNT:
		args->vec_cnt = arg ? num : 0;
		break;

	case XNVMEC_OPT_END:
	case XNVMEC_OPT_NONE:
		errno = EINVAL;
		XNVME_DEBUG("opt_attr->opt: 0x%x", opt_attr->opt);
		return -1;
	}

	cli->given[opt_attr->opt] = 1;

	return 0;
}

struct xnvmec_counts {
	int posa;
	int lreq;
	int lopt;
	int lflg;

	int total_long;
	int total_req;
	int total;
};

void
xnvmec_counts_pr(const struct xnvmec_counts *counts)
{
	printf("xnvmec_counts:");

	if (!counts) {
		printf(" ~\n");
		return;
	}

	printf("\n");
	printf("  posa: %d", counts->posa);
	printf("  lreq: %d", counts->lreq);
	printf("  lopt: %d", counts->lopt);
	printf("  lflg: %d", counts->lflg);

	printf("  total_long: %d", counts->total_long);
	printf("  total_req: %d", counts->total_req);
	printf("  total: %d", counts->total);
	printf("\n");
}

/**
 * Parse at most XNVMEC_SUB_OPTS_LEN arguments
 */
int
xnvmec_parse(struct xnvmec *cli)
{
	struct xnvmec_args *args = &cli->args;
	struct option long_options[XNVMEC_SUB_OPTS_LEN] = {0};
	struct xnvmec_opt_attr *pos_args[XNVMEC_SUB_OPTS_LEN] = {0};
	struct xnvmec_sub *sub;
	struct xnvmec_counts signature = {0};
	struct xnvmec_counts parsed = {0};

	const char *sub_name = NULL;

	if (cli->argc < 2) {
		xnvmec_pinf("Insufficient arguments: no <command> given");
		errno = EINVAL;
		return -1;
	}

	// Parse the sub-command so we know which PSAs and OPTs to scan for
	sub_name = cli->argv[1];

	sub = sub_by_name(cli, sub_name);
	if (!sub) {
		xnvmec_pinf("%s: invalid command: '%s'", cli->argv[0], sub_name);
		errno = EINVAL;
		return -1;
	}

	cli->sub = sub; // Boom!

	// Convert from XNVMEC_OPT to getopt long_options
	for (int oi = 0; oi < XNVMEC_SUB_OPTS_LEN; ++oi) {
		struct xnvmec_sub_opt *sub_opt = &sub->opts[oi];
		struct xnvmec_opt_attr *opt_attr = NULL;
		struct option *lopt = NULL;

		if ((oi + 1) >= (XNVMEC_SUB_MAXOPTS)) {
			xnvmec_pinf("Invalid arguments: nargs-exceeding '%d'", XNVMEC_SUB_MAXOPTS);
			errno = EINVAL;
			return -1;
		}

		if ((sub_opt->opt == XNVMEC_OPT_END) || (sub_opt->opt == XNVMEC_OPT_NONE)) {
			break;
		}

		opt_attr = xnvmec_opt_attr_by_opt(sub_opt->opt, xnvmec_opts);
		if (!opt_attr) {
			xnvmec_pinf("Invalid arguments: cannot parse value");
			errno = EINVAL;
			return -1;
		}

		// "Dynamically" create the option-character, starting at 'a'
		opt_attr->getoptval = 97 + oi;

		switch (sub_opt->type) {
		case XNVMEC_LFLG:
			++signature.lflg;
			++signature.total_long;
			break;
		case XNVMEC_LOPT:
			++signature.lopt;
			++signature.total_long;
			break;

		case XNVMEC_LREQ:
			++signature.lreq;
			++signature.total_long;
			++signature.total_req;
			break;

		case XNVMEC_POSA:
			++signature.posa;
			++signature.total_req;
			break;
		}
		++signature.total;

		switch (sub_opt->type) {
		case XNVMEC_LFLG:
		case XNVMEC_LREQ:
		case XNVMEC_LOPT:

			lopt = &long_options[signature.total_long - 1];
			lopt->name = opt_attr->name;
			lopt->flag = NULL;
			lopt->val = opt_attr->getoptval;

			lopt->has_arg = required_argument;
			if (sub_opt->type == XNVMEC_LFLG) {
				lopt->has_arg = no_argument;
			}

			break;

		case XNVMEC_POSA:
			pos_args[signature.posa - 1] = opt_attr;
			break;
		}
	}

	// Parse the long-opts
	for (int count = 0; count < signature.total_long; ++count) {
		struct xnvmec_sub_opt *sub_opt = NULL;
		struct xnvmec_opt_attr *opt_attr = NULL;
		int optidx = 0;
		int ret = 0;
		int found = 0;

		ret = getopt_long(cli->argc, cli->argv, "", long_options, &optidx);
		if (ret == -1) {
			break;
		}

		// Find the option, and the option-attributes matching the getopt-optionchar/ret
		for (int oi = 0; oi < XNVMEC_SUB_OPTS_LEN; ++oi) {
			sub_opt = &sub->opts[oi];

			opt_attr = xnvmec_opt_attr_by_opt(sub_opt->opt, xnvmec_opts);
			if (!opt_attr) {
				xnvmec_pinf("no joy");
				errno = EINVAL;
				return -1;
			}

			if (opt_attr->getoptval != ret) {
				continue;
			}

			switch (sub_opt->type) {
			case XNVMEC_LFLG:
				++parsed.lflg;
				break;

			case XNVMEC_LOPT:
				++parsed.lopt;
				break;

			case XNVMEC_LREQ:
				++parsed.lreq;
				++parsed.total_req;
				break;

			case XNVMEC_POSA:
				XNVME_DEBUG("Positional out of place");
				errno = EINVAL;
				return -1;
			}

			++parsed.total_long;
			++parsed.total;
			found = 1;
			break;
		}

		if (!found) {
			XNVME_DEBUG("Chaos");
			errno = EINVAL;
			return -1;
		}

		if (xnvmec_assign_arg(cli, opt_attr, optarg, sub_opt->type)) {
			XNVME_DEBUG("FAILED: xnvmec_assign_arg()");
			xnvmec_pinf("invalid argument value(%s)", optarg);
			errno = EINVAL;
			return -1;
		}
	}

	if (args->help) {
		return 0;
	}

	// Should now be able to parse PCAs as they are now shuffled to the end
	// of argv

	// Parse positional arguments
	if (signature.posa) {
		int npos_given;

		if (cli->argc < 3) {
			xnvmec_pinf("Insufficient arguments, see: --help");
			errno = EINVAL;
			return -1;
		}

		npos_given = cli->argc - (optind + 1);

		if (npos_given != signature.posa) {
			xnvmec_pinf("Insufficient arguments, see: --help");
			errno = EINVAL;
			return -1;
		}

		// Skip the sub-command and parse positional arguments
		for (int pos = 0; pos < signature.posa; ++pos) {
			struct xnvmec_opt_attr *attr = NULL;
			int idx = pos + (optind + 1);

			if (idx > cli->argc) {
				break;
			}

			attr = pos_args[pos];

			if (xnvmec_assign_arg(cli, attr, cli->argv[idx], XNVMEC_POSA)) {
				XNVME_DEBUG("FAILED: xnvmec_assign_arg()");
				xnvmec_pinf("invalid argument value(%s)", cli->argv[idx]);
				errno = EINVAL;
				return -1;
			}

			++parsed.posa;
			++parsed.total_req;
			++parsed.total;
		}
	}

	// Didn't ask for help and insufficient arguments provided
	if ((!args->help) && (parsed.total_req < signature.total_req)) {
		xnvmec_pinf("Insufficient required arguments, see: --help");
		errno = EINVAL;
		return -1;
	}

	return 0;
}

int
xnvmec(struct xnvmec *cli, int argc, char **argv, int opts)
{
	int err = 0;

	if (!cli) {
		err = -EINVAL;
		xnvmec_perr("xnvmec(!cli)", err);
		return err;
	}

	cli->argc = argc;
	cli->argv = argv;

	if (!cli->ver_pr) {
		cli->ver_pr = xnvme_ver_pr;
	}

	if ((argc < 2) || (!strcmp(argv[1], "--help")) || (!strcmp(argv[1], "-h"))) {
		xnvmec_usage(cli);
		return 0;
	}

	for (int i = 0; i < cli->nsubs; ++i) { // We all need help! ;)
		struct xnvmec_sub *sub = &cli->subs[i];

		if (!sub->name) {
			break;
		}

		for (int oi = 0; oi < XNVMEC_SUB_OPTS_LEN; ++oi) {
			struct xnvmec_sub_opt *sopt = &sub->opts[oi];

			if (sopt->opt == XNVMEC_OPT_NONE) {
				sopt->opt = XNVMEC_OPT_HELP;
				sopt->type = XNVMEC_LFLG;
				break;
			}
		}
	}

	err = xnvmec_parse(cli);
	if (err) {
		xnvmec_perr("xnvmec()", errno);
		return err;
	}

	// Arguments parsed without any errors
	if (cli->args.help) {
		xnvmec_usage_sub_long(cli, cli->sub);
		return 0;
	}

	if ((opts & XNVMEC_INIT_DEV_OPEN) && cli->args.uri) {
		struct xnvme_opts opts = xnvme_opts_default();

		if (xnvmec_cli_to_opts(cli, &opts)) {
			xnvmec_perr("xnvmec_cli_to_opts()", errno);
			return -1;
		}

		cli->args.dev = xnvme_dev_open(cli->args.uri, &opts);
		if (!cli->args.dev) {
			err = -errno;
			xnvmec_perr("xnvme_dev_open()", err);
			return -1;
		}
		cli->args.geo = xnvme_dev_get_geo(cli->args.dev);
	}

	err = cli->sub->command(cli);
	if (err) {
		xnvmec_perr(cli->sub->name, err);
	}

	if (cli->args.verbose) {
		xnvmec_args_pr(&cli->args, 0x0);
	}

	if ((opts & XNVMEC_INIT_DEV_OPEN) && cli->args.dev) {
		xnvme_dev_close(cli->args.dev);
	}

	return err ? 1 : 0;
}

int
xnvmec_cli_to_opts(const struct xnvmec *cli, struct xnvme_opts *opts)
{
	opts->be = cli->args.be;
	// opts->dev = cli->args.dev;
	opts->mem = cli->args.mem;
	opts->sync = cli->args.sync;
	opts->async = cli->args.async;
	opts->admin = cli->args.admin;

	opts->nsid = cli->args.dev_nsid;

	opts->oflags = cli->args.oflags;
	opts->rdonly = cli->args.rdonly;
	opts->wronly = cli->args.wronly;
	opts->rdwr = cli->args.rdwr;
	opts->create = cli->args.create;
	opts->truncate = cli->args.truncate;
	opts->direct = cli->args.direct;

	opts->create_mode = cli->args.create_mode;

	opts->poll_io = cli->args.poll_io;
	opts->poll_sq = cli->args.poll_sq;
	opts->register_files = cli->args.register_files;
	opts->register_buffers = cli->args.register_buffers;

	opts->css.value = cli->args.css.value;
	opts->css.given = cli->args.css.given;

	opts->use_cmb_sqs = cli->args.use_cmb_sqs;
	opts->shm_id = cli->args.shm_id;
	opts->main_core = cli->args.main_core;
	opts->core_mask = cli->args.core_mask;
	opts->adrfam = cli->args.adrfam;

	errno = 0;

	return 0;
}

void
xnvme_enumeration_free(struct xnvme_enumeration *list)
{
	free(list);
}

int
xnvme_enumeration_alloc(struct xnvme_enumeration **list, uint32_t capacity)
{
	*list = malloc(sizeof(**list) + sizeof(*(*list)->entries) * capacity);
	if (!(*list)) {
		XNVME_DEBUG("FAILED: malloc(list + entry * cap(%u))", capacity);
		return -errno;
	}
	(*list)->capacity = capacity;
	(*list)->nentries = 0;

	return 0;
}

int
xnvme_enumeration_append(struct xnvme_enumeration *list, const struct xnvme_ident *entry)
{
	if (!list->capacity) {
		XNVME_DEBUG("FAILED: syslist->capacity: %u", list->capacity);
		return -ENOMEM;
	}
	list->entries[(list->nentries)++] = *entry;
	list->capacity--;

	return 0;
}

int
xnvme_enumeration_fpr(FILE *stream, struct xnvme_enumeration *list, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_enumeration:");

	if (!list) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  capacity: %u\n", list->capacity);
	wrtn += fprintf(stream, "  nentries: %u\n", list->nentries);
	wrtn += fprintf(stream, "  entries:");

	if (!list->nentries) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	for (uint64_t idx = 0; idx < list->nentries; ++idx) {
		struct xnvme_ident *entry = &list->entries[idx];

		wrtn += fprintf(stream, "\n  - {");
		wrtn += xnvme_ident_yaml(stream, entry, 0, ", ", 0);
		wrtn += fprintf(stream, "}");
	}
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_enumeration_pr(struct xnvme_enumeration *list, int opts)
{
	return xnvme_enumeration_fpr(stdout, list, opts);
}

/**
 * Check whether the given list has the trgt as associated with the given ident
 *
 * @return Returns 1 it is exist, 0 otherwise.
 */
static int
enumeration_has_ident(struct xnvme_enumeration *list, struct xnvme_ident *ident, uint32_t idx)
{
	uint32_t bound = XNVME_MIN(list->nentries, idx);

	for (uint32_t i = 0; i < bound; ++i) {
		struct xnvme_ident *id = &list->entries[i];

		if (id->nsid != ident->nsid) {
			continue;
		}
		if (id->csi != ident->csi) {
			continue;
		}
		if (id->dtype != ident->dtype) {
			continue;
		}
		if (!strncmp(ident->uri, id->uri, XNVME_IDENT_URI_LEN - 1)) {
			return 1;
		}
	}

	return 0;
}

int
xnvme_enumeration_fpp(FILE *stream, struct xnvme_enumeration *list, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_enumeration:");

	if (!list) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	if (!list->nentries) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	for (uint32_t idx = 0; idx < list->nentries; ++idx) {
		struct xnvme_ident *ident = &list->entries[idx];

		if (enumeration_has_ident(list, ident, idx)) {
			continue;
		}

		wrtn += fprintf(stream, "\n");
		wrtn += fprintf(stream, "  - uri: %s\n", ident->uri);
	}

	return wrtn;
}

int
xnvme_enumeration_pp(struct xnvme_enumeration *list, int opts)
{
	return xnvme_enumeration_fpp(stdout, list, opts);
}
