// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * The command-line library shared by the tools, tests and examples of xNVMe;
 * internal to xNVMe and not part of the public API.
 *
 * @file xnvme_cli.h
 */
#ifndef __INTERNAL_XNVME_CLI_H
#define __INTERNAL_XNVME_CLI_H

#include <libxnvme.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XNVME_CLI_CORE_OPTS                                                                  \
	{XNVME_CLI_OPT_ORCH_TITLE, XNVME_CLI_SKIP}, {XNVME_CLI_OPT_SUBNQN, XNVME_CLI_LOPT},  \
		{XNVME_CLI_OPT_HOSTNQN, XNVME_CLI_LOPT}, {XNVME_CLI_OPT_BE, XNVME_CLI_LOPT}, \
		{XNVME_CLI_OPT_SHM_ID, XNVME_CLI_LOPT},                                      \
	{                                                                                    \
		XNVME_CLI_OPT_HOMI_ID, XNVME_CLI_LOPT                                        \
	}

#define XNVME_CLI_ADMIN_OPTS                                                                \
	XNVME_CLI_CORE_OPTS, {XNVME_CLI_OPT_DEV_NSID, XNVME_CLI_LOPT},                      \
		{XNVME_CLI_OPT_ADMIN, XNVME_CLI_LOPT}, {XNVME_CLI_OPT_MEM, XNVME_CLI_LOPT}, \
	{                                                                                   \
		XNVME_CLI_OPT_DIRECT, XNVME_CLI_LOPT                                        \
	}

#define XNVME_CLI_SYNC_OPTS \
	XNVME_CLI_ADMIN_OPTS, { XNVME_CLI_OPT_SYNC, XNVME_CLI_LOPT }

#define XNVME_CLI_ASYNC_OPTS \
	XNVME_CLI_SYNC_OPTS, { XNVME_CLI_OPT_ASYNC, XNVME_CLI_LOPT }

#define XNVME_CLI_CPU_MAX_ID 1024

/**
 * Options are stored in an instance of this structure
 *
 * @struct xnvme_cli_args
 */
struct xnvme_cli_args {
	struct xnvme_dev *dev; ///< Created

	const char *uri;
	const char *sys_uri;

	const char *subnqn;
	const char *hostnqn;

	size_t data_nbytes;
	const char *data_input;
	const char *data_output;
	const char *meta_input;

	uint32_t cdw[16];

	uint64_t lbafl;
	uint32_t llb;
	uint32_t nlb;
	uint64_t slba;
	uint64_t elba;

	uint32_t uuid;
	uint32_t nsid;
	uint32_t dev_nsid;
	uint32_t cns;
	uint32_t setid;

	uint64_t cntid;
	uint32_t lid;
	uint32_t lsp;
	uint64_t lpo_nbytes;
	uint32_t rae;
	bool clear;
	uint32_t lbafu;
	uint32_t ses;
	uint32_t sel;
	uint32_t mset;
	bool ause;
	uint32_t ovrpat;
	uint32_t owpass;
	bool oipbp;
	bool nodas;

	uint32_t sanact;
	uint32_t zsa;

	uint32_t pi;
	uint32_t pil;

	uint32_t fid;
	uint32_t feat;

	uint32_t iosize;
	uint32_t qdepth;
	uint32_t dev_id;
	uint32_t max_io_bytes;
	bool direct;
	bool p2p_cq_mirror;
	bool sq_hostmem;
	bool buf_host_bounce;
	uint32_t limit;

	uint64_t count;
	uint64_t offset;

	bool all;

	bool save;
	bool verbose;
	bool no_cuse;
	bool user_copy;
	uint32_t help;

	const char *be;
	const char *alt_be;
	const char *alt_uri;
	const char *mem;
	const char *sync;
	const char *async;
	const char *admin;

	uint64_t shm_id;
	uint64_t homi_id;
	uint64_t host_heap_size;
	uint64_t device_heap_size;

	uint32_t gpu_id; ///< upcie-cuda/upcie-hip: GPU ordinal to use

	uint32_t poll_io;
	uint32_t poll_sq;

	bool ad;
	bool idw;
	bool idr;

	uint32_t vec_cnt;

	uint32_t dtype;
	uint32_t dspec;
	uint32_t doper;
	uint32_t endir;
	uint32_t tgtdir;
	uint32_t nsr;

	uint32_t lsi;
	uint32_t pid;

	const char *kv_key;
	const char *kv_val;
	bool kv_store_add;
	bool kv_store_update;
	bool kv_store_compress;

	bool pract;
	uint8_t prchk;
	uint32_t apptag;
	uint32_t apptag_mask;

	uint64_t sdlba;

	uint32_t runtime;
	double report_freq;
	const char *iopattern;
	const char *completion;

	const char *cpumask;

	const char *cpulist;
	uint16_t cpus[XNVME_CLI_CPU_MAX_ID]; //< Maximum of 1024 pinned CPUs
	uint16_t ncpus;

	uint32_t nqueues;

	const char **posn; //< Remaining positional args (points into argv)
	int posn_count;    ///< Number of remaining positional args
};

void
xnvme_cli_args_pr(struct xnvme_cli_args *args, int opts);

/**
 * Command-Line Options, each command-line option can be one of ::xnvme_cli_opt_type
 *
 * @enum xnvme_cli_opt
 */
enum xnvme_cli_opt {
	XNVME_CLI_OPT_NONE = 0, ///< XNVME_CLI_OPT_NONE

	XNVME_CLI_OPT_CDW11, ///< XNVME_CLI_OPT_CDW11
	XNVME_CLI_OPT_CDW12, ///< XNVME_CLI_OPT_CDW12

	XNVME_CLI_OPT_DATA_NBYTES, ///< XNVME_CLI_OPT_DATA_NBYTES
	XNVME_CLI_OPT_DATA_INPUT,  ///< XNVME_CLI_OPT_DATA_INPUT
	XNVME_CLI_OPT_DATA_OUTPUT, ///< XNVME_CLI_OPT_DATA_OUTPUT

	XNVME_CLI_OPT_META_INPUT, ///< XNVME_CLI_OPT_META_INPUT

	XNVME_CLI_OPT_LBAFL, ///< XNVME_CLI_OPT_LBAFL
	XNVME_CLI_OPT_SLBA,  ///< XNVME_CLI_OPT_SLBA
	XNVME_CLI_OPT_ELBA,  ///< XNVME_CLI_OPT_ELBA
	XNVME_CLI_OPT_NLB,   ///< XNVME_CLI_OPT_NLB

	XNVME_CLI_OPT_URI,     ///< XNVME_CLI_OPT_URI
	XNVME_CLI_OPT_SYS_URI, ///< XNVME_CLI_OPT_SYS_URI
	XNVME_CLI_OPT_UUID,    ///< XNVME_CLI_OPT_UUID
	XNVME_CLI_OPT_NSID,    ///< XNVME_CLI_OPT_NSID
	XNVME_CLI_OPT_CNS,     ///< XNVME_CLI_OPT_CNS
	XNVME_CLI_OPT_SETID,   ///< XNVME_CLI_OPT_SETID

	XNVME_CLI_OPT_CNTID,      ///< XNVME_CLI_OPT_CNTID
	XNVME_CLI_OPT_LID,        ///< XNVME_CLI_OPT_LID
	XNVME_CLI_OPT_LSP,        ///< XNVME_CLI_OPT_LSP
	XNVME_CLI_OPT_LPO_NBYTES, ///< XNVME_CLI_OPT_LPO_NBYTES
	XNVME_CLI_OPT_RAE,        ///< XNVME_CLI_OPT_RAE
	XNVME_CLI_OPT_CLEAR,      ///< XNVME_CLI_OPT_CLEAR
	XNVME_CLI_OPT_LBAFU,      ///< XNVME_CLI_OPT_LBAFU
	XNVME_CLI_OPT_SES,        ///< XNVME_CLI_OPT_SES
	XNVME_CLI_OPT_SEL,        ///< XNVME_CLI_OPT_SEL
	XNVME_CLI_OPT_MSET,       ///< XNVME_CLI_OPT_MSET
	XNVME_CLI_OPT_AUSE,       ///< XNVME_CLI_OPT_AUSE
	XNVME_CLI_OPT_OVRPAT,     ///< XNVME_CLI_OPT_OVRPAT
	XNVME_CLI_OPT_OWPASS,     ///< XNVME_CLI_OPT_OWPASS
	XNVME_CLI_OPT_OIPBP,      ///< XNVME_CLI_OPT_OIPBP
	XNVME_CLI_OPT_NODAS,      ///< XNVME_CLI_OPT_NODAS

	XNVME_CLI_OPT_SANACT, ///< XNVME_CLI_OPT_SANACT
	XNVME_CLI_OPT_ZSA,    ///< XNVME_CLI_OPT_ZSA

	XNVME_CLI_OPT_PI,  ///< XNVME_CLI_OPT_PI
	XNVME_CLI_OPT_PIL, ///< XNVME_CLI_OPT_PIL

	XNVME_CLI_OPT_FID,  ///< XNVME_CLI_OPT_FID
	XNVME_CLI_OPT_FEAT, ///< XNVME_CLI_OPT_FEAT

	XNVME_CLI_OPT_LIMIT,  ///< XNVME_CLI_OPT_LIMIT
	XNVME_CLI_OPT_IOSIZE, ///< XNVME_CLI_OPT_IOSIZE
	XNVME_CLI_OPT_QDEPTH, ///< XNVME_CLI_OPT_QDEPTH
	XNVME_CLI_OPT_DIRECT, ///< XNVME_CLI_OPT_DIRECT

	XNVME_CLI_OPT_SAVE,    ///< XNVME_CLI_OPT_SAVE
	XNVME_CLI_OPT_VERBOSE, ///< XNVME_CLI_OPT_VERBOSE
	XNVME_CLI_OPT_HELP,    ///< XNVME_CLI_OPT_HELP

	XNVME_CLI_OPT_COUNT,  ///< XNVME_CLI_OPT_COUNT
	XNVME_CLI_OPT_OFFSET, ///< XNVME_CLI_OPT_OFFSET

	XNVME_CLI_OPT_ALL, ///< XNVME_CLI_OPT_ALL

	XNVME_CLI_OPT_BE,    ///< XNVME_CLI_OPT_BE
	XNVME_CLI_OPT_MEM,   ///< XNVME_CLI_OPT_MEM
	XNVME_CLI_OPT_SYNC,  ///< XNVME_CLI_OPT_SYNC
	XNVME_CLI_OPT_ASYNC, ///< XNVME_CLI_OPT_ASYNC
	XNVME_CLI_OPT_ADMIN, ///< XNVME_CLI_OPT_ADMIN

	XNVME_CLI_OPT_SHM_ID, ///< XNVME_CLI_OPT_SHM_ID

	XNVME_CLI_OPT_POLL_IO, ///< XNVME_CLI_OPT_POLL_IO
	XNVME_CLI_OPT_POLL_SQ, ///< XNVME_CLI_OPT_POLL_SQ

	XNVME_CLI_OPT_DEV_NSID, ///< XNVME_CLI_OPT_DEV_NSID

	XNVME_CLI_OPT_VEC_CNT, ///< XNVME_CLI_OPT_VEC_CNT

	XNVME_CLI_OPT_SUBNQN,  ///< XNVME_OPT_SUBNQN
	XNVME_CLI_OPT_HOSTNQN, ///< XNVME_CLI_OPT_HOSTNQN

	XNVME_CLI_OPT_DTYPE,  ///< XNVME_CLI_OPT_DTYPE
	XNVME_CLI_OPT_DSPEC,  ///< XNVME_CLI_OPT_DSPEC
	XNVME_CLI_OPT_DOPER,  ///< XNVME_CLI_OPT_DOPER
	XNVME_CLI_OPT_ENDIR,  ///< XNVME_CLI_OPT_ENDIR
	XNVME_CLI_OPT_TGTDIR, ///< XNVME_CLI_OPT_TGTDIR
	XNVME_CLI_OPT_NSR,    ///< XNVME_CLI_OPT_NSR

	XNVME_CLI_OPT_POSA_TITLE,     ///< XNVME_CLI_OPT_POSA_TITLE
	XNVME_CLI_OPT_NON_POSA_TITLE, ///< XNVME_CLI_OPT_NON_POSA_TITLE

	XNVME_CLI_OPT_ORCH_TITLE, ///< XNVME_CLI_OPT_ORCH_TITLE

	XNVME_CLI_OPT_AD,  ///< XNVME_CLI_OPT_AD
	XNVME_CLI_OPT_IDW, ///< XNVME_CLI_OPT_IDW
	XNVME_CLI_OPT_IDR, ///< XNVME_CLI_OPT_IDR
	XNVME_CLI_OPT_LLB, ///< XNVME_CLI_OPT_LLB

	XNVME_CLI_OPT_LSI, ///< XNVME_CLI_OPT_LSI
	XNVME_CLI_OPT_PID, ///< XNVME_CLI_OPT_PID

	XNVME_CLI_OPT_KV_KEY, ///< XNVME_CLI_OPT_KV_KEY
	XNVME_CLI_OPT_KV_VAL, ///< XNVME_CLI_OPT_KV_VAL

	XNVME_CLI_OPT_KV_STORE_UPDATE,   ///< XNVME_CLI_OPT_KV_STORE_UPDATE
	XNVME_CLI_OPT_KV_STORE_ADD,      ///< XNVME_CLI_OPT_KV_STORE_ADD
	XNVME_CLI_OPT_KV_STORE_COMPRESS, ///< XNVME_CLI_OPT_KV_STORE_COMPRESS

	XNVME_CLI_OPT_PRACT,       ///< XNVME_CLI_OPT_PRACT
	XNVME_CLI_OPT_PRCHK,       ///< XNVME_CLI_OPT_PRCHK
	XNVME_CLI_OPT_APPTAG,      ///< XNVME_CLI_OPT_APPTAG
	XNVME_CLI_OPT_APPTAG_MASK, ///< XNVME_CLI_OPT_APPTAG_MASK

	XNVME_CLI_OPT_SDLBA,

	XNVME_CLI_OPT_RUNTIME,   ///< XNVME_CLI_OPT_RUNTIME
	XNVME_CLI_OPT_IOPATTERN, ///< XNVME_CLI_OPT_IOPATTERN
	XNVME_CLI_OPT_CPUMASK,   ///< XNVME_CLI_OPT_CPUMASK
	XNVME_CLI_OPT_NQUEUES,   ///< XNVME_CLI_OPT_NQUEUES

	XNVME_CLI_OPT_ALT_BE, ///< XNVME_CLI_OPT_ALT_BE

	XNVME_CLI_OPT_GPU_ID, ///< XNVME_CLI_OPT_GPU_ID

	XNVME_CLI_OPT_CPULIST, ///< XNVME_CLI_OPT_CPULIST

	XNVME_CLI_OPT_HOST_HEAP_SIZE,   ///< XNVME_CLI_OPT_HOST_HEAP_SIZE
	XNVME_CLI_OPT_DEVICE_HEAP_SIZE, ///< XNVME_CLI_OPT_DEVICE_HEAP_SIZE

	XNVME_CLI_OPT_DEV_ID, ///< XNVME_CLI_OPT_DEV_ID

	XNVME_CLI_OPT_MAX_IO_BYTES, ///< XNVME_CLI_OPT_MAX_IO_BYTES

	XNVME_CLI_OPT_HOMI_ID, ///< XNVME_CLI_OPT_HOMI_ID

	XNVME_CLI_OPT_REPORT_FREQ, ///< XNVME_CLI_OPT_REPORT_FREQ

	XNVME_CLI_OPT_NO_CUSE, ///< XNVME_CLI_OPT_NO_CUSE

	XNVME_CLI_OPT_ALT_URI, ///< XNVME_CLI_OPT_ALT_URI

	XNVME_CLI_OPT_P2P_CQ_MIRROR, ///< XNVME_CLI_OPT_P2P_CQ_MIRROR

	XNVME_CLI_OPT_SQ_HOSTMEM, ///< XNVME_CLI_OPT_SQ_HOSTMEM

	XNVME_CLI_OPT_BUF_HOST_BOUNCE, ///< XNVME_CLI_OPT_BUF_HOST_BOUNCE

	XNVME_CLI_OPT_USER_COPY, ///< XNVME_CLI_OPT_USER_COPY

	XNVME_CLI_OPT_COMPLETION, ///< XNVME_CLI_OPT_COMPLETION

	XNVME_CLI_OPT_END, ///< XNVME_CLI_OPT_END
};

/**
 * XNVME_CLI_LFLG: ./cli --verbose; always optional
 * XNVME_CLI_LOPT: ./cli --arg 0x0; optionally provide a value
 * XNVME_CLI_LREQ: ./cli --arg 0x0; require providing a value
 * XNVME_CLI_POSA: ./cli arg1 arg2; required, ordered, and provides a value
 * XNVME_CLI_POSN: ./cli arg1a arg1b; optional, ordered, and provides zero or more values
 * XNVME_CLI_SKIP: ; This argument is used for formatting etc.
 *
 * @enum xnvme_cli_opt_type
 */
enum xnvme_cli_opt_type {
	XNVME_CLI_POSA = 0x1, ///< XNVME_CLI_POSA
	XNVME_CLI_LFLG = 0x2, ///< XNVME_CLI_LFLG
	XNVME_CLI_LOPT = 0x3, ///< XNVME_CLI_LOPT
	XNVME_CLI_LREQ = 0x4, ///< XNVME_CLI_LREQ
	XNVME_CLI_SKIP = 0x5, ///< XNVME_CLI_SKIP
	XNVME_CLI_POSN = 0x6, ///< XNVME_CLI_POSN
};

enum xnvme_cli_opt_value_type {
	XNVME_CLI_OPT_VTYPE_URI   = 0x1,
	XNVME_CLI_OPT_VTYPE_NUM   = 0x2,
	XNVME_CLI_OPT_VTYPE_HEX   = 0x3,
	XNVME_CLI_OPT_VTYPE_FILE  = 0x4,
	XNVME_CLI_OPT_VTYPE_STR   = 0x5,
	XNVME_CLI_OPT_VTYPE_SKIP  = 0x6,
	XNVME_CLI_OPT_VTYPE_FLOAT = 0x7,
};

struct xnvme_cli_opt_attr {
	enum xnvme_cli_opt opt;
	enum xnvme_cli_opt_value_type vtype;
	const char *name;
	const char *descr;

	char getoptval; // character returned by getopt_log() when found
};

const struct xnvme_cli_opt_attr *
xnvme_cli_get_opt_attr(enum xnvme_cli_opt opt);

/**
 * @struct xnvme_cli_sub_opt
 */
struct xnvme_cli_sub_opt {
	enum xnvme_cli_opt opt;
	enum xnvme_cli_opt_type type;
};

/**
 * Forward declaration
 */
struct xnvme_cli;

typedef int (*xnvme_cli_subfunc)(struct xnvme_cli *);

#define XNVME_CLI_SUB_OPTS_LEN 200

#define XNVME_CLI_SUB_NAME_LEN_MAX 30

/**
 * Filled by the user of libxnvmc, usually statically right above main
 */
struct xnvme_cli_sub {
	const char *name;        ///< Short name of the sub-command
	const char *descr_short; ///< Short sub-command description
	const char *descr_long;  ///< Long sub-command description

	xnvme_cli_subfunc command; ///< Function to execute

	///< Options to parse into args and pass onto func
	struct xnvme_cli_sub_opt opts[XNVME_CLI_SUB_OPTS_LEN];
};

struct xnvme_cli {
	const char *title;       ///< Setup by user
	const char *descr_short; ///< Setup by user
	const char *descr_long;  ///< Setup by user

	int nsubs;                  ///< Setup by user
	struct xnvme_cli_sub *subs; ///< Setup by user

	int (*ver_pr)(int); ///< Setup by library if unset

	/**
	 * Setup by user: the revision the program was built from, XNVME_VCS_TAG
	 *
	 * When set, xnvme_cli_run() compares it against xnvme_ver_vcs() and warns
	 * when the library loaded at runtime is not the one the program was built
	 * with. Leave it NULL to skip the check.
	 */
	const char *vcs;

	int argc;                   ///< Setup by library
	char **argv;                ///< Setup by library
	struct xnvme_cli_args args; ///< Setup by library

	int given[XNVME_CLI_OPT_END]; ///< Setup by library

	struct xnvme_cli_sub *sub; ///< Setup by library

	struct xnvme_timer timer; ///< Used by xnvme_cli_timer_*
};

enum xnvme_cli_opts {
	XNVME_CLI_INIT_NONE     = 0x0,
	XNVME_CLI_INIT_DEV_OPEN = 0x1,
};

/**
 * List of devices found on the system usable with xNVMe
 *
 * @struct xnvme_cli_enumeration
 */
struct xnvme_cli_enumeration {
	uint32_t capacity;            ///< Remaining unused entries
	uint32_t nentries;            ///< Used entries
	struct xnvme_ident entries[]; ///< Device entries
};

int
xnvme_cli_enumeration_alloc(struct xnvme_cli_enumeration **list, uint32_t capacity);

void
xnvme_cli_enumeration_free(struct xnvme_cli_enumeration *list);

int
xnvme_cli_enumeration_append(struct xnvme_cli_enumeration *list, const struct xnvme_ident *entry);

/**
 * Prints the given ::xnvme_cli_enumeration to the given output stream
 *
 * @param stream output stream used for printing
 * @param list pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_cli_enumeration_fpr(FILE *stream, struct xnvme_cli_enumeration *list, int opts);

int
xnvme_cli_enumeration_fpp(FILE *stream, struct xnvme_cli_enumeration *list, int opts);

int
xnvme_cli_enumeration_pp(struct xnvme_cli_enumeration *list, int opts);

/**
 * Prints the given ::xnvme_cli_enumeration to stdout
 *
 * @param list pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_cli_enumeration_pr(struct xnvme_cli_enumeration *list, int opts);

uint64_t
xnvme_cli_timer_start(struct xnvme_cli *cli);

uint64_t
xnvme_cli_timer_stop(struct xnvme_cli *cli);

void
xnvme_cli_timer_bw_pr(struct xnvme_cli *cli, const char *prefix, size_t nbytes);

void
xnvme_cli_pinf(const char *format, ...);

void
xnvme_cli_perr(const char *msg, int err);

/**
 * Parse the command line by the subcommands of the given ::xnvme_cli and run the one named
 *
 * Without arguments, or with '--help', prints the usage; with '--version', the version of the
 * library and the revision the program was built from. With '--list-cmds' as the only
 * argument, prints the name of each subcommand on a line of its own, for a shell completion.
 *
 * @param cli the command-line interface: its title, descriptions and subcommands
 * @param argc the argument count as given to main()
 * @param argv the arguments as given to main()
 * @param opts ::xnvme_cli_opts, such as XNVME_CLI_INIT_DEV_OPEN to open the device named
 *
 * @return On success, 0 or the return value of the subcommand. On error, non-zero.
 */
int
xnvme_cli_run(struct xnvme_cli *cli, int argc, char **argv, int opts);

/**
 * Fill the given 'opts' with values parsed in the given 'cli'
 *
 * @param cli The command-line-interface to parse arguments for
 * @param opts The device-options to fill
 *
 * @return On success, 0 is returned.
 */
int
xnvme_cli_to_opts(const struct xnvme_cli *cli, struct xnvme_opts *opts);

/**
 * Split a device argument into its URI and the namespace it names
 *
 * A device argument is a URI, optionally followed by a query naming the namespace:
 * "<uri>/?nsid=<nsid>" or "<uri>?nsid=<nsid>", the identifier in decimal or hex. The
 * device-open helpers of xnvme_cli use this, so every tool takes such arguments. It is a
 * convention of the command line only: ::xnvme_dev_open takes the URI without a query.
 *
 * @param arg The device argument
 * @param uri Set to the URI without the query
 * @param uri_nbytes Size of the buffer 'uri' points to
 * @param nsid Set to the namespace identifier when the query names one, left as is otherwise
 *
 * @return 1 when the query names a namespace, 0 when there is no query. On error, negative
 * errno is returned: -EINVAL for an unknown key, a malformed identifier, or a URI that does
 * not fit 'uri'.
 */
int
xnvme_cli_uri_parse(const char *arg, char *uri, size_t uri_nbytes, uint32_t *nsid);

/**
 * Open a device for each of the given URIs
 *
 * A URI may name its namespace as described for ::xnvme_cli_uri_parse; a namespace named
 * that way must be active, or the open fails with -ENODEV.
 *
 * @param uris The device URIs to open
 * @param count The number of URIs
 * @param opts The device-options to open them with
 * @param devs Set to an array of 'count' devices, release it with ::xnvme_cli_dev_close_multi
 *
 * @return On success, 0 is returned. On error, negative errno is returned and no device
 * is left open.
 */
int
xnvme_cli_dev_open_multi(const char **uris, int count, struct xnvme_opts *opts,
			 struct xnvme_dev ***devs);

/**
 * Close the devices opened by ::xnvme_cli_dev_open_multi and free the array
 *
 * @param devs The devices to close
 * @param count The number of devices
 */
void
xnvme_cli_dev_close_multi(struct xnvme_dev **devs, int count);

#ifdef __cplusplus
}
#endif

#endif
