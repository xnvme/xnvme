// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
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

int
xnvme_mem_map_get_req(const struct xnvme_dev *dev, struct xnvme_mem_map_req *req)
{
	struct xnvme_mem_map_req filled = {0};
	int err;

	if (!dev || !req) {
		return -EINVAL;
	}
	if (!dev->be.mem.mem_map_req) {
		return -ENOSYS;
	}

	err = dev->be.mem.mem_map_req(dev, &filled);
	if (err) {
		return err;
	}

	*req = filled;

	return 0;
}
