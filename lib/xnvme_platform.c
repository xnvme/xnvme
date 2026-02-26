// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_cref.h>
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
 * Resolve the memory backend for a backend config, applying user overrides.
 *
 * When the user requests a specific memory backend via opts->mem and it differs
 * from the config default, search the config's mem_overrides list for a match.
 *
 * @param be Backend instance to update with the resolved memory backend
 * @param cfg Backend config providing default and override memory backends
 * @param opts User options, may request a specific memory backend via opts->mem
 *
 * @return 0 on success (default matches or override found), -1 when no match
 */
static int
_dev_open_resolve_mem(struct xnvme_be *be, const struct xnvme_be_config *cfg,
		      const struct xnvme_opts *opts)
{
	if (!opts || !opts->mem || !strcmp(opts->mem, cfg->mem->id)) {
		return 0;
	}

	if (cfg->mem_overrides) {
		for (int j = 0; cfg->mem_overrides[j]; ++j) {
			if (!strcmp(cfg->mem_overrides[j]->id, opts->mem)) {
				be->mem = *cfg->mem_overrides[j];
				return 0;
			}
		}
	}

	XNVME_DEBUG("INFO: skipping config; no mem override '%s'", opts->mem);
	return -1;
}

/**
 * Initialize a controller reference for the device, reusing an existing one
 * or creating a new one via ctrlr_init.
 *
 * Looks up an existing cref for the URI and config name. When none exists,
 * checks for conflicts with other backend configs, initializes a new controller,
 * and inserts it into the cref table. On success, stores the controller
 * pointer in be->state[0].
 *
 * @param dev Device being opened
 * @param be Backend instance with dev operations
 * @param cfg Backend config for cref keying
 *
 * @return 0 on success, negative errno on failure
 */
static int
_dev_open_init_cref(struct xnvme_dev *dev, struct xnvme_be *be, const struct xnvme_be_config *cfg)
{
	void *ctrlr = NULL;
	int cref_inserted = 0;
	int err;

	if (!be->dev.ctrlr_init) {
		return 0;
	}

	ctrlr = xnvme_be_cref_lookup(dev->ident.uri, cfg->attr.name);
	if (ctrlr) {
		((void **)be->state)[0] = ctrlr;
		return 0;
	}

	{
		void *conflict = xnvme_be_cref_lookup(dev->ident.uri, NULL);

		if (conflict) {
			xnvme_be_cref_deref(conflict, XNVME_BE_CREF_NONE);
			XNVME_DEBUG("FAILED: uri '%s' claimed by another backend config",
				    dev->ident.uri);
			return -EBUSY;
		}
	}

	ctrlr = be->dev.ctrlr_init(dev);
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: ctrlr_init for uri '%s'", dev->ident.uri);
		return errno ? -errno : -EIO;
	}

	err = xnvme_be_cref_insert(dev->ident.uri, cfg->attr.name, ctrlr, be->dev.ctrlr_term);
	if (err) {
		XNVME_DEBUG("FAILED: cref_insert for uri '%s'", dev->ident.uri);
		be->dev.ctrlr_term(ctrlr);
		return err;
	}
	cref_inserted = 1;

	((void **)be->state)[0] = ctrlr;

	err = be->dev.dev_open(dev);
	if (err && cref_inserted) {
		xnvme_be_cref_deref(ctrlr, XNVME_BE_CREF_DESTROY_IMMEDIATE);
	}

	return err;
}

/**
 * Attempt to open a device using a specific backend config.
 *
 * Instantiates the backend from the config, resolves memory backend overrides,
 * initializes controller references if needed, and calls dev_open. On success,
 * backfills opts with the selected backend names.
 *
 * @param dev Device to open
 * @param cfg Backend config to try
 * @param opts User-provided options
 *
 * @return 0 on success, negative errno on failure
 */
static int
_dev_open_try_config(struct xnvme_dev *dev, const struct xnvme_be_config *cfg,
		     struct xnvme_opts *opts)
{
	struct xnvme_be be = {0};
	int err;

	be.async = *cfg->async;
	be.sync = *cfg->sync;
	be.admin = *cfg->admin;
	be.dev = *cfg->dev;
	be.mem = *cfg->mem;
	be.attr = cfg->attr;

	if (_dev_open_resolve_mem(&be, cfg, opts)) {
		return -EINVAL;
	}

	dev->be = be;
	dev->opts = *opts;

	XNVME_DEBUG("INFO: selected config: be='%s' async='%s' sync='%s'", cfg->attr.name,
		    cfg->async->id, cfg->sync->id);

	err = _dev_open_init_cref(dev, &dev->be, cfg);
	if (err) {
		return err;
	}

	if (!dev->be.dev.ctrlr_init) {
		err = dev->be.dev.dev_open(dev);
		if (err) {
			return err;
		}
	}

	XNVME_DEBUG("INFO: obtained device handle");
	dev->opts.be = dev->be.attr.name;
	dev->opts.admin = dev->be.admin.id;
	dev->opts.sync = dev->be.sync.id;
	dev->opts.async = dev->be.async.id;
	dev->opts.mem = dev->be.mem.id;
	dev->opts.dev = g_xnvme_platform->name;

	return 0;
}

/**
 * Open a device by iterating platform backend configs until one succeeds.
 *
 * Classifies the URI to determine device capabilities, then tries each
 * registered backend config that matches the user-provided backend options.
 * Stops on the first successful open, on permission errors, or when a
 * caps-matched config fails.
 *
 * @param dev Device to open, with ident.uri already set
 * @param opts User-provided options for backend selection
 *
 * @return 0 on success, negative errno on failure (-ENXIO when no config matches)
 */
int
xnvme_platform_dev_open(struct xnvme_dev *dev, struct xnvme_opts *opts)
{
	int has_backend_opts = opts && (opts->async || opts->sync || opts->admin);
	uint32_t uri_cap = 0;
	int err = 0;

	if (!has_backend_opts && g_xnvme_platform->classify) {
		uri_cap = g_xnvme_platform->classify(dev->ident.uri);
	}

	XNVME_DEBUG("INFO: uri='%s' classified cap=0x%x has_backend_opts=%d", dev->ident.uri,
		    uri_cap, has_backend_opts);

	for (int i = 0; g_xnvme_platform->backends[i]; ++i) {
		const struct xnvme_be_config *cfg = g_xnvme_platform->backends[i];

		if (opts && opts->be && strcmp(opts->be, cfg->attr.name) &&
		    strcmp(opts->be, g_xnvme_platform->name)) {
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

		err = _dev_open_try_config(dev, cfg, opts);
		if (!err) {
			return 0;
		}

		if (err == -EPERM || err == EPERM) {
			XNVME_DEBUG("FAILED: permission-error; stop trying");
			return err;
		}

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
	xnvme_buf_clear(idfy_buf, sizeof(*idfy_buf));

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

/**
 * Enumerate devices on the platform, opening each and invoking cb_func.
 *
 * When sys_uri is NULL, scans all local devices via the platform scan callback.
 * When sys_uri is provided (e.g. for fabrics), delegates directly to
 * enumerate_controller to iterate namespaces on the target controller.
 *
 * @param sys_uri System URI to enumerate, or NULL for local scan
 * @param opts Options for backend selection, or NULL for defaults
 * @param cb_func Callback invoked for each successfully opened device
 * @param cb_args Opaque argument passed through to cb_func
 *
 * @return 0 on success, negative errno on failure
 */
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
