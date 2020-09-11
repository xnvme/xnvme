// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_sgl.h>

/**
 * Calling this requires that opts at least has `XNVME_CMD_SGL_DATA`
 */
static inline void
cmd_setup_sgl(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd, void *data,
	      void *meta, int opts)
{
	struct xnvme_sgl *sgl = data;
	uint64_t phys;

	cmd->common.psdt = XNVME_SPEC_PSDT_SGL_MPTR_CONTIGUOUS;

	if (sgl->ndescr == 1) {
		cmd->common.dptr.sgl = sgl->descriptors[0];
	} else {
		uint64_t phys;
		xnvme_buf_vtophys(dev, sgl->descriptors, &phys);

		cmd->common.dptr.sgl.unkeyed.type = XNVME_SPEC_SGL_DESCR_TYPE_LAST_SEGMENT;
		cmd->common.dptr.sgl.unkeyed.len = sgl->ndescr * sizeof(struct xnvme_spec_sgl_descriptor);
		cmd->common.dptr.sgl.addr = phys;
	}

	if ((opts & XNVME_CMD_UPLD_SGLM) && meta) {
		sgl = meta;

		xnvme_buf_vtophys(dev, sgl->descriptors, &phys);

		cmd->common.psdt = XNVME_SPEC_PSDT_SGL_MPTR_SGL;

		if (sgl->ndescr == 1) {
			cmd->common.mptr = phys;
		} else {
			sgl->indirect->unkeyed.type = XNVME_SPEC_SGL_DESCR_TYPE_LAST_SEGMENT;
			sgl->indirect->unkeyed.len = sgl->ndescr * sizeof(struct xnvme_spec_sgl_descriptor);
			sgl->indirect->addr = phys;

			xnvme_buf_vtophys(dev, sgl->indirect, &phys);
			cmd->common.mptr = phys;
		}
	}
}

int
xnvme_cmd_pass(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd, void *dbuf,
	       size_t dbuf_nbytes, void *mbuf, size_t mbuf_nbytes, int opts,
	       struct xnvme_req *ret)
{
	const int cmd_opts = opts & XNVME_CMD_MASK;

	if ((cmd_opts & XNVME_CMD_MASK_UPLD) && dbuf) {
		cmd_setup_sgl(dev, cmd, dbuf, mbuf, opts);
	}

	switch (cmd_opts & XNVME_CMD_MASK_IOMD) {
	case XNVME_CMD_ASYNC:
		return dev->be.async.cmd_io(dev, cmd, dbuf, dbuf_nbytes, mbuf,
					    mbuf_nbytes, opts, ret);

	case XNVME_CMD_SYNC:
		return dev->be.sync.cmd_io(dev, cmd, dbuf, dbuf_nbytes, mbuf,
					   mbuf_nbytes, opts, ret);

	default:
		XNVME_DEBUG("FAILED: command-mode not provided");
		return -EINVAL;
	}
}

int
xnvme_cmd_pass_admin(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
		     void *dbuf, size_t dbuf_nbytes, void *mbuf,
		     size_t mbuf_nbytes, int opts, struct xnvme_req *ret)
{
	if (XNVME_CMD_ASYNC & opts) {
		XNVME_DEBUG("FAILED: Admin commands are always sync.");
		return -EINVAL;
	}
	if ((opts & XNVME_CMD_MASK_UPLD) && dbuf) {
		cmd_setup_sgl(dev, cmd, dbuf, mbuf, opts);
	}

	return dev->be.sync.cmd_admin(dev, cmd, dbuf, dbuf_nbytes, mbuf,
				      mbuf_nbytes, opts, ret);
}

int
xnvme_cmd_log(struct xnvme_dev *dev, uint8_t lid, uint8_t lsp,
	      uint64_t lpo_nbytes, uint32_t nsid, uint8_t rae, void *dbuf,
	      uint32_t dbuf_nbytes, struct xnvme_req *ret)
{
	struct xnvme_spec_cmd cmd = {0 };
	uint32_t numdw;

	if (!dbuf_nbytes) {
		XNVME_DEBUG("FAILED: !dbuf_nbytes");
		return -EINVAL;
	}
	if (lpo_nbytes & 0x3) {
		XNVME_DEBUG("FAILED: lpo_nbytes is not byte aligned");
		return -EINVAL;
	}

	// NOTE: yeah... well... we do not want the caller to rely on the
	// function to clear out the buffer... however... in case they forgot..
	// then it is done here
	memset(dbuf, 0, dbuf_nbytes);

	numdw = dbuf_nbytes / sizeof(uint32_t) - 1u;

	cmd.common.opcode = XNVME_SPEC_OPC_LOG;
	cmd.common.nsid = nsid;

	cmd.log.lid = lid;
	cmd.log.lsp = lsp;
	cmd.log.rae = rae;
	cmd.log.numdl = numdw & 0xFFFFu;
	cmd.log.numdu = (numdw >> 16) & 0xFFFFu;
	cmd.log.lpou = (uint32_t)(lpo_nbytes >> 32);
	cmd.log.lpol = (uint32_t)lpo_nbytes & 0xfffffff;

	// TODO: should we check ctrlr->lpa.edlp for ext. buf and lpo support?
	// TODO: add support for uuid?

	return xnvme_cmd_pass_admin(dev, &cmd, dbuf, dbuf_nbytes, NULL, 0x0,
				    0x0, ret);
}

int
xnvme_cmd_idfy(struct xnvme_dev *dev, uint8_t cns, uint16_t cntid, uint8_t nsid,
	       uint16_t nvmsetid, uint8_t uuid, struct xnvme_spec_idfy *dbuf,
	       struct xnvme_req *ret)
{
	const size_t dbuf_nbytes = sizeof(*dbuf);
	struct xnvme_spec_cmd cmd = { 0 };

	cmd.common.opcode = XNVME_SPEC_OPC_IDFY;
	cmd.common.nsid = nsid;
	cmd.idfy.cns = cns;
	cmd.idfy.cntid = cntid;
	cmd.idfy.nvmsetid = nvmsetid;
	cmd.idfy.uuid = uuid;

	return xnvme_cmd_pass_admin(dev, &cmd, dbuf, dbuf_nbytes, NULL, 0x0,
				    0x0, ret);
}

int
xnvme_cmd_idfy_ctrlr(struct xnvme_dev *dev, struct xnvme_spec_idfy *dbuf,
		     struct xnvme_req *req)
{
	const size_t dbuf_nbytes = sizeof(*dbuf);
	struct xnvme_spec_cmd cmd = { 0 };

	cmd.common.opcode = XNVME_SPEC_OPC_IDFY;
	cmd.idfy.cns = XNVME_SPEC_IDFY_CTRLR;

	return xnvme_cmd_pass_admin(dev, &cmd, dbuf, dbuf_nbytes, NULL, 0x0,
				    0x0, req);
}

int
xnvme_cmd_idfy_ctrlr_csi(struct xnvme_dev *dev, uint8_t csi,
			 struct xnvme_spec_idfy *dbuf, struct xnvme_req *req)
{
	const size_t dbuf_nbytes = sizeof(*dbuf);
	struct xnvme_spec_cmd cmd = { 0 };

	cmd.common.opcode = XNVME_SPEC_OPC_IDFY;
	cmd.idfy.cns = XNVME_SPEC_IDFY_CTRLR_IOCS;
	cmd.idfy.csi = csi;

	return xnvme_cmd_pass_admin(dev, &cmd, dbuf, dbuf_nbytes, NULL, 0x0,
				    0x0, req);
}

int
xnvme_cmd_idfy_ns(struct xnvme_dev *dev, uint32_t nsid,
		  struct xnvme_spec_idfy *dbuf, struct xnvme_req *req)
{
	const size_t dbuf_nbytes = sizeof(*dbuf);
	struct xnvme_spec_cmd cmd = { 0 };

	cmd.common.opcode = XNVME_SPEC_OPC_IDFY;
	cmd.common.nsid = (nsid) ? nsid : xnvme_dev_get_nsid(dev);
	cmd.idfy.cns = XNVME_SPEC_IDFY_NS;
	cmd.idfy.csi = XNVME_SPEC_CSI_NOCHECK;

	return xnvme_cmd_pass_admin(dev, &cmd, dbuf, dbuf_nbytes, NULL,
				    0x0, 0x0, req);
}

int
xnvme_cmd_idfy_ns_csi(struct xnvme_dev *dev, uint32_t nsid, uint8_t csi,
		      struct xnvme_spec_idfy *dbuf, struct xnvme_req *req)
{
	const size_t dbuf_nbytes = sizeof(*dbuf);
	struct xnvme_spec_cmd cmd = { 0 };

	cmd.common.opcode = XNVME_SPEC_OPC_IDFY;
	cmd.common.nsid = (nsid) ? nsid : xnvme_dev_get_nsid(dev);
	cmd.idfy.cns = XNVME_SPEC_IDFY_NS_IOCS;
	cmd.idfy.csi = csi;

	return xnvme_cmd_pass_admin(dev, &cmd, dbuf, dbuf_nbytes, NULL,
				    0x0, 0x0, req);
}

int
xnvme_cmd_gfeat(struct xnvme_dev *dev, uint32_t nsid, uint8_t fid, uint8_t sel,
		void *dbuf, size_t dbuf_nbytes, struct xnvme_req *ret)
{
	struct xnvme_spec_cmd cmd = { 0 };

	cmd.common.opcode = XNVME_SPEC_OPC_GFEAT;
	cmd.common.nsid = nsid;
	cmd.gfeat.fid = fid;
	cmd.gfeat.sel = sel;

	// TODO: cdw14/uuid?

	return xnvme_cmd_pass_admin(dev, &cmd, dbuf, dbuf_nbytes, NULL, 0x0,
				    0x0, ret);
}

int
xnvme_cmd_sfeat(struct xnvme_dev *dev, uint32_t nsid, uint8_t fid,
		uint32_t feat, uint8_t save, const void *dbuf,
		size_t dbuf_nbytes, struct xnvme_req *ret)
{
	struct xnvme_spec_cmd cmd = { 0 };

	cmd.common.opcode = XNVME_SPEC_OPC_SFEAT;
	cmd.common.nsid = nsid;
	cmd.sfeat.fid = fid;
	cmd.sfeat.feat.val = feat;
	cmd.sfeat.save = save;

	return xnvme_cmd_pass_admin(dev, &cmd, (void *)dbuf, dbuf_nbytes,
				    NULL, 0x0, 0x0, ret);
}

int
xnvme_cmd_format(struct xnvme_dev *dev, uint32_t nsid, uint8_t lbaf, uint8_t zf,
		 uint8_t mset, uint8_t ses, uint8_t pi, uint8_t pil,
		 struct xnvme_req *ret)
{
	struct xnvme_spec_cmd cmd = {0 };

	cmd.common.opcode = XNVME_SPEC_OPC_FMT_NVM;
	cmd.common.nsid = nsid;
	cmd.format.lbaf = lbaf;
	cmd.format.zf = zf;
	cmd.format.mset = mset;
	cmd.format.pi = pi;
	cmd.format.pil = pil;
	cmd.format.ses = ses;

	return xnvme_cmd_pass_admin(dev, &cmd, NULL, 0, NULL, 0, 0x0, ret);
}

int
xnvme_cmd_sanitize(struct xnvme_dev *dev, uint8_t sanact, uint8_t ause,
		   uint32_t ovrpat, uint8_t owpass, uint8_t oipbp,
		   uint8_t nodas, struct xnvme_req *ret)
{
	struct xnvme_spec_cmd cmd = { 0 };

	cmd.common.opcode = XNVME_SPEC_OPC_SANITIZE;
	cmd.sanitize.sanact = sanact;
	cmd.sanitize.ause = ause;
	cmd.sanitize.owpass = owpass;
	cmd.sanitize.oipbp = oipbp;
	cmd.sanitize.nodas = nodas;
	cmd.sanitize.ause = ause;
	cmd.sanitize.ovrpat = ovrpat;

	return xnvme_cmd_pass_admin(dev, &cmd, NULL, 0, NULL, 0, 0x0, ret);
}

int
xnvme_cmd_read(struct xnvme_dev *dev, uint32_t nsid, uint64_t slba,
	       uint16_t nlb, void *dbuf, void *mbuf, int opts,
	       struct xnvme_req *ret)
{
	size_t dbuf_nbytes = dbuf ? dev->geo.lba_nbytes * (nlb + 1) : 0;
	size_t mbuf_nbytes = mbuf ? dev->geo.nbytes_oob * (nlb + 1) : 0;
	struct xnvme_spec_cmd cmd = { 0 };

	// TODO: consider returning -EINVAL when mbuf is provided and namespace
	// have extended-lba in effect

	cmd.common.opcode = XNVME_SPEC_OPC_READ;
	cmd.common.nsid = nsid;
	cmd.lblk.slba = slba;
	cmd.lblk.nlb = nlb;

	return xnvme_cmd_pass(dev, &cmd, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes,
			      opts, ret);
}

int
xnvme_cmd_write(struct xnvme_dev *dev, uint32_t nsid, uint64_t slba,
		uint16_t nlb, const void *dbuf, const void *mbuf, int opts,
		struct xnvme_req *ret)
{
	void *cdbuf = (void *)dbuf;
	void *cmbuf = (void *)mbuf;

	size_t dbuf_nbytes = cdbuf ? dev->geo.lba_nbytes * (nlb + 1) : 0;
	size_t mbuf_nbytes = cmbuf ? dev->geo.nbytes_oob * (nlb + 1) : 0;
	struct xnvme_spec_cmd cmd = { 0 };

	// TODO: consider returning -EINVAL when mbuf is provided and namespace
	// have extended-lba in effect

	cmd.common.opcode = XNVME_SPEC_OPC_WRITE;
	cmd.common.nsid = nsid;
	cmd.lblk.slba = slba;
	cmd.lblk.nlb = nlb;

	return xnvme_cmd_pass(dev, &cmd, cdbuf, dbuf_nbytes, cmbuf, mbuf_nbytes,
			      opts, ret);
}
