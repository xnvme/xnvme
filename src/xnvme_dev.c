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

static inline int
xnvme_dev_cmd_opts_yaml(FILE *stream, const struct xnvme_dev *dev, int indent,
			const char *sep, int head)
{
	int wrtn = 0;

	if (head) {
		wrtn += fprintf(stream, "%*sxnvme_cmd_opts:", indent, "");
		indent += 2;
	}

	if (!dev) {
		wrtn += fprintf(stream, " ~");
		return wrtn;
	}

	if (head) {
		wrtn += fprintf(stream, "\n");
	}

	wrtn += fprintf(stream, "%*smask: '"XNVME_I32_FMT"'%s", indent, "",
			XNVME_I32_TO_STR(dev->cmd_opts), sep);

	wrtn += fprintf(stream, "%*siomd: '%s'%s", indent, "",
			(dev->cmd_opts & XNVME_CMD_SYNC) ? "SYNC" : "ASYNC",
			sep);
	wrtn += fprintf(stream, "%*spayload_data: '%s'%s", indent, "",
			(dev->cmd_opts & XNVME_CMD_UPLD_SGLD) ? "USER" : "DRV",
			sep);
	wrtn += fprintf(stream, "%*spayload_meta: '%s'%s", indent, "",
			(dev->cmd_opts & XNVME_CMD_UPLD_SGLM) ? "USER" : "DRV",
			sep);

	wrtn += fprintf(stream, "%*scsi: 0x%x%s", indent, "", dev->csi, sep);
	wrtn += fprintf(stream, "%*snsid: 0x%u%s", indent, "", dev->nsid, sep);

	wrtn += fprintf(stream, "%*sssw: %"PRIu64"", indent, "", dev->ssw);

	return 0;
}

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

	wrtn += xnvme_dev_cmd_opts_yaml(stream, dev, 2, "\n", 1);
	wrtn += fprintf(stream, "\n");

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
	return dev->nsid;
}

uint8_t
xnvme_dev_get_csi(const struct xnvme_dev *dev)
{
	return dev->csi;
}

uint64_t
xnvme_dev_get_ssw(const struct xnvme_dev *dev)
{
	return dev->ssw;
}

const void *
xnvme_dev_get_be_state(const struct xnvme_dev *dev)
{
	return dev->be.state;
}

struct xnvme_dev *
xnvme_dev_openf(const char *dev_uri, int cmd_opts)
{
	struct xnvme_dev *dev = NULL;
	int err;

	err = xnvme_be_factory(dev_uri, &dev);
	if (err) {
		XNVME_DEBUG("FAILED: failed opening uri: %s with flags: %d",
			    dev_uri, cmd_opts);
		errno = -err;
		return NULL;
	}

	dev->cmd_opts = 0;	// Setup CMD options

	if (cmd_opts & XNVME_CMD_MASK_IOMD) {
		dev->cmd_opts |= cmd_opts & XNVME_CMD_MASK_IOMD;
	} else {
		dev->cmd_opts |= XNVME_CMD_DEF_IOMD;
	}

	if (cmd_opts & XNVME_CMD_MASK_UPLD) {
		dev->cmd_opts |= cmd_opts & XNVME_CMD_MASK_UPLD;
	} else {
		dev->cmd_opts |= XNVME_CMD_DEF_UPLD;
	}

	return dev;
}

struct xnvme_dev *
xnvme_dev_open(const char *dev_uri)
{
	return xnvme_dev_openf(dev_uri, 0x0);
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
