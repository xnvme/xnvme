// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <libxnvme.h>

void
xnvme_req_pr(const struct xnvme_req *req, int XNVME_UNUSED(opts))
{
	printf("xnvme_req: ");

	if (!req) {
		printf("~\n");
		return;
	}

	printf("{cdw0: 0x%x, sc: 0x%x, sct: 0x%x}\n", req->cpl.cdw0,
	       req->cpl.status.sc, req->cpl.status.sct);
}

void
xnvme_req_clear(struct xnvme_req *req)
{
	memset(req, 0x0, sizeof(*req));
}
