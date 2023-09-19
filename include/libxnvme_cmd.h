/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @headerfile libxnvme_adm.h
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
	struct xnvme_spec_cmd cmd;        ///< Command to be processed
	struct xnvme_spec_cpl cpl;        ///< Completion result from processing
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
