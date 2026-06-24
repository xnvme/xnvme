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

#endif