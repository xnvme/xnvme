// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_SPEC_H
#define __INTERNAL_XNVME_SPEC_H

#include <libxnvme.h>

enum xnvme_spec_pseudo_opc {
	XNVME_SPEC_PSEUDO_OPC_SHOW_REGS        = 0x02,
	XNVME_SPEC_PSEUDO_OPC_CONTROLLER_RESET = 0x10,
	XNVME_SPEC_PSEUDO_OPC_SUBSYSTEM_RESET  = 0x11,
	XNVME_SPEC_PSEUDO_OPC_NAMESPACE_RESCAN = 0x12,
};

/**
 * Fill 'dbuf' with a Namespace Identification Descriptor list declaring the command set 'csi'
 *
 * This is the answer to Identify CNS 0x3 of a backend emulating a command set: one descriptor of
 * type NIDT 0x4 followed by zeroes, which terminate the list.
 *
 * @param dbuf Pointer to the data buffer to fill
 * @param dbuf_nbytes Size of 'dbuf' in bytes
 * @param csi Command set identifier to declare
 *
 * @return On success, 0 is returned. On error, negative EINVAL is returned when 'dbuf' is too
 * small for a descriptor.
 */
int
xnvme_spec_idfy_ns_desc_fill(void *dbuf, size_t dbuf_nbytes, uint8_t csi);

#endif /* __INTERNAL_XNVME_SPEC_H */
