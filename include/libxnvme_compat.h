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
