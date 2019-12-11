// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#define XNVME_BE_FIOC_BID 0xFB5D
#define XNVME_BE_FIOC_NAME "FIOC"

int
xnvme_be_fioc_ident_from_uri(const char *uri, struct xnvme_ident *ident)
{
	char tail[XNVME_IDENT_URI_LEN] = { 0 };
	char uri_prefix[10] = { 0 };
	int uri_has_prefix = 0;
	uint32_t cntid, nsid;
	int matches;

	uri_has_prefix = uri_parse_prefix(uri, uri_prefix) == 0;
	if (uri_has_prefix && strncmp(uri_prefix, "fioc", 4)) {
		errno = EINVAL;
		return -1;
	}

	strncpy(ident->uri, uri, XNVME_IDENT_URI_LEN);
	ident->bid = XNVME_BE_FIOC_BID;

	matches = sscanf(uri, uri_has_prefix ? \
			 "fioc://" XNVME_FREEBSD_NS_SCAN : \
			 XNVME_FREEBSD_NS_SCAN,
			 &cntid, &nsid, tail);

	if (matches == 2) {
		ident->type = XNVME_IDENT_NS;
		ident->nsid = nsid;
		ident->nst = XNVME_SPEC_NSTYPE_NOCHECK;
		snprintf(ident->be_uri, XNVME_IDENT_URI_LEN,
			 XNVME_FREEBSD_NS_FMT, cntid, nsid);
		return 0;
	}

	matches = sscanf(uri, uri_has_prefix ? \
			 "fioc://" XNVME_FREEBSD_CTRLR_SCAN : \
			 XNVME_FREEBSD_CTRLR_SCAN,
			 &cntid, tail);
	if (matches == 1) {
		ident->type = XNVME_IDENT_CTRLR;
		snprintf(ident->be_uri, XNVME_IDENT_URI_LEN,
			 XNVME_FREEBSD_CTRLR_FMT, cntid);
		return 0;
	}

	errno = EINVAL;
	return -1;
}

#ifndef XNVME_BE_FIOC_ENABLED
struct xnvme_be xnvme_be_fioc = {
	.bid = XNVME_BE_FIOC_BID,
	.name = XNVME_BE_FIOC_NAME,

	.ident_from_uri = xnvme_be_fioc_ident_from_uri,

	.enumerate = xnvme_be_nosys_enumerate,

	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,

	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,

	.async_init = xnvme_be_nosys_async_init,
	.async_term = xnvme_be_nosys_async_term,
	.async_poke = xnvme_be_nosys_async_poke,
	.async_wait = xnvme_be_nosys_async_wait,

	.cmd_pass = xnvme_be_nosys_cmd_pass,
	.cmd_pass_admin = xnvme_be_nosys_cmd_pass_admin,
};
#else

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
#include <xnvme_be_fioc.h>
#include <xnvme_dev.h>

static int
_nvme_filter(const struct dirent *d)
{
	char path[264];
	struct stat bd;
	int ctrl, ns, part;

	if (d->d_name[0] == '.') {
		return 0;
	}

	if (strstr(d->d_name, XNVME_PRFX_CTRLR)) {
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
xnvme_be_fioc_enumerate(struct xnvme_enumeration *list, int XNVME_UNUSED(opts))
{
	struct dirent **dent = NULL;
	int ndev = 0;

	ndev = scandir(_PATH_DEV, &dent, _nvme_filter, alphasort);

	for (int di = 0; di < ndev; ++di) {
		struct xnvme_ident entry = { 0 };
		int cntid, nsid;

		switch (sscanf(dent[di]->d_name,
			       XNVME_PRFX_CTRLR
			       "%d"
			       XNVME_PRFX_NS_FBSD
			       "%d",
			       &cntid, &nsid)) {
		case 2:
			entry.type = XNVME_IDENT_NS;
			snprintf(entry.uri, sizeof(entry.uri),
				 _PATH_DEV
				 XNVME_PRFX_CTRLR
				 "%d"
				 XNVME_PRFX_NS_FBSD
				 "%d", cntid, nsid);

			break;
		case 1:
			entry.type = XNVME_IDENT_CTRLR;
			snprintf(entry.uri, sizeof(entry.uri),
				 _PATH_DEV
				 XNVME_PRFX_CTRLR
				 "%d", cntid);
			break;

		default:
			continue;
		}

		entry.be_id = XNVME_BE_FIOC;

		if (xnvme_enumeration_append(list, &entry)) {
			XNVME_DEBUG("FAILED: adding entry");
		}
	}

	for (int di = 0; di < ndev; ++di) {
		free(dent[di]);
	}
	free(dent);

	return 0;
}

void *
xnvme_be_fioc_buf_alloc(const struct xnvme_dev *XNVME_UNUSED(dev),
			size_t nbytes, uint64_t *XNVME_UNUSED(phys))
{
	//NOTE: Assign virt to phys?
	return xnvme_buf_virt_alloc(getpagesize(), nbytes);
}

void *
xnvme_be_fioc_buf_realloc(const struct xnvme_dev *XNVME_UNUSED(dev),
			  void *XNVME_UNUSED(buf), size_t XNVME_UNUSED(nbytes),
			  uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: XNVME_BE_FIOC: does not support realloc");
	errno = ENOSYS;
	return NULL;
}

void
xnvme_be_fioc_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	xnvme_buf_virt_free(buf);
}

int
xnvme_be_fioc_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev),
			  void *XNVME_UNUSED(buf), uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: XNVME_BE_FIOC: does not support DMA alloc");
	errno = ENOSYS;
	return -1;
}

void
xnvme_be_fioc_state_term(struct xnvme_be_fioc *state)
{
	if (!state) {
		return;
	}

	close(state->fd);

	free(state);
}

struct xnvme_be_fioc *
xnvme_be_fioc_state_init(const struct xnvme_ident *ident, int XNVME_UNUSED(flags))
{
	struct xnvme_be_fioc *state = NULL;

	state = malloc(sizeof(*state));
	if (!state) {
		XNVME_DEBUG("FAILED: malloc(xnvme_be_fioc/state)");
		return NULL;
	}
	memset(state, 0, sizeof(*state));

	state->be = xnvme_be_fioc;

	state->fd = open(ident->be_uri, O_RDWR);
	if (state->fd < 0) {
		free(state);
		XNVME_DEBUG("FAILED: open(be_uri(%s)), state->fd(%d)\n",
			    ident->uri, state->fd);
		// Propagate errno from open
		return NULL;
	}

	return state;
}

struct xnvme_dev *
xnvme_be_fioc_dev_open(const struct xnvme_ident *ident, int flags)
{
	struct xnvme_be_fioc *state = NULL;
	struct xnvme_dev *dev = NULL;
	int err;

	state = xnvme_be_fioc_state_init(ident, flags);
	if (!state) {
		XNVME_DEBUG("FAILED: xnvme_be_fioc_state_init");
		return NULL;
	}

	dev = malloc(sizeof(*dev));
	if (!dev) {
		xnvme_be_fioc_state_term(state);
		XNVME_DEBUG("FAILED: allocating 'struct xnvme_dev'\n");
		// Propagate errno from malloc
		return NULL;
	}
	memset(dev, 0, sizeof(*dev));
	dev->ident = *ident;

	dev->be = (struct xnvme_be *)state;

	{
		struct xnvme_spec_idfy *idfy = NULL;
		struct xnvme_req req = { 0 };

		idfy = xnvme_buf_alloc(dev, sizeof(*idfy), NULL);
		if (!idfy) {
			XNVME_DEBUG("FAILED: failed allocing buffer");
			return NULL;
		}

		memset(idfy, 0, sizeof(*idfy));
		err = xnvme_cmd_idfy(dev, XNVME_SPEC_IDFY_CTRLR, 0, 0, 0, 0,
				     0x0, idfy, &ret);
		if (err || xnvme_ret_cpl_status(&ret)) {
			XNVME_DEBUG("FAILED: identify controller");
			xnvme_buf_free(dev, idfy);
			xnvme_be_fioc_state_term(state);
			free(dev);
			return NULL;
		}
		memcpy(&dev->ctrlr, idfy, sizeof(*idfy));

		{
			int ioctl_nsid = dev->ident.nsid ? dev->ident.nsid : 1;

			memset(idfy, 0, sizeof(*idfy));
			err = xnvme_cmd_idfy(dev, XNVME_SPEC_IDFY_NS, 0, 0xFF,
					     ioctl_nsid, 0, 0x0, idfy, &ret);
			if (err || xnvme_ret_cpl_status(&ret)) {
				XNVME_DEBUG("FAILED: identify namespace, err: %d", err);
				xnvme_buf_free(dev, idfy);
				xnvme_be_fioc_state_term(state);
				free(dev);
				return NULL;
			}
			memcpy(&dev->ns, idfy, sizeof(*idfy));
		}

		xnvme_buf_free(dev, idfy);
	}

	xnvme_be_dev_derive_geometry(dev);

	return dev;
}

void
xnvme_be_fioc_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	xnvme_be_fioc_state_term((struct xnvme_be_fioc *)dev->be);
	dev->be = NULL;
}

struct xnvme_be xnvme_be_fioc = {
	.bid = XNVME_BE_FIOC_BID,
	.name = XNVME_BE_FIOC_NAME,

	.ident_from_uri = xnvme_be_fioc_ident_from_uri,

	.enumerate = xnvme_be_fioc_enumerate,

	.dev_open = xnvme_be_fioc_dev_open,
	.dev_close = xnvme_be_fioc_dev_close,

	.buf_alloc = xnvme_be_fioc_buf_alloc,
	.buf_realloc = xnvme_be_fioc_buf_realloc,
	.buf_free = xnvme_be_fioc_buf_free,
	.buf_vtophys = xnvme_be_fioc_buf_vtophys,

	.async_init = xnvme_be_nosys_async_init,
	.async_term = xnvme_be_nosys_async_term,
	.async_poke = xnvme_be_nosys_async_poke,
	.async_wait = xnvme_be_nosys_async_wait,

	.cmd_pass = xnvme_be_nosys_cmd_pass,
	.cmd_pass_admin = xnvme_be_nosys_cmd_pass_admin,
};
#endif
