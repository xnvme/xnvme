/**
 * Deprecated names, kept so code written for earlier releases, such as the xNVMe engine of fio,
 * builds unchanged
 *
 * Each function calls the function replacing it. In a build with XNVME_DEBUG_ENABLED defined, as
 * xNVMe's debug builds are, the first call of each also prints a deprecation warning to stderr;
 * otherwise nothing is added to the call. They are inline, so they are not part of the library,
 * nor of its bindings. Each other name is a macro expanding to the name replacing it.
 *
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_compat.h
 */
#include <errno.h>

/**
 * @deprecated Use XNVME_SPEC_IDFY_NS_DESC
 */
#define XNVME_SPEC_IDFY_NSDSCR XNVME_SPEC_IDFY_NS_DESC

static inline void
xnvme_compat_deprecated(int *warned, const char *name, const char *instead)
{
#ifdef XNVME_DEBUG_ENABLED
	if (*warned) {
		return;
	}
	*warned = 1;
	fprintf(stderr, "xnvme: %s() is deprecated; use %s()\n", name, instead);
#else
	(void)warned;
	(void)name;
	(void)instead;
#endif
}

/**
 * @deprecated Use struct xnvme_spec_cqe
 */
#define xnvme_spec_cpl xnvme_spec_cqe

/**
 * @deprecated Use xnvme_spec_sqe_fpr()
 *
 * @param stream output stream used for printing
 * @param sqe Pointer to the ::xnvme_spec_sqe to print
 * @param opts Printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
static inline int
xnvme_spec_cmd_fpr(FILE *stream, union xnvme_spec_sqe *sqe, int opts)
{
	static int warned;

	xnvme_compat_deprecated(&warned, "xnvme_spec_cmd_fpr", "xnvme_spec_sqe_fpr");

	return xnvme_spec_sqe_fpr(stream, sqe, opts);
}

/**
 * @deprecated Use xnvme_spec_sqe_pr()
 *
 * @param sqe Pointer to the ::xnvme_spec_sqe to print
 * @param opts Printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
static inline int
xnvme_spec_cmd_pr(union xnvme_spec_sqe *sqe, int opts)
{
	static int warned;

	xnvme_compat_deprecated(&warned, "xnvme_spec_cmd_pr", "xnvme_spec_sqe_pr");

	return xnvme_spec_sqe_pr(sqe, opts);
}

/**
 * @deprecated Use xnvme_opts_set_defaults() on zero-initialized options
 *
 * @return Zero-initialized options with their defaults set
 */
static inline struct xnvme_opts
xnvme_opts_default(void)
{
	static int warned;
	struct xnvme_opts opts = {0};

	xnvme_compat_deprecated(&warned, "xnvme_opts_default", "xnvme_opts_set_defaults");
	xnvme_opts_set_defaults(&opts);

	return opts;
}

/**
 * @deprecated Use xnvme_dev_get_cmd_ctx()
 *
 * @param dev Device handle (::xnvme_dev) obtained with xnvme_dev_open()
 *
 * @return A command-context for issuing synchronous commands to the given device
 */
static inline struct xnvme_cmd_ctx
xnvme_cmd_ctx_from_dev(struct xnvme_dev *dev)
{
	static int warned;
	struct xnvme_cmd_ctx ctx = {0};

	xnvme_compat_deprecated(&warned, "xnvme_cmd_ctx_from_dev", "xnvme_dev_get_cmd_ctx");
	xnvme_dev_get_cmd_ctx(dev, &ctx);

	return ctx;
}

/**
 * @deprecated Use xnvme_queue_get_cmd_ctx()
 *
 * @param queue The ::xnvme_queue to retrieve a command-context from
 *
 * @return On success, a command-context; on error, NULL, with `errno` set
 */
static inline struct xnvme_cmd_ctx *
xnvme_cmd_ctx_from_queue(struct xnvme_queue *queue)
{
	static int warned;

	xnvme_compat_deprecated(&warned, "xnvme_cmd_ctx_from_queue", "xnvme_queue_get_cmd_ctx");

	return xnvme_queue_get_cmd_ctx(queue);
}

/**
 * @deprecated Use xnvme_cmd_pass_iov(), which takes the meta-payload as a single buffer
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param dvec array of data iovecs
 * @param dvec_cnt number of elements in dvec
 * @param dvec_nbytes size of the data-payload in bytes
 * @param mvec array of metadata iovecs, of at most one element
 * @param mvec_cnt number of elements in mvec
 * @param mvec_nbytes size of the meta-payload in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned: -EINVAL when
 * `mvec_cnt` is larger than one.
 */
static inline int
xnvme_cmd_passv(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt, size_t dvec_nbytes,
		struct iovec *mvec, size_t mvec_cnt, size_t mvec_nbytes)
{
	static int warned;

	xnvme_compat_deprecated(&warned, "xnvme_cmd_passv", "xnvme_cmd_pass_iov");
	if (mvec_cnt > 1) {
		return -EINVAL;
	}

	return xnvme_cmd_pass_iov(ctx, dvec, dvec_cnt, dvec_nbytes, mvec ? mvec[0].iov_base : NULL,
				  mvec_nbytes);
}

/**
 * @deprecated Use xnvme_queue_drain()
 *
 * @param queue Pointer to the ::xnvme_queue to process commands on
 *
 * @return On success, number of commands processed, may be 0. On error, negative `errno` is
 * returned.
 */
static inline int
xnvme_queue_wait(struct xnvme_queue *queue)
{
	static int warned;

	xnvme_compat_deprecated(&warned, "xnvme_queue_wait", "xnvme_queue_drain");

	return xnvme_queue_drain(queue);
}

/**
 * @deprecated Use xnvme_dev_get_cmd_ctx()
 *
 * @param fh File handle (::xnvme_dev) obtained with xnvme_file_open()
 *
 * @return A command-context for issuing synchronous commands to the given file
 */
static inline struct xnvme_cmd_ctx
xnvme_file_get_cmd_ctx(struct xnvme_dev *fh)
{
	static int warned;
	struct xnvme_cmd_ctx ctx = {0};

	xnvme_compat_deprecated(&warned, "xnvme_file_get_cmd_ctx", "xnvme_dev_get_cmd_ctx");
	xnvme_dev_get_cmd_ctx(fh, &ctx);

	return ctx;
}

/**
 * @deprecated Use xnvme_znd_report_get_descr()
 *
 * Unlike the functions above, it does not warn that it is deprecated
 */
#define XNVME_ZND_REPORT_DESCR(rprt, nth) \
	((struct xnvme_spec_znd_descr *)&(rprt->storage[nth * rprt->zrent_nbytes]))
