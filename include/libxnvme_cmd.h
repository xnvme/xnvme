/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_cmd.h
 */

/**
 * Attributes for asynchronous command-contexts
 *
 * @struct xnvme_cmd_ctx_async
 */
struct xnvme_cmd_ctx_async {
	struct xnvme_queue *queue; ///< Queue used for command processing
	xnvme_queue_cb cb;         ///< User defined callback function
	void *cb_arg;              ///< User defined callback function arguments
};

/**
 * The xNVMe Command Context
 *
 * @struct xnvme_cmd_ctx
 */
struct xnvme_cmd_ctx {
	union xnvme_spec_sqe cmd;         ///< Command to be processed
	struct xnvme_spec_cqe cpl;        ///< Completion result from processing
	struct xnvme_dev *dev;            ///< Device associated with the command
	struct xnvme_cmd_ctx_async async; ///< Fields for command option: XNVME_CMD_ASYNC
	///< Field containing command-options, the field is initialized by helper-functions
	uint32_t opts;

	uint8_t be_rsvd[12]; ///< Fields reserved for use by library internals
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_ctx) == 128, "Incorrect size")

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
 * Assign a pointer of your own to the given ::xnvme_cmd_ctx, for xnvme_cmd_ctx_get_data() to
 * retrieve when xnvme_queue_reap() hands the command-context back
 *
 * The pointer is kept in `async.cb_arg`, which xnvme_queue_set_cb() and xnvme_cmd_ctx_set_cb()
 * assign as well.
 *
 * @param ctx Pointer to the ::xnvme_cmd_ctx
 * @param data The pointer to assign
 */
void
xnvme_cmd_ctx_set_data(struct xnvme_cmd_ctx *ctx, void *data);

/**
 * Retrieve the pointer assigned to the given ::xnvme_cmd_ctx with xnvme_cmd_ctx_set_data()
 *
 * @param ctx Pointer to the ::xnvme_cmd_ctx
 *
 * @return The pointer assigned with xnvme_cmd_ctx_set_data()
 */
void *
xnvme_cmd_ctx_get_data(const struct xnvme_cmd_ctx *ctx);

/**
 * Initialize the given command-context for issuing synchronous commands to the given device
 *
 * @param dev Device handle (::xnvme_dev) obtained with xnvme_dev_open_uri(), or with
 * xnvme_file_open()
 * @param ctx The ::xnvme_cmd_ctx to initialize; all of it is overwritten
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned: -EINVAL when
 * `dev` or `ctx` is NULL.
 *
 * @errno
 */
int
xnvme_dev_get_cmd_ctx(struct xnvme_dev *dev, struct xnvme_cmd_ctx *ctx);

/**
 * Clears/resets the given ::xnvme_cmd_ctx
 *
 * @param ctx Pointer to the ::xnvme_cmd_ctx to clear
 */
void
xnvme_cmd_ctx_clear(struct xnvme_cmd_ctx *ctx);

/**
 * Check the status of the completion in the given ::xnvme_cmd_ctx
 *
 * @param ctx Pointer to the ::xnvme_cmd_ctx to check status on
 *
 * @return 0 when the Status Code and the Status Code Type of the completion are both zero,
 * otherwise 1.
 */
int
xnvme_cmd_ctx_cpl_status(const struct xnvme_cmd_ctx *ctx);

/**
 * Pass an NVMe IO Command through to the device via the given ::xnvme_cmd_ctx
 *
 * The mode of submission is determined by the command context: a context
 * obtained via xnvme_dev_get_cmd_ctx() submits synchronously and returns once
 * the command has completed, whereas a context obtained via
 * xnvme_queue_get_cmd_ctx() submits asynchronously and returns once the command
 * has been submitted; its completion is then processed via xnvme_queue_poke(), or the context
 * handed back by xnvme_queue_reap().
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param dbuf pointer to data-payload
 * @param dbuf_nbytes size of data-payload in bytes
 * @param mbuf pointer to meta-payload
 * @param mbuf_nbytes size of the meta-payload in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 *
 * @errno
 */
int
xnvme_cmd_pass(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
	       size_t mbuf_nbytes);

/**
 * Pass a vectored NVMe IO Command through to the device via the given
 * ::xnvme_cmd_ctx
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param dvec array of data iovecs
 * @param dvec_cnt number of elements in dvec
 * @param dvec_nbytes size of the meta-payload in bytes
 * @param mbuf pointer to meta-payload
 * @param mbuf_nbytes size of the meta-payload in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 *
 * @errno
 */
int
xnvme_cmd_pass_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
		   size_t dvec_nbytes, void *mbuf, size_t mbuf_nbytes);

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
 *
 * @errno
 */
int
xnvme_cmd_pass_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
		     size_t mbuf_nbytes);
