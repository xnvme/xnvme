// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <unistd.h>

#include <xnvme_transport_debug.h>
#include <xnvme_transport_ctrlr.h>
#include <xnvme_transport_rbtree.h>

#include <xnvme_be_rdma.h>
#include <xnvme_be_rdma_mem.h>

#define TRANSPORT_DEBUG_CATEGORY TRANSPORT_DEBUG_CATEGORY_MEM

static inline int
_rdma_mem_contains(struct xnvme_transport_memory_handle *handle, uintptr_t addr, size_t nbytes)
{
	return addr >= handle->buf && addr + nbytes <= handle->buf + handle->nbytes;
}

static inline int
_rdma_mem_compare_handles(void *_key, void *_current)
{
	struct xnvme_transport_memory_handle *key = _key;
	struct xnvme_transport_memory_handle *current = _current;

	if (_rdma_mem_contains(current, key->buf, key->nbytes)) {
		return 0;
	} else if (key->buf < current->buf)
		return -1;
	else
		return 1;
}

int
xnvme_be_rdma_mem_init(struct xnvme_dev *dev)
{
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(dev);

	state->util_rbtree = (void *)rbtNew(_rdma_mem_compare_handles);
	state->mem_cache = NULL;

	if (!state->util_rbtree) {
		TRANSPORT_DEBUG("FAILED: rbtNew()");
		return -ENOMEM;
	}

	return 0;
}

int
xnvme_be_rdma_mem_term(struct xnvme_dev *dev)
{
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(dev);
	RbtHandle rbt = (RbtHandle)state->util_rbtree;

	if (rbt) {
		if (!rbtEmpty(rbt)) {
			TRANSPORT_DEBUG("Cannot delete a non-empty red-black tree, dev=%p", dev);
			TRANSPORT_DEBUG(
				"Destroy all buffer allocations before calling xnvme_dev_close()");
			return -EBUSY;
		}

		rbtDelete(rbt);
		state->util_rbtree = NULL;
	}

	return 0;
}

static inline int
_rdma_mem_register_buf(const struct xnvme_dev *dev, void *user_buffer, size_t nbytes,
		       long alignment)
{
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(dev);
	struct xnvme_transport_memory_handle *reg_handle = NULL;
	struct xnvme_transport_ctrlr *ctrlr = TO_TRANSPORT_CTRLR(state->ctrlr);
	RbtHandle *rbt = (RbtHandle *)state->util_rbtree;
	RbtStatus status;
	int err;

	reg_handle = xnvme_buf_virt_alloc(alignment, sizeof(*reg_handle));
	if (!reg_handle) {
		XNVME_DEBUG("FAILED: xnvme_buf_virt_alloc() for handle_mem");
		return -ENOMEM;
	}

	reg_handle->buf = (uintptr_t)user_buffer;
	reg_handle->nbytes = nbytes;

	err = xnvme_transport_ctrlr_reg(ctrlr, user_buffer, nbytes, &reg_handle->handle,
					&reg_handle->key);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_ctrlr_reg() for data buffer, err: %d",
				err);
		goto free_handle;
	}
	TRANSPORT_DEBUG("INFO: Registered buffer with handle %p and key %u", reg_handle->handle,
			reg_handle->key);

	status = rbtInsert(rbt, reg_handle, reg_handle);
	if (status != RBT_STATUS_OK) {
		XNVME_DEBUG("FAILED: rbtInsert()");
		err = -ENOSPC;
		goto deregister_region;
	}
	TRANSPORT_DEBUG("INFO: Inserted buffer into red-black tree, user_buffer=%p, length=%zu",
			user_buffer, nbytes);

	return 0;

deregister_region:
	xnvme_transport_ctrlr_dereg(ctrlr, reg_handle->handle);
free_handle:
	xnvme_buf_virt_free(reg_handle);
	return err;
}

static void *
xnvme_be_rdma_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *XNVME_UNUSED(buf))
{
	void *user_buffer = NULL;
	long sz = sysconf(_SC_PAGESIZE);
	int err;

	if (sz == -1) {
		XNVME_DEBUG("FAILED: sysconf(), errno: %d", errno);
		return NULL;
	}

	user_buffer = xnvme_buf_virt_alloc(sz, nbytes);
	if (!user_buffer) {
		XNVME_DEBUG("FAILED: xnvme_buf_virt_alloc()");
		return NULL;
	}
	TRANSPORT_DEBUG("INFO: Allocated user buffer %p of size %zu", user_buffer, nbytes);

	err = _rdma_mem_register_buf(dev, user_buffer, nbytes, sz);
	if (err) {
		XNVME_DEBUG("FAILED: _rdma_mem_register_buf(), err: %d", err);
		goto free_user_buffer;
	}

	return user_buffer;

free_user_buffer:
	xnvme_buf_virt_free(user_buffer);
	return NULL;
}

int
xnvme_transport_registration_lookup_key(struct xnvme_dev *dev, void *addr, size_t nbytes,
					uint32_t *key)
{
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(dev);
	RbtHandle rbt = (RbtHandle)state->util_rbtree;
	RbtIterator entry;
	struct xnvme_transport_memory_handle to_find = {
		.buf = (uintptr_t)addr,
		.nbytes = nbytes,
	};
	struct xnvme_transport_memory_handle *found_handle = state->mem_cache;

	// IO usually keeps hitting the same buffer, so try the last hit before the tree
	if (!found_handle || !_rdma_mem_contains(found_handle, to_find.buf, nbytes)) {
		entry = rbtFind(rbt, &to_find);
		if (!entry) {
			return -ENOENT;
		}

		found_handle = *rbtValuePtr(rbt, entry);
		state->mem_cache = found_handle;
	}

	*key = found_handle->key;

	return 0;
}

static inline int
_rdma_mem_deregister_buf(const struct xnvme_dev *dev, void *buf)
{
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(dev);
	struct xnvme_transport_ctrlr *ctrlr = TO_TRANSPORT_CTRLR(state->ctrlr);
	RbtHandle *rbt = (RbtHandle *)state->util_rbtree;
	RbtIterator entry;
	RbtStatus status;
	struct xnvme_transport_memory_handle to_find = {
		.buf = (uintptr_t)buf,
		.nbytes = 1,
	};
	struct xnvme_transport_memory_handle *mem_handle;
	int err;

	// remove entry
	entry = rbtFind(rbt, &to_find);
	if (!entry) {
		TRANSPORT_DEBUG("FAILED: rbtFind()");
		return -ENOENT;
	}

	mem_handle = *rbtValuePtr(rbt, entry);
	if (state->mem_cache == mem_handle) {
		state->mem_cache = NULL;
	}

	status = rbtErase(rbt, entry);
	if (status != RBT_STATUS_OK) {
		TRANSPORT_DEBUG("FAILED: rbtErase()");
		return -ENOENT;
	}

	// deregister and free the memory handle
	err = xnvme_transport_ctrlr_dereg(ctrlr, mem_handle->handle);
	if (err) {
		TRANSPORT_DEBUG("FAILED: xnvme_transport_ctrlr_dereg(), err: %d", err);
		xnvme_buf_virt_free(mem_handle); // best-effort free on failure
		return err;
	}

	xnvme_buf_virt_free(mem_handle);

	return 0;
}

static void
xnvme_be_rdma_buf_free(const struct xnvme_dev *dev, void *buf)
{
	int err;

	err = _rdma_mem_deregister_buf(dev, buf);
	if (err) {
		TRANSPORT_DEBUG("FAILED: _rdma_mem_deregister_buf(), err: %d, buf=%p", err, buf);
	}

	xnvme_buf_virt_free(buf);
	return;
}

static void *
xnvme_be_rdma_buf_realloc(const struct xnvme_dev *XNVME_UNUSED(dev), void *XNVME_UNUSED(buf),
			  size_t XNVME_UNUSED(nbytes), uint64_t *XNVME_UNUSED(phys))
{
	TRANSPORT_DEBUG("FAILED: rdma: does not support realloc");
	errno = ENOSYS;
	return NULL;
}

static int
xnvme_be_rdma_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev), void *XNVME_UNUSED(buf),
			  uint64_t *XNVME_UNUSED(phys))
{
	TRANSPORT_DEBUG("FAILED: rdma: does not support phys/DMA alloc");
	return -ENOSYS;
}

struct xnvme_be_mem g_xnvme_be_rdma_mem = {
	.id = "rdma",
	.buf_alloc = xnvme_be_rdma_buf_alloc,
	.buf_free = xnvme_be_rdma_buf_free,
	.buf_realloc = xnvme_be_rdma_buf_realloc,
	.buf_vtophys = xnvme_be_rdma_buf_vtophys,
};