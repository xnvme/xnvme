// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#define _GNU_SOURCE
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#define XNVME_BE_LIOC_BID 0xDEAD
#define XNVME_BE_LIOC_NAME "LIOC"

int
xnvme_be_lioc_ident_from_uri(const char *uri, struct xnvme_ident *ident)
{
	char tail[XNVME_IDENT_URI_LEN] = { 0 };
	char uri_prefix[10] = { 0 };
	int uri_has_prefix = 0;
	uint32_t cntid, nsid;
	int matches;

	uri_has_prefix = uri_parse_prefix(uri, uri_prefix) == 0;
	if (uri_has_prefix && strncmp(uri_prefix, "lioc", 4)) {
		errno = EINVAL;
		return -1;
	}

	strncpy(ident->uri, uri, XNVME_IDENT_URI_LEN);
	ident->bid = XNVME_BE_LIOC_BID;

	matches = sscanf(uri, uri_has_prefix ? \
			 "lioc://" XNVME_LINUX_NS_SCAN : \
			 XNVME_LINUX_NS_SCAN,
			 &cntid, &nsid, tail);
	if (matches == 2) {
		ident->type = XNVME_IDENT_NS;
		ident->nsid = nsid;
		ident->nst = XNVME_SPEC_NSTYPE_NOCHECK;
		snprintf(ident->be_uri, XNVME_IDENT_URI_LEN,
			 XNVME_LINUX_NS_FMT, cntid, nsid);
		return 0;
	}

	matches = sscanf(uri, uri_has_prefix ? \
			 "lioc://" XNVME_LINUX_CTRLR_SCAN : \
			 XNVME_LINUX_CTRLR_SCAN,
			 &cntid, tail);
	if (matches == 1) {
		ident->type = XNVME_IDENT_CTRLR;
		snprintf(ident->be_uri, XNVME_IDENT_URI_LEN,
			 XNVME_LINUX_CTRLR_FMT, cntid);
		return 0;
	}

	errno = EINVAL;
	return -1;
}

#ifndef XNVME_BE_LIOC_ENABLED
struct xnvme_be xnvme_be_lioc = {
	.bid = XNVME_BE_LIOC_BID,
	.name = XNVME_BE_LIOC_NAME,

	.ident_from_uri = xnvme_be_lioc_ident_from_uri,

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
#include <fcntl.h>
#include <errno.h>
#include <linux/fs.h>
#include <linux/nvme_ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <paths.h>
#include <xnvme_be_lioc.h>
#include <xnvme_dev.h>

/**
 * Encapsulation of NVMe command representation as sent via the Linux IOCTLs
 *
 * Defs in: linux/nvme_ioctl.h
 */
struct xnvme_be_lioc_ioctl {
	union {
		struct nvme_user_io user_io;
		struct nvme_passthru_cmd passthru_cmd;
		struct nvme_passthru_cmd admin_cmd;

		struct {
			struct xnvme_spec_cmd nvme;
			uint32_t timeout_ms;
			uint32_t result;
		};

		uint32_t cdw[18];	///< IOCTL as an array of dwords
	};
};

void
xnvme_be_lioc_cmd_pr(struct xnvme_be_lioc_ioctl *kcmd)
{
	printf("xnvme_be_lioc_ioctl:\n");
	for (int32_t i = 0; i < 16; ++i)
		printf("  cdw%02"PRIi32": "XNVME_I32_FMT"\n", i,
		       XNVME_I32_TO_STR(kcmd->cdw[i]));
}

#ifdef XNVME_DEBUG_ENABLED
static const char *
ioctl_request_to_str(unsigned long req)
{
	switch (req) {
	case NVME_IOCTL_ID:
		return "NVME_IOCTL_ID";
	case NVME_IOCTL_ADMIN_CMD:
		return "NVME_IOCTL_ADMIN_CMD";
	case NVME_IOCTL_SUBMIT_IO:
		return "NVME_IOCTL_SUBMIT_IO";
	case NVME_IOCTL_IO_CMD:
		return "NVME_IOCTL_IO_CMD";
	case NVME_IOCTL_RESET:
		return "NVME_IOCTL_RESET";
	case NVME_IOCTL_SUBSYS_RESET:
		return "NVME_IOCTL_SUBSYS_RESET";
	case NVME_IOCTL_RESCAN:
		return "NVME_IOCTL_RESCAN";

	default:
		return "NVME_IOCTL_UNKNOWN";
	}
}
#endif

void *
xnvme_be_lioc_buf_alloc(const struct xnvme_dev *XNVME_UNUSED(dev),
			size_t nbytes, uint64_t *XNVME_UNUSED(phys))
{
	//NOTE: Assign virt to phys?
	return xnvme_buf_virt_alloc(getpagesize(), nbytes);
}

void *
xnvme_be_lioc_buf_realloc(const struct xnvme_dev *XNVME_UNUSED(dev),
			  void *XNVME_UNUSED(buf), size_t XNVME_UNUSED(nbytes),
			  uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: xnvme_be_lioc: does not support realloc");
	errno = ENOSYS;
	return NULL;
}

void
xnvme_be_lioc_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	xnvme_buf_virt_free(buf);
}

int
xnvme_be_lioc_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev),
			  void *XNVME_UNUSED(buf), uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: xnvme_be_lioc: does not support DMA alloc");
	errno = ENOSYS;
	return -1;
}

static inline int
ioctl_wrap(struct xnvme_dev *dev, unsigned long ioctl_req,
	   struct xnvme_be_lioc_ioctl *kcmd, struct xnvme_req *req)
{
	struct xnvme_be_lioc *state = (struct xnvme_be_lioc *)dev->be;
	int err = ioctl(state->fd, ioctl_req, kcmd);

	if (req) {	// TODO: fix return / completion data from kcmd
		req->cpl.cdw0 = kcmd->result;
	}
	if (!err) {	// No errors
		return 0;
	}

	XNVME_DEBUG("FAILED: ioctl(%s), err(%d), errno(%d) (%s)",
		    ioctl_request_to_str(ioctl_req), err, errno,
		    strerror(errno));

	if (!req) {
		XNVME_DEBUG("INFO: !req => setting errno and returning err");
		errno = errno ? errno : EIO;
		return err;
	}

	// Transform ioctl EINVAL to Invalid field in command
	if (err == -1 && errno == EINVAL) {
		XNVME_DEBUG("INFO: ioctl-errno(EINVAL) => INV_FIELD_IN_CMD");
		XNVME_DEBUG("INFO: overwrr. err(%d) with '0x2'", err);
		err = 0x2;
	}
	if (err > 0 && !req->cpl.status.val) {
		XNVME_DEBUG("INFO: overwr. cpl.status.val(0x%x) with '%d'",
			    req->cpl.status.val, err);
		req->cpl.status.val = err;
	}
	if (!errno) {
		XNVME_DEBUG("INFO: !errno, setting errno=EIO");
		errno = EIO;
	}

	return err;
}

int
xnvme_be_lioc_cmd_pass(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
		       void *dbuf, size_t dbuf_nbytes, void *mbuf,
		       size_t mbuf_nbytes, int opts, struct xnvme_req *req)
{
	struct xnvme_be_lioc_ioctl kcmd = { 0 };
	int err;

	if (XNVME_CMD_ASYNC & opts) {
		XNVME_DEBUG("FAILED: XNVME_CMD_ASYNC is not implemented");
		errno = EINVAL;
		return -1;
	}

	kcmd.nvme = *cmd;
	kcmd.nvme.common.dptr.lnx_ioctl.data = (uint64_t)dbuf;
	kcmd.nvme.common.dptr.lnx_ioctl.data_len = dbuf_nbytes;

	kcmd.nvme.common.mptr = (uint64_t)mbuf;
	kcmd.nvme.common.dptr.lnx_ioctl.metadata_len = mbuf_nbytes;

	err = ioctl_wrap(dev, NVME_IOCTL_IO_CMD, &kcmd, req);
	if (err) {
		XNVME_DEBUG("FAILED: ioctl_wrap(), err: %d", err);
		return -1; // NOTE: Propagate errno
	}

	return 0;
}

int
xnvme_be_lioc_cmd_pass_admin(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
			     void *dbuf, size_t dbuf_nbytes, void *mbuf,
			     size_t mbuf_nbytes, int opts,
			     struct xnvme_req *req)
{
	struct xnvme_be_lioc_ioctl kcmd = { 0 };
	int err;

	if (XNVME_CMD_ASYNC & opts) {
		XNVME_DEBUG("FAILED: XNVME_CMD_ASYNC is not implemented");
		errno = EINVAL;
		return -1;
	}

	kcmd.nvme = *cmd;
	kcmd.nvme.common.dptr.lnx_ioctl.data = (uint64_t)dbuf;
	kcmd.nvme.common.dptr.lnx_ioctl.data_len = dbuf_nbytes;

	kcmd.nvme.common.mptr = (uint64_t)mbuf;
	kcmd.nvme.common.dptr.lnx_ioctl.metadata_len = mbuf_nbytes;

	err = ioctl_wrap(dev, NVME_IOCTL_ADMIN_CMD, &kcmd, req);
	if (err) {
		XNVME_DEBUG("FAILED: ioctl_wrap() err: %d", err);
		return -1; // NOTE: Propagate errno
	}

	return 0;
}

/*

**
 * Kernel-issue workaround, using deprecated IOCTL for scalar commands with meta
 * due to a bug in the kernel (see kernel commit 9b382768), we have to
 * use the deprecated ioctl NVME_IOCTL_SUBMIT_IO command to submit the
 * request if it uses meta data.
 *
 * NOTE: NVME_IOCTL_SUBMIT_IO does NOT work if the request does NOT have
 * metadata the bug is fixed in linux v4.18.

static inline int
cmd_wr_dep_ioc(struct xnvme_dev *dev,
	       uint64_t slba, int nlb,
	       void *dbuf, void *mbuf,
	       uint16_t XNVME_UNUSED(flags),
	       uint16_t opcode,
	       struct xnvme_req *req)
{
	struct xnvme_be_lioc_ioctl kcmd = { 0 };

	kcmd.user_io.opcode = opcode;
	kcmd.user_io.nblocks = nlb;
	kcmd.user_io.metadata = (__u64)(uintptr_t) mbuf;
	kcmd.user_io.addr = (__u64)(uintptr_t) dbuf;
	kcmd.user_io.slba = slba;

	int err = ioctl_wrap(dev, NVME_IOCTL_SUBMIT_IO, &kcmd, req);
	if (err) {
		XNVME_DEBUG("ioctl_wrap");
		return -1;
	}

	return 0;
}

// Helper function for NVMe IO: write/read/append
static inline int cmd_wr(struct xnvme_dev *dev, uint32_t nsid, uint64_t slba,
			 uint16_t nlb, void *dbuf, void *mbuf, uint16_t flags,
			 uint16_t opcode, struct xnvme_req *req)
{
	struct xnvme_be_lioc_ioctl kcmd = { 0 };
	int err = 0;

	if (flags & XNVME_CMD_ASYNC) {
		XNVME_DEBUG("FAILED: xnvme_be_lioc ENOSYS XNVME_CMD_ASYNC");
		errno = EINVAL;
		return -1;
	}

	if (mbuf) {
		return cmd_wr_dep_ioc(dev, slba, nlb, dbuf, mbuf, flags,
				      opcode, req);
	}

	kcmd.nvme.common.opcode = opcode;
	kcmd.nvme.common.nsid = nsid;
	kcmd.nvme.common.mptr = (uint64_t) mbuf;
	kcmd.nvme.common.dptr.lnx_ioctl.data = (uint64_t) dbuf;
	kcmd.nvme.common.dptr.lnx_ioctl.data_len = dev->geo.nbytes * (nlb + 1);
	kcmd.nvme.common.dptr.lnx_ioctl.metadata_len = mbuf ? dev->geo.nbytes_oob * (nlb + 1) : 0;
	kcmd.nvme.lblk.slba = slba;
	kcmd.nvme.lblk.nlb = nlb;

	err = ioctl_wrap(dev, NVME_IOCTL_IO_CMD, &kcmd, req);
	if (err) {
		XNVME_DEBUG("FAILED: ioctl_wrap, err: '%#x'", err);
		return -1;
	}

	return 0;
}
*/

void
xnvme_be_lioc_state_term(struct xnvme_be_lioc *state)
{
	if (!state) {
		return;
	}

	close(state->fd);

	free(state);
}

struct xnvme_be_lioc *
xnvme_be_lioc_state_init(const struct xnvme_ident *ident, int flags)
{
	struct xnvme_be_lioc *state = NULL;
	struct stat dev_stat;
	int err;

	state = malloc(sizeof(*state));
	if (!state) {
		XNVME_DEBUG("FAILED: malloc(xnvme_be_lioc/state)");
		return NULL;
	}
	memset(state, 0, sizeof(*state));

	state->be = xnvme_be_lioc;

	switch (flags) {
	case XNVME_BE_LIOC_WRITABLE:
		state->fd = open(ident->be_uri, O_RDWR | O_DIRECT);
		break;

	default:
		state->fd = open(ident->be_uri, O_RDONLY);
		break;
	}
	if (state->fd < 0) {
		free(state);
		XNVME_DEBUG("FAILED: open(be_uri(%s)), state->fd(%d)\n",
			    ident->be_uri, state->fd);
		// Propagate errno from dev_open
		return NULL;
	}

	err = fstat(state->fd, &dev_stat);
	if (err < 0) {
		free(state);
		// Propagate errno from fstat
		return NULL;
	}
	if (!S_ISBLK(dev_stat.st_mode)) {
		free(state);
		XNVME_DEBUG("FAILED: device is not a block device");
		errno = ENOTBLK;
		return NULL;
	}

	return state;
}

// Needs ident.uri, and ident.nsid, assigns dev->ident = *ident; + nst
struct xnvme_dev *
xnvme_be_lioc_dev_open(const struct xnvme_ident *ident, int flags)
{
	struct xnvme_be_lioc *state = NULL;
	struct xnvme_spec_idfy *idfy = NULL;
	struct xnvme_dev *dev = NULL;
	struct xnvme_req req = { 0 };
	int ioctl_nsid;
	int err;

	state = xnvme_be_lioc_state_init(ident, flags);
	if (!state) {
		XNVME_DEBUG("FAILED: xnvme_be_lioc_state_init");
		return NULL;
	}

	dev = malloc(sizeof(*dev));
	if (!dev) {
		xnvme_be_lioc_state_term(state);
		XNVME_DEBUG("FAILED: allocating 'struct xnvme_dev'\n");
		// Propagate errno from malloc
		return NULL;
	}
	memset(dev, 0, sizeof(*dev));

	dev->be = (struct xnvme_be *)state;
	dev->ident = *ident;

	ioctl_nsid = ioctl(state->fd, NVME_IOCTL_ID);
	if (ioctl_nsid < 1) {
		XNVME_DEBUG("FAILED: retrieving nsid, got: %x", ioctl_nsid);
		// Propagate errno from ioctl
		xnvme_be_lioc_state_term(state);
		free(dev);
		return NULL;
	}
	if ((uint32_t)ioctl_nsid != dev->ident.nsid) {
		XNVME_DEBUG("EINVAL: ioctl_nsid(0x%x) != dev->ident.nsid(0x%x)",
			    ioctl_nsid, dev->ident.nsid);
		xnvme_be_lioc_state_term(state);
		free(dev);
		errno = EINVAL;
		return NULL;
	}

	idfy = xnvme_buf_alloc(dev, sizeof(*idfy), NULL);
	if (!idfy) {
		XNVME_DEBUG("FAILED: failed allocating buffer");
		return NULL;
	}

	// Store idfy-ctrl
	memset(idfy, 0, sizeof(*idfy));
	memset(&req, 0, sizeof(req));
	err = xnvme_cmd_idfy_ctrlr(dev, idfy, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		XNVME_DEBUG("FAILED: identify controller");
		xnvme_buf_free(dev, idfy);
		xnvme_be_lioc_state_term(state);
		free(dev);
		return NULL;
	}
	memcpy(&dev->ctrlr, idfy, sizeof(*idfy));

	// Store unchecked ns-idfy for future reference
	memset(idfy, 0, sizeof(*idfy));
	memset(&req, 0, sizeof(req));
	err = xnvme_cmd_idfy_ns(dev, dev->ident.nsid, XNVME_SPEC_NSTYPE_NOCHECK,
				idfy, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		XNVME_DEBUG("FAILED: identify namespace, err: %d", err);
		xnvme_buf_free(dev, idfy);
		xnvme_be_lioc_state_term(state);
		free(dev);
		return NULL;
	}
	memcpy(&dev->ns, idfy, sizeof(*idfy));

	// Attempt to identify LBLK Namespace
	memset(&req, 0, sizeof(req));
	err = xnvme_cmd_idfy_ns(dev, dev->ident.nsid, XNVME_SPEC_NSTYPE_LBLK,
				idfy, &req);
	if (!(err || xnvme_req_cpl_status(&req))) {
		XNVME_DEBUG("INFO: Got XNVME_SPEC_NSTYPE_LBLK");
		dev->ident.nst = XNVME_SPEC_NSTYPE_LBLK;
		goto exit;
	}

	// Attempt to identify Zoned Namespace
	memset(&req, 0, sizeof(req));
	err = xnvme_cmd_idfy_ns(dev, dev->ident.nsid, XNVME_SPEC_NSTYPE_ZONED,
				idfy, &req);
	if (!(err || xnvme_req_cpl_status(&req))) {
		XNVME_DEBUG("INFO: Got XNVME_SPEC_NSTYPE_ZONED");
		dev->ident.nst = XNVME_SPEC_NSTYPE_ZONED;
		goto exit;
	}

	XNVME_DEBUG("FAILED: could not identify namespace...");

exit:
	xnvme_be_dev_derive_geometry(dev);

	xnvme_buf_free(dev, idfy);

	return dev;
}

void
xnvme_be_lioc_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	xnvme_be_lioc_state_term((struct xnvme_be_lioc *)dev->be);
	dev->be = NULL;
}

int
xnvme_path_nvme_filter(const struct dirent *d)
{
	char path[264];
	struct stat bd;
	int ctrl, ns, part;

	if (d->d_name[0] == '.') {
		return 0;
	}

	if (strstr(d->d_name, "nvme")) {
		snprintf(path, sizeof(path), "%s%s", "/dev/", d->d_name);
		if (stat(path, &bd)) {
			return 0;
		}
		if (!S_ISBLK(bd.st_mode)) {
			return 0;
		}
		if (sscanf(d->d_name, "nvme%dn%dp%d", &ctrl, &ns, &part) == 3) {
			// Do not want to use e.g. nvme0n1p2 etc.
			return 0;
		}
		return 1;
	}

	return 0;
}

/**
 * Scanning /sys/class/nvme can give device names, such as "nvme0c65n1", which
 * are linked as virtual devices to the block device. So instead of scanning
 * that dir, then instead /sys/block/ is scanned under the assumption that
 * block-devices with "nvme" in them are NVMe devices with namespaces attached
 */
int
xnvme_be_lioc_enumerate(struct xnvme_enumeration *list, int XNVME_UNUSED(opts))
{
	struct dirent **ns = NULL;
	int nns = 0;

	// TODO: add enumeration of NS vs CTRLR and use _PATH_DEV

	nns = scandir("/sys/block", &ns, xnvme_path_nvme_filter, alphasort);
	for (int ni = 0; ni < nns; ++ni) {
		struct xnvme_ident ident = { 0 };
		int nsid, fd;

		snprintf(ident.be_uri, sizeof(ident.uri), _PATH_DEV "%s",
			 ns[ni]->d_name);
		snprintf(ident.uri, sizeof(ident.uri), "lioc://" _PATH_DEV "%s",
			 ns[ni]->d_name);

		ident.type = XNVME_IDENT_NS;
		ident.bid = XNVME_BE_LIOC_BID;

		fd = open(ident.be_uri, O_RDWR | O_DIRECT);
		nsid = ioctl(fd, NVME_IOCTL_ID);
		close(fd);
		if (nsid < 1) {
			XNVME_DEBUG("FAILED: retrieving nsid, got: %x", nsid);
			continue;
		}

		ident.nsid = nsid;

		// Attempt to "open" the device, to determine NST
		{
			struct xnvme_dev *dev = NULL;

			dev = xnvme_be_lioc_dev_open(&ident, 0x0);
			if (!dev) {
				XNVME_DEBUG("FAILED: xnvme_be_lioc_dev_open()");
				continue;
			}

			ident = dev->ident;

			xnvme_be_lioc_dev_close(dev);
		}

		if (xnvme_enumeration_append(list, &ident)) {
			XNVME_DEBUG("FAILED: adding entry");
		}
	}

	for (int ni = 0; ni < nns; ++ni) {
		free(ns[ni]);
	}
	free(ns);

	return 0;
}

struct xnvme_be xnvme_be_lioc = {
	.bid = XNVME_BE_LIOC_BID,
	.name = XNVME_BE_LIOC_NAME,

	.ident_from_uri = xnvme_be_lioc_ident_from_uri,

	.enumerate = xnvme_be_lioc_enumerate,

	.dev_open = xnvme_be_lioc_dev_open,
	.dev_close = xnvme_be_lioc_dev_close,

	.buf_alloc = xnvme_be_lioc_buf_alloc,
	.buf_realloc = xnvme_be_lioc_buf_realloc,
	.buf_free = xnvme_be_lioc_buf_free,
	.buf_vtophys = xnvme_be_lioc_buf_vtophys,

	.async_init = xnvme_be_nosys_async_init,
	.async_term = xnvme_be_nosys_async_term,
	.async_poke = xnvme_be_nosys_async_poke,
	.async_wait = xnvme_be_nosys_async_wait,

	.cmd_pass = xnvme_be_lioc_cmd_pass,
	.cmd_pass_admin = xnvme_be_lioc_cmd_pass_admin,
};
#endif
