// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <stddef.h>
#include <string.h>
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

int
xnvme_platform_dev_open(struct xnvme_dev *dev, struct xnvme_opts *opts)
{
	int be_opts = opts && (opts->async || opts->sync || opts->admin);
	uint32_t uri_cap = 0;
	int err = 0;

	if (!be_opts && g_xnvme_platform->classify) {
		uri_cap = g_xnvme_platform->classify(dev->ident.uri);
	}

	XNVME_DEBUG("INFO: uri='%s' classified cap=0x%x be_opts=%d", dev->ident.uri, uri_cap,
		    be_opts);

	for (int i = 0; g_xnvme_platform->backends[i]; ++i) {
		const struct xnvme_be_config *cfg = g_xnvme_platform->backends[i];
		struct xnvme_be be = {0};

		if (opts && opts->be && strcmp(opts->be, cfg->attr.name)) {
			continue;
		}
		if (opts && opts->async && strcmp(opts->async, cfg->async->id)) {
			continue;
		}
		if (opts && opts->sync && strcmp(opts->sync, cfg->sync->id)) {
			continue;
		}
		if (opts && opts->admin && strcmp(opts->admin, cfg->admin->id)) {
			continue;
		}
		if (uri_cap && cfg->attr.caps && !(cfg->attr.caps & uri_cap)) {
			XNVME_DEBUG("INFO: skipping config '%s'; caps 0x%x vs uri 0x%x",
				    cfg->attr.descr, cfg->attr.caps, uri_cap);
			continue;
		}
		be.async = *cfg->async;
		be.sync = *cfg->sync;
		be.admin = *cfg->admin;
		be.dev = *cfg->dev;
		be.mem = *cfg->mem;
		be.attr = cfg->attr;

		if (opts && opts->mem && strcmp(opts->mem, cfg->mem->id)) {
			const struct xnvme_be_mem *mem_override = NULL;

			if (cfg->mem_overrides) {
				for (int j = 0; cfg->mem_overrides[j]; ++j) {
					if (!strcmp(cfg->mem_overrides[j]->id, opts->mem)) {
						mem_override = cfg->mem_overrides[j];
						break;
					}
				}
			}
			if (!mem_override) {
				XNVME_DEBUG("INFO: skipping config; no mem '%s'", opts->mem);
				continue;
			}
			be.mem = *mem_override;
		}

		dev->be = be;
		dev->opts = *opts;

		XNVME_DEBUG("INFO: selected config: be='%s' async='%s' sync='%s'", cfg->attr.name,
			    cfg->async->id, cfg->sync->id);

		err = be.dev.dev_open(dev);
		if (!err) {
			XNVME_DEBUG("INFO: obtained device handle");
			dev->opts.be = be.attr.name;
			dev->opts.admin = dev->be.admin.id;
			dev->opts.sync = dev->be.sync.id;
			dev->opts.async = dev->be.async.id;
			dev->opts.mem = dev->be.mem.id;
			dev->opts.dev = g_xnvme_platform->name;
			return 0;
		}
		if (err == -EPERM || err == EPERM) {
			XNVME_DEBUG("FAILED: permission-error; stop trying");
			return err;
		}

		/* When caps matched, this was the right config -- return its error */
		if (uri_cap && cfg->attr.caps) {
			XNVME_DEBUG("FAILED: caps-matched config returned err: %d", err);
			return err;
		}

		XNVME_DEBUG("INFO: skipping config due to err: %d", err);
	}

	XNVME_DEBUG("FAILED: no config for uri: '%s'", dev->ident.uri);

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
