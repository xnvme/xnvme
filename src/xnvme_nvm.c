// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <libxnvme_nvm.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

int
xnvme_adm_format(struct xnvme_dev *dev, uint32_t nsid, uint8_t lbaf, uint8_t zf, uint8_t mset,
		 uint8_t ses, uint8_t pi, uint8_t pil, struct xnvme_cmd_ctx *ctx)
{
	struct xnvme_spec_cmd cmd = {0 };

	cmd.common.opcode = XNVME_SPEC_NVM_OPC_FMT;
	cmd.common.nsid = nsid;
	cmd.format.lbaf = lbaf;
	cmd.format.zf = zf;
	cmd.format.mset = mset;
	cmd.format.pi = pi;
	cmd.format.pil = pil;
	cmd.format.ses = ses;

	return xnvme_cmd_pass_admin(dev, &cmd, NULL, 0, NULL, 0, 0x0, ctx);
}

int
xnvme_nvm_sanitize(struct xnvme_dev *dev, uint8_t sanact, uint8_t ause, uint32_t ovrpat,
		   uint8_t owpass, uint8_t oipbp, uint8_t nodas, struct xnvme_cmd_ctx *ctx)
{
	struct xnvme_spec_cmd cmd = { 0 };

	cmd.common.opcode = XNVME_SPEC_NVM_OPC_SANITIZE;
	cmd.sanitize.sanact = sanact;
	cmd.sanitize.ause = ause;
	cmd.sanitize.owpass = owpass;
	cmd.sanitize.oipbp = oipbp;
	cmd.sanitize.nodas = nodas;
	cmd.sanitize.ause = ause;
	cmd.sanitize.ovrpat = ovrpat;

	return xnvme_cmd_pass_admin(dev, &cmd, NULL, 0, NULL, 0, 0x0, ctx);
}

int
xnvme_nvm_read(struct xnvme_dev *dev, uint32_t nsid, uint64_t slba, uint16_t nlb, void *dbuf,
	       void *mbuf, int opts, struct xnvme_cmd_ctx *ctx)
{
	size_t dbuf_nbytes = dbuf ? dev->geo.lba_nbytes * (nlb + 1) : 0;
	size_t mbuf_nbytes = mbuf ? dev->geo.nbytes_oob * (nlb + 1) : 0;
	struct xnvme_spec_cmd cmd = { 0 };

	// TODO: consider returning -EINVAL when mbuf is provided and namespace
	// have extended-lba in effect

	cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
	cmd.common.nsid = nsid;
	cmd.nvm.slba = slba;
	cmd.nvm.nlb = nlb;

	return xnvme_cmd_pass(dev, &cmd, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes,
			      opts, ctx);
}

int
xnvme_nvm_write(struct xnvme_dev *dev, uint32_t nsid, uint64_t slba, uint16_t nlb,
		const void *dbuf,
		const void *mbuf, int opts, struct xnvme_cmd_ctx *ctx)
{
	void *cdbuf = (void *)dbuf;
	void *cmbuf = (void *)mbuf;

	size_t dbuf_nbytes = cdbuf ? dev->geo.lba_nbytes * (nlb + 1) : 0;
	size_t mbuf_nbytes = cmbuf ? dev->geo.nbytes_oob * (nlb + 1) : 0;
	struct xnvme_spec_cmd cmd = { 0 };

	// TODO: consider returning -EINVAL when mbuf is provided and namespace
	// have extended-lba in effect

	cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
	cmd.common.nsid = nsid;
	cmd.nvm.slba = slba;
	cmd.nvm.nlb = nlb;

	return xnvme_cmd_pass(dev, &cmd, cdbuf, dbuf_nbytes, cmbuf, mbuf_nbytes,
			      opts, ctx);
}

int
xnvme_nvm_write_uncorrectable(struct xnvme_dev *dev, uint32_t nsid, uint64_t slba, uint16_t nlb,
			      int opts, struct xnvme_cmd_ctx *ret)
{
	struct xnvme_spec_cmd cmd = {0};

	cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE_UNCORRECTABLE;
	cmd.common.nsid = nsid;
	cmd.nvm.slba = slba;
	cmd.nvm.nlb = nlb;

	return xnvme_cmd_pass(dev, &cmd, NULL, 0, NULL, 0, opts, ret);
}

int
xnvme_nvm_write_zeroes(struct xnvme_dev *dev, uint32_t nsid, uint64_t slba, uint16_t nlb, int opts,
		       struct xnvme_cmd_ctx *ret)
{
	struct xnvme_spec_cmd cmd = { 0 };

	cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE_ZEROES;
	cmd.common.nsid = nsid;
	cmd.write_zeroes.slba = slba;
	cmd.write_zeroes.nlb = nlb;

	return xnvme_cmd_pass(dev, &cmd, NULL, 0, NULL, 0, opts, ret);
}

int
xnvme_nvm_scopy(struct xnvme_dev *dev, uint32_t nsid, uint64_t sdlba,
		struct xnvme_spec_nvm_scopy_fmt_zero *ranges, uint8_t nr,
		enum xnvme_nvm_scopy_fmt copy_fmt, int opts, struct xnvme_cmd_ctx *ret)
{
	size_t ranges_nbytes = 0;

	if (copy_fmt & XNVME_NVM_SCOPY_FMT_ZERO) {
		ranges_nbytes = (nr + 1) * sizeof(*ranges);
	}
	if (copy_fmt & XNVME_NVM_SCOPY_FMT_SRCLEN) {
		ranges_nbytes = (nr + 1) * sizeof(struct xnvme_spec_nvm_cmd_scopy_fmt_srclen);
	}

	struct xnvme_spec_cmd cmd = { 0 };

	cmd.common.opcode = XNVME_SPEC_NVM_OPC_SCOPY;
	cmd.common.nsid = nsid;
	cmd.scopy.sdlba = sdlba;
	cmd.scopy.nr = nr;

	// TODO: consider the remaining command-fields

	return xnvme_cmd_pass(dev, &cmd, ranges, ranges_nbytes, NULL, 0, opts, ret);
}

