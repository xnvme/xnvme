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
xnvme_sgl_setup(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd, void *data, void *meta, int opts)
{
	struct xnvme_sgl *sgl = data;
	uint64_t phys;

	cmd->common.psdt = XNVME_SPEC_PSDT_SGL_MPTR_CONTIGUOUS;

	if (sgl->ndescr == 1) {
		cmd->common.dptr.sgl = sgl->descriptors[0];
	} else {
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
	       struct xnvme_req *req)
{
	const int cmd_opts = opts & XNVME_CMD_MASK;

	if ((cmd_opts & XNVME_CMD_MASK_UPLD) && dbuf) {
		xnvme_sgl_setup(dev, cmd, dbuf, mbuf, opts);
	}

	switch (cmd_opts & XNVME_CMD_MASK_IOMD) {
	case XNVME_CMD_ASYNC:
		return dev->be.async.cmd_io(dev, cmd, dbuf, dbuf_nbytes, mbuf,
					    mbuf_nbytes, opts, req);

	case XNVME_CMD_SYNC:
		return dev->be.sync.cmd_io(dev, cmd, dbuf, dbuf_nbytes, mbuf,
					   mbuf_nbytes, opts, req);

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
		xnvme_sgl_setup(dev, cmd, dbuf, mbuf, opts);
	}

	return dev->be.sync.cmd_admin(dev, cmd, dbuf, dbuf_nbytes, mbuf,
				      mbuf_nbytes, opts, ret);
}

