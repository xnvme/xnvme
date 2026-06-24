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
				      g_upcie_rte.mproc->primary_hugepage, &g_upcie_rte.config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_hugepage_import(); err(%d)", err);
		free(g_upcie_rte.mproc->primary_hugepage);
		return err;
	}

	return 0;
}

void
xnvme_be_upcie_ctrlr_mutex_lock(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	struct xnvme_be_upcie_ctrlr_shm *shm = ctrlr->shm;

	if (!shm) {
		return;
	}

	pthread_mutex_lock(&shm->aq_mutex);

	ctrlr->ctrl->aq.tail = shm->ctrl.aq.tail;
	ctrlr->ctrl->aq.tail_last_written = UINT16_MAX;
	ctrlr->ctrl->aq.head = shm->ctrl.aq.head;
	ctrlr->ctrl->aq.phase = shm->ctrl.aq.phase;
}

void
xnvme_be_upcie_ctrlr_mutex_unlock(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	struct xnvme_be_upcie_ctrlr_shm *shm = ctrlr->shm;

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
xnvme_be_upcie_mproc_ctrlr_shm_term(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	if (g_upcie_rte.mproc->is_primary) {
		int attached = atomic_load(&ctrlr->shm->refcount);
		if (attached > 1) {
			XNVME_DEBUG("WARNING: terminating controller with %d secondary "
				    "process(es) still "
				    "attached",
				    attached - 1);
		}
	}

	atomic_fetch_sub(&ctrlr->shm->refcount, 1);

	if (g_upcie_rte.mproc->is_primary) {
		pthread_mutex_destroy(&ctrlr->shm->aq_mutex);
	}

	munmap(ctrlr->shm, sizeof(*ctrlr->shm));

	if (g_upcie_rte.mproc->is_primary) {
		close(ctrlr->shm_fd);
		shm_unlink(ctrlr->shm_name);
		ctrlr->ctrl = NULL; // was a pointer to shared memory, so remove this

		flock(ctrlr->lock_fd, LOCK_UN);
		close(ctrlr->lock_fd);
		unlink(ctrlr->lock_name);
	}
}

int
xnvme_be_upcie_mproc_ctrlr_shm_init(struct xnvme_dev *dev, struct xnvme_be_upcie_ctrlr *ctrlr,
				    char *driver_name)
{
	struct xnvme_be_upcie_ctrlr_shm *shm;
	char shm_name[64];
	size_t shm_size = sizeof(*shm);
	int shm_fd, lock_fd, err;

	xnvme_be_upcie_shm_bdf_name(dev->ident.uri, shm_name, sizeof(shm_name));

	/* Use flock to determine whether another primary process has claimed device */
	snprintf(ctrlr->lock_name, sizeof(ctrlr->lock_name), "/tmp%s-lock", shm_name);
	ctrlr->lock_fd = -1;

	lock_fd = open(ctrlr->lock_name, O_CREAT | O_RDWR, 0600);
	if (lock_fd < 0) {
		err = -errno;
		XNVME_DEBUG("FAILED: open() with lock_name(%s); err(%d)", ctrlr->lock_name, err);
		goto failed;
	}

	err = flock(lock_fd, LOCK_EX | LOCK_NB);
	if (err) {
		err = -errno;
		XNVME_DEBUG("FAILED: flock; err(%d)", err);
		goto failed;
	}

	ctrlr->lock_fd = lock_fd;
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
	ctrlr->shm = shm;
	ctrlr->shm_fd = shm_fd;
	snprintf(ctrlr->shm_name, sizeof(ctrlr->shm_name), "%s", shm_name);

	return 0;

failed:
	if (ctrlr->lock_fd >= 0) {
		flock(ctrlr->lock_fd, LOCK_UN);
	}

	if (lock_fd >= 0) {
		close(lock_fd);
		ctrlr->lock_fd = -1;
	}

	if (shm_fd >= 0) {
		close(shm_fd);
		shm_unlink(shm_name);
	}

	ctrlr->shm_fd = -1;
	ctrlr->shm = NULL;

	return err;
}

#endif