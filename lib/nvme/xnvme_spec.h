// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_SPEC_H
#define __INTERNAL_XNVME_SPEC_H

enum xnvme_spec_pseudo_opc {
	XNVME_SPEC_PSEUDO_OPC_SHOW_REGS        = 0x02,
	XNVME_SPEC_PSEUDO_OPC_CONTROLLER_RESET = 0x10,
	XNVME_SPEC_PSEUDO_OPC_SUBSYSTEM_RESET  = 0x11,
	XNVME_SPEC_PSEUDO_OPC_NAMESPACE_RESCAN = 0x12,
};

#endif /* __INTERNAL_XNVME_SPEC_H */
