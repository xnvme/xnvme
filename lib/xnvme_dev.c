// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_cmd.h>
#include <xnvme_dev.h>
#include <xnvme_geo.h>

int
xnvme_dev_fpr(FILE *stream, const struct xnvme_dev *dev, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_dev:");

	if (!dev) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");

	wrtn += xnvme_ident_yaml(stream, &dev->ident, 2, "\n", 1);
	wrtn += fprintf(stream, "\n");

	wrtn += xnvme_be_yaml(stream, &dev->be, 2, "\n", 1);
	wrtn += fprintf(stream, "\n");

	wrtn += fprintf(stream, "  xnvme_opts:\n");
	wrtn += fprintf(stream, "    be: '%s'\n", dev->opts.be);
	wrtn += fprintf(stream, "    mem: '%s'\n", dev->opts.mem);
	wrtn += fprintf(stream, "    dev: '%s'\n", dev->opts.dev);
	wrtn += fprintf(stream, "    admin: '%s'\n", dev->opts.admin);
	wrtn += fprintf(stream, "    sync: '%s'\n", dev->opts.sync);
	wrtn += fprintf(stream, "    async: '%s'\n", dev->opts.async);
	wrtn += fprintf(stream, "    oflags: 0x%x\n", dev->opts.oflags);

	wrtn += xnvme_geo_yaml(stream, &dev->geo, 2, "\n", 1);
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_dev_pr(const struct xnvme_dev *dev, int opts)
{
	return xnvme_dev_fpr(stdout, dev, opts);
}

const struct xnvme_geo *
xnvme_dev_get_geo(const struct xnvme_dev *dev)
{
	return &dev->geo;
}

const struct xnvme_spec_idfy_ctrlr *
xnvme_dev_get_ctrlr(const struct xnvme_dev *dev)
{
	return &dev->id.ctrlr;
}

const struct xnvme_spec_idfy_ctrlr *
xnvme_dev_get_ctrlr_css(const struct xnvme_dev *dev)
{
	return &dev->idcss.ctrlr;
}

const struct xnvme_spec_idfy_ns *
xnvme_dev_get_ns(const struct xnvme_dev *dev)
{
	return &dev->id.ns;
}

const struct xnvme_spec_idfy_ns *
xnvme_dev_get_ns_css(const struct xnvme_dev *dev)
{
	return &dev->idcss.ns;
}

uint32_t
xnvme_dev_get_nsid(const struct xnvme_dev *dev)
{
	return dev->ident.nsid;
}

uint8_t
xnvme_dev_get_csi(const struct xnvme_dev *dev)
{
	return dev->ident.csi;
}

const struct xnvme_ident *
xnvme_dev_get_ident(const struct xnvme_dev *dev)
{
	return &dev->ident;
}

uint64_t
xnvme_dev_get_ssw(const struct xnvme_dev *dev)
{
	return dev->geo.ssw;
}

const void *
xnvme_dev_get_be_state(const struct xnvme_dev *dev)
{
	return dev->be.state;
}

struct xnvme_dev *
xnvme_dev_open(const char *dev_uri, struct xnvme_opts *opts)
{
	struct xnvme_opts opts_default = xnvme_opts_default();
	struct xnvme_dev *dev = NULL;
	int err;

	if (!opts) { ///< Set defaults when none are given
		opts = &opts_default;
	}
	if (!opts->oflags) { ///< Set a default open-mode
		opts->rdwr = opts_default.rdwr;
	}
	if (opts->create && !opts->create_mode) { ///< Set a default umask/mode_t/create-mode
		opts->create_mode = opts_default.create_mode;
	}

	err = xnvme_dev_alloc(&dev);
	if (err) {
		XNVME_DEBUG("FAILED: failed xnvme_dev_alloc()");
		errno = -err;
		return NULL;
	}

	err = xnvme_ident_from_uri(dev_uri, &dev->ident);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_ident_from_uri(), err: %d", err);
		errno = -err;
		free(dev);
		return NULL;
	}

	err = xnvme_be_factory(dev, opts);
	if (err) {
		XNVME_DEBUG("FAILED: failed opening uri: %s", dev_uri);
		errno = -err;
		free(dev);
		return NULL;
	}

	return dev;
}

void
xnvme_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	dev->be.dev.dev_close(dev);
	free(dev);
}

int
xnvme_dev_alloc(struct xnvme_dev **dev)
{
	(*dev) = malloc(sizeof(**dev));
	if (!(*dev)) {
		XNVME_DEBUG("FAILED: allocating 'struct xnvme_dev'\n");
		return -errno;
	}
	memset(*dev, 0, sizeof(**dev));

	return 0;
}
