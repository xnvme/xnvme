// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <stddef.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_platform.h>

struct xnvme_platform *g_xnvme_platform =
#if defined(XNVME_PLATFORM_LINUX_ENABLED)
	&g_xnvme_platform_linux;
#elif defined(XNVME_PLATFORM_FREEBSD_ENABLED)
	&g_xnvme_platform_freebsd;
#elif defined(XNVME_PLATFORM_MACOS_ENABLED)
	&g_xnvme_platform_macos;
#elif defined(XNVME_PLATFORM_WINDOWS_ENABLED)
	&g_xnvme_platform_windows;
#else
#error "No platform enabled. Define one of XNVME_PLATFORM_{LINUX,FREEBSD,MACOS,WINDOWS}_ENABLED"
#endif

/**
 * Set up mixin of 'be' of the given 'mtype' and matching 'opts' when provided
 */
static int
be_setup(struct xnvme_be *be, enum xnvme_be_mixin_type mtype, struct xnvme_opts *opts)
{
	for (uint64_t j = 0; (j < be->nobjs); ++j) {
		const struct xnvme_be_mixin *mixin = &be->objs[j];

		if (mixin->mtype != mtype) {
			continue;
		}

		switch (mtype) {
		case XNVME_BE_ASYNC:
			if ((opts) && (opts->async) && strcmp(opts->async, mixin->name)) {
				XNVME_DEBUG("INFO: skipping async: '%s' != '%s'", mixin->name,
					    opts->async);
				continue;
			}
			be->async = *mixin->async;
			break;

		case XNVME_BE_SYNC:
			if ((opts) && (opts->sync) && strcmp(opts->sync, mixin->name)) {
				XNVME_DEBUG("INFO: skipping sync: '%s' != '%s'", mixin->name,
					    opts->sync);
				continue;
			}
			be->sync = *mixin->sync;
			break;

		case XNVME_BE_ADMIN:
			if ((opts) && (opts->admin) && strcmp(opts->admin, mixin->name)) {
				XNVME_DEBUG("INFO: skipping admin: '%s' != '%s'", mixin->name,
					    opts->admin);
				continue;
			}
			be->admin = *mixin->admin;
			break;

		case XNVME_BE_DEV:
			be->dev = *mixin->dev;
			break;

		case XNVME_BE_MEM:
			if ((opts) && (opts->mem) && strcmp(opts->mem, mixin->name)) {
				XNVME_DEBUG("INFO: skipping mem: '%s' != '%s'", mixin->name,
					    opts->mem);
				continue;
			}
			be->mem = *mixin->mem;
			break;

		case XNVME_BE_ATTR:
		case XNVME_BE_END:
			XNVME_DEBUG("FAILED: attr | end");
			return -EINVAL;
		}

		return mtype;
	}

	XNVME_DEBUG("FAILED: be: '%s' has no matching mixin of mtype: %x, mtype_key: '%s'",
		    be->attr.name, mtype, xnvme_be_mixin_key(mtype));

	return -ENOSYS;
}

int
xnvme_platform_dev_open(struct xnvme_dev *dev, struct xnvme_opts *opts)
{
	int err = 0;

	for (int i = 0; g_xnvme_platform->backends[i]; ++i) {
		struct xnvme_be be = *g_xnvme_platform->backends[i];
		int setup = 0;
		XNVME_DEBUG("INFO: checking be: '%s'", be.attr.name);

		if (!be.attr.enabled) {
			XNVME_DEBUG("INFO: skipping be: '%s'; !enabled", be.attr.name);
			continue;
		}
		if (opts && (opts->be) && strcmp(opts->be, be.attr.name)) {
			XNVME_DEBUG("INFO: skipping be: '%s' != '%s'", be.attr.name, opts->be);
			continue;
		}

		for (int j = 0; j < 5; ++j) {
			int mtype = 1 << j;

			err = be_setup(&be, mtype, opts);
			if (err < 0) {
				XNVME_DEBUG("FAILED: be_setup(%s); err: %d", be.attr.name, err);
				continue;
			}

			setup |= err;
		}

		if (setup != XNVME_BE_CONFIGURED) {
			XNVME_DEBUG("INFO: !configured be(%s); err: %d", be.attr.name, err);
			continue;
		}

		dev->be = be;
		dev->opts = *opts;

		XNVME_DEBUG("INFO: obtained backend instance be: '%s'", be.attr.name);

		err = be.dev.dev_open(dev);
		switch (err < 0 ? -err : err) {
		case 0:
			XNVME_DEBUG("INFO: obtained device handle");
			dev->opts.be = be.attr.name;
			dev->opts.admin = dev->be.admin.id;
			dev->opts.sync = dev->be.sync.id;
			dev->opts.async = dev->be.async.id;
			dev->opts.mem = dev->be.mem.id;

			dev->opts.dev = g_xnvme_platform->name;
			return 0;

		case EPERM:
			XNVME_DEBUG("FAILED: permission-error; failed and stop trying");
			return err;

		default:
			XNVME_DEBUG("INFO: skipping backend due to err: %d", err);
			break;
		}
	}

	XNVME_DEBUG("FAILED: no backend for uri: '%s'", dev->ident.uri);

	return err ? err : -ENXIO;
}

int
xnvme_platform_enumerate(const char *sys_uri, struct xnvme_opts *opts, xnvme_enumerate_cb cb_func,
			 void *cb_args)
{
	struct xnvme_opts opts_default = xnvme_opts_default();
	int err;

	if (!opts) {
		opts = &opts_default;
	}

	for (int i = 0; g_xnvme_platform->backends[i]; ++i) {
		struct xnvme_be be = *g_xnvme_platform->backends[i];

		if (!be.attr.enabled) {
			XNVME_DEBUG("INFO: skipping be: '%s'; !enabled", be.attr.name);
			continue;
		}

		if (opts && (opts->be) && strcmp(opts->be, be.attr.name)) {
			// skip if opts->be != be.attr.name
			continue;
		}

		err = be_setup(&be, XNVME_BE_DEV, NULL);
		if (err < 0) {
			XNVME_DEBUG("FAILED: be_setup(); err: %d", err);
			continue;
		}

		err = be.dev.enumerate(sys_uri, opts, cb_func, cb_args);
		switch (err) {
		case 0:
			break;
		case -ENOSYS:
			// fail silently if the error is ENOSYS,
			// unless the user asks for a specific backend
			if (opts->be) {
				XNVME_DEBUG("FAILED: Requested backend (%s) doesn't support "
					    "enumeration, err: '%s'",
					    g_xnvme_platform->backends[i]->attr.name,
					    strerror(-err));
				return err;
			}
			break;
		case -ESRCH:
			if (!strcmp("spdk", be.attr.name)) {
				XNVME_DEBUG("WARNING: SPDK could not be initialized");
				break;
			}
			// !! FALLTHROUGH !!
		default:
			XNVME_DEBUG("FAILED: %s->enumerate(...), err: '%s', i: %d",
				    g_xnvme_platform->backends[i]->attr.name, strerror(-err), i);
			return err;
		}
	}

	return 0;
}

int
xnvme_scan(const char *sys_uri, struct xnvme_opts *opts, xnvme_scan_cb cb_func, void *cb_args)
{
	return g_xnvme_platform->scan(sys_uri, opts, cb_func, cb_args);
}

int
xnvme_enumerate(const char *sys_uri, struct xnvme_opts *opts, xnvme_enumerate_cb cb_func,
		void *cb_args)
{
	return g_xnvme_platform->enumerate(sys_uri, opts, cb_func, cb_args);
}
