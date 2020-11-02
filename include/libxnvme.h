/**
 * Cross-platform I/O library for NVMe based devices
 *
 * Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
 * Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file libxnvme.h
 */
#ifndef __LIBXNVME_H
#define __LIBXNVME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <libxnvme_spec.h>
#include <libxnvme_util.h>
#include <libxnvme_geo.h>
#include <libxnvme_dev.h>

/**
 * Representation of xNVMe library backend attributes
 *
 * @struct xnvme_be_attr_list
 */
struct xnvme_be_attr {
	const char *name;	///< Backend name

	/**
	 * The default URI 'scheme', as in, the URI-definition below:
	 *
	 * scheme:target[?option=val]
	 *
	 * For which this backend identifies devices
	 */
	const char **schemes;

	uint32_t nschemes;		///< Number of schemes in 'schemes'
	uint32_t enabled;		///< Whether the backend is 'enabled'
};

/**
 * List of xNVMe library backend attributes
 *
 * @struct xnvme_be_attr_list
 */
struct xnvme_be_attr_list {
	uint32_t capacity;		///< Remaining unused entries
	int count;			///< Number of used entries
	struct xnvme_be_attr item[];	///< Array of items
};

/**
 * List backends supported by the library
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_be_attr_list(struct xnvme_be_attr_list **list);

#define XNVME_IDENT_URI_LEN 384
#define XNVME_IDENT_URI_LEN_MIN 10

#define XNVME_IDENT_SCHM_LEN 5
#define XNVME_IDENT_TRGT_LEN 155
#define XNVME_IDENT_OPTS_LEN 160
#define XNVME_IDENT_OPTS_SEP '?'

/**
 * Representation of device identifiers once decoded from text-representation
 *
 * @struct xnvme_ident
 */
struct xnvme_ident {
	char uri[XNVME_IDENT_URI_LEN];

	char schm[XNVME_IDENT_SCHM_LEN];
	char trgt[XNVME_IDENT_TRGT_LEN];
	char opts[XNVME_IDENT_OPTS_LEN];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_ident) == 704, "Incorrect size")

/**
 * Parse the given 'uri' into 'ident'
 *
 * @param uri
 * @param ident Pointer to ident to fill with values parsed from 'ident_uri'
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_ident_from_uri(const char *uri, struct xnvme_ident *ident);

/**
 * List of devices found on the system usable with xNVMe
 *
 * @struct xnvme_sys_list
 */
struct xnvme_enumeration {
	uint32_t capacity;		///< Remaining unused entries
	uint32_t nentries;		///< Used entries
	struct xnvme_ident entries[];	///< Device entries
};

/**
 * Enumerate devices on the given system
 *
 * @param list Pointer to pointer of the list of device enumerated
 * @param sys_uri URI of the system to enumerate on, when NULL, localhost/PCIe
 * @param opts System enumeration options
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_enumerate(struct xnvme_enumeration **list, const char *sys_uri, int opts);

/**
 * Opaque device handle.
 *
 * @struct xnvme_dev
 */
struct xnvme_dev;

/**
 * Creates a handle to given device identifier
 *
 * @param dev_uri File path "/dev/nvme0n1" or "pci://0000:04.01"
 *
 * @return On success, a handle to the device. On error, NULL is returned and
 * `errno` set to indicate the error.
 */
struct xnvme_dev *
xnvme_dev_open(const char *dev_uri);

/**
 * Destroys device-handle
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 */
void
xnvme_dev_close(struct xnvme_dev *dev);

/**
 * Allocate a buffer for IO with the given device
 *
 * The buffer will be aligned to device geometry and DMA allocated if required
 * by the backend for IO
 *
 * @note
 * nbytes must be greater than zero and a multiple of minimal granularity
 * @note
 * De-allocate the buffer using xnvme_buf_free()
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nbytes The size of the allocated buffer in bytes
 * @param phys A pointer to the variable to hold the physical address of the
 * allocated buffer. If NULL, the physical address is not returned.
 *
 * @return On success, a pointer to the allocated memory is returned. On error,
 * NULL is returned and `errno` set to indicate the error.
 */
void *
xnvme_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys);

/**
 * Reallocate a buffer for IO with the given device
 *
 * The buffer will be aligned to device geometry and DMA allocated if required
 * by the backend for IO
 *
 * @note
 * nbytes must be greater than zero and a multiple of minimal granularity
 * @note
 * De-allocate the buffer using xnvme_buf_free()
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param buf The buffer to reallocate
 * @param nbytes The size of the allocated buffer in bytes
 * @param phys A pointer to the variable to hold the physical address of the
 * allocated buffer. If NULL, the physical address is not returned.
 *
 * @return On success, a pointer to the allocated memory is returned. On error,
 * NULL is returned and `errno` set to indicate the error.
 */
void *
xnvme_buf_realloc(const struct xnvme_dev *dev, void *buf, size_t nbytes,
		  uint64_t *phys);

/**
 * Free the given IO buffer allocated with xnvme_buf_alloc()
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param buf Pointer to a buffer allocated with xnvme_buf_alloc()
 */
void
xnvme_buf_free(const struct xnvme_dev *dev, void *buf);

/**
 * Retrieve the physical address of the given buffer
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param buf Pointer to a buffer allocated with xnvme_buf_alloc()
 * @param phys A pointer to the variable to hold the physical address of the
 * given buffer.
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_buf_vtophys(const struct xnvme_dev *dev, void *buf, uint64_t *phys);

/**
 * Allocate a buffer of virtual memory of the given `alignment` and `nbytes`
 *
 * @note
 * You must use xnvme_buf_virt_free() to de-allocate the buffer
 *
 * @param alignment The alignment in bytes
 * @param nbytes The size of the buffer in bytes
 *
 * @return On success, a pointer to the allocated memory is return. On error,
 * NULL is returned and `errno` set to indicate the error
 */
void *
xnvme_buf_virt_alloc(size_t alignment, size_t nbytes);

/**
 * Free the given virtual memory buffer
 *
 * @param buf Pointer to a buffer allocated with xnvme_buf_virt_alloc()
 */
void
xnvme_buf_virt_free(void *buf);

/**
 * Opaque asynchronous context as provided by xnvme_async_init()
 *
 * @see xnvme_async_init
 * @see xnvme_async_term
 *
 * @struct xnvme_async_ctx
 */
struct xnvme_async_ctx;

/**
 * Async / queue flags
 * @enum xnvme_async_opts
 */
enum xnvme_async_opts {
	XNVME_ASYNC_IOPOLL = 0x1,       ///< XNVME_ASYNC_IOPOLL: ctx. is polled for completions
	XNVME_ASYNC_SQPOLL = 0x1 << 1,  ///< XNVME_ASYNC_SQPOLL: ctx. is polled for submissions
};

/**
 * Allocate an asynchronous context for command submission of the given depth
 * for submission of commands to the given device
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param ctx Pointer-pointer to initialized context
 * @param depth Maximum iodepth / qdepth, maximum number of outstanding commands
 * @param flags Initialization flags
 * of the returned context, note that is must be a power of 2 within the range
 * [1,4096]
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_async_init(struct xnvme_dev *dev, struct xnvme_async_ctx **ctx, uint16_t depth, int flags);

/**
 * Get the I/O depth of the context.
 *
 * @param ctx Asynchronous context
 *
 * @return On success, depth of the given context is returned. On error, 0 is
 * returned e.g. errors are silent
 */
uint32_t
xnvme_async_get_depth(struct xnvme_async_ctx *ctx);

/**
 * Get the number of outstanding I/O.
 *
 * @param ctx Asynchronous context
 *
 * @return On success, number of outstanding commands are returned. On error, 0
 * is returned e.g. errors are silent
 */
uint32_t
xnvme_async_get_outstanding(struct xnvme_async_ctx *ctx);

/**
 * Tear down the given Asynchronous context
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param ctx
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_async_term(struct xnvme_dev *dev, struct xnvme_async_ctx *ctx);

/**
 * Process completions from the given Asynchronous context
 *
 * Set process 'max' to limit number of completions, 0 means no max.
 *
 * @return On success, number of completions processed, may be 0. On error,
 * negative `errno` is returned.
 */
int
xnvme_async_poke(struct xnvme_dev *dev, struct xnvme_async_ctx *ctx,
		 uint32_t max);

/**
 * Wait for completion of all outstanding commands in the given 'ctx'
 *
 * @return On success, number of completions processed, may be 0. On error,
 * negative `errno` is returned.
 */
int
xnvme_async_wait(struct xnvme_dev *dev, struct xnvme_async_ctx *ctx);

/**
 * Forward declaration, see definition further down
 */
struct xnvme_req;
struct xnvme_req_pool;

/**
 * Signature of function used with asynchronous callbacks.
 */
typedef void (*xnvme_async_cb)(struct xnvme_req *req, void *opaque);

/**
 * Encapsulation and representation of lower-level error conditions
 *
 * @struct xnvme_req
 */
struct xnvme_req {
	struct xnvme_spec_cpl cpl;		///< NVMe completion

	///< Fields for CMD_OPT: XNVME_CMD_ASYNC
	struct {
		struct xnvme_async_ctx *ctx;	///< Asynchronous context
		xnvme_async_cb cb;		///< User callback function
		void *cb_arg;			///< User callback arguments

		///< Per request backend specific data
		uint8_t be_rsvd[8];
	} async;

	///< Fields for request-pool
	struct xnvme_req_pool *pool;
	SLIST_ENTRY(xnvme_req) link;
};

struct xnvme_req_pool {
	SLIST_HEAD(, xnvme_req) head;
	uint32_t capacity;
	struct xnvme_req elm[];
};

int
xnvme_req_pool_alloc(struct xnvme_req_pool **pool, uint32_t capacity);

int
xnvme_req_pool_init(struct xnvme_req_pool *pool, struct xnvme_async_ctx *ctx,
		    xnvme_async_cb cb, void *cb_args);

void
xnvme_req_pool_free(struct xnvme_req_pool *pool);

/**
 * Clears/resets the given #xnvme_req
 *
 * @param req Pointer to the #xnvme_req to clear
 */
void
xnvme_req_clear(struct xnvme_req *req);

/**
 * Encapsulate completion-error checking here for now.
 *
 * @todo re-think this
 * @param req Pointer to the #xnvme_req to check status on
 *
 * @return On success, 0 is return. On error, a non-zero value is returned.
 */
static inline int
xnvme_req_cpl_status(struct xnvme_req *req)
{
	return req->cpl.status.sc || req->cpl.status.sct;
}

/**
 * Enumeration of `xnvme_cmd` options
 *
 * @enum xnvme_cmd_opts
 */
enum xnvme_cmd_opts {
	XNVME_CMD_SYNC		= 0x1 << 0,	///< XNVME_CMD_SYNC: Synchronous command
	XNVME_CMD_ASYNC		= 0x1 << 1,	///< XNVME_CMD_ASYNC: Asynchronous command

	XNVME_CMD_UPLD_SGLD	= 0x1 << 2,	///< XNVME_CMD_UPLD_SGLD: User-managed SGL data
	XNVME_CMD_UPLD_SGLM	= 0x1 << 3,	///< XNVME_CMD_UPLD_SGLM: User-managed SGL meta
};

#define XNVME_CMD_MASK_IOMD ( XNVME_CMD_SYNC | XNVME_CMD_ASYNC )
#define XNVME_CMD_MASK_UPLD ( XNVME_CMD_UPLD_SGLD | XNVME_CMD_UPLD_SGLM )
#define XNVME_CMD_MASK ( XNVME_CMD_MASK_IOMD | XNVME_CMD_MASK_UPLD )

#define XNVME_CMD_DEF_IOMD XNVME_CMD_SYNC
#define XNVME_CMD_DEF_UPLD ( 0x0 )

/**
 * Pass a NVMe IO Command through to the device with minimal intervention
 *
 * When constructing the command then take note of the following:
 *
 * - The CID is managed at a lower level and probably over-written if assigned
 * - When 'opts' include XNVME_CMD_PRP then just pass buffers allocated with
 *   `xnvme_buf_alloc`, the construction of PRP-lists, assignment to command and
 *   assignment of pdst is managed at lower levels
 * - When 'opts'' include XNVME_CMD_SGL then both data, and meta, when given
 *   must be setup via the `xnvme_sgl` helper functions, pdst, data, and meta
 *   fields must be also be set by you
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param cmd Pointer to the NVMe command to submit
 * @param dbuf pointer to data-payload
 * @param dbuf_nbytes size of data-payload in bytes
 * @param mbuf pointer to meta-payload
 * @param mbuf_nbytes size of the meta-payload in bytes
 * @param opts Command options; see
 * @param req Pointer to structure for async. context and NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_cmd_pass(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd, void *dbuf, size_t dbuf_nbytes,
	       void *mbuf, size_t mbuf_nbytes, int opts, struct xnvme_req *req);

/**
 * Pass a NVMe Admin Command through to the device with minimal intervention
 *
 * When constructing the command then take note of the following:
 *
 * - The CID is managed at a lower level and probably over-written if assigned
 * - When 'opts' include XNVME_CMD_PRP then just pass buffers allocated with
 *   `xnvme_buf_alloc`, the construction of PRP-lists, assignment to command and
 *   assignment of pdst is managed at lower levels
 * - When 'opts'' include XNVME_CMD_SGL then both data, and meta, when given
 *   must be setup via the `xnvme_sgl` helper functions, pdst, data, and meta
 *   fields must be also be set by you
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param cmd Pointer to the NVMe command to submit
 * @param dbuf pointer to data-payload
 * @param dbuf_nbytes size of data-payload in bytes
 * @param mbuf pointer to meta-payload
 * @param mbuf_nbytes size of the meta-payload in bytes
 * @param opts Command options; see
 * @param req Pointer to structure for async. context and NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_cmd_pass_admin(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd, void *dbuf,
		     size_t dbuf_nbytes, void *mbuf, size_t mbuf_nbytes, int opts,
		     struct xnvme_req *req);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_H */
