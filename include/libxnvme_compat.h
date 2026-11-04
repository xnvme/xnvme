/**
 * Deprecated functions, kept so code written for earlier releases, such as the xNVMe engine of
 * fio, builds unchanged
 *
 * Each calls the function replacing it. In a build with XNVME_DEBUG_ENABLED defined, as xNVMe's
 * debug builds are, the first call of each also prints a deprecation warning to stderr; otherwise
 * nothing is added to the call. They are inline, so they are not part of the library, nor of its
 * bindings.
 *
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_compat.h
 */

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
