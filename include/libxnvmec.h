/**
 * Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file libxnvmec.h
 */
#ifndef __LIBXNVMEC_H
#define __LIBXNVMEC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libxnvme.h>
#include <libxnvme_util.h>
#include <libxnvme_ver.h>
#include <libxnvme_pp.h>

/**
 * Fills `buf` with content `nbytes` of content
 *
 * @param buf Pointer to the buffer to fill
 * @param content Name of a file, or special "zero", "anum", "rand-k", "rand-t"
 * @param nbytes Amount of bytes to fill in buf
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvmec_buf_fill(void *buf, size_t nbytes, const char *content);

void *
xnvmec_buf_clear(void *buf, size_t nbytes);

/**
 * Returns the number of bytes where expected is different from actual
 *
 * @param expected
 * @param actual
 * @param nbytes
 *
 * @return On success, returns number of bytes that differ
 */
size_t
xnvmec_buf_diff(const void *expected, const void *actual, size_t nbytes);

/**
 * Prints the number and value of bytes where expected is different from actual
 *
 * @param expected
 * @param actual
 * @param nbytes
 * @param opts
 */
void
xnvmec_buf_diff_pr(const void *expected, const void *actual, size_t nbytes, int opts);

/**
 * Write content of buffer into file
 *
 * - If file exists, then it is truncated / overwritten
 * - If file does NOT exist, then it is created
 * - When file is created, permissions are set to user WRITE + READ
 *
 * @param buf Pointer to the buffer
 * @param nbytes Size of buf
 * @param path Destination where buffer will be dumped to
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvmec_buf_to_file(void *buf, size_t nbytes, const char *path);

/**
 * Read content of file into buffer
 *
 * @param buf Pointer to the buffer
 * @param nbytes Size of buf
 * @param path Source to read from
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvmec_buf_from_file(void *buf, size_t nbytes, const char *path);

/**
 * Fill the the given 'cmd' with content of file at 'fpath'
 *
 * @param cmd the command to fill
 * @param fpath path to file
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvmec_cmd_from_file(struct xnvme_spec_cmd *cmd, const char *fpath);

/**
 * Stores the given `cmd` in `fpath`
 *
 * @param cmd the command to store
 * @param fpath path to store content of cmd in
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvmec_cmd_to_file(const struct xnvme_spec_cmd *cmd, const char *fpath);

/**
 * Options are stored in an instance of this structure
 *
 * @struct xnvmec_args
 */
struct xnvmec_args {
	struct xnvme_dev *dev;       ///< Created
	const struct xnvme_geo *geo; ///< Created

	const char *uri;
	const char *sys_uri;

	const char *subnqn;
	const char *hostnqn;

	const char *cmd_input;
	const char *cmd_output;

	size_t data_nbytes;
	const char *data_input;
	const char *data_output;
	size_t meta_nbytes;
	const char *meta_input;
	const char *meta_output;

	uint32_t cdw[16];

	uint64_t lbaf;
	uint64_t lba;
	uint32_t nlb;
	uint64_t slba;
	uint64_t elba;

	uint32_t uuid;
	uint32_t nsid;
	uint32_t dev_nsid;
	uint32_t cns;
	uint32_t csi;
	uint64_t index;
	uint32_t setid;

	uint64_t cntid;
	uint32_t lid;
	uint32_t lsp;
	uint64_t lpo_nbytes;
	uint32_t rae;
	uint32_t clear;
	uint32_t zf;
	uint32_t ses;
	uint32_t sel;
	uint32_t mset;
	uint32_t ause;
	uint32_t ovrpat;
	uint32_t owpass;
	uint32_t oipbp;
	uint32_t nodas;

	uint32_t action;
	uint32_t zrms;

	uint32_t pi;
	uint32_t pil;

	uint32_t fid;
	uint32_t feat;

	uint32_t seed;
	uint32_t iosize;
	uint32_t qdepth;
	uint32_t direct;
	uint32_t limit;

	uint64_t count;
	uint64_t offset;

	uint64_t opcode;
	uint64_t flags;
	uint64_t all;

	uint32_t status;
	uint32_t save;
	uint32_t reset;
	uint32_t verbose;
	uint32_t help;

	const char *be;
	const char *mem;
	const char *sync;
	const char *async;
	const char *admin;

	uint64_t shm_id;
	uint32_t main_core;
	const char *core_mask;

	struct {
		uint32_t value : 31;
		uint32_t given : 1;
	} css; ///< SPDK controller-setup: do command-set-selection

	uint32_t use_cmb_sqs;
	const char *adrfam;

	uint32_t poll_io;
	uint32_t poll_sq;
	uint32_t register_files;
	uint32_t register_buffers;

	uint32_t truncate;
	uint32_t rdonly;
	uint32_t wronly;
	uint32_t rdwr;
	uint32_t create;
	uint32_t create_mode;
	uint32_t oflags;

	uint32_t vec_cnt;

	uint32_t dtype;
	uint32_t dspec;
	uint32_t doper;
	uint32_t endir;
	uint32_t tgtdir;
	uint32_t nsr;
};

void
xnvmec_args_pr(struct xnvmec_args *args, int opts);

/**
 * Command-Line Options, each command-line option can be one of ::xnvmec_opt_type
 *
 * @enum xnvmec_opt
 */
enum xnvmec_opt {
	XNVMEC_OPT_NONE = 0, ///< XNVMEC_OPT_NONE

	XNVMEC_OPT_CDW00 = 1,  ///< XNVMEC_OPT_CDW00
	XNVMEC_OPT_CDW01 = 2,  ///< XNVMEC_OPT_CDW01
	XNVMEC_OPT_CDW02 = 3,  ///< XNVMEC_OPT_CDW02
	XNVMEC_OPT_CDW03 = 4,  ///< XNVMEC_OPT_CDW03
	XNVMEC_OPT_CDW04 = 5,  ///< XNVMEC_OPT_CDW04
	XNVMEC_OPT_CDW05 = 6,  ///< XNVMEC_OPT_CDW05
	XNVMEC_OPT_CDW06 = 7,  ///< XNVMEC_OPT_CDW06
	XNVMEC_OPT_CDW07 = 8,  ///< XNVMEC_OPT_CDW07
	XNVMEC_OPT_CDW08 = 9,  ///< XNVMEC_OPT_CDW08
	XNVMEC_OPT_CDW09 = 10, ///< XNVMEC_OPT_CDW09
	XNVMEC_OPT_CDW10 = 11, ///< XNVMEC_OPT_CDW10
	XNVMEC_OPT_CDW11 = 12, ///< XNVMEC_OPT_CDW11
	XNVMEC_OPT_CDW12 = 13, ///< XNVMEC_OPT_CDW12
	XNVMEC_OPT_CDW13 = 14, ///< XNVMEC_OPT_CDW13
	XNVMEC_OPT_CDW14 = 15, ///< XNVMEC_OPT_CDW14
	XNVMEC_OPT_CDW15 = 16, ///< XNVMEC_OPT_CDW15

	XNVMEC_OPT_CMD_INPUT  = 17, ///< XNVMEC_OPT_CMD_INPUT
	XNVMEC_OPT_CMD_OUTPUT = 18, ///< XNVMEC_OPT_CMD_OUTPUT

	XNVMEC_OPT_DATA_NBYTES = 19, ///< XNVMEC_OPT_DATA_NBYTES
	XNVMEC_OPT_DATA_INPUT  = 20, ///< XNVMEC_OPT_DATA_INPUT
	XNVMEC_OPT_DATA_OUTPUT = 21, ///< XNVMEC_OPT_DATA_OUTPUT

	XNVMEC_OPT_META_NBYTES = 22, ///< XNVMEC_OPT_META_NBYTES
	XNVMEC_OPT_META_INPUT  = 23, ///< XNVMEC_OPT_META_INPUT
	XNVMEC_OPT_META_OUTPUT = 24, ///< XNVMEC_OPT_META_OUTPUT

	XNVMEC_OPT_LBAF = 25, ///< XNVMEC_OPT_LBAF
	XNVMEC_OPT_SLBA = 26, ///< XNVMEC_OPT_SLBA
	XNVMEC_OPT_ELBA = 27, ///< XNVMEC_OPT_ELBA
	XNVMEC_OPT_LBA  = 28, ///< XNVMEC_OPT_LBA
	XNVMEC_OPT_NLB  = 29, ///< XNVMEC_OPT_NLB

	XNVMEC_OPT_URI     = 30, ///< XNVMEC_OPT_URI
	XNVMEC_OPT_SYS_URI = 31, ///< XNVMEC_OPT_SYS_URI
	XNVMEC_OPT_UUID    = 32, ///< XNVMEC_OPT_UUID
	XNVMEC_OPT_NSID    = 33, ///< XNVMEC_OPT_NSID
	XNVMEC_OPT_CNS     = 34, ///< XNVMEC_OPT_CNS
	XNVMEC_OPT_CSI     = 35, ///< XNVMEC_OPT_CSI
	XNVMEC_OPT_INDEX   = 36, ///< XNVMEC_OPT_INDEX
	XNVMEC_OPT_SETID   = 37, ///< XNVMEC_OPT_SETID

	XNVMEC_OPT_CNTID      = 38, ///< XNVMEC_OPT_CNTID
	XNVMEC_OPT_LID        = 39, ///< XNVMEC_OPT_LID
	XNVMEC_OPT_LSP        = 40, ///< XNVMEC_OPT_LSP
	XNVMEC_OPT_LPO_NBYTES = 41, ///< XNVMEC_OPT_LPO_NBYTES
	XNVMEC_OPT_RAE        = 42, ///< XNVMEC_OPT_RAE
	XNVMEC_OPT_CLEAR      = 43, ///< XNVMEC_OPT_CLEAR
	XNVMEC_OPT_ZF         = 44, ///< XNVMEC_OPT_ZF
	XNVMEC_OPT_SES        = 45, ///< XNVMEC_OPT_SES
	XNVMEC_OPT_SEL        = 46, ///< XNVMEC_OPT_SEL
	XNVMEC_OPT_MSET       = 47, ///< XNVMEC_OPT_MSET
	XNVMEC_OPT_AUSE       = 48, ///< XNVMEC_OPT_AUSE
	XNVMEC_OPT_OVRPAT     = 49, ///< XNVMEC_OPT_OVRPAT
	XNVMEC_OPT_OWPASS     = 50, ///< XNVMEC_OPT_OWPASS
	XNVMEC_OPT_OIPBP      = 51, ///< XNVMEC_OPT_OIPBP
	XNVMEC_OPT_NODAS      = 52, ///< XNVMEC_OPT_NODAS

	XNVMEC_OPT_ACTION = 53, ///< XNVMEC_OPT_ACTION
	XNVMEC_OPT_ZRMS   = 54, ///< XNVMEC_OPT_ZRMS

	XNVMEC_OPT_PI  = 55, ///< XNVMEC_OPT_PI
	XNVMEC_OPT_PIL = 56, ///< XNVMEC_OPT_PIL

	XNVMEC_OPT_FID  = 57, ///< XNVMEC_OPT_FID
	XNVMEC_OPT_FEAT = 58, ///< XNVMEC_OPT_FEAT

	XNVMEC_OPT_SEED   = 59, ///< XNVMEC_OPT_SEED
	XNVMEC_OPT_LIMIT  = 60, ///< XNVMEC_OPT_LIMIT
	XNVMEC_OPT_IOSIZE = 61, ///< XNVMEC_OPT_IOSIZE
	XNVMEC_OPT_QDEPTH = 62, ///< XNVMEC_OPT_QDEPTH
	XNVMEC_OPT_DIRECT = 63, ///< XNVMEC_OPT_DIRECT

	XNVMEC_OPT_STATUS  = 64, ///< XNVMEC_OPT_STATUS
	XNVMEC_OPT_SAVE    = 65, ///< XNVMEC_OPT_SAVE
	XNVMEC_OPT_RESET   = 66, ///< XNVMEC_OPT_RESET
	XNVMEC_OPT_VERBOSE = 67, ///< XNVMEC_OPT_VERBOSE
	XNVMEC_OPT_HELP    = 68, ///< XNVMEC_OPT_HELP

	XNVMEC_OPT_COUNT  = 69, ///< XNVMEC_OPT_COUNT
	XNVMEC_OPT_OFFSET = 70, ///< XNVMEC_OPT_OFFSET

	XNVMEC_OPT_OPCODE = 71, ///< XNVMEC_OPT_OPCODE
	XNVMEC_OPT_FLAGS  = 72, ///< XNVMEC_OPT_FLAGS

	XNVMEC_OPT_ALL = 73, ///< XNVMEC_OPT_ALL

	XNVMEC_OPT_BE    = 74, ///< XNVMEC_OPT_BE
	XNVMEC_OPT_MEM   = 75, ///< XNVMEC_OPT_MEM
	XNVMEC_OPT_SYNC  = 76, ///< XNVMEC_OPT_SYNC
	XNVMEC_OPT_ASYNC = 77, ///< XNVMEC_OPT_ASYNC
	XNVMEC_OPT_ADMIN = 78, ///< XNVMEC_OPT_ADMIN

	XNVMEC_OPT_SHM_ID    = 79, ///< XNVMEC_OPT_SHM_ID
	XNVMEC_OPT_MAIN_CORE = 80, ///< XNVMEC_OPT_MAIN_CORE
	XNVMEC_OPT_CORE_MASK = 81, ///< XNVMEC_OPT_CORE_MASK

	XNVMEC_OPT_USE_CMB_SQS = 82, ///< XNVMEC_OPT_USE_CMB_SQS
	XNVMEC_OPT_CSS         = 83, ///< XNVMEC_OPT_CSS

	XNVMEC_OPT_POLL_IO          = 84, ///< XNVMEC_OPT_POLL_IO
	XNVMEC_OPT_POLL_SQ          = 85, ///< XNVMEC_OPT_POLL_SQ
	XNVMEC_OPT_REGISTER_FILES   = 86, ///< XNVMEC_OPT_REGISTER_FILES
	XNVMEC_OPT_REGISTER_BUFFERS = 87, ///< XNVMEC_OPT_REGISTER_BUFFERS

	XNVMEC_OPT_TRUNCATE = 88, ///< XNVMEC_OPT_TRUNCATE
	XNVMEC_OPT_RDONLY   = 89, ///< XNVMEC_OPT_RDONLY
	XNVMEC_OPT_WRONLY   = 90, ///< XNVMEC_OPT_WRONLY
	XNVMEC_OPT_RDWR     = 91, ///< XNVMEC_OPT_RDWR

	XNVMEC_OPT_CREATE      = 92, ///< XNVMEC_OPT_CREATE
	XNVMEC_OPT_CREATE_MODE = 93, ///< XNVMEC_OPT_CREATE_MODE

	XNVMEC_OPT_OFLAGS = 94, ///< XNVMEC_OPT_OFLAGS

	XNVMEC_OPT_ADRFAM   = 95, ///< XNVMEC_OPT_ADRFAM
	XNVMEC_OPT_DEV_NSID = 96, ///< XNVMEC_OPT_DEV_NSID

	XNVMEC_OPT_VEC_CNT = 97, ///< XNVMEC_OPT_VEC_CNT

	XNVMEC_OPT_SUBNQN  = 98, ///< XNVME_OPT_SUBNQN
	XNVMEC_OPT_HOSTNQN = 99, ///< XNVMEC_OPT_HOSTNQN

	XNVMEC_OPT_DTYPE  = 100, ///< XNVMEC_OPT_DTYPE
	XNVMEC_OPT_DSPEC  = 101, ///< XNVMEC_OPT_DSPEC
	XNVMEC_OPT_DOPER  = 102, ///< XNVMEC_OPT_DOPER
	XNVMEC_OPT_ENDIR  = 103, ///< XNVMEC_OPT_ENDIR
	XNVMEC_OPT_TGTDIR = 104, ///< XNVMEC_OPT_TGTDIR
	XNVMEC_OPT_NSR    = 105, ///< XNVMEC_OPT_NSR

	XNVMEC_OPT_END = 106, ///< XNVMEC_OPT_END
};

/**
 * XNVMEC_LFLG: ./cli --verbose; always optional
 * XNVMEC_LOPT: ./cli --arg 0x0; optionally provide a value
 * XNVMEC_LREQ: ./cli --arg 0x0; require providing a value
 * XNVMEC_POSA: ./cli arg1 arg2; required, ordered, and provides a value
 *
 * @enum xnvmec_opt_type
 */
enum xnvmec_opt_type {
	XNVMEC_POSA = 0x1, ///< XNVMEC_POSA
	XNVMEC_LFLG = 0x2, ///< XNVMEC_LFLG
	XNVMEC_LOPT = 0x3, ///< XNVMEC_LOPT
	XNVMEC_LREQ = 0x4, ///< XNVMEC_LREQ
};

enum xnvmec_opt_value_type {
	XNVMEC_OPT_VTYPE_URI  = 0x1,
	XNVMEC_OPT_VTYPE_NUM  = 0x2,
	XNVMEC_OPT_VTYPE_HEX  = 0x3,
	XNVMEC_OPT_VTYPE_FILE = 0x4,
	XNVMEC_OPT_VTYPE_STR  = 0x5,
};

struct xnvmec_opt_attr {
	enum xnvmec_opt opt;
	enum xnvmec_opt_value_type vtype;
	const char *name;
	const char *descr;

	char getoptval; // character returned by getopt_log() when found
};

const struct xnvmec_opt_attr *
xnvmec_get_opt_attr(enum xnvmec_opt opt);

/**
 * @struct xnvmec_sub_opt
 */
struct xnvmec_sub_opt {
	enum xnvmec_opt opt;
	enum xnvmec_opt_type type;
};

/**
 * Forward declaration
 */
struct xnvmec;

typedef int (*xnvmec_subfunc)(struct xnvmec *);

#define XNVMEC_SUB_OPTS_LEN 200

#define XNVMEC_SUB_NAME_LEN_MAX 30

/**
 * Filled by the user of libxnvmc, usually statically right above main
 */
struct xnvmec_sub {
	const char *name;        ///< Short name of the sub-command
	const char *descr_short; ///< Short sub-command description
	const char *descr_long;  ///< Long sub-command description

	xnvmec_subfunc command; ///< Function to execute

	///< Options to parse into args and pass onto func
	struct xnvmec_sub_opt opts[XNVMEC_SUB_OPTS_LEN];
};

struct xnvmec {
	const char *title;       ///< Setup by user
	const char *descr_short; ///< Setup by user
	const char *descr_long;  ///< Setup by user

	int nsubs;               ///< Setup by user
	struct xnvmec_sub *subs; ///< Setup by user

	int (*ver_pr)(int); ///< Setup by library if unset

	int argc;                ///< Setup by library
	char **argv;             ///< Setup by library
	struct xnvmec_args args; ///< Setup by library

	int given[XNVMEC_OPT_END]; ///< Setup by library

	struct xnvmec_sub *sub; ///< Setup by library

	struct xnvme_timer timer; ///< Used by xnvmec_timer_*
};

enum xnvmec_opts {
	XNVMEC_INIT_NONE     = 0x0,
	XNVMEC_INIT_DEV_OPEN = 0x1,
};

/**
 * List of devices found on the system usable with xNVMe
 *
 * @struct xnvme_enumeration
 */
struct xnvme_enumeration {
	uint32_t capacity;            ///< Remaining unused entries
	uint32_t nentries;            ///< Used entries
	struct xnvme_ident entries[]; ///< Device entries
};

int
xnvme_enumeration_alloc(struct xnvme_enumeration **list, uint32_t capacity);

void
xnvme_enumeration_free(struct xnvme_enumeration *list);

int
xnvme_enumeration_append(struct xnvme_enumeration *list, const struct xnvme_ident *entry);

static inline uint64_t
xnvmec_timer_start(struct xnvmec *cli)
{
	cli->timer.start = _xnvme_timer_clock_sample();
	return cli->timer.start;
}

static inline uint64_t
xnvmec_timer_stop(struct xnvmec *cli)
{
	cli->timer.stop = _xnvme_timer_clock_sample();
	return cli->timer.stop;
}

static inline void
xnvmec_timer_bw_pr(struct xnvmec *cli, const char *prefix, size_t nbytes)
{
	xnvme_timer_bw_pr(&cli->timer, prefix, nbytes);
}

void
xnvmec_pinf(const char *format, ...);

void
xnvmec_perr(const char *msg, int err);

int
xnvmec(struct xnvmec *cli, int argc, char **argv, int opts);

/**
 * Fill the given 'opts' with values parsed in the given 'cli'
 *
 * @param cli The command-line-interface to parse arguments for
 * @param opts The device-options to fill
 *
 * @return On success, 0 is returned.
 */
int
xnvmec_cli_to_opts(const struct xnvmec *cli, struct xnvme_opts *opts);

#ifdef __cplusplus
}
#endif

#endif /* LIBXNVMEC_H */
