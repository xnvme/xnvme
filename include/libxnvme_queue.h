/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @headerfile libxnvme_adm.h
 */

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
