// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>
#include <xnvme_dev.h>
#include <xnvme_be.h>
#include <xnvme_lock.h>
#include <xnvme_mem.h>

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

int
xnvme_mem_noreq_map(const struct xnvme_dev *XNVME_UNUSED(dev), void *vaddr, size_t nbytes,
		    uint64_t *phys)
{
	if (!vaddr || !nbytes) {
		XNVME_DEBUG("FAILED: invalid vaddr(%p) or nbytes(%zu)", vaddr, nbytes);
		errno = EINVAL;
		return -EINVAL;
	}
	if (phys) {
		XNVME_DEBUG("FAILED: no physical address for memory that is not registered");
		errno = ENOSYS;
		return -ENOSYS;
	}

	return 0;
}

int
xnvme_mem_noreq_unmap(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	if (!buf) {
		XNVME_DEBUG("FAILED: invalid buf(%p)", buf);
		errno = EINVAL;
		return -EINVAL;
	}

	return 0;
}

int
xnvme_mem_noreq_map_req(const struct xnvme_dev *XNVME_UNUSED(dev), struct xnvme_mem_map_req *req)
{
	req->align = sizeof(void *);

	return 0;
}
