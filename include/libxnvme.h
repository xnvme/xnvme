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
#include <libxnvme_util.h>
#include <libxnvme_spec.h>

/**
 * Options for pretty-printer (``*_pr``, ``*_fpr``) functions
 *
 * @enum xnvme_pr
 */
enum xnvme_pr {
	XNVME_PR_DEF = 0x0,	///< XNVME_PR_DEF: Default options
	XNVME_PR_YAML = 0x1,	///< XNVME_PR_YAML: Print formatted as YAML
	XNVME_PR_TERSE = 0x2	///< XNVME_PR_TERSE: Print without formatting
};

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

	int nschemes;		///< Number of schemes in 'schemes'
	int enabled;		///< Whether the backend is 'enabled'
};

/**
 * Prints the given backend attribute to the given output stream
 *
 * @param stream output stream used for printing
 * @param attr Pointer to the ::xnvme_be_attr to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_be_attr_fpr(FILE *stream, const struct xnvme_be_attr *attr, int opts);

/**
 * Prints the given backend attribute to stdout
 *
 * @param attr Pointer to the ::xnvme_be_attr to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_be_attr_pr(const struct xnvme_be_attr *attr, int opts);

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
 * Prints the given backend attribute list to the given output stream
 *
 * @param stream output stream used for printing
 * @param list Pointer to the backend attribute list to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_be_attr_list_fpr(FILE *stream, const struct xnvme_be_attr_list *list,
		       int opts);

/**
 * Prints the given backend attribute list to standard out
 *
 * @param list Pointer to the backend attribute list to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_be_attr_list_pr(const struct xnvme_be_attr_list *list, int opts);

/**
 * List backends supported by the library
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_be_attr_list(struct xnvme_be_attr_list **list);

/**
 * Prints the given LBA to the given output stream
 *
 * @param stream output stream used for printing
 * @param lba the LBA to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_lba_fpr(FILE *stream, uint64_t lba, enum xnvme_pr opts);

/**
 * Prints the given Logical Block Addresses (LBA) to stdout
 *
 * @param lba the LBA to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_lba_pr(uint64_t lba, enum xnvme_pr opts);

/**
 * Prints the given list of Logical Block Addresses (LBAs)to the given output
 * stream
 *
 * @param stream output stream used for printing
 * @param lba Pointer to an array of LBAs to print
 * @param nlb Number of LBAs to print from the given list
 * @param opts printer options, see ::xnvme_pr
 */
int
xnvme_lba_fprn(FILE *stream, const uint64_t *lba, uint16_t nlb,
	       enum xnvme_pr opts);

/**
 * Print a list of Logical Block Addresses (LBAs)
 *
 * @param lba Pointer to an array of LBAs to print
 * @param nlb Length of the array in items
 * @param opts Printer options
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_lba_prn(const uint64_t *lba, uint16_t nlb, enum xnvme_pr opts);

/**
 * Produces the "major" version of the library
 *
 * @return On success, the "major" version of the library is returned
 */
int
xnvme_ver_major(void);

/**
 * Produces the "minor" version of the library
 *
 * @return On success, the "minor" version of the library is returned
 */
int
xnvme_ver_minor(void);

/**
 * Produces the "patch" version of the library
 *
 * @return On success, the "patch" version of the library is returned
 */
int
xnvme_ver_patch(void);

/**
 * Prints the library version to the given 'stream'
 *
 * @param stream output stream used for printing
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_ver_fpr(FILE *stream, int opts);

/**
 * Prints the library version to stdout
 *
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_ver_pr(int opts);

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
 * Prints the given ::xnvme_ident to the given output stream
 *
 * @param stream output stream used for printing
 * @param ident pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_ident_fpr(FILE *stream, const struct xnvme_ident *ident, int opts);

/**
 * Prints the given ::xnvme_ident to stdout
 *
 * @param ident pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_ident_pr(const struct xnvme_ident *ident, int opts);

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
 * Prints the given ::xnvme_enumeration to the given output stream
 *
 * @param stream output stream used for printing
 * @param list pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_enumeration_fpr(FILE *stream, struct xnvme_enumeration *list, int opts);

/**
 * Prints the given ::xnvme_enumeration to stdout
 *
 * @param list pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_enumeration_pr(struct xnvme_enumeration *list, int opts);

/**
 * Enumerate devices on the system
 *
 * @param list Pointer to pointer of the list of device enumerated
 * @param opts System enumeration options
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_enumerate(struct xnvme_enumeration **list, int opts);

/**
 * Opaque device handle.
 *
 * @struct xnvme_dev
 */
struct xnvme_dev;

/**
 * Prints the given ::xnvme_dev to the given output stream
 *
 * @param stream output stream used for printing
 * @param dev pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_dev_fpr(FILE *stream, const struct xnvme_dev *dev, int opts);

/**
 * Prints the given ::xnvme_dev to stdout
 *
 * @param dev pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_dev_pr(const struct xnvme_dev *dev, int opts);

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
 * Opaque handle for Scatter Gather List (SGL).
 *
 * @struct xnvme_sgl
 */
struct xnvme_sgl;

/**
 * Opaque handle for Scatter Gather List (SGL) pool
 *
 * @note A separate pool should be used for each asynchronous context
 *
 * @struct xnvme_sgl_pool
 */
struct xnvme_sgl_pool;

/**
 * Create an pool of Scather-Gather-List (SGL)
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 *
 * @return An initialized pool that can be used to amortize the cost of repeated
 * SGL allocations
 */
struct xnvme_sgl_pool *
xnvme_sgl_pool_create(struct xnvme_dev *dev);

/**
 * Destroy an SGL pool (and all SGLs in the pool).
 *
 * @param pool Pointer to the #xnvme_sgl_pool to destroy
 */
void
xnvme_sgl_pool_destroy(struct xnvme_sgl_pool *pool);

/**
 * Create a Scatter-Gather-List (SGL)
 *
 * @see xnvme_sgl_destroy
 * @see xnvme_sgl_alloc
 * @see xnvme_sgl_free
 * @see xnvme_sgl_add
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param hint Allocation hint.
 *
 * @return On success, an initialized (empty) SGL is returned. On error, NULL is
 * returned and `errno` is set to indicate any error.
 */
struct xnvme_sgl *
xnvme_sgl_create(struct xnvme_dev *dev, int hint);

/**
 * Destroy an SGL, freeing the memory used
 *
 * @see xnvme_sgl_free
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param sgl Pointer to SGL as allocated by xnvme_sgl_alloc() or
 * xnvme_sgl_create()
 */
void
xnvme_sgl_destroy(struct xnvme_dev *dev, struct xnvme_sgl *sgl);

/**
 * Allocate an SGL from a pool
 *
 * @see xnvme_sgl_create
 *
 * @param pool Pool to allocate SGL from
 *
 * @return On success, an initialized (empty) SGL is returned. On error, NULL is
 * returned and `errno` is set to indicate any error.
 */
struct xnvme_sgl *
xnvme_sgl_alloc(struct xnvme_sgl_pool *pool);

/**
 * Free an SGL, returning it to the pool
 *
 * @see xnvme_sgl_destroy
 *
 * @param pool Pool to return the given `sgl` to
 * @param sgl Pointer to SGL as allocated by xnvme_sgl_alloc() or
 *  xnvme_sgl_create()
 */
void
xnvme_sgl_free(struct xnvme_sgl_pool *pool, struct xnvme_sgl *sgl);

/**
 * Reset an SGL
 *
 * @param sgl Pointer to SGL as allocated by xnvme_sgl_alloc() or
 * xnvme_sgl_create()
 */
void
xnvme_sgl_reset(struct xnvme_sgl *sgl);

/**
 * Add an entry to the SGL
 *
 * @see xnvme_sgl_alloc
 * @see xnvme_buf_alloc
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param sgl Pointer to sgl as allocated by xnvme_sgl_alloc()
 * @param buf Pointer to buffer as allocated with xnvme_buf_alloc()
 * @param nbytes Size of the given buffer in bytes
 */
int
xnvme_sgl_add(struct xnvme_dev *dev, struct xnvme_sgl *sgl, void *buf,
	      size_t nbytes);

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
 * Allocate an asynchronous context for command submission of the given depth
 * for submission of commands to the given device
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param ctx Pointer-pointer to initialized context
 * @param depth Maximum iodepth / qdepth, maximum number of outstanding commands
 * of the returned context, note that is must be a power of 2 within the range
 * [1,4096]
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_async_init(struct xnvme_dev *dev, struct xnvme_async_ctx **ctx,
		 uint16_t depth);

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
		uint8_t be_rsvd[16];
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
 * Prints a humanly readable representation the given #xnvme_req
 *
 * @param req Pointer to the #xnvme_req to print
 * @param opts Printer options
 */
void
xnvme_req_pr(const struct xnvme_req *req, int opts);

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
 * Representation of the type of device / geo / namespace
 *
 * @enum xnvme_geo_type
 */
enum xnvme_geo_type {
	XNVME_GEO_UNKNOWN = 0x0,
	XNVME_GEO_CONVENTIONAL = 0x1,
	XNVME_GEO_ZONED = 0x2
};

/**
 * Representation of device "geometry"
 *
 * This will remain in some, encapsulating IO parameters such as MDTS, ZONE
 * APPEND MDTS, nbytes, nsect etc. mapping to zone characteristics
 *
 * @struct xnvme_geo
 */
struct xnvme_geo {
	enum xnvme_geo_type type;

	/**
	 * Pseudo-geometry npugrp and npunit are irrelevant...
	 */
	uint32_t npugrp;		///< Nr. of Parallel Unit Groups
	uint32_t npunit;		///< Nr. of Parallel Units in PUG

	uint32_t nzone;		///< Nr. of zones in PU
	uint64_t nsect;		///< Nr. of sectors per zone
	uint32_t nbytes;	///< Nr. of bytes per sector
	uint32_t nbytes_oob;	///< Nr. of bytes per sector in OOB

	uint64_t tbytes;		///< Total # bytes in geometry

	uint32_t mdts_nbytes;	///< Maximum data transfer size in bytes

	uint8_t _rsvd[20];
};

/**
 * Prints the given ::xnvme_geo to the given output stream
 *
 * @param stream output stream used for printing
 * @param geo pointer to the the ::xnvme_geo to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_geo_fpr(FILE *stream, const struct xnvme_geo *geo, int opts);

/**
 * Prints the given ::xnvme_geo to stdout
 *
 * @param geo pointer to the the ::xnvme_geo to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_geo_pr(const struct xnvme_geo *geo, int opts);

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
xnvme_cmd_pass(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd, void *dbuf,
	       size_t dbuf_nbytes, void *mbuf, size_t mbuf_nbytes, int opts,
	       struct xnvme_req *req);

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
xnvme_cmd_pass_admin(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
		     void *dbuf, size_t dbuf_nbytes, void *mbuf,
		     size_t mbuf_nbytes, int opts, struct xnvme_req *req);

/**
 * Submit and wait for completion of an NVMe Identify command
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param cns the ::xnvme_spec_idfy_cns to retrieve
 * @param cntid Controller Identifier
 * @param nsid Namespace Identifier
 * @param nst Namespace Type
 * @param nvmsetid NVM set Identifier
 * @param uuid UUID index
 * @param dbuf ponter to data-payload
 * @param req Pointer to structure for NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_cmd_idfy(struct xnvme_dev *dev, uint8_t cns, uint16_t cntid, uint8_t nsid,
	       uint8_t nst, uint16_t nvmsetid, uint8_t uuid,
	       struct xnvme_spec_idfy *dbuf, struct xnvme_req *req);

/**
 * Submit and wait for completion of an NVMe Identify command for the
 * controller associated with the given device
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param dbuf pointer to data-payload
 * @param req Pointer to structure for NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_cmd_idfy_ctrlr(struct xnvme_dev *dev, struct xnvme_spec_idfy *dbuf,
		     struct xnvme_req *req);

/**
 * Submit and wait for completion of an NVMe Identify command for the given nsid
 *
 * Execute an NVMe identify namespace command for the namespace associated with
 * the given 'dev' when 'nsid'=0, execute for the given 'nsid' when it is > 0,
 * when the given 'nsid' == 0, then the command is executed for the namespace
 * associated with the given device
 *
 * @todo document
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace identifier
 * @param nstype Namespace type, identify namespace of the specific type. Ignore
 * the namespace typecheck by providning 0xFF
 * @param dbuf Buffer allocated by xnvme_buf_alloc()
 * @param req Pointer to structure for NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_cmd_idfy_ns(struct xnvme_dev *dev, uint32_t nsid, uint8_t nstype,
		  struct xnvme_spec_idfy *dbuf, struct xnvme_req *req);

/**
 * Submit and wait for completion of an NVMe Get Log Page command
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param lid Log Page Identifier for the log to retrieve entries for
 * @param lsp Log Specific Field for the log to retrieve entries for
 * @param lpo_nbytes Log page Offset in BYTES
 * @param nsid Namespace Identifier
 * @param rae Retain Asynchronous Event, 0=Clear, 1=Retain
 * @param dbuf Buffer allocated with `xnvme_buf_alloc`
 * @param dbuf_nbytes Number of BYTES to write from log-page to buf
 * @param req Pointer to structure for NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_cmd_log(struct xnvme_dev *dev, uint8_t lid, uint8_t lsp,
	      uint64_t lpo_nbytes, uint32_t nsid, uint8_t rae, void *dbuf,
	      uint32_t dbuf_nbytes, struct xnvme_req *req);

/**
 * Submit and wait for completion of an NVMe Get Features (gfeat) command
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace identifier
 * @param fid Feature identifier
 * @param sel Select which value of the feature to select, that is, one of
 * current/default/saved/supported
 * @param dbuf pointer to data-payload, use if required for the given `fid`
 * @param dbuf_nbytes size of data-payload in bytes
 * @param req Pointer to structure for NVMe completion
 *
 * @return
 */
int
xnvme_cmd_gfeat(struct xnvme_dev *dev, uint32_t nsid, uint8_t fid, uint8_t sel,
		void *dbuf, size_t dbuf_nbytes, struct xnvme_req *req);

/**
 * Submit and wait for completion of an NVMe Set Feature (sfeat) command
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace identifier
 * @param fid Feature identifier (see NVMe 1.3; Figure 84)
 * @param feat Structure defining feature attributes
 * @param save
 * @param dbuf pointer to data-payload
 * @param dbuf_nbytes size of the data-payload in bytes
 * @param req Pointer to structure for NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_cmd_sfeat(struct xnvme_dev *dev, uint32_t nsid, uint8_t fid,
		uint32_t feat, uint8_t save, const void *dbuf,
		size_t dbuf_nbytes, struct xnvme_req *req);

/**
 * Submit and wait for completion of an NVMe Format NVM command
 *
 * TODO: consider timeout, reset, and need for library/dev re-initialization
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace identifier
 * @param lbaf The LBA format to format the Namespace with
 * @param zf Zone Format
 * @param mset Metadata Settings
 * @param ses Secure erase settings; 0x0: No Secure Erase, 0x1: User Data erase,
 * 0x2: Cryptographic Erase
 * @param pil Protection information location; 0x0: last eight bytes of
 * metadata, 0x1: first eight bytes of metadata
 * @param pi Protection information; 0x0: Disabled, 0x1: Enabled(Type1), 0x2:
 * Enabled(Type2), 0x3: Enabled(Type3)
 * @param req Pointer to structure for NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_cmd_format(struct xnvme_dev *dev, uint32_t nsid, uint8_t lbaf, uint8_t zf,
		 uint8_t mset, uint8_t ses, uint8_t pi, uint8_t pil,
		 struct xnvme_req *req);

/**
 * Submit and wait for completion of an Sanitize command
 *
 * @todo consider timeout, reset, and need for library/dev re-initialization
 *
 * @note Success of this function only means that the sanitize *command*
 * succeeded and that the sanitize *operation* has started.
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param sanact Sanitize action; 0x1: Exit failure mode, 0x2: Block Erase, 0x3
 * Overwrite, 0x4 Crypto Erase
 * @param ause: Allow Unrestricted Sanitize Exit; 0x0: Restricted Mode, 0x1:
 * Unrestricted Mode
 * @param ovrpat Overwrite Pattern; 32-bit pattern used by the Overwrite action
 * @param owpass Overwrite pass Count, how many times the media is to be
 * overwritten; 0x0: 15 overwrite passes
 * @param oipbp Overwrite invert pattern between passes; 0x0: Disabled, 0x1:
 * Enabled
 * @param nodas No Deallocate After Sanitize; 0x0: Attempt to deallocate; 0x1 Do
 * not attempt to deallocate
 * @param req Pointer to structure for NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_cmd_sanitize(struct xnvme_dev *dev, uint8_t sanact, uint8_t ause,
		   uint32_t ovrpat, uint8_t owpass, uint8_t oipbp,
		   uint8_t nodas, struct xnvme_req *req);

/**
 * Submit, and optionally wait for completion of, a NVMe Write
 *
 * @see xnvme_cmd_opts
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace Identifier
 * @param slba The LBA to start the write at
 * @param nlb Number of LBAs to be written. NOTE: nlb is a zero-based value
 * @param dbuf Pointer to buffer; Payload as indicated by 'opts'
 * @param mbuf Pointer to buffer; Payload as indicated by 'opts'
 * @param opts command options, see ::xnvme_cmd_opts
 * @param req Pointer to structure for NVMe completion and async. context
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_cmd_write(struct xnvme_dev *dev, uint32_t nsid, uint64_t slba,
		uint16_t nlb, const void *dbuf, const void *mbuf, int opts,
		struct xnvme_req *req);

/**
 * Submit, and optionally wait for completion of, a NVMe Read
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace Identifier
 * @param slba The LBA to start reading from
 * @param nlb The number of LBAs to read. NOTE: nlb is a zero-based value
 * @param dbuf Pointer to data-payload
 * @param mbuf Pointer to meta-payload
 * @param opts command options, see ::xnvme_cmd_opts
 * @param req Pointer to structure for NVMe completion and async. context
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_cmd_read(struct xnvme_dev *dev, uint32_t nsid, uint64_t slba,
	       uint16_t nlb, void *dbuf, void *mbuf, int opts,
	       struct xnvme_req *req);

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
 * Creates a handle to given device path
 *
 * @param dev_uri Identifier of the device to open e.g. "/dev/nvme0n1"
 * @param opts options for opening device
 *
 * @return On success, a handle to the device. On error, NULL is returned and
 * `errno` set to indicate the error.
 */
struct xnvme_dev *
xnvme_dev_openf(const char *dev_uri, int opts);

/**
 * Destroys device-handle
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 */
void
xnvme_dev_close(struct xnvme_dev *dev);

/**
 * Returns the geometry of the given device
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 *
 * @return The geometry (struct xnvme_geo) of given device handle
 */
const struct xnvme_geo *
xnvme_dev_get_geo(const struct xnvme_dev *dev);

/**
 * Returns the NVMe identify controller structure associated with the given
 * device
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 *
 * @return On success, pointer to namespace structure. On error, NULL is
 * returned and `errno` is set to indicate the error
 */
const struct xnvme_spec_idfy_ctrlr *
xnvme_dev_get_ctrlr(const struct xnvme_dev *dev);

/**
 * Returns the NVMe identify namespace structure associated with the given
 * device
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 *
 * @return On success, pointer to namespace structure. On error, NULL is
 * returned and `errno` is set to indicate the error
 */
const struct xnvme_spec_idfy_ns *
xnvme_dev_get_ns(const struct xnvme_dev *dev);

/**
 * Returns the NVMe namespace identifier associated with the given `dev`
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 *
 * @return On success, NVMe namespace identifier is returned
 */
uint32_t
xnvme_dev_get_nsid(const struct xnvme_dev *dev);

/**
 * Returns the internal backend state of the given `dev`
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 *
 * @return On success, the internal backend state is returned.
 */
const void *
xnvme_dev_get_be_state(const struct xnvme_dev *dev);

/**
 * Returns the sector-shift-width of the device, that is, the value used for
 * converting block-device offset to lba, and vice-versa
 *
 * lba = ofz >> ssw
 * ofz = lba << ssw
 *
 * @return On success, the ssw is returned.
 */
uint64_t xnvme_dev_get_ssw(const struct xnvme_dev *dev);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_H */
