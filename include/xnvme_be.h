// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_H
#define __INTERNAL_XNVME_BE_H
#include <paths.h>
#include <libxnvme_pp.h>
#include <xnvme.h>
#include <xnvme_be_registry.h>
#include <stdbool.h>

#define XNVME_BE_QUEUE_STATE_NBYTES 256

#define XNVME_BE_ASYNC_NBYTES 48
#define XNVME_BE_SYNC_NBYTES 16
#define XNVME_BE_ADMIN_NBYTES 16
#define XNVME_BE_DEV_NBYTES 24
#define XNVME_BE_MEM_NBYTES 32
#define XNVME_BE_ATTR_NBYTES 24
#define XNVME_BE_STATE_NBYTES 128
#define XNVME_BE_MIXINS_NBYTES 16
#define XNVME_BE_NBYTES \
	( XNVME_BE_ASYNC_NBYTES + XNVME_BE_SYNC_NBYTES + XNVME_BE_ADMIN_NBYTES + \
	  XNVME_BE_DEV_NBYTES + XNVME_BE_MEM_NBYTES + XNVME_BE_ATTR_NBYTES + \
	  XNVME_BE_STATE_NBYTES + XNVME_BE_MIXINS_NBYTES)

XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_attr) == XNVME_BE_ATTR_NBYTES, "Incorrect size");

struct xnvme_be_async {
	// Submit an async io command to be processed on the backend's io path
	int (*cmd_io)(struct xnvme_cmd_ctx *, void *, size_t, void *, size_t);

	// Non-blocking reaping of up to `max` io completions
	int (*poke)(struct xnvme_queue *, uint32_t);

	// Blocking reaping of all outstanding io completions
	int (*wait)(struct xnvme_queue *);

	// Do initialization of the underlying backend's io path
	int (*init)(struct xnvme_queue *, int);

	// Close resources allocated for the underlying backend's io path
	int (*term)(struct xnvme_queue *);

	// Check if the backend is supported in the current environment
	const char *id;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_async) == XNVME_BE_ASYNC_NBYTES, "Incorrect size")

struct xnvme_be_sync {
	/**
	 * Pass a NVMe I/O Command Through to the device with minimal driver
	 * intervention
	 */
	int (*cmd_io)(struct xnvme_cmd_ctx *, void *, size_t, void *, size_t);

	const char *id;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_sync) == XNVME_BE_SYNC_NBYTES, "Incorrect size")

struct xnvme_be_admin {
	/**
	 * Pass a NVMe Admin Command Through to the device with minimal driver intervention
	 */
	int (*cmd_admin)(struct xnvme_cmd_ctx *, void *, size_t, void *, size_t);

	const char *id;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_admin) == XNVME_BE_ADMIN_NBYTES, "Incorrect size")

struct xnvme_be_dev {
	/**
	 * Enumerate devices on/at the given 'sys_uri' when NULL local devices
	 */
	int (*enumerate)(struct xnvme_enumeration *, const char *, int);

	/**
	 * Construct a device from the given identifier
	 */
	int (*dev_open)(struct xnvme_dev *);

	/**
	 * Close the given device
	 */
	void (*dev_close)(struct xnvme_dev *);
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_dev) == XNVME_BE_DEV_NBYTES, "Incorrect size")

struct xnvme_be_mem {
	/**
	 * Allocate a buffer usable for NVMe commands
	 */
	void *(*buf_alloc)(const struct xnvme_dev *, size_t, uint64_t *);

	/**
	 * Resolve the physical address of the given buffer
	 */
	int (*buf_vtophys)(const struct xnvme_dev *, void *, uint64_t *);

	/**
	 * Re-allocate a buffer usable for NVMe commands
	 */
	void *(*buf_realloc)(const struct xnvme_dev *, void *, size_t,
			     uint64_t *);

	/**
	 * Free a buffer usable for NVMe commands
	 */
	void (*buf_free)(const struct xnvme_dev *, void *);
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_mem) == XNVME_BE_MEM_NBYTES, "Incorrect size")

/**
 * Backend function-interface
 */
enum xnvme_be_mixin_type {
	XNVME_BE_MEM	= 0x1 << 0,
	XNVME_BE_ADMIN	= 0x1 << 1,
	XNVME_BE_SYNC	= 0x1 << 2,
	XNVME_BE_ASYNC	= 0x1 << 3,
	XNVME_BE_DEV	= 0x1 << 4,
	XNVME_BE_ATTR	= 0x1 << 5,
	XNVME_BE_END	= 0x1 << 6
};
#define XNVME_BE_CONFIGURED (	\
	XNVME_BE_ADMIN |	\
	XNVME_BE_DEV |		\
	XNVME_BE_MEM |		\
	XNVME_BE_SYNC |		\
	XNVME_BE_ASYNC)

static inline const char *
xnvme_be_mixin_key(enum xnvme_be_mixin_type mtype)
{
	switch (mtype) {
	case XNVME_BE_ASYNC:
		return "async";
	case XNVME_BE_SYNC:
		return "sync";
	case XNVME_BE_ADMIN:
		return "admin";
	case XNVME_BE_DEV:
		return "dev";
	case XNVME_BE_ATTR:
		return "attr";
	case XNVME_BE_MEM:
		return "mem";
	case XNVME_BE_END:
		return "end";
	}

	return "enosys";
}

struct xnvme_be_mixin {
	enum xnvme_be_mixin_type mtype;
	const char *name;
	const char *descr;
	union {
		struct xnvme_be_async *async;
		struct xnvme_be_sync *sync;
		struct xnvme_be_admin *admin;
		struct xnvme_be_mem *mem;
		struct xnvme_be_dev *dev;
		void *obj;
	};
	int (*check_support)(struct xnvme_dev *, uint32_t);
};

#define XNVME_BE_MIXIN_NAME_LEN 32
#define XNVME_BE_OPTION_CHAR_LEN 128

struct xnvme_be_options {
	union {
		struct {
			uint32_t admin : 1;
			uint32_t sync : 1;
			uint32_t async : 1;
			uint32_t mem : 1;
			uint32_t dev : 1;

			uint32_t oflags : 1;
			uint32_t mode : 1;
			uint32_t poll_io : 1;
			uint32_t poll_sq : 1;
			uint32_t nsid : 1;
			uint32_t cmb_sqs: 1;
			uint32_t css : 1;
			uint32_t shm_id : 1;
			uint32_t corelist : 1;
			uint32_t adrfam : 1;

			uint32_t _rsvd : 16;
		};
		uint32_t val;
	} provided;

	char admin[XNVME_BE_MIXIN_NAME_LEN];	///< Name of the admin-interface to use
	char sync[XNVME_BE_MIXIN_NAME_LEN];	///< Name of the sync-interface to use
	char async[XNVME_BE_MIXIN_NAME_LEN];	///< Name of async-interface to use
	char mem[XNVME_BE_MIXIN_NAME_LEN];	///< Name of the memory-interface to use
	char dev[XNVME_BE_MIXIN_NAME_LEN];	///< Name of the device-handle interface to use

	int oflags;		///< File open flags XNVME_FILE_OFLG_CREATE
	int mode;		///< File permissions/mode when oflags | XNVME_OFLG_CREATE

	uint32_t poll_io;	///< For kernel-side completion-polling in io_uring
	uint32_t poll_sq;	///< For kernel-side submission-polling in io_uring

	uint32_t nsid;		///< Identifier of the namespace to use
	uint32_t cmb_sqs;	///< Use submission-queues in controller-memory-buffers
	uint32_t css;		///< Use 'css' as 'command-set-selection'
	uint32_t shm_id;	///< Shared memory id
	char corelist[XNVME_BE_OPTION_CHAR_LEN];	///< List of cores to bind to
	char adrfam[XNVME_BE_OPTION_CHAR_LEN];		///< List of cores to bind to
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_options) == 452, "Incorrect size")

/**
 * Backend interface consisting of functions, attributes and instance state
 */
struct xnvme_be {
	struct xnvme_be_async async;		///< Asynchronous I/O command interface
	struct xnvme_be_sync sync;		///< Synchronous I/O command interface
	struct xnvme_be_admin admin;		///< Administrative command interface
	struct xnvme_be_dev dev;		///< Device functions
	struct xnvme_be_attr attr;		///< Backend Attributes
	struct xnvme_be_mem mem;		///< Memory Management interface
	uint8_t state[XNVME_BE_STATE_NBYTES];	///< Backend instance state

	struct xnvme_be_mixin *objs;
	uint64_t nobjs;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be) == XNVME_BE_NBYTES, "Incorrect size")

/**
 * Auxilary helpers
 */

/**
 * Resolve the name of the given backend identifier
 */
const char *
xnvme_be_id2name(int bid);

/**
 * Resolve the internal identifier of the given backend name.
 *
 * @returns Returns numerical identifier on success. On error, e.g. the given
 * name does not exists in the list of backend implementations, -1 is returned.
 */
int
xnvme_be_name2id(const char *bname);

/**
 * Produce a device with a backend attached
 */
int
xnvme_be_factory(const char *uri, struct xnvme_dev *dev);

int
xnvme_be_yaml(FILE *stream, const struct xnvme_be *be, int indent, const char *sep, int head);

int
xnvme_be_fpr(FILE *stream, const struct xnvme_be *be, enum xnvme_pr opts);

int
xnvme_be_pr(const struct xnvme_be *be, enum xnvme_pr opts);

int
xnvme_be_dev_derive_geometry(struct xnvme_dev *dev);

int
xnvme_be_dev_idfy(struct xnvme_dev *dev);

int
uri_parse_scheme(const char *uri, char *scheme);

int
xnvme_ident_yaml(FILE *stream, const struct xnvme_ident *ident, int indent, const char *sep,
		 int head);

int
xnvme_enumeration_alloc(struct xnvme_enumeration **list, uint32_t capacity);

void
xnvme_enumeration_free(struct xnvme_enumeration *list);

int
xnvme_enumeration_append(struct xnvme_enumeration *list, struct xnvme_ident *entry);

bool
has_scheme(const char *needle, const char *haystack[], int len);

int
path_to_ll(const char *path, uint64_t *val);

bool
xnvme_ident_opt_to_val(const struct xnvme_ident *ident, const char *opt, uint32_t *val);

/**
 * Copies an option value to the given buf
 *
 * The given 'opts' string is on the form "?opt1=val_x?opt2=val_y"
 * The given 'opt' is a string on  the form "opt2"
 * The provided 'buf' must contain atleast 'buf_len' bytes of memory
 */
bool
xnvme_ident_optval_to_buf(const char *opts, const char *opt, char *buf, uint32_t buf_len);

static inline int
xnvme_be_supported(struct xnvme_dev *XNVME_UNUSED(dev), uint32_t XNVME_UNUSED(opts))
{
	return 1;
}

bool
check_cmask_validity(const char *cmask, int nproc);

int
xnvme_be_options_from_ident(const struct xnvme_ident *ident, struct xnvme_be_options *opts);

#endif /* __INTERNAL_XNVME_BE_H */
