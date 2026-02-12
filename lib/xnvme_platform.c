// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
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

struct enumerate_scan_ctx {
	struct xnvme_opts *opts;
	xnvme_enumerate_cb cb_func;
	void *cb_args;
};

/**
 * Open controller with nsid=0, send Identify Active Namespace List,
 * then open each namespace and invoke cb_func.
 */
static int
enumerate_controller(const char *uri, struct xnvme_opts *opts, xnvme_enumerate_cb cb_func,
		     void *cb_args)
{
	struct xnvme_opts ctrl_opts = *opts;
	struct xnvme_dev *ctrl_dev;
	struct xnvme_spec_idfy *idfy_buf;
	struct xnvme_cmd_ctx ctx;
	uint32_t *nslist;
	int err;

	ctrl_opts.nsid = 0;
	ctrl_dev = xnvme_dev_open(uri, &ctrl_opts);
	if (!ctrl_dev) {
		XNVME_DEBUG("FAILED: xnvme_dev_open(%s) for controller", uri);
		return -errno;
	}

	idfy_buf = xnvme_buf_alloc(ctrl_dev, sizeof(*idfy_buf));
	if (!idfy_buf) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc()");
		xnvme_dev_close(ctrl_dev);
		return -ENOMEM;
	}
	memset(idfy_buf, 0, sizeof(*idfy_buf));

	ctx = xnvme_cmd_ctx_from_dev(ctrl_dev);
	err = xnvme_adm_idfy(&ctx, XNVME_SPEC_IDFY_NSLIST, 0, 0, 0, 0, idfy_buf);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("FAILED: Identify Active NS List, err: %d", err);
		xnvme_buf_free(ctrl_dev, idfy_buf);
		xnvme_dev_close(ctrl_dev);
		return err ? err : -EIO;
	}

	nslist = (uint32_t *)idfy_buf;
	for (int i = 0; i < 1024 && nslist[i]; ++i) {
		struct xnvme_opts ns_opts = *opts;
		struct xnvme_dev *ns_dev;

		ns_opts.nsid = nslist[i];

		ns_dev = xnvme_dev_open(uri, &ns_opts);
		if (!ns_dev) {
			XNVME_DEBUG("FAILED: xnvme_dev_open(%s, nsid=%u)", uri, nslist[i]);
			continue;
		}

		if (cb_func(ns_dev, cb_args)) {
			xnvme_dev_close(ns_dev);
		}
	}

	xnvme_buf_free(ctrl_dev, idfy_buf);
	xnvme_dev_close(ctrl_dev);
	return 0;
}

/**
 * Returns true when the controller is bound to the kernel NVMe driver.
 * Scan reports namespace idents for these, so enumerate handles them via
 * dev_open. Named "nvme" on both Linux and FreeBSD.
 */
static int
is_kernel_nvme_driver(const char *kernel_driver)
{
	return !strcmp(kernel_driver, "nvme");
}

static int
enumerate_scan_cb(const struct xnvme_ident *ident, void *cb_args)
{
	struct enumerate_scan_ctx *ctx = cb_args;

	/**
	 * Controllers bound to the kernel NVMe driver are skipped — scan
	 * reports their namespace idents separately and those are handled
	 * via dev_open below. Controllers NOT on the kernel driver (e.g.
	 * vfio-pci, nic_uio, uio_pci_generic) have no /dev/ namespace
	 * idents, so open the controller and iterate namespaces via Identify.
	 */
	if (ident->dtype == XNVME_DEV_TYPE_NVME_CONTROLLER) {
		if (!is_kernel_nvme_driver(ident->kernel_driver)) {
			enumerate_controller(ident->uri, ctx->opts, ctx->cb_func, ctx->cb_args);
		}
		return 0;
	}

	{
		struct xnvme_opts opts = *ctx->opts;
		struct xnvme_dev *dev;

		if (!opts.nsid) {
			opts.nsid = ident->nsid ? ident->nsid : 1;
		}

		dev = xnvme_dev_open(ident->uri, &opts);
		if (!dev) {
			XNVME_DEBUG("FAILED: xnvme_dev_open(%s)", ident->uri);
			return 0;
		}

		if (ctx->cb_func(dev, ctx->cb_args)) {
			xnvme_dev_close(dev);
		}
	}

	return 0;
}

int
xnvme_platform_enumerate(const char *sys_uri, struct xnvme_opts *opts, xnvme_enumerate_cb cb_func,
			 void *cb_args)
{
	struct xnvme_opts opts_default = xnvme_opts_default();

	if (!opts) {
		opts = &opts_default;
	}

	if (!sys_uri) {
		struct enumerate_scan_ctx ctx = {
			.opts = opts,
			.cb_func = cb_func,
			.cb_args = cb_args,
		};

		return g_xnvme_platform->scan(NULL, opts, enumerate_scan_cb, &ctx);
	}

	/** Fabrics / explicit sys_uri: delegate directly to enumerate_controller */
	return enumerate_controller(sys_uri, opts, cb_func, cb_args);
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
