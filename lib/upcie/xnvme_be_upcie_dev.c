// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <fcntl.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_ENABLED
#include <limits.h>
#include <stdatomic.h>
#include <xnvme_dev.h>
#include <xnvme_be_upcie.h>

#define XNVME_BE_UPCIE_QUEUE_DEPTH 256

static _Atomic int g_ctrlr_count;

int
xnvme_be_upcie_get_driver_name(const char *bdf, char *driver_name, size_t driver_name_len)
{
	char path[PATH_MAX] = {0};
	char link[PATH_MAX] = {0};
	ssize_t nbytes;
	char *base;

	snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/driver", bdf);

	nbytes = readlink(path, link, sizeof(link) - 1);
	if (nbytes < 0) {
		return -errno;
	}

	base = strrchr(link, '/');
	if (!base || !base[1]) {
		return -EINVAL;
	}

	snprintf(driver_name, driver_name_len, "%s", base + 1);

	return 0;
}

/**
 * Terminate the uPCIe runtime-environment
 *
 * The state is globally accessible via g_upcie_rte, this function terminates it, unless it is
 * not initialized, then it exits early.
 */
static void
_rte_term(void)
{
	if (!g_upcie_rte.is_initialized) {
		return;
	}

	hostmem_heap_term(&g_upcie_rte.heap);

	g_upcie_rte.is_initialized = 0;
}

/**
 * Initialize the uPCIe runtime-environment
 *
 * The state is globally accessible via g_upcie_rte, this function initializes it, unless it is
 * already initialized, then it exits early.
 */
static int
_rte_init(void)
{
	int err;

	if (g_upcie_rte.is_initialized) {
		return 0;
	}

	err = hostmem_config_init(&g_upcie_rte.config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_config_init(); err(%d)", err);
		return err;
	}

	err = hostmem_heap_init(&g_upcie_rte.heap, 256 * 1024 * 1024, &g_upcie_rte.config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_heap_init(); err(%d)", err);
		return err;
	}

	g_upcie_rte.is_initialized = 1;

	return 0;
}

/**
 * Enable PCI Bus Master for the given BDF
 *
 * Reads the PCI Command register via sysfs config space and sets the Bus Master Enable bit,
 * unless it is already set, then it exits early.
 *
 * @param bdf PCI Bus-Device-Function string, e.g. "0000:01:00.0"
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
static int
_pci_enable_bus_master(const char *bdf)
{
	char path[256] = {0};
	uint16_t cmd = 0;
	int fd;
	ssize_t ret;

	snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/config", bdf);

	fd = open(path, O_RDWR);
	if (fd < 0) {
		XNVME_DEBUG("FAILED: open(%s); errno(%d)", path, errno);
		return -errno;
	}

	ret = pread(fd, &cmd, sizeof(cmd), 0x04);
	if (ret != sizeof(cmd)) {
		XNVME_DEBUG("FAILED: pread(PCI_COMMAND); ret(%zd)", ret);
		close(fd);
		return -EIO;
	}

	if (cmd & 0x4) {
		close(fd);
		return 0;
	}

	cmd |= 0x4;

	ret = pwrite(fd, &cmd, sizeof(cmd), 0x04);
	if (ret != sizeof(cmd)) {
		XNVME_DEBUG("FAILED: pwrite(PCI_COMMAND); ret(%zd)", ret);
		close(fd);
		return -EIO;
	}

	close(fd);
	return 0;
}

static void
_upcie_shm_name(const char *bdf, char *buf, size_t buflen)
{
	int i;

	snprintf(buf, buflen, "/xnvme-upcie-%s", bdf);
	for (i = 1; buf[i]; i++) {
		if (buf[i] == ':' || buf[i] == '.') {
			buf[i] = '-';
		}
	}
}

static int
_upcie_send_delete_iosq(struct nvme_controller *ctrl, uint16_t qid)
{
	struct nvme_command cmd = {0};
	struct nvme_completion cpl = {0};

	cmd.opc = 0x00; ///< Delete I/O Submission Queue
	cmd.cdw10 = qid;

	return nvme_qpair_submit_sync(&ctrl->aq, &cmd, ctrl->timeout_ms, &cpl);
}

static int
_upcie_send_delete_iocq(struct nvme_controller *ctrl, uint16_t qid)
{
	struct nvme_command cmd = {0};
	struct nvme_completion cpl = {0};

	cmd.opc = 0x04; ///< Delete I/O Completion Queue
	cmd.cdw10 = qid;

	return nvme_qpair_submit_sync(&ctrl->aq, &cmd, ctrl->timeout_ms, &cpl);
}

/**
 * Claim a pre-allocated queue pair from the shared qidmap.
 *
 * On success, the caller owns the returned qpair's rpool and is responsible for
 * freeing it. Ring memory (sq/cq) is NOT owned by the caller.
 *
 * @param ctrlr Controller whose qidmap to claim from.
 * @param ring_base Base virtual address from which ring offsets are computed.
 * @return The claimed queue ID (>= 1) on success, or -errno on error.
 */
int
_upcie_claim_qpair(struct xnvme_be_upcie_ctrlr *ctrlr, void *ring_base, struct nvme_qpair *dest)
{
	struct xnvme_be_upcie_mproc *mp = ctrlr->mproc;
	struct xnvme_be_upcie_qinfo *qi;
	unsigned int qid_pos, qid;
	uint64_t old, new_val, cap;
	uint8_t *bar0;
	int dstrd, err;

	bar0 = ctrlr->ctrl->func.bars[0].region;
	cap = nvme_mmio_cap_read(bar0);
	dstrd = nvme_reg_cap_get_dstrd(cap);

	do {
		old = atomic_load_explicit(&mp->qidmap, memory_order_relaxed);
		if (!old) {
			XNVME_DEBUG("FAILED: no free queue identifiers");
			return -EBUSY;
		}
		qid_pos = (unsigned int)__builtin_ffsll((long long)old);
		if (qid_pos < 2) {
			XNVME_DEBUG("FAILED: no free queue identifiers");
			return -EBUSY;
		}
		new_val = old & ~(1ULL << (qid_pos - 1));
	} while (!atomic_compare_exchange_weak_explicit(
		&mp->qidmap, &old, new_val, memory_order_acq_rel, memory_order_relaxed));

	qid = qid_pos - 1;
	qi = &mp->queues[qid];

	dest->sq = (uint8_t *)ring_base + qi->sq_offset;
	dest->cq = (uint8_t *)ring_base + qi->cq_offset;
	dest->sqdb = bar0 + 0x1000 + ((2 * qid) << (2 + dstrd));
	dest->cqdb = bar0 + 0x1000 + ((2 * qid + 1) << (2 + dstrd));
	dest->qid = qid;
	dest->depth = qi->depth;
	dest->tail = qi->sq_tail;
	dest->tail_last_written = UINT16_MAX;
	dest->head = qi->cq_head;
	dest->phase = qi->cq_phase;
	dest->heap = &g_upcie_rte.heap;

	dest->rpool = calloc(1, sizeof(*dest->rpool));
	if (!dest->rpool) {
		atomic_fetch_or_explicit(&mp->qidmap, 1ULL << qid, memory_order_release);
		return -ENOMEM;
	}
	nvme_request_pool_init(dest->rpool);

	err = nvme_request_pool_init_prps(dest->rpool, &g_upcie_rte.heap);
	if (err) {
		free(dest->rpool);
		dest->rpool = NULL;
		atomic_fetch_or_explicit(&mp->qidmap, 1ULL << qid, memory_order_release);
		return err;
	}

	return (int)qid;
}

/**
 * Release a claimed queue pair's per-process resources back to the pool.
 *
 * Does NOT free ring memory — that is always owned by the primary's heap.
 */
void
_upcie_release_qpair(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_qpair *qp)
{
	struct xnvme_be_upcie_qinfo *qi = &ctrlr->mproc->queues[qp->qid];

	if (qp->rpool) {
		nvme_request_pool_term_prps(qp->rpool, qp->heap);
		free(qp->rpool);
		qp->rpool = NULL;
	}

	qi->sq_tail = qp->tail;
	qi->cq_head = qp->head;
	qi->cq_phase = qp->phase;
	atomic_fetch_or_explicit(&ctrlr->mproc->qidmap, 1ULL << qp->qid, memory_order_release);
}

static void
_upcie_free_preallocated_queues(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	for (int qid = 1; qid < XNVME_BE_UPCIE_NQUEUES_MAX; qid++) {
		struct nvme_qpair *qp = &ctrlr->preallocated[qid];

		if (!qp->sq) {
			continue;
		}

		_upcie_send_delete_iosq(ctrlr->ctrl, (uint16_t)qid);
		_upcie_send_delete_iocq(ctrlr->ctrl, (uint16_t)qid);

		if (qp->rpool) {
			nvme_request_pool_term_prps(qp->rpool, qp->heap);
			free(qp->rpool);
		}
		hostmem_dma_free(qp->heap, qp->sq);
		hostmem_dma_free(qp->heap, qp->cq);
	}
	free(ctrlr->preallocated);
	ctrlr->preallocated = NULL;
}

int
_upcie_mproc_primary_init(struct xnvme_dev *dev, struct xnvme_be_upcie_ctrlr *ctrlr)
{
	struct xnvme_be_upcie_mproc *mproc;
	struct hostmem_heap *heap = &g_upcie_rte.heap;
	char shm_name[64];
	size_t shm_size = sizeof(*mproc);
	int shm_fd, err;

	_upcie_shm_name(dev->ident.uri, shm_name, sizeof(shm_name));
	snprintf(ctrlr->shm_name, sizeof(ctrlr->shm_name), "%s", shm_name);

	shm_unlink(shm_name);
	shm_fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0600);
	if (shm_fd < 0) {
		err = -errno;
		XNVME_DEBUG("FAILED: shm_open(%s): %d", shm_name, err);
		goto failed;
	}

	err = ftruncate(shm_fd, (off_t)shm_size);
	if (err) {
		err = -errno;
		XNVME_DEBUG("FAILED: ftruncate(); err(%d)", err);
		goto failed_close;
	}

	mproc = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (mproc == MAP_FAILED) {
		err = -errno;
		XNVME_DEBUG("FAILED: mmap() shm; err(%d)", err);
		goto failed_close;
	}

	memset(mproc, 0, shm_size);
	mproc->nqueues = 0;

	snprintf(mproc->hugepage_path, sizeof(mproc->hugepage_path), "%s", heap->memory.path);
	mproc->hugepage_size = heap->memory.size;
	mproc->nphys = (uint32_t)heap->nphys;

	if (heap->nphys > XNVME_BE_UPCIE_MAX_HUGEPAGES) {
		err = -ENOSPC;
		XNVME_DEBUG("FAILED: too many hugepages (%zu > %d)", heap->nphys,
			    XNVME_BE_UPCIE_MAX_HUGEPAGES);
		goto failed_munmap;
	}

	for (size_t i = 0; i < heap->nphys; i++) {
		mproc->phys_lut[i] = heap->phys_lut[i];
	}

	ctrlr->mproc = mproc;
	ctrlr->shm_fd = shm_fd;

	ctrlr->preallocated = calloc(XNVME_BE_UPCIE_NQUEUES_MAX, sizeof(*ctrlr->preallocated));
	if (!ctrlr->preallocated) {
		XNVME_DEBUG("FAILED: calloc(preallocated)");
		err = -ENOMEM;
		goto failed_munmap;
	}

	atomic_store(&mproc->refcount, 1);

	err = nvme_controller_create_io_qpair(ctrlr->ctrl, &ctrlr->sync, 16);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_controller_create_io_qpair(%d)", err);
		goto failed_free_preallocated;
	}

	return 0;

failed_free_preallocated:
	free(ctrlr->preallocated);
	ctrlr->preallocated = NULL;
failed_munmap:
	munmap(mproc, shm_size);
failed_close:
	close(shm_fd);
	shm_unlink(shm_name);
	ctrlr->shm_fd = -2;
failed:
	ctrlr->mproc = NULL;
	return err;
}

/**
 * Initialize the uPCIe controller.
 *
 * For primary (proc_role == XNVME_PROC_PRIMARY): initializes the runtime
 * environment, opens the NVMe controller, pre-allocates all I/O queue pairs,
 * stores shared state in POSIX shm, and claims a sync queue pair. Secondary
 * processes attach via ctrlr_attach and claim pre-allocated queue pairs
 * atomically.
 *
 * For single (proc_role == XNVME_PROC_SINGLE): original single-process behavior
 * with a single on-demand sync queue pair and no shared memory.
 */
void *
xnvme_be_upcie_ctrlr_init(struct xnvme_dev *dev)
{
	struct xnvme_be_upcie_ctrlr *ctrlr;
	char driver_name[sizeof(dev->ident.kernel_driver)] = {0};
	int err;

	err = _rte_init();
	if (err) {
		XNVME_DEBUG("FAILED: _rte_init()");
		errno = -err;
		return NULL;
	}

	err = _pci_enable_bus_master(dev->ident.uri);
	if (err) {
		XNVME_DEBUG("FAILED: _pci_enable_bus_master(%s)", dev->ident.uri);
		errno = -err;
		return NULL;
	}

	ctrlr = calloc(1, sizeof(*ctrlr));
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: calloc(ctrlr)");
		errno = ENOMEM;
		return NULL;
	}
	ctrlr->shm_fd = -2;

	ctrlr->ctrl = calloc(1, sizeof(*ctrlr->ctrl));
	if (!ctrlr->ctrl) {
		XNVME_DEBUG("FAILED: calloc(ctrl)");
		errno = ENOMEM;
		free(ctrlr);
		return NULL;
	}

	err = xnvme_be_upcie_get_driver_name(dev->ident.uri, driver_name, sizeof(driver_name));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_get_driver_name(%s); err(%d)", dev->ident.uri,
			    err);
		errno = -err;
		free(ctrlr->ctrl);
		free(ctrlr);
		return NULL;
	}
	snprintf(dev->ident.kernel_driver, sizeof(dev->ident.kernel_driver), "%s", driver_name);

	if (!strcmp(driver_name, "vfio-pci")) {
		ctrlr->backend = NVME_BACKEND_VFIO;
		err = nvme_controller_open_vfio(ctrlr->ctrl, &ctrlr->vfio, dev->ident.uri,
						&g_upcie_rte.heap);
	} else if (!strcmp(driver_name, "uio_pci_generic")) {
		ctrlr->backend = NVME_BACKEND_SYSFS;
		err = nvme_controller_open(ctrlr->ctrl, dev->ident.uri, &g_upcie_rte.heap);
	} else {
		XNVME_DEBUG("FAILED: unsupported driver '%s'", driver_name);
		err = -ENOTSUP;
	}
	if (err) {
		XNVME_DEBUG("FAILED: %s(%s)",
			    ctrlr->backend == NVME_BACKEND_VFIO ? "nvme_controller_open_vfio"
								: "nvme_controller_open",
			    dev->ident.uri);
		errno = -err;
		free(ctrlr->ctrl);
		free(ctrlr);
		return NULL;
	}

	if (dev->opts.proc_role == XNVME_PROC_PRIMARY) {
		err = _upcie_mproc_primary_init(dev, ctrlr);
		if (err) {
			XNVME_DEBUG("FAILED: _upcie_mproc_primary_init(); err(%d)", err);
			errno = -err;
			goto fail_close;
		}
	} else {
		err = nvme_controller_create_io_qpair(ctrlr->ctrl, &ctrlr->sync, 16);
		if (err) {
			XNVME_DEBUG("FAILED: nvme_controller_create_io_qpair; err(%d)", err);
			errno = -err;
			goto fail_close;
		}
	}

	g_ctrlr_count++;

	return ctrlr;

fail_close:
	if (ctrlr->backend == NVME_BACKEND_VFIO) {
		nvme_controller_close_vfio(ctrlr->ctrl, &ctrlr->vfio);
	} else {
		nvme_controller_close(ctrlr->ctrl);
	}
	free(ctrlr->ctrl);
	free(ctrlr);
	return NULL;
}

/**
 * Attach to an already-initialized uPCIe controller as a secondary process.
 *
 * Supports sysfs (uio_pci_generic) backend only. Opens the device without
 * resetting the controller, imports the primary's hugepage for shared ring
 * memory access, and claims a sync queue pair atomically from the shared
 * qidmap without sending admin commands.
 */
static void *
xnvme_be_upcie_ctrlr_attach(struct xnvme_dev *dev)
{
	struct xnvme_be_upcie_ctrlr *ctrlr;
	struct xnvme_be_upcie_mproc *mproc;
	char driver_name[sizeof(dev->ident.kernel_driver)] = {0};
	char shm_name[64];
	size_t shm_size = sizeof(*mproc);
	int err;

	err = xnvme_be_upcie_get_driver_name(dev->ident.uri, driver_name, sizeof(driver_name));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_get_driver_name(%s); err(%d)", dev->ident.uri,
			    err);
		return NULL;
	}

	if (strcmp(driver_name, "uio_pci_generic") != 0) {
		XNVME_DEBUG("FAILED: ctrlr_attach only supported for uio_pci_generic, got '%s'",
			    driver_name);
		errno = ENOTSUP;
		return NULL;
	}

	err = _rte_init();
	if (err) {
		XNVME_DEBUG("FAILED: _rte_init()");
		errno = -err;
		return NULL;
	}

	err = _pci_enable_bus_master(dev->ident.uri);
	if (err) {
		XNVME_DEBUG("FAILED: _pci_enable_bus_master(%s)", dev->ident.uri);
		errno = -err;
		goto fail_rte;
	}

	ctrlr = calloc(1, sizeof(*ctrlr));
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: calloc(ctrlr)");
		errno = ENOMEM;
		goto fail_rte;
	}

	ctrlr->shm_fd = -1;
	ctrlr->imported_hugepage.fd = -1;
	ctrlr->backend = NVME_BACKEND_SYSFS;

	ctrlr->ctrl = calloc(1, sizeof(*ctrlr->ctrl));
	if (!ctrlr->ctrl) {
		XNVME_DEBUG("FAILED: calloc(ctrl)");
		errno = ENOMEM;
		free(ctrlr);
		goto fail_rte;
	}

	_upcie_shm_name(dev->ident.uri, shm_name, sizeof(shm_name));
	snprintf(ctrlr->shm_name, sizeof(ctrlr->shm_name), "%s", shm_name);

	{
		int fd;

		fd = shm_open(shm_name, O_RDWR, 0);
		if (fd < 0) {
			XNVME_DEBUG("FAILED: shm_open(%s): %d", shm_name, errno);
			goto fail_ctrl;
		}

		mproc = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (mproc == MAP_FAILED) {
			XNVME_DEBUG("FAILED: mmap shm: %d", errno);
			close(fd);
			goto fail_ctrl;
		}
		close(fd);
	}

	ctrlr->mproc = mproc;

	err = hostmem_hugepage_import(mproc->hugepage_path, &ctrlr->imported_hugepage,
				      &g_upcie_rte.config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_hugepage_import(%s): %d", mproc->hugepage_path, err);
		goto fail_shm;
	}

	err = pci_func_open(dev->ident.uri, &ctrlr->ctrl->func);
	if (err) {
		XNVME_DEBUG("FAILED: pci_func_open(%s): %d", dev->ident.uri, err);
		goto fail_hugepage;
	}

	err = pci_bar_map(dev->ident.uri, 0, &ctrlr->ctrl->func.bars[0]);
	if (err) {
		XNVME_DEBUG("FAILED: pci_bar_map(%s, BAR0): %d", dev->ident.uri, err);
		pci_func_close(&ctrlr->ctrl->func);
		goto fail_hugepage;
	}

	ctrlr->ctrl->heap = &g_upcie_rte.heap;

	atomic_fetch_add_explicit(&mproc->refcount, 1, memory_order_release);

	{
		uint8_t *bar0 = ctrlr->ctrl->func.bars[0].region;
		uint64_t cap = nvme_mmio_cap_read(bar0);
		int qpid;

		ctrlr->ctrl->timeout_ms = nvme_reg_cap_get_to(cap) * 500;

		qpid = _upcie_claim_qpair(ctrlr, ctrlr->imported_hugepage.virt, &ctrlr->sync);
		if (qpid < 0) {
			XNVME_DEBUG("FAILED: claim sync qpair: %d", qpid);
			atomic_fetch_sub_explicit(&mproc->refcount, 1, memory_order_release);
			pci_bar_unmap(&ctrlr->ctrl->func.bars[0]);
			pci_func_close(&ctrlr->ctrl->func);
			goto fail_hugepage;
		}

		snprintf(dev->ident.kernel_driver, sizeof(dev->ident.kernel_driver), "%s",
			 driver_name);
	}

	g_ctrlr_count++;

	return ctrlr;

fail_hugepage:
	if (ctrlr->imported_hugepage.virt) {
		munmap(ctrlr->imported_hugepage.virt, ctrlr->imported_hugepage.size);
	}
	if (ctrlr->imported_hugepage.fd >= 0) {
		close(ctrlr->imported_hugepage.fd);
	}

fail_shm:
	munmap(mproc, shm_size);
	ctrlr->mproc = NULL;

fail_ctrl:
	free(ctrlr->ctrl);
	free(ctrlr);

fail_rte:
	_rte_term();
	return NULL;
}

int
xnvme_be_upcie_ctrlr_term(void *handle)
{
	struct xnvme_be_upcie_ctrlr *ctrlr = handle;
	struct xnvme_be_upcie_mproc *mproc = ctrlr->mproc;

	if (ctrlr->shm_fd >= 0) {
		int32_t attached;

		attached = atomic_load_explicit(&mproc->refcount, memory_order_acquire);
		if (attached > 1) {
			XNVME_DEBUG("WARNING: closing primary with %d secondary process(es) still "
				    "attached; their queue state will be corrupted",
				    attached - 1);
		}

		nvme_qpair_term(&ctrlr->sync);
		_upcie_free_preallocated_queues(ctrlr);

		munmap(mproc, sizeof(*mproc));
		close(ctrlr->shm_fd);

		shm_unlink(ctrlr->shm_name);

		if (ctrlr->backend == NVME_BACKEND_VFIO) {
			nvme_controller_close_vfio(ctrlr->ctrl, &ctrlr->vfio);
		} else {
			nvme_controller_close(ctrlr->ctrl);
		}
	} else if (ctrlr->shm_fd == -1) {
		_upcie_release_qpair(ctrlr, &ctrlr->sync);

		atomic_fetch_sub_explicit(&mproc->refcount, 1, memory_order_release);

		munmap(mproc, sizeof(*mproc));

		if (ctrlr->imported_hugepage.virt) {
			munmap(ctrlr->imported_hugepage.virt, ctrlr->imported_hugepage.size);
		}
		if (ctrlr->imported_hugepage.fd >= 0) {
			close(ctrlr->imported_hugepage.fd);
		}

		pci_bar_unmap(&ctrlr->ctrl->func.bars[0]);
		pci_func_close(&ctrlr->ctrl->func);
	} else {
		nvme_qpair_term(&ctrlr->sync);

		if (ctrlr->backend == NVME_BACKEND_VFIO) {
			nvme_controller_close_vfio(ctrlr->ctrl, &ctrlr->vfio);
		} else {
			nvme_controller_close(ctrlr->ctrl);
		}
	}

	free(ctrlr->ctrl);
	free(ctrlr);

	if (--g_ctrlr_count == 0) {
		_rte_term();
	}

	return 0;
}

void
xnvme_be_upcie_dev_close(struct xnvme_dev *XNVME_UNUSED(dev))
{
}

static void
_upcie_mproc_publish_idfy(struct xnvme_dev *dev)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;
	struct xnvme_be_upcie_ctrlr *ctrlr = state->ctrlr;
	struct xnvme_be_upcie_mproc *mp = ctrlr->mproc;

	memcpy(&mp->id_ctrlr, &dev->id.ctrlr, sizeof(mp->id_ctrlr));

	if (dev->ident.dtype != XNVME_DEV_TYPE_NVME_CONTROLLER) {
		memcpy(&mp->id_ns, &dev->id.ns, sizeof(mp->id_ns));
		memcpy(&mp->idcss_ctrlr, &dev->idcss.ctrlr, sizeof(mp->idcss_ctrlr));
		memcpy(&mp->idcss_ns, &dev->idcss.ns, sizeof(mp->idcss_ns));
		mp->nsid = dev->ident.nsid;
		mp->csi = dev->ident.csi;
	}

	atomic_store_explicit(&mp->id_ready, 1, memory_order_release);
}

int
xnvme_be_upcie_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;
	struct xnvme_be_upcie_ctrlr *ctrlr = state->ctrlr;
	int err;

	if (dev->opts.proc_role == XNVME_PROC_SECONDARY && ctrlr->shm_fd != -1) {
		XNVME_DEBUG("FAILED: secondary cannot reuse a primary/single ctrlr in the same "
			    "process");
		return -ENOTSUP;
	}

	dev->ident.dtype =
		dev->opts.nsid ? XNVME_DEV_TYPE_NVME_NAMESPACE : XNVME_DEV_TYPE_NVME_CONTROLLER;
	dev->ident.csi = XNVME_SPEC_CSI_NVM;
	dev->ident.nsid = dev->opts.nsid;

	if (dev->opts.proc_role == XNVME_PROC_SECONDARY) {
		struct xnvme_be_upcie_mproc *mp = ctrlr->mproc;

		if (!atomic_load_explicit(&mp->id_ready, memory_order_acquire)) {
			XNVME_DEBUG("FAILED: primary identify data not yet published");
			return -ENOENT;
		}

		memcpy(&dev->id.ctrlr, &mp->id_ctrlr, sizeof(dev->id.ctrlr));
		memcpy(dev->ident.subnqn, dev->id.ctrlr.subnqn, sizeof(dev->ident.subnqn));

		if (dev->ident.dtype != XNVME_DEV_TYPE_NVME_CONTROLLER) {
			if (dev->ident.nsid != mp->nsid) {
				XNVME_DEBUG("FAILED: nsid mismatch: want %u, have %u",
					    dev->ident.nsid, mp->nsid);
				return -ENOENT;
			}
			memcpy(&dev->id.ns, &mp->id_ns, sizeof(dev->id.ns));
			memcpy(&dev->idcss.ctrlr, &mp->idcss_ctrlr, sizeof(dev->idcss.ctrlr));
			memcpy(&dev->idcss.ns, &mp->idcss_ns, sizeof(dev->idcss.ns));
			dev->ident.csi = mp->csi;
		}
	}

	if (dev->opts.proc_role == XNVME_PROC_PRIMARY && ctrlr->mproc) {
		err = xnvme_dev_derive_geo(dev);
		if (err) {
			return err;
		}
		_upcie_mproc_publish_idfy(dev);
	}

	return 0;
}

#endif

struct xnvme_be_dev g_xnvme_be_upcie_dev = {
#ifdef XNVME_BE_UPCIE_ENABLED
	.dev_open = xnvme_be_upcie_dev_open,
	.dev_close = xnvme_be_upcie_dev_close,
	.id = "upcie",
	.ctrlr_init = xnvme_be_upcie_ctrlr_init,
	.ctrlr_term = xnvme_be_upcie_ctrlr_term,
	.ctrlr_attach = xnvme_be_upcie_ctrlr_attach,
#else
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
	.id = "nosys",
	.ctrlr_init = NULL,
	.ctrlr_term = NULL,
	.ctrlr_attach = NULL,
#endif
};
