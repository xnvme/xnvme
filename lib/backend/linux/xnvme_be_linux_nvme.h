// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_LINUX_NVME_H
#define __INTERNAL_XNVME_BE_LINUX_NVME_H
#include <xnvme_dev.h>

int
xnvme_be_linux_nvme_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t mbuf_nbytes);

int
xnvme_be_linux_nvme_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			      void *mbuf, size_t mbuf_nbytes);

int
xnvme_be_linux_nvme_dev_nsid(struct xnvme_dev *dev);

int
xnvme_be_linux_nvme_map_cpl(struct xnvme_cmd_ctx *ctx, unsigned long ioctl_req, int err);

#endif /* __INTERNAL_XNVME_BE_LINUX_NVME_H */
