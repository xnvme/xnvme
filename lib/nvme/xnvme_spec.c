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
	struct xnvme_spec_ns_desc *desc = dbuf;

	if (dbuf_nbytes < sizeof(*desc)) {
		return -EINVAL;
	}

	memset(dbuf, 0, dbuf_nbytes);
	desc->nidt = XNVME_SPEC_NS_DESC_NIDT_CSI;
	desc->nidl = 1;
	desc->nid[0] = csi;

	return 0;
}
