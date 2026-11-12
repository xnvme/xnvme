// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_cref.h>
#include <xnvme_platform.h>
#include <xnvme_cmd.h>
#include <xnvme_dev.h>
#include <xnvme_geo.h>
#include <xnvme_lock.h>
#include <xnvme_be_state.h>

#ifdef XNVME_PLATFORM_LINUX_ENABLED
/**
 * Read a queue attribute from sysfs for the NVMe namespace associated with the given device
 *
 * Uses /sys/class/nvme/<ctrl>/<ctrl>n<ns>/queue/<attr> which exists for all NVMe
 * namespaces, including those without a block device (e.g. KV namespaces).
 *
 * @returns On success, 0 is returned. On error, negative errno is returned.
 */
static int
_linux_queue_attr(struct xnvme_dev *dev, const char *attr, uint32_t *val)
{
	char path[512];
	char *bname;
	FILE *fp;
	unsigned long rval = 0;
	int ctrl = 0;
	int ns = 0;
	int len;

	bname = strrchr(dev->ident.uri, '/');
	if (!bname || strlen(bname) < 2) {
		XNVME_DEBUG("FAILED: invalid uri '%s'", dev->ident.uri);
		return -EINVAL;
	}
	bname++;

	if ((sscanf(bname, "nvme%dn%d", &ctrl, &ns) != 2) &&
	    (sscanf(bname, "ng%dn%d", &ctrl, &ns) != 2)) {
		XNVME_DEBUG("FAILED: parsing ctrl/ns from '%s'", bname);
		return -EINVAL;
	}

	len = snprintf(path, sizeof(path), "/sys/class/nvme/nvme%d/nvme%dn%d/queue/%s", ctrl, ctrl,
		       ns, attr);
	if (len < 0) {
		XNVME_DEBUG("FAILED: snprintf(), err: %d", len);
		return len;
	}
	if ((size_t)len >= sizeof(path)) {
		XNVME_DEBUG("FAILED: path truncated for '%s'", bname);
		return -EINVAL;
	}

	fp = fopen(path, "r");
	if (!fp) {
		XNVME_DEBUG("FAILED: fopen('%s'), err: %d", path, -errno);
		return -errno;
	}

	if (fscanf(fp, "%lu", &rval) != 1) {
		XNVME_DEBUG("FAILED: fscanf('%s')", path);
		fclose(fp);
		return -EIO;
	}
	fclose(fp);

	*val = rval;

	return 0;
}
#endif

static int
_zoned_geometry(struct xnvme_dev *dev)
{
	struct xnvme_spec_idfy_ns *nvm = (void *)xnvme_dev_get_ns(dev);
	struct xnvme_spec_lbaf *lbaf =
		&nvm->lbaf[xnvme_bits_get(nvm->flbas, XNVME_SPEC_IDFY_NS_FLBAS_FIDXL)];
	struct xnvme_spec_znd_idfy_ns *zns = (void *)xnvme_dev_get_ns_css(dev);
	struct xnvme_spec_znd_idfy_lbafe *lbafe =
		&zns->lbafe[xnvme_bits_get(nvm->flbas, XNVME_SPEC_IDFY_NS_FLBAS_FIDXL)];
	struct xnvme_geo *geo = &dev->geo;

	if (!zns->lbafe[0].zsze) {
		XNVME_DEBUG("FAILED: !zns.lbafe[0].zsze");
		return -EINVAL;
	}

	geo->type = XNVME_GEO_ZONED;

	geo->npugrp = 1;
	geo->npunit = 1;
	geo->nzone = nvm->nsze / lbafe->zsze;
	geo->nsect = lbafe->zsze;

	geo->nbytes = 2 << (lbaf->lbads - 1);
	geo->nbytes_oob = lbaf->ms;

	geo->lba_nbytes = geo->nbytes;
	geo->lba_extended =
		xnvme_bits_get(nvm->flbas, XNVME_SPEC_IDFY_NS_FLBAS_MTELBA) && lbaf->ms;
	if (geo->lba_extended) {
		geo->lba_nbytes += geo->nbytes_oob;
	}

	return 0;
}

static inline int
_conventional_geometry(struct xnvme_dev *dev)
{
	const struct xnvme_spec_idfy_ctrlr *ctrlr = (void *)xnvme_dev_get_ctrlr(dev);
	const struct xnvme_spec_idfy_ns *nvm = (void *)xnvme_dev_get_ns(dev);
	const struct xnvme_spec_lbaf *lbaf =
		&nvm->lbaf[xnvme_bits_get(nvm->flbas, XNVME_SPEC_IDFY_NS_FLBAS_FIDXL)];
	struct xnvme_spec_nvm_idfy_ns *nns = (void *)xnvme_dev_get_ns_css(dev);
	struct xnvme_spec_elbaf *elbaf =
		&nns->elbaf[xnvme_bits_get(nvm->flbas, XNVME_SPEC_IDFY_NS_FLBAS_FIDXL)];
	struct xnvme_geo *geo = &dev->geo;

	geo->type = XNVME_GEO_CONVENTIONAL;

	geo->npugrp = 1;
	geo->npunit = 1;
	geo->nzone = 1;

	geo->nsect = dev->id.ns.nsze;
	geo->nbytes = 2 << (lbaf->lbads - 1);
	geo->nbytes_oob = lbaf->ms;

	geo->lba_nbytes = geo->nbytes;
	geo->lba_extended =
		xnvme_bits_get(nvm->flbas, XNVME_SPEC_IDFY_NS_FLBAS_MTELBA) && lbaf->ms;
	if (geo->lba_extended) {
		geo->lba_nbytes += geo->nbytes_oob;
	}

	geo->pi_type = xnvme_bits_get(nvm->dps, XNVME_SPEC_IDFY_NS_DPS_PIT);

	if (geo->pi_type) {
		geo->pi_format = XNVME_SPEC_NVM_NS_16B_GUARD;
		geo->pi_loc = xnvme_bits_get(nvm->dps, XNVME_SPEC_IDFY_NS_DPS_PIP);
	}

	if (xnvme_bits_get(ctrlr->ctratt, XNVME_SPEC_IDFY_CTRLR_CTRATT_ELBAS) && geo->pi_type) {
		geo->pi_format = xnvme_bits_get(elbaf->bits, XNVME_SPEC_ELBAF_BITS_PIF);
	}

	return 0;
}

static int
_fs_geometry(struct xnvme_dev *dev)
{
	const struct xnvme_spec_fs_idfy_ns *ns = (void *)xnvme_dev_get_ns_css(dev);
	struct xnvme_geo *geo = &dev->geo;

	geo->type = XNVME_GEO_CONVENTIONAL;

	geo->npugrp = 1;
	geo->npunit = 1;
	geo->nzone = 1;
	geo->nsect = 1;

	geo->nbytes = 1;
	geo->nbytes_oob = 0;

	geo->lba_nbytes = 512;
	geo->lba_extended = 0;

	geo->tbytes = ns->nuse;

	geo->mdts_nbytes = 1 << 20;
	geo->ssw = XNVME_ILOG2(geo->lba_nbytes);

	return 0;
}

static int
_kvs_geometry(struct xnvme_dev *dev)
{
	struct xnvme_geo *geo = &dev->geo;

	///< note: a key-value namespace has no logical blocks, so nbytes, lba_nbytes, ssw and
	///< tbytes stay zero; mdts is setup in derive-geometry
	geo->type = XNVME_GEO_KV;

	return 0;
}

static int
_controller_geometry(struct xnvme_dev *dev)
{
	struct xnvme_geo *geo = &dev->geo;

	memset(geo, 0, sizeof(*geo));
	geo->type = XNVME_GEO_UNKNOWN;

	///< note: mdts is setup in derive-geometry, so this function does nothing

	return 0;
}

/**
 * Send an Identify command with the given CNS, NSID and CSI to the device of 'ctx', into 'dbuf'
 */
static int
_dev_idfy_cmd(struct xnvme_dev *dev, struct xnvme_cmd_ctx *ctx, uint8_t cns, uint32_t nsid,
	      uint8_t csi, union xnvme_spec_idfy *dbuf)
{
	struct xnvme_cmd_prep_adm_identify_args args = {
		.args_nbytes = sizeof(args),
		.nsid = nsid,
		.cns = cns,
		.csi = csi,
	};
	int err;

	xnvme_dev_get_cmd_ctx(dev, ctx);
	err = xnvme_cmd_prep_adm_identify(&ctx->cmd, &args);
	if (err) {
		return err;
	}

	return xnvme_cmd_pass_admin(ctx, dbuf, sizeof(*dbuf), NULL, 0);
}

/**
 * Helper function mapping the outcome of an identify command to an errno
 *
 * The controller has not answered when the completion carries an NVMe status, or when the
 * backend returns a positive value or -EIO. -EIO is what the Linux NVMe, SPDK and macOS admin
 * backends return for an NVMe status; libvfn sets the completion status, which is checked first.
 * -EIO from a command that never reached the controller, such as an SPDK submission failure or a
 * macOS DriverKit IOConnect failure, is folded in too.
 *
 * @return 0 when the command completed without error. -ENOENT when the controller has not
 * answered. Any other negative errno from the backend is returned as is.
 */
static int
_dev_idfy_cmd_err(struct xnvme_cmd_ctx *ctx, int err)
{
	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		return -ENOENT;
	}
	if ((err < 0) && (err != -EIO)) {
		return err;
	}

	return err ? -ENOENT : 0;
}

/**
 * Helper function retrieving the command set identifier of the namespace from the Namespace
 * Identification Descriptor list (CNS 0x3, NIDT 0x4)
 *
 * A namespace without a descriptor list is classified NVM by the caller. As the list is not
 * answered when the completion carries an NVMe status, or when the backend returns a positive
 * value or -EIO, see _dev_idfy_cmd_err(), a namespace whose identify never reached the controller
 * with -EIO, as an SPDK submission failure or a macOS DriverKit IOConnect failure, is NVM too.
 *
 * @return On success, 0 is returned and 'csi' assigned. -ENOENT is returned when the device has
 * no descriptor list or the list has no CSI descriptor. Any other negative errno, from the
 * command or a buffer operation, is returned as is.
 */
static int
_dev_idfy_ns_desc_csi(struct xnvme_dev *dev, union xnvme_spec_idfy *idfy, uint8_t *csi)
{
	struct xnvme_cmd_ctx ctx;
	uint8_t list[sizeof(union xnvme_spec_idfy)];
	size_t ofz = 0;
	int err;

	err = xnvme_buf_clear(idfy, sizeof(*idfy));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_buf_clear(idfy), err: %d", err);
		return err;
	}

	xnvme_dev_get_cmd_ctx(dev, &ctx);
	ctx.cmd.common.opc = XNVME_SPEC_ADM_OPC_IDFY;
	ctx.cmd.common.nsid = dev->ident.nsid;
	ctx.cmd.idfy.cns = XNVME_SPEC_IDFY_NS_DESC;

	err = xnvme_cmd_pass_admin(&ctx, idfy, sizeof(*idfy), NULL, 0x0);
	err = _dev_idfy_cmd_err(&ctx, err);
	if (err) {
		XNVME_DEBUG("INFO: !idfy(NS_DESC), err: %d", err);
		return err;
	}

	// The buffer may be device memory, which the CPU cannot read
	err = xnvme_buf_memcpy(list, idfy, sizeof(list));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_buf_memcpy(idfy), err: %d", err);
		return err;
	}

	// Walk the list; each descriptor is a 4 byte header followed by 'nidl' bytes. Per NVMe
	// Base 2.3 5.2.13.2.3 a NIDL of zero ends the list, and descriptors of other types are
	// skipped
	while (ofz + 4 <= sizeof(list)) {
		const struct xnvme_spec_ns_desc *desc = (const void *)&list[ofz];

		if ((!desc->nidl) || (ofz + 4 + desc->nidl > sizeof(list))) {
			break;
		}
		if ((desc->nidt == XNVME_SPEC_NS_DESC_NIDT_CSI) && (desc->nidl == 1)) {
			*csi = desc->nid[0];
			return 0;
		}

		ofz += 4 + desc->nidl;
	}

	return -ENOENT;
}

/**
 * Helper function reading and validating the command set specific identify data of 'csi'
 *
 * @return On success, 0 is returned. -ENOENT is returned when the controller has not answered
 * one of the identifies, see _dev_idfy_cmd_err(), and -EINVAL when the data is not valid. Any
 * other negative errno, from the command or a buffer operation, is returned as is.
 */
static int
_dev_idfy_csi_set(struct xnvme_dev *dev, uint8_t csi, union xnvme_spec_idfy *idfy_ns,
		  union xnvme_spec_idfy *idfy_ctrlr)
{
	struct xnvme_spec_znd_idfy_ns *zns = (void *)idfy_ns;
	struct xnvme_spec_fs_idfy_ctrlr *fs_ctrlr = (void *)idfy_ctrlr;
	struct xnvme_spec_fs_idfy_ns *fs_ns = (void *)idfy_ns;
	struct xnvme_cmd_ctx ctx;
	int err;

	err = xnvme_buf_clear(idfy_ctrlr, sizeof(*idfy_ctrlr));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_buf_clear(idfy_ctrlr), err: %d", err);
		return err;
	}

	err = _dev_idfy_cmd(dev, &ctx, XNVME_SPEC_IDFY_CTRLR_IOCS, 0, csi, idfy_ctrlr);
	err = _dev_idfy_cmd_err(&ctx, err);
	if (err) {
		XNVME_DEBUG("INFO: !_dev_idfy_cmd(CTRLR_IOCS, csi: 0x%x), err: %d", csi, err);
		return err;
	}
	if ((csi == XNVME_SPEC_CSI_FS) && !(fs_ctrlr->ac == 0xAC && fs_ctrlr->dc == 0xDC)) {
		XNVME_DEBUG("INFO: invalid values in idfy-ctrlr(CSI_FS)");
		return -EINVAL;
	}

	err = xnvme_buf_clear(idfy_ns, sizeof(*idfy_ns));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_buf_clear(idfy_ns), err: %d", err);
		return err;
	}

	err = _dev_idfy_cmd(dev, &ctx, XNVME_SPEC_IDFY_NS_IOCS, dev->ident.nsid, csi, idfy_ns);
	err = _dev_idfy_cmd_err(&ctx, err);
	if (err) {
		XNVME_DEBUG("INFO: !_dev_idfy_cmd(NS_IOCS, csi: 0x%x), err: %d", csi, err);
		return err;
	}
	if ((csi == XNVME_SPEC_CSI_ZONED) && !zns->lbafe[0].zsze) {
		XNVME_DEBUG("INFO: invalid values in idfy-ns(CSI_ZONED)");
		return -EINVAL;
	}
	if ((csi == XNVME_SPEC_CSI_FS) &&
	    !(fs_ns->nsze && fs_ns->ncap && fs_ns->ac == 0xAC && fs_ns->dc == 0xDC)) {
		XNVME_DEBUG("INFO: invalid values in idfy-ns(CSI_FS)");
		return -EINVAL;
	}

	err = xnvme_buf_memcpy(&dev->idcss.ctrlr, idfy_ctrlr, sizeof(*idfy_ctrlr));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_buf_memcpy(idfy_ctrlr), err: %d", err);
		return err;
	}

	err = xnvme_buf_memcpy(&dev->idcss.ns, idfy_ns, sizeof(*idfy_ns));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_buf_memcpy(idfy_ns), err: %d", err);
		return err;
	}

	return 0;
}

/**
 * Helper function for determining command set identifier
 *
 * The namespace declares its command set in the identification descriptors; without a CSI
 * descriptor it is an NVM namespace. The identify data of a declared non-NVM set must be readable
 * and valid, for NVM it is optional. A CSI other than NVM, zoned, FS and KV is kept as declared
 * without reading its identify data.
 *
 * @return On success, 0 is returned. -EIO is returned when the controller has not answered the
 * identifies of a declared zoned, FS or KV set, and -EINVAL when their data is not valid. Any
 * other negative errno, from a command or a buffer operation, is returned as is.
 */
static int
_dev_idfy_csi(struct xnvme_dev *dev, union xnvme_spec_idfy *idfy_ns,
	      union xnvme_spec_idfy *idfy_ctrlr)
{
	uint8_t csi = XNVME_SPEC_CSI_NVM;
	int err;

	err = _dev_idfy_ns_desc_csi(dev, idfy_ns, &csi);
	if (err && (err != -ENOENT)) {
		return err;
	}

	dev->ident.csi = csi;

	if ((csi != XNVME_SPEC_CSI_NVM) && (csi != XNVME_SPEC_CSI_ZONED) &&
	    (csi != XNVME_SPEC_CSI_FS) && (csi != XNVME_SPEC_CSI_KV)) {
		XNVME_DEBUG("INFO: unknown csi: 0x%x", csi);
		return 0;
	}

	err = _dev_idfy_csi_set(dev, csi, idfy_ns, idfy_ctrlr);
	if (err && (err != -ENOENT)) {
		return err;
	}
	// For NVM the set specific identify data is optional, so an unanswered identify is ignored
	if (err && (csi != XNVME_SPEC_CSI_NVM)) {
		XNVME_DEBUG("FAILED: no idfy for declared csi: 0x%x", csi);
		return -EIO;
	}

	return 0;
}

static int
_dev_idfy(struct xnvme_dev *dev)
{
	struct xnvme_cmd_ctx ctx = {0};
	union xnvme_spec_idfy *idfy_ctrlr = NULL, *idfy_ns = NULL;
	int err;

	dev->attempted_dev_idfy = true;
	//
	// Identify controller and namespace
	//

	// Allocate buffers for idfy
	idfy_ctrlr = xnvme_buf_alloc(dev, sizeof(*idfy_ctrlr));
	if (!idfy_ctrlr) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc()");
		err = -errno;
		goto exit;
	}
	idfy_ns = xnvme_buf_alloc(dev, sizeof(*idfy_ns));
	if (!idfy_ns) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc()");
		err = -errno;
		goto exit;
	}

	// Retrieve idfy-ctrlr
	err = xnvme_buf_clear(idfy_ctrlr, sizeof(*idfy_ctrlr));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_buf_clear(idfy_ctrlr), err: %d", err);
		goto exit;
	}

	err = _dev_idfy_cmd(dev, &ctx, XNVME_SPEC_IDFY_CTRLR, 0, 0, idfy_ctrlr);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		err = err ? err : -EIO;
		XNVME_DEBUG("FAILED: _dev_idfy_cmd(CTRLR), err: %d", err);
		goto exit;
	}

	// Store idfy-ctrlr in device instance
	err = xnvme_buf_memcpy(&dev->id.ctrlr, idfy_ctrlr, sizeof(*idfy_ctrlr));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_buf_memcpy(idfy_ctrlr), err: %d", err);
		goto exit;
	}

	// Store subnqn in device-identifier
	err = xnvme_buf_memcpy(dev->ident.subnqn, dev->id.ctrlr.subnqn, sizeof(dev->ident.subnqn));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_buf_memcpy(subnqn), err: %d", err);
		goto exit;
	}

	if (dev->ident.dtype == XNVME_DEV_TYPE_NVME_CONTROLLER) {
		goto exit;
	}

	// Retrieve idfy-ns
	err = xnvme_buf_clear(idfy_ns, sizeof(*idfy_ns));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_buf_clear(idfy_ns), err: %d", err);
		goto exit;
	}

	err = _dev_idfy_cmd(dev, &ctx, XNVME_SPEC_IDFY_NS, dev->ident.nsid, 0, idfy_ns);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		err = err ? err : -EIO;
		XNVME_DEBUG("FAILED: _dev_idfy_cmd(NS), err: %d", err);
		goto exit;
	}

	// Store idfy-ns in device instance
	err = xnvme_buf_memcpy(&dev->id.ns, idfy_ns, sizeof(*idfy_ns));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_buf_memcpy(idfy_ns), err: %d", err);
		goto exit;
	}

	err = _dev_idfy_csi(dev, idfy_ns, idfy_ctrlr);
	if (err) {
		XNVME_DEBUG("FAILED: _dev_idfy_csi(), err: %d", err);
	}

exit:
	xnvme_buf_free(dev, idfy_ctrlr);
	xnvme_buf_free(dev, idfy_ns);

	return err;
}

int
xnvme_dev_derive_geo(struct xnvme_dev *dev)
{
	struct xnvme_geo *geo = &dev->geo;
	int err;

	dev->attempted_derive_geo = true;

	err = _dev_idfy(dev);
	if (err) {
		XNVME_DEBUG("FAILED: identifying device; skipping derive-geo");
		return err;
	}

	switch (dev->ident.dtype) {
	case XNVME_DEV_TYPE_NVME_CONTROLLER:
		return _controller_geometry(dev);

	case XNVME_DEV_TYPE_FS_FILE:
		return _fs_geometry(dev);

	case XNVME_DEV_TYPE_RAMDISK:
	case XNVME_DEV_TYPE_BLOCK_DEVICE:
	case XNVME_DEV_TYPE_NVME_NAMESPACE:
		if (dev->ident.csi == XNVME_SPEC_CSI_FS) {
			if (_conventional_geometry(dev)) {
				XNVME_DEBUG("FAILED: _conventional_geometry");
				return -EINVAL;
			}
			break;
		}

		switch (dev->ident.csi) {
		case XNVME_SPEC_CSI_ZONED:
			if (_zoned_geometry(dev)) {
				XNVME_DEBUG("FAILED: _zoned_geometry");
				return -EINVAL;
			}
			break;

		case XNVME_SPEC_CSI_NVM:
			if (_conventional_geometry(dev)) {
				XNVME_DEBUG("FAILED: _conventional_geometry");
				return -EINVAL;
			}
			break;

		case XNVME_SPEC_CSI_KV:
			if (_kvs_geometry(dev)) {
				XNVME_DEBUG("FAILED: _kvs_geometry");
				return -EINVAL;
			}
			break;

		default:
			XNVME_DEBUG("FAILED: unhandled csi: 0x%x", dev->ident.csi);
			return -ENOSYS;
			break;
		}
		break;
	default:
		XNVME_DEBUG("FAILED: unhandled dtype: 0x%x", dev->ident.dtype);
		return -ENOSYS;
	}

	geo->tbytes =
		(unsigned long)(geo->npugrp) * geo->npunit * geo->nzone * geo->nsect * geo->nbytes;

	/* Derive the sector-shift-width for LBA mapping */
	geo->ssw = XNVME_ILOG2(dev->geo.nbytes);

	//
	// If the controller reports that MDTS is unbounded, that is, it can be
	// infinitely large then we cap it here to something that just might
	// exists in finite physical space and time withing this realm of
	// what we perceive as reality in our universe. Note, that this might
	// change with quantum devices...
	//
	// TODO: read the mpsmin register...
	{
		size_t mpsmin = 0;
		size_t mdts = dev->id.ctrlr.mdts;

		if (!mdts) {
			geo->mdts_nbytes = 1 << 20;
		} else {
			geo->mdts_nbytes = 1 << (mdts + 12 + mpsmin);
		}

		// Fabrics work-around
		if ((geo->mdts_nbytes > (16 * 1024)) && (dev->opts.spdk_fabrics)) {
			geo->mdts_nbytes = 16 * 1024;
		}
	}

	// TODO: add zamdts

#ifdef XNVME_PLATFORM_LINUX_ENABLED
	/** Cap MDTS to kernel queue max_segments for Linux NVMe ioctl passthrough **/
	if (!strncmp(dev->be.sync.id, "nvme", 4)) {
		uint32_t max_seg = 32;
		uint32_t phys_blk = 512;
		uint32_t max_xfer;
		int err;

		err = _linux_queue_attr(dev, "max_segments", &max_seg);
		if (err) {
			XNVME_DEBUG("FAILED: reading max_segments; err: %d; using default: %u",
				    err, max_seg);
		}
		err = _linux_queue_attr(dev, "physical_block_size", &phys_blk);
		if (err) {
			XNVME_DEBUG("FAILED: reading physical_block_size; err: %d; using default: "
				    "%u",
				    err, phys_blk);
		}

		max_xfer = (max_seg - 1) * phys_blk;
		if (dev->geo.mdts_nbytes > max_xfer) {
			dev->geo.mdts_nbytes = max_xfer;
		}
	}
#endif

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

	wrtn += fprintf(stream, "  xnvme_opts:\n");
	wrtn += fprintf(stream, "    be: '%s'\n", dev->opts.be);
	wrtn += fprintf(stream, "    mem: '%s'\n", dev->opts.mem);
	wrtn += fprintf(stream, "    dev: '%s'\n", dev->opts.dev);
	wrtn += fprintf(stream, "    admin: '%s'\n", dev->opts.admin);
	wrtn += fprintf(stream, "    sync: '%s'\n", dev->opts.sync);
	wrtn += fprintf(stream, "    async: '%s'\n", dev->opts.async);

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
	if ((!dev->attempted_derive_geo) && xnvme_dev_derive_geo((struct xnvme_dev *)dev)) {
		XNVME_DEBUG("FAILED: xnvme_dev_derive_geo()");
	}

	return &dev->geo;
}

const struct xnvme_spec_idfy_ctrlr *
xnvme_dev_get_ctrlr(const struct xnvme_dev *dev)
{
	if ((!dev->attempted_dev_idfy) && _dev_idfy((struct xnvme_dev *)dev)) {
		XNVME_DEBUG("FAILED: open() : _dev_idfy()");
	}
	return &dev->id.ctrlr;
}

const struct xnvme_spec_idfy_ctrlr *
xnvme_dev_get_ctrlr_css(const struct xnvme_dev *dev)
{
	if ((!dev->attempted_dev_idfy) && _dev_idfy((struct xnvme_dev *)dev)) {
		XNVME_DEBUG("FAILED: open() : _dev_idfy()");
	}
	return &dev->idcss.ctrlr;
}

const struct xnvme_spec_idfy_ns *
xnvme_dev_get_ns(const struct xnvme_dev *dev)
{
	if ((!dev->attempted_dev_idfy) && _dev_idfy((struct xnvme_dev *)dev)) {
		XNVME_DEBUG("FAILED: open() : _dev_idfy()");
	}
	return &dev->id.ns;
}

const struct xnvme_spec_idfy_ns *
xnvme_dev_get_ns_css(const struct xnvme_dev *dev)
{
	if ((!dev->attempted_dev_idfy) && _dev_idfy((struct xnvme_dev *)dev)) {
		XNVME_DEBUG("FAILED: open() : _dev_idfy()");
	}
	return &dev->idcss.ns;
}

uint32_t
xnvme_dev_get_nsid(const struct xnvme_dev *dev)
{
	if ((!dev->attempted_dev_idfy) && _dev_idfy((struct xnvme_dev *)dev)) {
		XNVME_DEBUG("FAILED: open() : _dev_idfy()");
	}
	return dev->ident.nsid;
}

uint8_t
xnvme_dev_get_csi(const struct xnvme_dev *dev)
{
	if ((!dev->attempted_dev_idfy) && _dev_idfy((struct xnvme_dev *)dev)) {
		XNVME_DEBUG("FAILED: open() : _dev_idfy()");
	}
	return dev->ident.csi;
}

const struct xnvme_ident *
xnvme_dev_get_ident(const struct xnvme_dev *dev)
{
	if ((!dev->attempted_dev_idfy) && _dev_idfy((struct xnvme_dev *)dev)) {
		XNVME_DEBUG("FAILED: open() : _dev_idfy()");
	}
	return &dev->ident;
}

const struct xnvme_opts *
xnvme_dev_get_opts(const struct xnvme_dev *dev)
{
	return &dev->opts;
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

int
xnvme_dev_open_uri(const char *dev_uri, struct xnvme_opts *opts, struct xnvme_dev **dev)
{
	struct xnvme_opts opts_default = {0};
	int err;

	if (!dev) {
		return -EINVAL;
	}
	*dev = NULL;
	if (!dev_uri) {
		return -EINVAL;
	}

	xnvme_opts_set_defaults(&opts_default);

	if (!opts) { ///< Set defaults when none are given
		opts = &opts_default;
	}
	if (!(opts->rdonly | opts->wronly | opts->rdwr)) { ///< Set a default open-mode
		opts->rdwr = opts_default.rdwr;
	}
	if (opts->create && !opts->create_mode) { ///< Set a default umask/mode_t/create-mode
		opts->create_mode = opts_default.create_mode;
	}

	err = xnvme_dev_alloc(dev);
	if (err) {
		XNVME_DEBUG("FAILED: failed xnvme_dev_alloc()");
		return err;
	}

	err = xnvme_ident_from_uri(dev_uri, &(*dev)->ident);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_ident_from_uri(), err: %d", err);
		goto failed;
	}

	xnvme_lock();
	err = g_xnvme_platform->dev_open(*dev, opts);
	xnvme_unlock();
	if (err) {
		XNVME_DEBUG("FAILED: failed opening uri: %s", dev_uri);
		goto failed;
	}

	return 0;

failed:
	free(*dev);
	*dev = NULL;

	return err;
}

void
xnvme_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	xnvme_lock();
	{
		void *ctrlr = xnvme_be_state(dev)->ctrlr;

		dev->be.dev.dev_close(dev);

		if (dev->be.dev.ctrlr_term && ctrlr) {
			xnvme_be_cref_put(ctrlr);
		}
	}
	xnvme_unlock();

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
