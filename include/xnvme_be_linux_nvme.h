// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_LINUX_NVME_H
#define __INTERNAL_XNVME_BE_LINUX_NVME_H
#include <xnvme_dev.h>

int
xnvme_be_linux_nvme_cmd_io(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
			   void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t mbuf_nbytes, int XNVME_UNUSED(opts),
			   struct xnvme_cmd_ctx *req);

int
xnvme_be_linux_nvme_cmd_admin(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
			      void *dbuf, size_t dbuf_nbytes, void *mbuf,
			      size_t mbuf_nbytes, int opts,
			      struct xnvme_cmd_ctx *req);

int
xnvme_be_linux_nvme_dev_nsid(struct xnvme_dev *dev);

#endif /* __INTERNAL_XNVME_BE_LINUX_NVME_H */
