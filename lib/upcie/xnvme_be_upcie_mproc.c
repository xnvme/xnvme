// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_ENABLED
#include <limits.h>
#include <stdatomic.h>
#include <sys/file.h>
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
	int err;

	mproc = calloc(1, sizeof(*mproc));
	if (!mproc) {
		err = -ENOMEM;
		XNVME_DEBUG("FAILED: calloc(xnvme_be_upcie_mproc); err(%d)", err);
		return err;
	}

	mproc->is_primary = true;

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

	g_upcie_rte.mproc = mproc;

	return 0;

failed:
	if (mproc && mproc->lock_fd >= 0) {
		flock(mproc->lock_fd, LOCK_UN);
		close(mproc->lock_fd);
	}

	free(mproc);
	return err;
}

#endif