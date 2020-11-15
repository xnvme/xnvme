// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#define XNVME_BE_FBSD_NAME "fbsd"
#define XNVME_BE_FBSD_SCHM "file"

#ifdef XNVME_BE_FBSD_ENABLED
#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");
#include <sys/param.h>
#include <err.h>
#include <fcntl.h>
#include <paths.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <dev/nvme/nvme.h>

#include <libxnvme.h>
#include <libxnvme_spec.h>
#include <libxnvme_util.h>
#include <xnvme_be.h>
#include <xnvme_be_fbsd.h>
#include <xnvme_dev.h>

void
xnvme_be_fbsd_state_term(struct xnvme_be_fbsd_state *state)
{
	if (!state) {
		return;
	}

	close(state->fd);
}

void
xnvme_be_fbsd_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	xnvme_be_fbsd_state_term((void *)dev->be.state);
	memset(&dev->be, 0, sizeof(dev->be));
}

int
xnvme_be_fbsd_state_init(struct xnvme_dev *dev, void *XNVME_UNUSED(opts))
{
	struct xnvme_be_fbsd_state *state = (void *)dev->be.state;

	state->fd = open(dev->ident.trgt, O_RDWR);
	if (state->fd < 0) {
		XNVME_DEBUG("FAILED: open(parts.trgt(%s)), state->fd(%d)\n",
			    dev->ident.trgt, state->fd);
		return -errno;
	}

	return 0;
}

int
xnvme_be_fbsd_dev_idfy(struct xnvme_dev *dev)
{
	struct xnvme_be_fbsd_state *state = (void *)dev->be.state;
	struct xnvme_spec_idfy *idfy = NULL;
	struct xnvme_req req = { 0 };
	int err;

	dev->dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;

	idfy = xnvme_buf_alloc(dev, sizeof(*idfy), NULL);
	if (!idfy) {
		XNVME_DEBUG("FAILED: failed allocing buffer");
		return -errno;
	}

	memset(idfy, 0, sizeof(*idfy));
	memset(&req, 0, sizeof(req));
	err = xnvme_cmd_idfy_ctrlr(dev, idfy, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		XNVME_DEBUG("FAILED: identify controller");
		xnvme_buf_free(dev, idfy);
		xnvme_be_fbsd_state_term(state);
		free(dev);
		return err;
	}
	memcpy(&dev->id.ctrlr, idfy, sizeof(*idfy));

	{
		int ioctl_nsid = dev->nsid ? dev->nsid : 1;

		memset(idfy, 0, sizeof(*idfy));
		memset(&req, 0, sizeof(req));
		err = xnvme_cmd_idfy_ns(dev, ioctl_nsid, idfy, &req);
		if (err || xnvme_req_cpl_status(&req)) {
			XNVME_DEBUG("FAILED: identify namespace, err: %d", err);
			xnvme_buf_free(dev, idfy);
			xnvme_be_fbsd_state_term(state);
			free(dev);
			return err;
		}
		memcpy(&dev->id.ns, idfy, sizeof(*idfy));
	}

	xnvme_buf_free(dev, idfy);

	return 0;
}

static int
_nvme_filter(const struct dirent *d)
{
	char path[264];
	struct stat bd;
	int ctrl, ns, part;

	if (d->d_name[0] == '.') {
		return 0;
	}

	if (strstr(d->d_name, "nvme")) {
		snprintf(path, sizeof(path), "%s%s", _PATH_DEV, d->d_name);
		if (stat(path, &bd)) {
			XNVME_DEBUG("cannot stat");
			return 0;
		}
		if (sscanf(d->d_name, "nvme%dn%dp%d", &ctrl, &ns, &part) == 3) {
			// Do not want to use e.g. nvme0n1p2 etc.
			XNVME_DEBUG("matches too much");
			return 0;
		}

		return 1;
	}

	return 0;
}

int
xnvme_be_fbsd_dev_from_ident(const struct xnvme_ident *ident,
			     struct xnvme_dev **dev)
{
	int err;

	err = xnvme_dev_alloc(dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_dev_alloc()");
		return err;
	}
	(*dev)->ident = *ident;
	(*dev)->be = xnvme_be_fbsd;

	err = xnvme_be_fbsd_state_init(*dev, NULL);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_fbsd_state_init()");
		free(*dev);
		return err;
	}
	err = xnvme_be_fbsd_dev_idfy(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_fbsd_dev_idfy()");
		xnvme_be_fbsd_state_term((void *)(*dev)->be.state);
		free(*dev);
		return err;
	}
	err = xnvme_be_dev_derive_geometry(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_dev_derive_geometry()");
		xnvme_be_fbsd_state_term((void *)(*dev)->be.state);
		free(*dev);
		return err;
	}

	return 0;
}

int
xnvme_be_fbsd_enumerate(struct xnvme_enumeration *list, const char *sys_uri,
			int XNVME_UNUSED(opts))
{
	struct dirent **dent = NULL;
	int ndev = 0;

	if (sys_uri) {
		XNVME_DEBUG("FAILED: sys_uri: %s is not supported", sys_uri);
		return -ENOSYS;
	}

	ndev = scandir(_PATH_DEV, &dent, _nvme_filter, alphasort);

	for (int di = 0; di < ndev; ++di) {
		char uri[XNVME_IDENT_URI_LEN] = { 0 };
		struct xnvme_ident ident = { 0 };
		struct xnvme_dev *dev;

		snprintf(uri, XNVME_IDENT_URI_LEN - 1,
			 XNVME_BE_FBSD_NAME ":" _PATH_DEV "%s",
			 dent[di]->d_name);
		if (xnvme_ident_from_uri(uri, &ident)) {
			XNVME_DEBUG("uri: '%s'", uri);
			continue;
		}
		if (xnvme_be_fbsd_dev_from_ident(&ident, &dev)) {
			XNVME_DEBUG("FAILED: xnvme_be_fbsd_dev_from_ident()");
			continue;
		}
		xnvme_be_fbsd_dev_close(dev);
		free(dev);

		if (xnvme_enumeration_append(list, &ident)) {
			XNVME_DEBUG("FAILED: adding ident");
		}
	}

	for (int di = 0; di < ndev; ++di) {
		free(dent[di]);
	}
	free(dent);

	return 0;
}

void *
xnvme_be_fbsd_buf_alloc(const struct xnvme_dev *XNVME_UNUSED(dev),
			size_t nbytes, uint64_t *XNVME_UNUSED(phys))
{
	//NOTE: Assign virt to phys?
	return xnvme_buf_virt_alloc(getpagesize(), nbytes);
}

void *
xnvme_be_fbsd_buf_realloc(const struct xnvme_dev *XNVME_UNUSED(dev),
			  void *XNVME_UNUSED(buf), size_t XNVME_UNUSED(nbytes),
			  uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: XNVME_BE_FBSD: does not support realloc");
	errno = ENOSYS;
	return NULL;
}

void
xnvme_be_fbsd_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	xnvme_buf_virt_free(buf);
}

int
xnvme_be_fbsd_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev),
			  void *XNVME_UNUSED(buf), uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: XNVME_BE_FBSD: does not support DMA alloc");
	return -ENOSYS;
}

#endif

static const char *g_schemes[] = {
	"file",
	XNVME_BE_FBSD_NAME,
};

struct xnvme_be xnvme_be_fbsd = {
#ifdef XNVME_BE_FBSD_ENABLED
	.mem = {
		.buf_alloc = xnvme_be_fbsd_buf_alloc,
		.buf_realloc = xnvme_be_fbsd_buf_realloc,
		.buf_free = xnvme_be_fbsd_buf_free,
		.buf_vtophys = xnvme_be_fbsd_buf_vtophys,
	},
	.dev = {
		.enumerate = xnvme_be_fbsd_enumerate,
		.dev_from_ident = xnvme_be_fbsd_dev_from_ident,
		.dev_close = xnvme_be_fbsd_dev_close,
	},
#else
	.mem = XNVME_BE_NOSYS_MEM,
	.dev = XNVME_BE_NOSYS_DEV,
#endif
	.sync = XNVME_BE_NOSYS_SYNC,
	.async = XNVME_BE_NOSYS_QUEUE,
	.attr = {
		.name = XNVME_BE_FBSD_NAME,
#ifdef XNVME_BE_FBSD_ENABLED
		.enabled = 1,
#else
		.enabled = 0,
#endif
		.schemes = g_schemes,
		.nschemes = sizeof g_schemes / sizeof(*g_schemes),
	},
	.state = { 0 },
};
