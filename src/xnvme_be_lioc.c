// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#define XNVME_BE_LIOC_NAME "lioc"

#ifdef XNVME_BE_LIOC_ENABLED
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
#include <libznd.h>

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
	XNVME_DEBUG("FAILED: xnvme_be_lioc: does not support phys/DMA alloc");
	return -ENOSYS;
}

static inline int
ioctl_wrap(struct xnvme_dev *dev, unsigned long ioctl_req,
	   struct xnvme_be_lioc_ioctl *kcmd, struct xnvme_req *req)
{
	struct xnvme_be_lioc_state *state = (void *)dev->be.state;
	int err = ioctl(state->fd, ioctl_req, kcmd);

	if (req) {	// TODO: fix return / completion data from kcmd
		req->cpl.cdw0 = kcmd->result;
	}
	if (!err) {	// No errors
		return 0;
	}

	XNVME_DEBUG("FAILED: ioctl(%s), err(%d), errno(%d)",
		    ioctl_request_to_str(ioctl_req), err, errno);

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
		return -EINVAL;
	}

	kcmd.nvme = *cmd;
	kcmd.nvme.common.dptr.lnx_ioctl.data = (uint64_t)dbuf;
	kcmd.nvme.common.dptr.lnx_ioctl.data_len = dbuf_nbytes;

	kcmd.nvme.common.mptr = (uint64_t)mbuf;
	kcmd.nvme.common.dptr.lnx_ioctl.metadata_len = mbuf_nbytes;

	err = ioctl_wrap(dev, NVME_IOCTL_IO_CMD, &kcmd, req);
	if (err) {
		XNVME_DEBUG("FAILED: ioctl_wrap(), err: %d", err);
		return err;
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
		return -EINVAL;
	}

	kcmd.nvme = *cmd;
	kcmd.nvme.common.dptr.lnx_ioctl.data = (uint64_t)dbuf;
	kcmd.nvme.common.dptr.lnx_ioctl.data_len = dbuf_nbytes;

	kcmd.nvme.common.mptr = (uint64_t)mbuf;
	kcmd.nvme.common.dptr.lnx_ioctl.metadata_len = mbuf_nbytes;

	err = ioctl_wrap(dev, NVME_IOCTL_ADMIN_CMD, &kcmd, req);
	if (err) {
		XNVME_DEBUG("FAILED: ioctl_wrap() err: %d", err);
		return err;
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
xnvme_be_lioc_state_term(struct xnvme_be_lioc_state *state)
{
	if (!state) {
		return;
	}

	close(state->fd);
}

int
xnvme_be_lioc_state_init(struct xnvme_dev *dev, void *opts)
{
	struct xnvme_be_lioc_state *state = (void *)dev->be.state;
	struct stat dev_stat;
	uint32_t opt_val;
	int err;

	switch ((uint64_t)opts) {
	case XNVME_BE_LIOC_WRITABLE:
		state->fd = open(dev->ident.trgt, O_RDWR | O_DIRECT);
		break;

	default:
		state->fd = open(dev->ident.trgt, O_RDONLY);
		break;
	}
	if (state->fd < 0) {
		XNVME_DEBUG("FAILED: open(trgt(%s)), state->fd(%d)\n",
			    dev->ident.trgt, state->fd);
		return -errno;
	}

	err = fstat(state->fd, &dev_stat);
	if (err < 0) {
		return -errno;
	}
	if (!S_ISBLK(dev_stat.st_mode)) {
		XNVME_DEBUG("FAILED: device is not a block device");
		return -ENOTBLK;
	}

	if (xnvme_ident_opt_to_val(&dev->ident, "pseudo", &opt_val)) {
		state->pseudo = opt_val == 1;
	}

	XNVME_DEBUG("state->pseudo: %d", state->pseudo);

	return 0;
}

/**
 * Determine the following:
 *
 * - Determine device-type	(setup: dev->dtype)
 *
 * - Identify controller	(setup: dev->ctrlr)
 *
 * - Identify namespace		(setup: dev->ns)
 * - Identify namespace-id	(setup: dev->nsid)
 * - Determine Command Set	(setup: dev->csi)
 *
 * TODO: fixup for XNVME_DEV_TYPE_NVME_NAMESPACE
 * TODO: fixup for XNVME_DEV_TYPE_BLOCK_DEVICE
 */
int
xnvme_be_lioc_dev_idfy(struct xnvme_dev *dev)
{
	struct xnvme_be_lioc_state *state = (void *)dev->be.state;
	struct xnvme_spec_idfy *idfy_ctrlr = NULL, *idfy_ns = NULL;
	struct xnvme_req req = { 0 };
	int err;

	if (state->pseudo) {
		dev->dtype = XNVME_DEV_TYPE_BLOCK_DEVICE;
		dev->nsid = 1;
		dev->csi = XNVME_SPEC_CSI_LBLK;
		return 0;
	}

	dev->dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
	dev->csi = XNVME_SPEC_CSI_NOCHECK;
	err = ioctl(state->fd, NVME_IOCTL_ID);
	if (err < 1) {
		XNVME_DEBUG("FAILED: retrieving nsid, got: %x", err);
		// Propagate errno from ioctl
		goto exit;
	}
	dev->nsid = err;

	// Allocate buffers for idfy
	idfy_ctrlr = xnvme_buf_alloc(dev, sizeof(*idfy_ctrlr), NULL);
	if (!idfy_ctrlr) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc()");
		err = -errno;
		goto exit;
	}
	idfy_ns = xnvme_buf_alloc(dev, sizeof(*idfy_ns), NULL);
	if (!idfy_ns) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc()");
		err = -errno;
		goto exit;
	}

	// Retrieve and store ctrl and ns
	memset(idfy_ctrlr, 0, sizeof(*idfy_ctrlr));
	memset(&req, 0, sizeof(req));
	err = xnvme_cmd_idfy_ctrlr(dev, idfy_ctrlr, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		XNVME_DEBUG("FAILED: identify controller");
		err = err ? err : -EIO;
		goto exit;
	}
	memset(idfy_ns, 0, sizeof(*idfy_ns));
	memset(&req, 0, sizeof(req));
	err = xnvme_cmd_idfy_ns(dev, dev->nsid, idfy_ns, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		XNVME_DEBUG("FAILED: identify namespace, err: %d", err);
		goto exit;
	}
	memcpy(&dev->id.ctrlr, idfy_ctrlr, sizeof(*idfy_ctrlr));
	memcpy(&dev->id.ns, idfy_ns, sizeof(*idfy_ns));

	//
	// Determine command-set / namespace type by probing
	//
	dev->csi = XNVME_SPEC_CSI_LBLK;		// Assume NVM

	// Attempt to identify Zoned Namespace
	{
		struct znd_idfy_ns *zns = (void *)idfy_ns;

		memset(idfy_ctrlr, 0, sizeof(*idfy_ctrlr));
		memset(&req, 0, sizeof(req));
		err = xnvme_cmd_idfy_ctrlr_csi(dev, XNVME_SPEC_CSI_ZONED,
					       idfy_ctrlr, &req);
		if (err || xnvme_req_cpl_status(&req)) {
			XNVME_DEBUG("INFO: !id-ctrlr-zns");
			goto not_zns;
		}

		memset(idfy_ns, 0, sizeof(*idfy_ns));
		memset(&req, 0, sizeof(req));
		err = xnvme_cmd_idfy_ns_csi(dev, dev->nsid,
					    XNVME_SPEC_CSI_ZONED, idfy_ns,
					    &req);
		if (err || xnvme_req_cpl_status(&req)) {
			XNVME_DEBUG("INFO: !id-ns-zns");
			goto not_zns;
		}

		if (!zns->lbafe[0].zsze) {
			goto not_zns;
		}

		memcpy(&dev->idcss.ctrlr, idfy_ctrlr, sizeof(*idfy_ctrlr));
		memcpy(&dev->idcss.ns, idfy_ns, sizeof(*idfy_ns));
		dev->csi = XNVME_SPEC_CSI_ZONED;

		XNVME_DEBUG("INFO: looks like csi(ZNS)");
		goto exit;

not_zns:
		XNVME_DEBUG("INFO: failed idfy with csi(ZNS)");
	}

	// Attempt to identify LBLK Namespace
	memset(idfy_ns, 0, sizeof(*idfy_ns));
	memset(&req, 0, sizeof(req));
	err = xnvme_cmd_idfy_ns_csi(dev, dev->nsid, XNVME_SPEC_CSI_LBLK, idfy_ns, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		XNVME_DEBUG("INFO: not csi-specific id-NVM");
		XNVME_DEBUG("INFO: falling back to NVM assumption");
		err = 0;
		goto exit;
	}
	memcpy(&dev->idcss.ns, idfy_ns, sizeof(*idfy_ns));

exit:
	xnvme_buf_free(dev, idfy_ctrlr);
	xnvme_buf_free(dev, idfy_ns);

	return err;
}

int
xnvme_be_lioc_dev_from_ident(const struct xnvme_ident *ident,
			     struct xnvme_dev **dev)
{
	int err;

	err = xnvme_dev_alloc(dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_dev_alloc()");
		return err;
	}
	(*dev)->ident = *ident;
	(*dev)->be = xnvme_be_lioc;

	// TODO: determine nsid, csi, and device-type

	err = xnvme_be_lioc_state_init(*dev, NULL);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_lioc_state_init()");
		free(*dev);
		return err;
	}
	err = xnvme_be_lioc_dev_idfy(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_lioc_dev_idfy()");
		xnvme_be_lioc_state_term((void *)(*dev)->be.state);
		free(*dev);
		return err;
	}
	err = xnvme_be_dev_derive_geometry(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_dev_derive_geometry()");
		xnvme_be_lioc_state_term((void *)(*dev)->be.state);
		free(*dev);
		return err;
	}

	// TODO: consider this. Due to Kernel-segment constraint force mdts down
	if (((*dev)->geo.mdts_nbytes / (*dev)->geo.lba_nbytes) > 127) {
		(*dev)->geo.mdts_nbytes = (*dev)->geo.lba_nbytes * 127;
	}

	return 0;
}

void
xnvme_be_lioc_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	xnvme_be_lioc_state_term((void *)dev->be.state);
	memset(&dev->be, 0, sizeof(dev->be));
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
 *
 * TODO: add enumeration of NS vs CTRLR
 */
int
xnvme_be_lioc_enumerate(struct xnvme_enumeration *list, const char *sys_uri,
			int XNVME_UNUSED(opts))
{
	struct dirent **ns = NULL;
	int nns = 0;

	if (sys_uri) {
		XNVME_DEBUG("FAILED: sys_uri: %s is not supported", sys_uri);
		return -ENOSYS;
	}

	nns = scandir("/sys/block", &ns, xnvme_path_nvme_filter, alphasort);
	for (int ni = 0; ni < nns; ++ni) {
		char uri[XNVME_IDENT_URI_LEN] = { 0 };
		struct xnvme_ident ident = { 0 };
		struct xnvme_dev *dev;

		snprintf(uri, XNVME_IDENT_URI_LEN - 1,
			 XNVME_BE_LIOC_NAME ":" _PATH_DEV "%s",
			 ns[ni]->d_name);
		if (xnvme_ident_from_uri(uri, &ident)) {
			continue;
		}

		if (xnvme_be_lioc_dev_from_ident(&ident, &dev)) {
			XNVME_DEBUG("FAILED: xnvme_be_lioc_dev_from_ident()");
			continue;
		}
		xnvme_be_lioc_dev_close(dev);
		free(dev);

		if (xnvme_enumeration_append(list, &ident)) {
			XNVME_DEBUG("FAILED: adding ident");
		}
	}

	for (int ni = 0; ni < nns; ++ni) {
		free(ns[ni]);
	}
	free(ns);

	return 0;
}
#endif

static const char *g_schemes[] = {
	XNVME_BE_LIOC_NAME,
	"file",
};

struct xnvme_be xnvme_be_lioc = {
#ifdef XNVME_BE_LIOC_ENABLED
	.func = {
		.cmd_pass = xnvme_be_lioc_cmd_pass,
		.cmd_pass_admin = xnvme_be_lioc_cmd_pass_admin,

		.async_init = xnvme_be_nosys_async_init,
		.async_term = xnvme_be_nosys_async_term,
		.async_poke = xnvme_be_nosys_async_poke,
		.async_wait = xnvme_be_nosys_async_wait,

		.buf_alloc = xnvme_be_lioc_buf_alloc,
		.buf_realloc = xnvme_be_lioc_buf_realloc,
		.buf_free = xnvme_be_lioc_buf_free,
		.buf_vtophys = xnvme_be_lioc_buf_vtophys,

		.enumerate = xnvme_be_lioc_enumerate,

		.dev_from_ident = xnvme_be_lioc_dev_from_ident,
		.dev_close = xnvme_be_lioc_dev_close,
	},
#else
	.func = XNVME_BE_NOSYS_FUNC,
#endif
	.attr = {
		.name = XNVME_BE_LIOC_NAME,
#ifdef XNVME_BE_LIOC_ENABLED
		.enabled = 1,
#else
		.enabled = 0,
#endif
		.schemes = g_schemes,
		.nschemes = sizeof g_schemes / sizeof(*g_schemes),
	},
	.state = { 0 },
};

