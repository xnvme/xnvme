// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_ENABLED
#include <limits.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <xnvme_dev.h>
#include <xnvme_be_upcie.h>

void
xnvme_be_upcie_mproc_rte_term()
{
	struct xnvme_be_upcie_mproc *mproc = g_upcie_rte.mproc;

	if (!mproc) {
		return;
	}

	if (mproc->is_primary) {
		int attached = atomic_load(&mproc->shm->refcount);
		if (attached > 1) {
			XNVME_DEBUG("WARNING: closing primary with %d secondary process(es) still "
				    "attached",
				    attached - 1);
		}
	}

	atomic_fetch_sub(&mproc->shm->refcount, 1);

	munmap(mproc->shm, sizeof(*mproc->shm));

	if (mproc->primary_hugepage) {
		munmap(mproc->primary_hugepage->virt, mproc->primary_hugepage->size);
		close(mproc->primary_hugepage->fd);
		free(mproc->primary_hugepage);
	}

	if (mproc->is_primary) {
		if (mproc->shm_fd >= 0) {
			close(mproc->shm_fd);
			shm_unlink(mproc->shm_name);
		}

		if (mproc->lock_fd >= 0) {
			flock(mproc->lock_fd, LOCK_UN);
			close(mproc->lock_fd);
		}
		unlink(mproc->lock_name);
	}

	free(mproc);
	g_upcie_rte.mproc = NULL;
}

int
xnvme_be_upcie_mproc_rte_init(int shm_id)
{
	struct xnvme_be_upcie_mproc *mproc = NULL;
	mode_t shm_mode;
	int shm_fd = -1, oflag, shm_size, err;

	mproc = calloc(1, sizeof(*mproc));
	if (!mproc) {
		err = -ENOMEM;
		XNVME_DEBUG("FAILED: calloc(xnvme_be_upcie_mproc); err(%d)", err);
		return err;
	}

	mproc->is_primary = true;

	/* Use flock to determine primary / secondary role */

	snprintf(mproc->lock_name, sizeof(mproc->lock_name), "/tmp/xnvme-upcie-flock-%d", shm_id);
	mproc->lock_fd = open(mproc->lock_name, O_CREAT | O_RDWR, 0600);
	if (mproc->lock_fd < 0) {
		err = -errno;
		XNVME_DEBUG("FAILED: open() with lock_name(%s); err(%d)", mproc->lock_name, err);
		goto failed;
	}

	err = flock(mproc->lock_fd, LOCK_EX | LOCK_NB);
	if (err) {
		if (errno == EWOULDBLOCK) {
			XNVME_DEBUG("INFO: Lock file already claimed, setting role to secondary");
			close(mproc->lock_fd);
			mproc->lock_fd = -1;
			mproc->is_primary = false;
			err = 0;
			errno = 0;
		} else {
			err = -errno;
			XNVME_DEBUG("FAILED: flock; err(%d)", err);
			goto failed;
		}
	}

	/* Map shared memory for hugepage information */

	snprintf(mproc->shm_name, sizeof(mproc->shm_name), "/xnvme-upcie-shm-%d", shm_id);
	mproc->shm_fd = -1;

	oflag = mproc->is_primary ? O_CREAT | O_EXCL | O_RDWR : O_RDWR;
	shm_mode = mproc->is_primary ? 0600 : 0;
	shm_size = sizeof(struct xnvme_be_upcie_mproc_shm);

	if (mproc->is_primary) {
		shm_unlink(mproc->shm_name);
	}

	// Wait at most ~1 second to open shared memory segment. We do not retry in primary
	// processes.
	for (int i = 0; i < 1000; i++) {
		shm_fd = shm_open(mproc->shm_name, oflag, shm_mode);
		if (shm_fd >= 0 || mproc->is_primary) {
			break;
		}
		usleep(1000);
	}

	if (shm_fd < 0) {
		err = -errno;
		XNVME_DEBUG("FAILED: shm_open(%s): %d", mproc->shm_name, err);
		goto failed;
	}

	if (mproc->is_primary) {
		err = ftruncate(shm_fd, shm_size);
		if (err) {
			err = -errno;
			XNVME_DEBUG("FAILED: ftruncate(); err(%d)", err);
			goto failed_unlink;
		}
	}

	mproc->shm = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (mproc->shm == MAP_FAILED) {
		err = -errno;
		XNVME_DEBUG("FAILED: mmap() shm; err(%d)", err);
		goto failed_unlink;
	}

	if (mproc->is_primary) {
		memset(mproc->shm, 0, shm_size);
		mproc->shm_fd = shm_fd;
	} else {
		close(shm_fd);
	}

	atomic_fetch_add(&mproc->shm->refcount, 1);

	g_upcie_rte.mproc = mproc;

	return 0;

failed_unlink:
	if (mproc->is_primary) {
		shm_unlink(mproc->shm_name);
	}

failed:
	if (mproc && mproc->lock_fd >= 0) {
		flock(mproc->lock_fd, LOCK_UN);
		close(mproc->lock_fd);
	}

	if (shm_fd >= 0) {
		close(shm_fd);
	}

	free(mproc);
	return err;
}

/**
 * Import the primary's hugepages for accessing the admin queue in secondary processes.
 */
int
xnvme_be_upcie_mproc_import_admin_hugepage()
{
	int err;

	g_upcie_rte.mproc->primary_hugepage =
		calloc(1, sizeof(*g_upcie_rte.mproc->primary_hugepage));
	if (!g_upcie_rte.mproc->primary_hugepage) {
		XNVME_DEBUG("FAILED: calloc(primary_hugepage)");
		return -ENOMEM;
	}

	err = hostmem_hugepage_import(g_upcie_rte.mproc->shm->hugepage_path,
				      g_upcie_rte.mproc->primary_hugepage,
				      &g_upcie_rte.mem.config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_hugepage_import(); err(%d)", err);
		free(g_upcie_rte.mproc->primary_hugepage);
		g_upcie_rte.mproc->primary_hugepage = NULL;
		return err;
	}

	return 0;
}

/**
 * Recover the shared admin completion queue after a lock holder has crashed
 *
 * If a lock holder crashes, there might be unreaped completions in the CQ, which must be drained
 * from head/phase in shared memory.
 * Note: The shared head/phase are only published on unlock, so they are never ahead of the
 * device; draining from the shared head can therefore only reap real or phantom completions, never
 * skip one.
 *
 * Drained completions are discarded (no live waiter remains) and the shared head/phase are
 * resynced with the device.
 */
static void
_recover_aq(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	struct xnvme_be_upcie_ctrlr_shm *shm = ctrlr->mproc.shm;
	struct nvme_qpair *aq = &ctrlr->ctrl->aq;
	struct nvme_completion cpl = {0};
	int drained = 0;

	// Point the local queue-pair at the shared head/phase; the caller re-syncs it from
	// shared memory once the mutex is marked consistent, so this is safe to clobber.
	aq->head = shm->ctrl.aq.head;
	aq->phase = shm->ctrl.aq.phase;

	// timeout_ms(1): one poll of the current slot; the drain stops at the first slot the
	// device has not written, it does not wait for completions still in flight
	while (!nvme_qpair_reap_cpl(aq, 1, &cpl)) {
		drained++;
	}

	shm->ctrl.aq.head = aq->head;
	shm->ctrl.aq.phase = aq->phase;

	if (drained) {
		XNVME_DEBUG("INFO: recovered admin CQ after owner death; drained(%d)", drained);
	}
}

int
xnvme_be_upcie_ctrlr_mutex_lock(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	struct xnvme_be_upcie_ctrlr_shm *shm = ctrlr->mproc.shm;
	int err;

	if (!shm) {
		return 0;
	}

	err = pthread_mutex_lock(&shm->aq_mutex);
	if (err == EOWNERDEAD) {
		// Previous owner died inside the critical section; drain any
		// completions it left behind and resync before marking consistent.
		_recover_aq(ctrlr);
		pthread_mutex_consistent(&shm->aq_mutex);
	} else if (err == ENOTRECOVERABLE) {
		XNVME_DEBUG("FAILED: aq_mutex unrecoverable (ENOTRECOVERABLE)");
		return err;
	} else if (err) {
		XNVME_DEBUG("FAILED: pthread_mutex_lock(aq_mutex); err(%d)", err);
		return err;
	}

	ctrlr->ctrl->aq.tail = shm->ctrl.aq.tail;
	ctrlr->ctrl->aq.tail_last_written = UINT16_MAX;
	ctrlr->ctrl->aq.head = shm->ctrl.aq.head;
	ctrlr->ctrl->aq.phase = shm->ctrl.aq.phase;

	return 0;
}

void
xnvme_be_upcie_ctrlr_mutex_unlock(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	struct xnvme_be_upcie_ctrlr_shm *shm = ctrlr->mproc.shm;

	if (!shm) {
		return;
	}

	shm->ctrl.aq.tail = ctrlr->ctrl->aq.tail;
	shm->ctrl.aq.head = ctrlr->ctrl->aq.head;
	shm->ctrl.aq.phase = ctrlr->ctrl->aq.phase;

	pthread_mutex_unlock(&shm->aq_mutex);
}

static void
xnvme_be_upcie_shm_bdf_name(const char *bdf, char *buf, size_t buflen)
{
	int i;

	snprintf(buf, buflen, "/xnvme-upcie-%s", bdf);
	for (i = 1; buf[i]; i++) {
		if (buf[i] == ':' || buf[i] == '.' || buf[i] == '/') {
			buf[i] = '-';
		}
	}
}

void
xnvme_be_upcie_mproc_free_all_queues(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	struct xnvme_be_upcie_ctrlr_shm *shm = ctrlr->mproc.shm;
	int err;

	if (!g_upcie_rte.mproc || !g_upcie_rte.mproc->is_primary) {
		XNVME_DEBUG("INFO: xnvme_be_upcie_mproc_free_all_queues() called in non-primary "
			    "process; skipping");
		return;
	}

	err = xnvme_be_upcie_ctrlr_mutex_lock(ctrlr);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_ctrlr_mutex_lock(); err(%d)", err);
		return;
	}

	for (uint32_t qid = 1; qid < NVME_QID_MAX; qid++) {
		if (nvme_qid_is_allocated(shm->ctrl.qids, qid)) {
			struct nvme_command cmd = {0};
			struct nvme_completion cpl = {0};
			int err;

			cmd.cdw10 = qid;

			cmd.opc = 0x00; ///< Delete I/O Submission Queue
			err = nvme_qpair_submit_sync(&ctrlr->ctrl->aq, &cmd,
						     ctrlr->ctrl->timeout_ms, &cpl);
			if (err) {
				XNVME_DEBUG("FAILED: nvme_qpair_submit_sync(); err(%d)", err);
			}

			cmd.opc = 0x04; ///< Delete I/O Completion Queue
			err = nvme_qpair_submit_sync(&ctrlr->ctrl->aq, &cmd,
						     ctrlr->ctrl->timeout_ms, &cpl);
			if (err) {
				XNVME_DEBUG("FAILED: nvme_qpair_submit_sync(); err(%d)", err);
			}
		}
	}

	xnvme_be_upcie_ctrlr_mutex_unlock(ctrlr);
}

void
xnvme_be_upcie_mproc_ctrlr_shm_term(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	if (g_upcie_rte.mproc->is_primary) {
		int attached = atomic_load(&ctrlr->mproc.shm->refcount);
		if (attached > 1) {
			XNVME_DEBUG("WARNING: terminating controller with %d secondary "
				    "process(es) still "
				    "attached",
				    attached - 1);
		}
	} else if (ctrlr->ctrl && ctrlr->ctrl->aq.rpool) {
		nvme_request_pool_term_prps_dmamem(ctrlr->ctrl->aq.rpool, &g_upcie_rte.mem.heap,
						   ctrlr->mproc.aq_rpool_prp_offset);
		free(ctrlr->ctrl->aq.rpool);
		ctrlr->ctrl->aq.rpool = NULL;
	}

	atomic_fetch_sub(&ctrlr->mproc.shm->refcount, 1);

	if (g_upcie_rte.mproc->is_primary) {
		pthread_mutex_destroy(&ctrlr->mproc.shm->aq_mutex);
	}

	munmap(ctrlr->mproc.shm, sizeof(*ctrlr->mproc.shm));

	if (g_upcie_rte.mproc->is_primary) {
		close(ctrlr->mproc.shm_fd);
		shm_unlink(ctrlr->mproc.shm_name);
		ctrlr->ctrl = NULL; // was a pointer to shared memory, so remove this

		flock(ctrlr->mproc.lock_fd, LOCK_UN);
		close(ctrlr->mproc.lock_fd);
		unlink(ctrlr->mproc.lock_name);
	}
}

int
xnvme_be_upcie_mproc_ctrlr_shm_init(struct xnvme_dev *dev, struct xnvme_be_upcie_ctrlr *ctrlr,
				    const char *driver_name)
{
	struct xnvme_be_upcie_ctrlr_shm *shm;
	char shm_name[64];
	size_t shm_size = sizeof(*shm);
	int shm_fd = -1, lock_fd = -1, err;

	xnvme_be_upcie_shm_bdf_name(dev->ident.uri, shm_name, sizeof(shm_name));

	/* Use flock to determine whether another primary process has claimed device */
	snprintf(ctrlr->mproc.lock_name, sizeof(ctrlr->mproc.lock_name),
		 "/tmp/xnvme-upcie-%s-lock", dev->ident.uri);
	ctrlr->mproc.lock_fd = -1;

	lock_fd = open(ctrlr->mproc.lock_name, O_CREAT | O_RDWR, 0600);
	if (lock_fd < 0) {
		err = -errno;
		XNVME_DEBUG("FAILED: open() with lock_name(%s); err(%d)", ctrlr->mproc.lock_name,
			    err);
		goto failed;
	}

	err = flock(lock_fd, LOCK_EX | LOCK_NB);
	if (err) {
		err = -errno;
		XNVME_DEBUG("FAILED: flock; err(%d)", err);
		goto failed;
	}

	ctrlr->mproc.lock_fd = lock_fd;
	shm_unlink(shm_name);

	shm_fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0600);
	if (shm_fd < 0) {
		err = -errno;
		XNVME_DEBUG("FAILED: shm_open(%s): err(%d)", shm_name, err);
		goto failed;
	}

	err = ftruncate(shm_fd, (off_t)shm_size);
	if (err) {
		err = -errno;
		XNVME_DEBUG("FAILED: ftruncate(); err(%d)", err);
		goto failed;
	}

	shm = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (shm == MAP_FAILED) {
		err = -errno;
		XNVME_DEBUG("FAILED: mmap() shm; err(%d)", err);
		goto failed;
	}

	memset(shm, 0, shm_size);
	atomic_store(&shm->refcount, 1);
	strncpy(shm->driver_name, driver_name, sizeof(shm->driver_name));

	{
		pthread_mutexattr_t attr;

		pthread_mutexattr_init(&attr);
		pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
		pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
		pthread_mutex_init(&shm->aq_mutex, &attr);
		pthread_mutexattr_destroy(&attr);
	}

	ctrlr->ctrl = &shm->ctrl;
	ctrlr->mproc.shm = shm;
	ctrlr->mproc.shm_fd = shm_fd;
	snprintf(ctrlr->mproc.shm_name, sizeof(ctrlr->mproc.shm_name), "%s", shm_name);

	return 0;

failed:
	if (ctrlr->mproc.lock_fd >= 0) {
		flock(ctrlr->mproc.lock_fd, LOCK_UN);
	}

	if (lock_fd >= 0) {
		close(lock_fd);
		ctrlr->mproc.lock_fd = -1;
	}

	if (shm_fd >= 0) {
		close(shm_fd);
		shm_unlink(shm_name);
	}

	ctrlr->mproc.shm_fd = -1;
	ctrlr->mproc.shm = NULL;

	return err;
}

/**
 * Attach to an existing controller in shared memory
 *
 * Waits (up to ~1s) for the primary to create, size and finish initializing the
 * shared segment, so starting the primary and secondary concurrently is safe;
 * if the primary does not become ready within the timeout, attach fails.
 */
int
xnvme_be_upcie_mproc_ctrlr_shm_attach(struct xnvme_dev *dev, struct xnvme_be_upcie_ctrlr *ctrlr)
{
	struct xnvme_be_upcie_ctrlr_shm *shm = NULL;
	char shm_name[64];
	size_t shm_size = sizeof(*shm);
	bool rpool_prps_ready = false;
	int shm_fd, err;

	ctrlr->mproc.shm_fd = -1; // File descripter should not be saved in secondaries

	xnvme_be_upcie_shm_bdf_name(dev->ident.uri, shm_name, sizeof(shm_name));

	// Wait for the primary to create and size the segment. shm_open() can
	// succeed before the primary has ftruncate()'d it, so mapping and touching
	// it then would SIGBUS; only proceed once it is at least shm_size bytes.
	for (int i = 0; i < 1000; i++) {
		struct stat st;

		shm_fd = shm_open(shm_name, O_RDWR, 0);
		if (shm_fd >= 0) {
			err = fstat(shm_fd, &st);
			if (err) {
				err = -errno;
				XNVME_DEBUG("FAILED: fstat(shm); err(%d)", err);
				goto failed;
			}
			if ((size_t)st.st_size >= shm_size) {
				break;
			}
			close(shm_fd);
			shm_fd = -1;
		} else if (errno != ENOENT) {
			err = -errno;
			XNVME_DEBUG("FAILED: shm_open(%s): err(%d)", shm_name, err);
			goto failed;
		}

		usleep(1000);
	}

	if (shm_fd < 0) {
		XNVME_DEBUG("FAILED: timed out waiting for primary to create shm(%s)", shm_name);
		err = -ENOENT;
		goto failed;
	}

	shm = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (shm == MAP_FAILED) {
		err = -errno;
		XNVME_DEBUG("FAILED: mmap() shm; err(%d)", err);
		goto failed;
	}

	close(shm_fd);
	shm_fd = -1;

	ctrlr->mproc.shm = shm;
	snprintf(ctrlr->mproc.shm_name, sizeof(ctrlr->mproc.shm_name), "%s", shm_name);

	// Wait for the primary to finish opening the controller before reading any
	// shared fields; they are undefined until is_initialized is published.
	for (int i = 0; i < 1000; i++) {
		if (atomic_load_explicit(&shm->is_initialized, memory_order_acquire)) {
			break;
		}
		usleep(1000);
	}

	if (!atomic_load_explicit(&shm->is_initialized, memory_order_acquire)) {
		XNVME_DEBUG("FAILED: timed out waiting for primary controller init");
		err = -ENOENT;
		goto failed;
	}

	if (strcmp(shm->driver_name, "uio_pci_generic")) {
		XNVME_DEBUG("FAILED: mproc requires uio_pci_generic, primary saw '%s'",
			    shm->driver_name);
		err = -ENOTSUP;
		goto failed;
	}

	ctrlr->ctrl = calloc(1, sizeof(*ctrlr->ctrl));
	if (!ctrlr->ctrl) {
		XNVME_DEBUG("FAILED: calloc(ctrl)");
		err = -ENOMEM;
		goto failed;
	}

	ctrlr->ctrl->timeout_ms = shm->ctrl.timeout_ms;

	/* Retrieve BAR0 register mapping (not in shared memory) */
	{
		uint8_t *bar0;
		uint64_t cap;
		int dstrd;

		err = pci_func_open(dev->ident.uri, &ctrlr->ctrl->func);
		if (err) {
			XNVME_DEBUG("FAILED: pci_func_open(%s): %d", dev->ident.uri, err);
			goto failed;
		}

		err = pci_bar_map(dev->ident.uri, 0, &ctrlr->ctrl->func.bars[0]);
		if (err) {
			XNVME_DEBUG("FAILED: pci_bar_map(%s, BAR0): %d", dev->ident.uri, err);
			goto failed_close_function;
		}

		bar0 = ctrlr->ctrl->func.bars[0].region;
		cap = nvme_mmio_cap_read(bar0);
		dstrd = nvme_reg_cap_get_dstrd(cap);

		ctrlr->ctrl->aq.sqdb = bar0 + 0x1000;
		ctrlr->ctrl->aq.cqdb = bar0 + 0x1000 + (1 << (2 + dstrd));
	}

	/* Import admin queue from shared memory */
	{
		uint64_t offset = (uint64_t)g_upcie_rte.mproc->primary_hugepage->virt -
				  g_upcie_rte.mproc->shm->hugepage_base;

		ctrlr->ctrl->aq.sq = (char *)shm->ctrl.aq.sq + offset;
		ctrlr->ctrl->aq.cq = (char *)shm->ctrl.aq.cq + offset;
		ctrlr->ctrl->aq.depth = shm->ctrl.aq.depth;
		ctrlr->ctrl->aq.phase = shm->ctrl.aq.phase;
		ctrlr->ctrl->aq.qid = shm->ctrl.aq.qid;
	}

	/* Allocate local request pool with per-process PRP scratch on the local dmamem heap.
	 * The rpool itself is per-process; the PRPs are addressed via dmamem offsets so the
	 * primary and secondaries do not step on each other's scratch. */
	ctrlr->ctrl->aq.rpool = calloc(1, sizeof(*ctrlr->ctrl->aq.rpool));
	if (!ctrlr->ctrl->aq.rpool) {
		err = -ENOMEM;
		XNVME_DEBUG("FAILED: calloc(aq.rpool)");
		goto failed_close_function;
	}
	nvme_request_pool_init(ctrlr->ctrl->aq.rpool);

	err = nvme_request_pool_init_prps_dmamem(ctrlr->ctrl->aq.rpool, &g_upcie_rte.mem.heap,
						 &ctrlr->mproc.aq_rpool_prp_offset);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_request_pool_init_prps_dmamem(aq.rpool); err(%d)", err);
		goto failed_close_function;
	}
	rpool_prps_ready = true;

	err = xnvme_be_upcie_mproc_create_io_qpair(ctrlr, &ctrlr->sync, 16, &ctrlr->sync_offsets);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_mproc_create_io_qpair(); err(%d)", err);
		errno = -err;
		goto failed_close_function;
	}

	atomic_fetch_add(&shm->refcount, 1);

	return 0;

failed_close_function:
	pci_func_close(&ctrlr->ctrl->func);

failed:
	if (ctrlr->ctrl) {
		/* The PRP scratch sits in the primary's heap, so it outlives this
		 * process; hand it back rather than leaking it into the shared heap. */
		if (rpool_prps_ready) {
			nvme_request_pool_term_prps_dmamem(ctrlr->ctrl->aq.rpool,
							   &g_upcie_rte.mem.heap,
							   ctrlr->mproc.aq_rpool_prp_offset);
		}
		free(ctrlr->ctrl->aq.rpool);
	}

	free(ctrlr->ctrl);
	ctrlr->ctrl = NULL;

	if (shm_fd >= 0) {
		close(shm_fd);
	}

	if (shm && shm != MAP_FAILED) {
		munmap(shm, sizeof(*shm));
	}
	ctrlr->mproc.shm = NULL;

	return err;
}

/**
 * Take the admin-queue mutex and import the shared I/O queue-id bitmap
 *
 * Wraps xnvme_be_upcie_ctrlr_mutex_lock() for callers that allocate or release
 * I/O queue identifiers. In a secondary process ctrlr->ctrl is a process-local
 * copy of the controller and has to see the current bitmap before it is
 * consulted; in a primary process ctrlr->ctrl points into the shared segment
 * itself so no import is needed, and outside multi-process mode ctrlr->mproc.shm is
 * NULL, so this is a no-op beyond the locking itself.
 *
 * @param ctrlr The backend controller
 *
 * @return On success, 0 is returned. On error, a non-zero value is returned.
 */
int
xnvme_be_upcie_mproc_qids_lock(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	int err;

	err = xnvme_be_upcie_ctrlr_mutex_lock(ctrlr);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_ctrlr_mutex_lock(); err(%d)", err);
		return err;
	}

	if (ctrlr->mproc.shm && ctrlr->ctrl != &ctrlr->mproc.shm->ctrl) {
		memcpy(ctrlr->ctrl->qids, ctrlr->mproc.shm->ctrl.qids, sizeof(ctrlr->ctrl->qids));
	}

	return 0;
}

/**
 * Publish the I/O queue-id bitmap and release the admin-queue mutex
 *
 * @param ctrlr The backend controller
 */
void
xnvme_be_upcie_mproc_qids_unlock(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	if (ctrlr->mproc.shm && ctrlr->ctrl != &ctrlr->mproc.shm->ctrl) {
		memcpy(ctrlr->mproc.shm->ctrl.qids, ctrlr->ctrl->qids,
		       sizeof(ctrlr->mproc.shm->ctrl.qids));
	}

	xnvme_be_upcie_ctrlr_mutex_unlock(ctrlr);
}

/**
 * Create an I/O queue pair under the admin-queue mutex
 *
 * Allocates SQ/CQ/PRP scratch from the process-local dmamem heap, sends the
 * create-io-{cq,sq} admin commands under the shared mutex, and publishes the
 * updated qid bitmap. The caller keeps the returned offsets so it can free the
 * queue with xnvme_be_upcie_mproc_delete_io_qpair.
 *
 * @param ctrlr    Backend controller
 * @param qpair    Queue pair to create
 * @param depth    Queue depth
 * @param offsets  Filled with the heap offsets needed to delete the queue
 *
 * @return 0 on success, negative errno on failure
 */
int
xnvme_be_upcie_mproc_create_io_qpair(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_qpair *qpair,
				     uint16_t depth, struct xnvme_be_upcie_qpair_offsets *offsets)
{
	int err;

	err = xnvme_be_upcie_mproc_qids_lock(ctrlr);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_mproc_qids_lock(); err(%d)", err);
		return err;
	}

	err = nvme_controller_create_io_qpair_dmamem(ctrlr->ctrl, qpair, depth,
						     &g_upcie_rte.mem.heap, &offsets->sq,
						     &offsets->cq, &offsets->prp);

	xnvme_be_upcie_mproc_qids_unlock(ctrlr);

	return err;
}

/**
 * Delete a queue pair created by xnvme_be_upcie_mproc_create_io_qpair
 *
 * Sends the delete-io-{sq,cq} admin commands under the shared mutex, frees the
 * SQ/CQ/PRP scratch from the process-local dmamem heap using the offsets
 * returned at create time, and publishes the updated qid bitmap.
 */
void
xnvme_be_upcie_mproc_delete_io_qpair(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_qpair *qpair,
				     const struct xnvme_be_upcie_qpair_offsets *offsets)
{
	int err;

	err = xnvme_be_upcie_mproc_qids_lock(ctrlr);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_mproc_qids_lock(); err(%d)", err);
		return;
	}

	nvme_controller_delete_io_qpair_dmamem(ctrlr->ctrl, qpair, &g_upcie_rte.mem.heap,
					       offsets->sq, offsets->cq, offsets->prp);

	xnvme_be_upcie_mproc_qids_unlock(ctrlr);
}

#endif