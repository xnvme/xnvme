// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/* Also needed by the stubs below, which report -ENOSYS where multi-process
 * mode is unavailable; only Linux picks it up transitively. */
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_ENABLED
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <xnvme_dev.h>
#include <xnvme_be_upcie.h>

/**
 * Take, test, or drop the runtime role-election lock
 *
 * OFD locks rather than flock(), because only fcntl() can ask whether a lock
 * is held without taking it first. That matters for more than tidiness: a
 * probe that took the lock, even for the microseconds between acquiring and
 * releasing it, would make a process electing at that instant see the runtime
 * as claimed and demote itself to secondary, where it waits for a primary that
 * never arrives. Testing has to be non-destructive or the probe changes what
 * it observes.
 *
 * Both of the runtime's locks use this, the per-runtime one that elects the
 * role and the per-controller one that says who owns a device, so there is one
 * locking mechanism to reason about rather than two with different semantics.
 *
 * Returns 1 when F_OFD_GETLK finds the lock held, 0 on success, negative errno
 * on failure. The lock is released when the descriptor is closed.
 */
static int
_rte_lock_op(int fd, int cmd, short type)
{
	struct flock fl = {
		.l_type = type,
		.l_whence = SEEK_SET,
		.l_start = 0,
		.l_len = 0,
	};

	if (fcntl(fd, cmd, &fl)) {
		return -errno;
	}

	return ((cmd == F_OFD_GETLK) && (fl.l_type != F_UNLCK)) ? 1 : 0;
}

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

	/* Role election, decided by an OFD lock so that a probe can ask who
	 * holds it without taking it. The lock is released when the descriptor
	 * closes, including when the process dies without cleaning up, which is
	 * what lets a killed primary be told apart from a live one.
	 */

	snprintf(mproc->lock_name, sizeof(mproc->lock_name), XNVME_BE_UPCIE_RTE_LOCK_FMT, shm_id);
	mproc->lock_fd = open(mproc->lock_name, O_CREAT | O_RDWR, 0600);
	if (mproc->lock_fd < 0) {
		err = -errno;
		XNVME_DEBUG("FAILED: open() with lock_name(%s); err(%d)", mproc->lock_name, err);
		goto failed;
	}

	err = _rte_lock_op(mproc->lock_fd, F_OFD_SETLK, F_WRLCK);
	if (err) {
		/* A contended lock is reported as either, depending on libc. */
		if ((err == -EAGAIN) || (err == -EACCES)) {
			XNVME_DEBUG("INFO: Lock file already claimed, setting role to secondary");
			close(mproc->lock_fd);
			mproc->lock_fd = -1;
			mproc->is_primary = false;
			err = 0;
			errno = 0;
		} else {
			XNVME_DEBUG("FAILED: claiming lock_name(%s); err(%d)", mproc->lock_name,
				    err);
			goto failed;
		}
	}

	/* Map shared memory for hugepage information */

	snprintf(mproc->shm_name, sizeof(mproc->shm_name), XNVME_BE_UPCIE_RTE_SHM_FMT, shm_id);
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
		mproc->shm->magic = XNVME_BE_UPCIE_SHM_MAGIC;
		mproc->shm->version = XNVME_BE_UPCIE_SHM_VERSION;
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

	/* Counted in a wider type than the qid itself: the bitmap spans
	 * NVME_QID_BITMAP_WORDS * BITS_PER_WORD entries, which can exceed what
	 * a uint16_t holds, and such a counter would then never reach the bound
	 * and would wrap instead. Stop at the highest qid the allocator admits,
	 * which is what the bitmap exists to track. */
	for (uint32_t qid = 1; qid < NVME_QID_MAX; ++qid) {
		if (nvme_qid_is_allocated(shm->ctrl.qids, (uint16_t)qid)) {
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

/**
 * Record that this primary holds `uri`, so a probe can enumerate the group
 *
 * Only the primary reaches here, from the path that opens a controller, so the
 * write needs no lock beyond the one already excluding other primaries: the
 * runtime is a process-wide global and opening devices from several threads at
 * once is unsupported for reasons older than this.
 */
static void
_ctrlr_register(const char *uri)
{
	struct xnvme_be_upcie_mproc_shm *shm;
	uint32_t nctrlrs;

	if (!g_upcie_rte.mproc || !g_upcie_rte.mproc->shm) {
		return;
	}
	shm = g_upcie_rte.mproc->shm;

	/* Counted whether or not it fits, so a probe can say how many are held
	 * rather than silently showing a truncated list as the whole of it. */
	shm->nctrlrs_held++;

	nctrlrs = atomic_load_explicit(&shm->nctrlrs, memory_order_relaxed);
	if (nctrlrs >= XNVME_MPROC_MAX_CTRLRS) {
		XNVME_DEBUG("FAILED: no room to record '%s'; probes will not list it", uri);
		return;
	}

	snprintf(shm->ctrlrs[nctrlrs], sizeof(shm->ctrlrs[0]), "%s", uri);

	/* Released after the bytes it covers, so a reader that sees the count
	 * sees the URI too. */
	atomic_store_explicit(&shm->nctrlrs, nctrlrs + 1, memory_order_release);
}

/**
 * Drop the record for `uri`, compacting so the list stays contiguous
 */
static void
_ctrlr_unregister(const char *uri)
{
	struct xnvme_be_upcie_mproc_shm *shm;
	uint32_t nctrlrs;

	if (!g_upcie_rte.mproc || !g_upcie_rte.mproc->shm) {
		return;
	}
	shm = g_upcie_rte.mproc->shm;

	if (shm->nctrlrs_held) {
		shm->nctrlrs_held--;
	}

	nctrlrs = atomic_load_explicit(&shm->nctrlrs, memory_order_relaxed);

	for (uint32_t i = 0; i < nctrlrs; ++i) {
		if (strcmp(shm->ctrlrs[i], uri)) {
			continue;
		}

		/* Shrunk before the bytes move, so a reader never counts a slot
		 * that is being overwritten as it reads. */
		atomic_store_explicit(&shm->nctrlrs, nctrlrs - 1, memory_order_release);

		if (i != (nctrlrs - 1)) {
			memcpy(shm->ctrlrs[i], shm->ctrlrs[nctrlrs - 1], sizeof(shm->ctrlrs[0]));
		}
		memset(shm->ctrlrs[nctrlrs - 1], 0, sizeof(shm->ctrlrs[0]));
		return;
	}
}

void
xnvme_be_upcie_mproc_ctrlr_shm_term(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	if (g_upcie_rte.mproc->is_primary) {
		_ctrlr_unregister(ctrlr->uri);
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

	/* Whether another primary has claimed this device, on the same kind of
	 * lock the runtime elects with. */
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

	err = _rte_lock_op(lock_fd, F_OFD_SETLK, F_WRLCK);
	if (err) {
		XNVME_DEBUG("FAILED: claiming lock_name(%s); err(%d)", ctrlr->mproc.lock_name,
			    err);
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
	shm->magic = XNVME_BE_UPCIE_SHM_MAGIC;
	shm->version = XNVME_BE_UPCIE_SHM_VERSION;
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
	snprintf(ctrlr->uri, sizeof(ctrlr->uri), "%s", dev->ident.uri);
	_ctrlr_register(ctrlr->uri);

	return 0;

failed:
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

/**
 * Refuse a segment this build cannot read
 *
 * The size check that precedes this catches a segment shorter than what gets
 * mapped, but not one whose layout differs at the same size or larger; that
 * one is read at this build's offsets and quietly misreported. The stamp is
 * what makes the refusal reliable. A zeroed stamp is a segment mid-creation
 * rather than a foreign one, so it is worth retrying instead of reporting.
 */
static int
_shm_stamp_check(uint32_t magic, uint32_t version)
{
	if (!magic) {
		return -EAGAIN;
	}

	if ((magic != XNVME_BE_UPCIE_SHM_MAGIC) || (version != XNVME_BE_UPCIE_SHM_VERSION)) {
		return -EPROTO;
	}

	return 0;
}

int
xnvme_mproc_primary_alive(uint32_t shm_id)
{
	char path[64];
	int fd, held;

	snprintf(path, sizeof(path), XNVME_BE_UPCIE_RTE_LOCK_FMT, (int)shm_id);

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		return (errno == ENOENT) ? 0 : -errno;
	}

	held = _rte_lock_op(fd, F_OFD_GETLK, F_WRLCK);
	close(fd);

	return held;
}

/**
 * Read the runtime segment for `shm_id` without attaching to it
 *
 * Same discipline as xnvme_mproc_get_ctrlr_info(): mapped read-only, no lock, a
 * snapshot that may be a moment stale.
 */
int
xnvme_mproc_get_info(uint32_t shm_id, struct xnvme_mproc_info *info)
{
	struct xnvme_be_upcie_mproc_shm *shm;
	char shm_name[64];
	struct stat st;
	uint32_t nctrlrs;
	int shm_fd, err = 0;

	if (!info) {
		return -EINVAL;
	}

	snprintf(shm_name, sizeof(shm_name), XNVME_BE_UPCIE_RTE_SHM_FMT, (int)shm_id);

	shm_fd = shm_open(shm_name, O_RDONLY, 0);
	if (shm_fd < 0) {
		return -errno;
	}

	if (fstat(shm_fd, &st)) {
		err = -errno;
		close(shm_fd);
		return err;
	}
	if ((size_t)st.st_size < sizeof(*shm)) {
		UPCIE_DEBUG("FAILED: segment(%s) is %zu bytes, expected at least %zu", shm_name,
			    (size_t)st.st_size, sizeof(*shm));
		close(shm_fd);
		return st.st_size ? -EPROTO : -EAGAIN;
	}

	shm = mmap(NULL, sizeof(*shm), PROT_READ, MAP_SHARED, shm_fd, 0);
	close(shm_fd);
	if (shm == MAP_FAILED) {
		return -errno;
	}

	err = _shm_stamp_check(shm->magic, shm->version);
	if (err) {
		UPCIE_DEBUG("FAILED: segment(%s) magic(0x%08x) version(%u); err(%d)", shm_name,
			    shm->magic, shm->version, err);
		munmap(shm, sizeof(*shm));
		return err;
	}

	memset(info, 0, sizeof(*info));
	info->nattached = (uint32_t)atomic_load(&shm->refcount);
	nctrlrs = atomic_load_explicit(&shm->nctrlrs, memory_order_acquire);
	info->nctrlrs = nctrlrs > XNVME_MPROC_MAX_CTRLRS ? XNVME_MPROC_MAX_CTRLRS : nctrlrs;
	info->nctrlrs_held = shm->nctrlrs_held;
	for (uint32_t i = 0; i < info->nctrlrs; ++i) {
		snprintf(info->ctrlrs[i], XNVME_IDENT_URI_LEN, "%s", shm->ctrlrs[i]);
	}

	munmap(shm, sizeof(*shm));

	return err;
}

/**
 * Read the shared controller segment without attaching to it
 *
 * Maps the segment read-only and reads a snapshot. No lock is taken: the
 * counts are advisory, and a monitoring caller that blocked the processes it
 * is monitoring would be worse than one reporting a value a moment stale.
 */
int
xnvme_mproc_get_ctrlr_info(const char *uri, struct xnvme_mproc_ctrlr_info *info)
{
	struct xnvme_be_upcie_ctrlr_shm *shm;
	char shm_name[64];
	struct stat st;
	uint32_t qids_used;
	int shm_fd, err = 0;

	if (!uri || !info) {
		return -EINVAL;
	}

	xnvme_be_upcie_shm_bdf_name(uri, shm_name, sizeof(shm_name));

	shm_fd = shm_open(shm_name, O_RDONLY, 0);
	if (shm_fd < 0) {
		return -errno;
	}

	/* A segment written by a build with a different layout can be shorter
	 * than what is mapped here, and reading past its end faults rather than
	 * failing. Refuse it instead: a stale segment is a mismatch to report,
	 * not a crash to take. */
	if (fstat(shm_fd, &st)) {
		err = -errno;
		close(shm_fd);
		return err;
	}
	if ((size_t)st.st_size < sizeof(*shm)) {
		UPCIE_DEBUG("FAILED: segment(%s) is %zu bytes, expected at least %zu", shm_name,
			    (size_t)st.st_size, sizeof(*shm));
		close(shm_fd);
		return st.st_size ? -EPROTO : -EAGAIN;
	}

	shm = mmap(NULL, sizeof(*shm), PROT_READ, MAP_SHARED, shm_fd, 0);
	close(shm_fd);
	if (shm == MAP_FAILED) {
		return -errno;
	}

	err = _shm_stamp_check(shm->magic, shm->version);
	if (err) {
		UPCIE_DEBUG("FAILED: segment(%s) magic(0x%08x) version(%u); err(%d)", shm_name,
			    shm->magic, shm->version, err);
		munmap(shm, sizeof(*shm));
		return err;
	}

	memset(info, 0, sizeof(*info));
	info->nattached = (uint32_t)atomic_load(&shm->refcount);
	info->initialized = atomic_load(&shm->is_initialized) ? 1 : 0;

	/* The bitmap is all zeroes until the primary marks the admin queue in
	 * use, a window this call can land in and reports as uninitialized.
	 * Subtracting the admin queue unconditionally would wrap it to 4G. */
	qids_used = nvme_qid_used(shm->ctrl.qids);
	info->nsq_used = qids_used ? qids_used - 1 : 0;
	info->ncq_used = info->nsq_used;
	info->nsq_total = shm->nsq_max;
	info->ncq_total = shm->ncq_max;

	munmap(shm, sizeof(*shm));

	return err;
}

#else

int
xnvme_mproc_primary_alive(uint32_t XNVME_UNUSED(shm_id))
{
	return -ENOSYS;
}

int
xnvme_mproc_get_info(uint32_t XNVME_UNUSED(shm_id), struct xnvme_mproc_info *XNVME_UNUSED(info))
{
	return -ENOSYS;
}

int
xnvme_mproc_get_ctrlr_info(const char *XNVME_UNUSED(uri),
			   struct xnvme_mproc_ctrlr_info *XNVME_UNUSED(info))
{
	return -ENOSYS;
}

#endif
