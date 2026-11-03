// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_dev.h>
#include <xnvme_be.h>
#include <xnvme_lock.h>

int
xnvme_mem_map(const struct xnvme_dev *dev, void *vaddr, size_t nbytes)
{
	int err;

	xnvme_lock();
	err = dev->be.mem.mem_map(dev, vaddr, nbytes, NULL);
	xnvme_unlock();

	return err;
}

int
xnvme_mem_unmap(const struct xnvme_dev *dev, void *buf)
{
	int err;

	xnvme_lock();
	err = dev->be.mem.mem_unmap(dev, buf);
	xnvme_unlock();

	return err;
}
