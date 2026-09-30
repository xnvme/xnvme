// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * A CUSE (character device in userspace) session answering the Linux
 * kernel NVMe driver's ioctl() interface for a single xNVMe device;
 * internal to xNVMe, shared by qublk (one session per block device it
 * serves) and homi (one per controller it holds open). Not part of the
 * public API.
 *
 * CUSE is a mode of the kernel's FUSE subsystem, not a separate mechanism:
 * a CUSE session speaks the same /dev/fuse request/reply wire protocol as
 * a filesystem session, just entered via /dev/cuse and backing one
 * character device instead of a mounted tree. This talks that protocol
 * directly (linux/fuse.h has every struct involved) rather than linking
 * libfuse3.
 *
 * @file xnvme_cuse.h
 */
#ifndef __INTERNAL_XNVME_CUSE_H
#define __INTERNAL_XNVME_CUSE_H

#include <pthread.h>
#include <signal.h>

#include <libxnvme.h>

/**
 * State for one CUSE session; zero-initialize before passing to
 * ::xnvme_cuse_start
 */
struct xnvme_cuse {
	pthread_t tid;
	int init_rc;
	volatile sig_atomic_t stop;
};

/**
 * Start a CUSE session mimicking the Linux kernel NVMe driver's ioctl()
 * interface (NVME_IOCTL_ID, NVME_IOCTL_ADMIN_CMD) for 'dev', creating
 * /dev/<name>
 *
 * Returns once the device is up or has failed to come up; does not block
 * waiting for it to stop. Tear it down with ::xnvme_cuse_stop even on
 * failure.
 *
 * @param cuse Session state, zero-initialized by the caller
 * @param dev The device to serve; must already be open and outlive the
 * session
 * @param name The character device's name; created as /dev/<name>, so the
 * caller is responsible for picking one that cannot collide with another
 * device already present or being served
 *
 * @return 0 on success, negative errno on error. Two errors mean there is
 * nothing to serve rather than a failure: -ENOSYS when xNVMe was built
 * without the CUSE mimic, and -ENOTSUP when 'dev' is not driven by one of
 * xNVMe's user-space NVMe drivers (SPDK, libvfn, uPCIe)
 */
int
xnvme_cuse_start(struct xnvme_cuse *cuse, struct xnvme_dev *dev, const char *name);

/**
 * Signal the CUSE session started by ::xnvme_cuse_start to exit and join its
 * thread
 *
 * Safe to call on a 'cuse' for which ::xnvme_cuse_start was never called, or
 * failed.
 *
 * @param cuse The session, as passed to ::xnvme_cuse_start
 */
void
xnvme_cuse_stop(struct xnvme_cuse *cuse);

#endif
