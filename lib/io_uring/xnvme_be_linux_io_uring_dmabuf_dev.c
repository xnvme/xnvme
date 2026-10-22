// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>

#ifdef XNVME_BE_LINUX_IO_URING_DMABUF_ENABLED
#include <xnvme_dev.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_io_uring_dmabuf.h>

static void
dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	xnvme_io_uring_dmabuf_term(dev);
	xnvme_be_linux_dev_close(dev);
}

struct xnvme_be_dev g_xnvme_be_dev_linux_io_uring_dmabuf = {
	.id = "linux",
	.dev_open = xnvme_be_linux_dev_open,
	.dev_close = dev_close,
};
#endif
