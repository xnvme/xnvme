// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libxnvme.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_LINUX_BLOCK_ENABLED
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <xnvme_be_posix.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_state.h>

#ifdef XNVME_BE_LINUX_BLOCK_ZONED_ENABLED
#include <linux/blkzoned.h>
#endif

#ifdef BLKGETNRZONES
int
xnvme_be_linux_block_nrzones(struct xnvme_dev *dev, uint64_t *nr_zones)
{
	struct xnvme_be_linux_state *state = &xnvme_be_state(dev)->linux_state;
	int err;

	err = ioctl(state->posix.fd, BLKGETNRZONES, nr_zones);
	if (err) {
		XNVME_DEBUG("FAILED: ioctl(BLKGETNRZONES), err: %d, errno: %d", err, errno);
		return err;
	}
	XNVME_DEBUG("INFO: got nr_zones: %zu", *nr_zones);

	return 0;
}
#else
int
xnvme_be_linux_block_nrzones(struct xnvme_dev *XNVME_UNUSED(dev), uint64_t *XNVME_UNUSED(nr_zones))
{
	XNVME_DEBUG("FAILED: nosys ioctl(BLKGETNRZONES)");
	return -ENOSYS;
}
#endif

#endif
