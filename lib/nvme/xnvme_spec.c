// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_spec.h>

int
xnvme_spec_idfy_ns_desc_fill(void *dbuf, size_t dbuf_nbytes, uint8_t csi)
{
	struct xnvme_spec_base_namespace_identification_descriptor *desc = dbuf;

	// The header and the one byte of a Command Set Identifier, Base 2.4 Figure 342
	if (dbuf_nbytes < sizeof(*desc) + 1) {
		return -EINVAL;
	}

	memset(dbuf, 0, dbuf_nbytes);
	desc->nidt =
		XNVME_SPEC_BASE_NAMESPACE_IDENTIFICATION_DESCRIPTOR_NIDT_COMMAND_SET_IDENTIFIER;
	desc->nidl = 1;
	((uint8_t *)dbuf)[sizeof(*desc)] = csi;

	return 0;
}
