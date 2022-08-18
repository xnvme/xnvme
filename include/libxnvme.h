/**
 * Cross-platform I/O library for NVMe devices
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
#include <stdbool.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/uio.h>
#include <libxnvme_be.h>
#include <libxnvme_dev.h>
#include <libxnvme_buf.h>
#include <libxnvme_geo.h>
#include <libxnvme_ident.h>
#include <libxnvme_spec.h>
#include <libxnvme_util.h>

/**
 * xNVMe options
 *
 * @see xnvme_dev_open()
 *
 * @struct xnvme_opts
 */
struct xnvme_opts {
	const char *be;    ///< Backend/system interface to use
	const char *dev;   ///< Device manager/enumerator
	const char *mem;   ///< Memory allocator to use for buffers
	const char *sync;  ///< Synchronous Command-interface
	const char *async; ///> Asynchronous Command-interface
	const char *admin; ///< Administrative Command-interface
	uint32_t nsid;     ///< Namespace identifier
	union {
		struct {
			uint32_t rdonly   : 1; ///< OS open; read-only
			uint32_t wronly   : 1; ///< OS open; write-only
			uint32_t rdwr     : 1; ///< OS open; for read and only
			uint32_t create   : 1; ///< OS open; create if it does not exist
			uint32_t truncate : 1; ///< OS open; truncate content
			uint32_t direct   : 1; ///< OS open; attempt bypass OS-caching
			uint32_t _rsvd    : 26;
		};
		uint32_t oflags;
	};
	uint32_t create_mode;     ///< OS file creation-mode
	uint8_t poll_io;          ///< io_uring: enable io-polling
	uint8_t poll_sq;          ///< io_uring: enable sqthread-polling
	uint8_t register_files;   ///< io_uring: enable file-regirations
	uint8_t register_buffers; ///< io_uring: enable buffer-registration
	struct {
		uint32_t value : 31;
		uint32_t given : 1;
	} css;                 ///< SPDK controller-setup: do command-set-selection
	uint32_t use_cmb_sqs;  ///< SPDK controller-setup: use controller-memory-buffer for sq
	uint32_t shm_id;       ///< SPDK multi-processing: shared-memory-id
	uint32_t main_core;    ///< SPDK multi-processing: main-core
	const char *core_mask; ///< SPDK multi-processing: core-mask
	const char *adrfam;    ///< SPDK fabrics: address-family, IPv4/IPv6
	const char *subnqn;    ///< SPDK fabrics: Subsystem NQN
	const char *hostnqn;   ///< SPDK fabrics: Host NQN
	uint32_t spdk_fabrics; ///< Is assigned a value by backend if SPDK uses fabrics
};

/**
 * Returns an initialized option-struct with default values
 *
 * @return Zero-initialized and with default values where applicable
 */
struct xnvme_opts
xnvme_opts_default(void);

/**
 * Opaque device handle.
 *
 * @see xnvme_dev_open()
 *
 * @struct xnvme_dev
 */
struct xnvme_dev;

enum xnvme_enumerate_action {
	XNVME_ENUMERATE_DEV_KEEP_OPEN = 0x0, ///< Keep device-handle open after callback returns
	XNVME_ENUMERATE_DEV_CLOSE     = 0x1  ///< Close device-handle when callback returns
};

/**
 * Signature of callback function used with 'xnvme_enumerate' invoked for each discoved device
 *
 * The callback function signals whether the device-handle it receives should by closed, that is,
 * backend with invoke 'xnvme_dev_close(), after the callback returns or kept open. In the latter
 * case then it is up to the user to invoke 'xnvme_dev_close()' on the device-handle.
 *
 * Each signal is represented by the enum #xnvme_enumerate_action, and the values
 * XNVME_ENUMERATE_DEV_KEEP_OPEN or XNVME_ENUMERATE_DEV_CLOSE.
 */
typedef int (*xnvme_enumerate_cb)(struct xnvme_dev *dev, void *cb_args);

/**
 * enumerate devices
 *
 * @param sys_uri URI of the system to enumerate, when NULL, localhost/PCIe
 * @param opts Options for instrumenting the runtime during enumeration
 * @param cb_func Callback function to invoke for each yielded device
 * @param cb_args Arguments passed to the callback function
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_enumerate(const char *sys_uri, struct xnvme_opts *opts, xnvme_enumerate_cb cb_func,
		void *cb_args);

/**
 * Creates a device handle (::xnvme_dev) based on the given device-uri
 *
 * @param dev_uri File path "/dev/nvme0n1" or "0000:04.01"
 * @param opts Options for library backend and system-interfaces
 *
 * @return On success, a handle to the device. On error, NULL is returned and `errno` set to
 * indicate the error.
 */
struct xnvme_dev *
xnvme_dev_open(const char *dev_uri, struct xnvme_opts *opts);

/**
 * Destroy the given device handle (::xnvme_dev)
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 */
void
xnvme_dev_close(struct xnvme_dev *dev);

/**
 * Allocate a buffer for IO with the given device
 *
 * The buffer will be aligned to device geometry and DMA allocated if required by the backend for
 * command payloads
 *
 * @note
 * nbytes must be greater than zero and a multiple of minimal granularity
 * @note
 * De-allocate the buffer using xnvme_buf_free()
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param nbytes The size of the allocated buffer in bytes
 *
 * @return On success, a pointer to the allocated memory is returned. On error, NULL is returned
 * and `errno` set to indicate the error.
 */
void *
xnvme_buf_alloc(const struct xnvme_dev *dev, size_t nbytes);

/**
 * Reallocate a buffer for IO with the given device
 *
 * The buffer will be aligned to device geometry and DMA allocated if required by the backend for
 * IO
 *
 * @note
 * nbytes must be greater than zero and a multiple of minimal granularity
 * @note
 * De-allocate the buffer using xnvme_buf_free()
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param buf The buffer to reallocate
 * @param nbytes The size of the allocated buffer in bytes
 *
 * @return On success, a pointer to the allocated memory is returned. On error, NULL is returned
 * and `errno` set to indicate the error.
 */
void *
xnvme_buf_realloc(const struct xnvme_dev *dev, void *buf, size_t nbytes);

/**
 * Free the given IO buffer allocated with xnvme_buf_alloc()
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param buf Pointer to a buffer allocated with xnvme_buf_alloc()
 */
void
xnvme_buf_free(const struct xnvme_dev *dev, void *buf);

/**
 * Opaque Command Queue Handle to be initialized by xnvme_queue_init()
 *
 * @see xnvme_queue_init
 * @see xnvme_queue_term
 *
 * @struct xnvme_queue
 */
struct xnvme_queue;

/**
 * Command Queue initialization options
 *
 * @enum xnvme_queue_opts
 */
enum xnvme_queue_opts {
	XNVME_QUEUE_IOPOLL = 0x1,      ///< XNVME_QUEUE_IOPOLL: queue. is polled for completions
	XNVME_QUEUE_SQPOLL = 0x1 << 1, ///< XNVME_QUEUE_SQPOLL: queue. is polled for submissions
};

/**
 * Allocate a Command Queue for asynchronous command submission and completion
 *
 * @param dev Device handle (::xnvme_dev) obtained with xnvme_dev_open()
 * @param capacity Maximum number of outstanding commands on the initialized queue, note that it
 * must be a power of 2 within the range [1,4096]
 * @param opts Queue options
 * @param queue Pointer-pointer to the ::xnvme_queue to initialize
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_queue_init(struct xnvme_dev *dev, uint16_t capacity, int opts, struct xnvme_queue **queue);

/**
 * Get the capacity of the given ::xnvme_queue
 *
 * @param queue Pointer to the ::xnvme_queue to query for capacity
 *
 * @return On success, capacity of given ::xnvme_queue text is returned. On error, 0 is returned
 * e.g. errors are silent
 */
uint32_t
xnvme_queue_get_capacity(struct xnvme_queue *queue);

/**
 * Get the number of outstanding commands on the given ::xnvme_queue
 *
 * @param queue Pointer to the ::xnvme_queue to query for outstanding commands
 *
 * @return On success, number of outstanding commands are returned. On error, 0 is returned e.g.
 * errors are silent
 */
uint32_t
xnvme_queue_get_outstanding(struct xnvme_queue *queue);

/**
 * Tear down the given ::xnvme_queue
 *
 * @param queue Pointer to the ::xnvme_queue to tear down
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_queue_term(struct xnvme_queue *queue);

/**
 * Process completions of commands on the given ::xnvme_queue
 *
 * Set process 'max' to limit number of completions, 0 means no max.
 *
 * @param queue Pointer to the ::xnvme_queue to poke for completions
 * @param max The max number of completions to complete
 *
 * @return On success, number of completions processed, may be 0. On error, negative `errno` is
 * returned.
 */
int
xnvme_queue_poke(struct xnvme_queue *queue, uint32_t max);

/**
 * Process outstanding commands on the given ::xnvme_queue until it is empty
 *
 * @param queue Pointer to the ::xnvme_queue to wait/process commands on
 *
 * @return On success, number of commands processed, may be 0. On error, negative `errno` is
 * returned.
 */
int
xnvme_queue_drain(struct xnvme_queue *queue);

/**
 * DEPRECATED: expect that this function will be removed in an upcoming release
 *
 * @param queue Pointer to the ::xnvme_queue to wait/process commands on
 *
 * @return On success, number of commands processed, may be 0. On error, negative `errno` is
 * returned.
 */
int
xnvme_queue_wait(struct xnvme_queue *queue);

/**
 * Retrieve a command-context from the given queue for async. command execution with the queue
 *
 * @note The command-context is managed by the queue, thus, return it to the queue via
 * ::xnvme_queue_put_cmd_ctx
 *
 * @note This is not thread-safe
 *
 * @param queue Pointer to the ::xnvme_queue to retrieve a command-context for
 *
 * @return On success, a command-context is returned. On error, NULL is returned and `errno` is set
 * to indicate the error.
 */
struct xnvme_cmd_ctx *
xnvme_queue_get_cmd_ctx(struct xnvme_queue *queue);

/**
 * Hand back a command-context previously retrieve using ::xnvme_queue_get_cmd_ctx
 *
 * @note This function is not thread-safe
 *
 * @param queue Pointer to the ::xnvme_queue to hand back the command-context to
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_queue_put_cmd_ctx(struct xnvme_queue *queue, struct xnvme_cmd_ctx *ctx);

/**
 * Signature of function used with Command Queues for async. callback upon command-completion
 */
typedef void (*xnvme_queue_cb)(struct xnvme_cmd_ctx *ctx, void *opaque);

/**
 * The xNVMe Command Context
 *
 * @struct xnvme_cmd_ctx
 */
struct xnvme_cmd_ctx {
	struct xnvme_spec_cmd cmd; ///< Command to be processed
	struct xnvme_spec_cpl cpl; ///< Completion result from processing

	struct xnvme_dev *dev; ///< Device associated with the command

	///< Fields for command option: XNVME_CMD_ASYNC
	struct {
		struct xnvme_queue *queue; ///< Queue used for command processing
		xnvme_queue_cb cb;         ///< User defined callback function
		void *cb_arg;              ///< User defined callback function arguments
	} async;

	///< Field containing command-options, the field is initialized by helper-functions
	uint32_t opts;

	uint8_t be_rsvd[4]; ///< Fields reserved for use by library internals

	///< Field for including the command-context in BSD-style singly-linked-lists (SLIST)
	SLIST_ENTRY(xnvme_cmd_ctx) link;
};

/**
 * Assign a callback-function and argument to be used with the given command-context
 *
 * @param ctx Pointer to the ::xnvme_cmd_ctx to setup callback for
 * @param cb The callback function to use
 * @param cb_arg The callback argument to use
 */
static inline void
xnvme_cmd_ctx_set_cb(struct xnvme_cmd_ctx *ctx, xnvme_queue_cb cb, void *cb_arg)
{
	ctx->async.cb     = cb;
	ctx->async.cb_arg = cb_arg;
}

/**
 * Assign a callback-function and argument to be used with the ::xnvme_cmd_ctx of the queue
 *
 * @param queue The ::xnvme_queue to assign default callback function for
 * @param cb The callback function to use
 * @param cb_arg The callback argument to use
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_queue_set_cb(struct xnvme_queue *queue, xnvme_queue_cb cb, void *cb_arg);

/**
 * Retrieve a command-context for issuing commands to the given device
 *
 * @param dev Device handle (::xnvme_dev) obtained with xnvme_dev_open()
 *
 * @return A ::xnvme_cmd_ctx initialized synchronous command on the given device
 */
struct xnvme_cmd_ctx
xnvme_cmd_ctx_from_dev(struct xnvme_dev *dev);

/**
 * Retrieve a command-text for issuing commands via the given queue
 *
 * @param queue Pointer to the ::xnvme_queue to retrieve a command-context for
 *
 * @return On success, a pointer to a ::xnvme_cmd_ctx is returned. On error, NULL is returned and
 * `errno` set to indicate the error.
 */
struct xnvme_cmd_ctx *
xnvme_cmd_ctx_from_queue(struct xnvme_queue *queue);

/**
 * Clears/resets the given ::xnvme_cmd_ctx
 *
 * @param ctx Pointer to the ::xnvme_cmd_ctx to clear
 */
void
xnvme_cmd_ctx_clear(struct xnvme_cmd_ctx *ctx);

/**
 * Encapsulate completion-error checking here for now.
 *
 * @param ctx Pointer to the ::xnvme_cmd_ctx to check status on
 *
 * @return On success, 0 is return. On error, a non-zero value is returned.
 */
static inline int
xnvme_cmd_ctx_cpl_status(struct xnvme_cmd_ctx *ctx)
{
	return ctx->cpl.status.sc || ctx->cpl.status.sct;
}

/**
 * Pass an NVMe IO Command through to the device via the given ::xnvme_cmd_ctx
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param dbuf pointer to data-payload
 * @param dbuf_nbytes size of data-payload in bytes
 * @param mbuf pointer to meta-payload
 * @param mbuf_nbytes size of the meta-payload in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_cmd_pass(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
	       size_t mbuf_nbytes);

/**
 * Pass a vectored NVMe IO Command through to the device via the given ::xnvme_cmd_ctx
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param dvec array of data iovecs
 * @param dvec_cnt number of elements in dvec
 * @param dvec_nbytes size of the meta-payload in bytes
 * @param mvec array of metadata iovecs
 * @param mvec_cnt number of elements in mvec
 * @param mvec_nbytes size of the meta-payload in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_cmd_passv(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt, size_t dvec_nbytes,
		struct iovec *mvec, size_t mvec_cnt, size_t mvec_nbytes);

/**
 * Pass a NVMe Admin Command through to the device with minimal intervention
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param dbuf pointer to data-payload
 * @param dbuf_nbytes size of data-payload in bytes
 * @param mbuf pointer to meta-payload
 * @param mbuf_nbytes size of the meta-payload in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_cmd_pass_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
		     size_t mbuf_nbytes);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_H */
