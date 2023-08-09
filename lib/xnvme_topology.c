// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <errno.h>
#include <xnvme_spec.h>
#include <xnvme_cmd.h>

int
xnvme_subsystem_reset(struct xnvme_dev *dev)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	int err;

	ctx.cmd.common.opcode = XNVME_SPEC_PSEUDO_OPC_SUBSYSTEM_RESET;

	err = xnvme_cmd_pass_pseudo(&ctx, NULL, 0x0, NULL, 0x0);
	if (err) {
		return err;
	}
	if (xnvme_cmd_ctx_cpl_status(&ctx)) {
		return -ctx.cpl.status.sc;
	}

	return 0;
}

int
xnvme_controller_reset(struct xnvme_dev *dev)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	int err;

	ctx.cmd.common.opcode = XNVME_SPEC_PSEUDO_OPC_CONTROLLER_RESET;

	err = xnvme_cmd_pass_pseudo(&ctx, NULL, 0x0, NULL, 0x0);
	if (err) {
		return err;
	}
	if (xnvme_cmd_ctx_cpl_status(&ctx)) {
		return -ctx.cpl.status.sc;
	}

	return 0;
}

int
xnvme_namespace_rescan(struct xnvme_dev *dev)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	int err;

	ctx.cmd.common.opcode = XNVME_SPEC_PSEUDO_OPC_NAMESPACE_RESCAN;

	err = xnvme_cmd_pass_pseudo(&ctx, NULL, 0x0, NULL, 0x0);
	if (err) {
		return err;
	}
	if (xnvme_cmd_ctx_cpl_status(&ctx)) {
		return -ctx.cpl.status.sc;
	}

	return 0;

	return -ENOSYS;
}
