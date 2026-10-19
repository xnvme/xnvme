// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_LINUX_ENABLED
#include <errno.h>
#include <dirent.h>
#include <paths.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <xnvme_dev.h>
#include <xnvme_be_cbi.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_nvme.h>

static inline void
_be_linux_state_term(struct xnvme_be_linux_state *state)
{
	if (!state) {
		return;
	}

	close(state->fd);
	state->fd = 0;
}

int
xnvme_be_linux_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_be_linux_state *state = (void *)dev->be.state;
	const struct xnvme_ident *ident = &dev->ident;
	struct xnvme_opts *opts = &dev->opts;
	struct stat dev_stat;
	int flags;
	int err;

	flags = xnvme_file_opts_to_linux(opts);

	XNVME_DEBUG("INFO: open() : flags: 0x%x, opts->create_mode: 0x%x", flags,
		    opts->create_mode);

	state->fd = open(ident->uri, flags, opts->create_mode);
	if (state->fd < 0) {
		XNVME_DEBUG("FAILED: open(uri: '%s') : state->fd: '%d', errno: %d", ident->uri,
			    state->fd, errno);

		opts->direct = 0;
		flags = xnvme_file_opts_to_linux(opts);
		state->fd = open(ident->uri, flags, opts->create_mode);
		if (state->fd < 0) {
			XNVME_DEBUG("FAILED: open(uri: '%s') : state->fd: '%d', errno: %d",
				    ident->uri, state->fd, errno);
			return -errno;
		}
	}

	err = fstat(state->fd, &dev_stat);
	if (err < 0) {
		XNVME_DEBUG("FAILED: open() : fstat() : err: %d, errno: %d", err, errno);
		return -errno;
	}

	// Refine identifier fields based on the underlying file type
	switch (dev_stat.st_mode & S_IFMT) {
	case S_IFREG:
		XNVME_DEBUG("INFO: open() : regular file");
		dev->ident.dtype = XNVME_DEV_TYPE_FS_FILE;
		dev->ident.csi = XNVME_SPEC_CSI_FS;
		dev->ident.nsid = 1;
		break;

	case S_IFBLK:
		dev->ident.dtype = XNVME_DEV_TYPE_BLOCK_DEVICE;
		dev->ident.csi = XNVME_SPEC_CSI_FS;
		dev->ident.nsid = 1;
		XNVME_DEBUG("INFO: open() : block-device file");

		err = xnvme_be_linux_nvme_dev_nsid(dev);
		if (err < 1) {
			XNVME_DEBUG("INFO: open() : retrieving nsid, got: {nsid: %x, err: %d}",
				    err, err);
			break;
		}
		dev->ident.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
		dev->ident.csi = XNVME_SPEC_CSI_NVM;
		dev->ident.nsid = err;
		XNVME_DEBUG("INFO: open() : block-device file (is a NVMe namespace)");

		break;

	case S_IFCHR:
		XNVME_DEBUG("INFO: open() : char-device-file");

		err = xnvme_be_linux_nvme_dev_nsid(dev);
		XNVME_DEBUG("INFO: open() : retrieving nsid, got: %x", err);
		if ((uint32_t)err ==
		    0xFFFFFFFF) { ///< Assuming a /dev/nvme0 -- nvme-controller handle
			dev->ident.dtype = XNVME_DEV_TYPE_NVME_CONTROLLER;
			dev->ident.csi = 0x0;
			dev->ident.nsid = err;
		} else if (err < 1) { ///< Assuming not a NVMe device
			return -EINVAL;
		} else { ///< Assuming a /dev/ngXnY -- nvme-namespace handle
			dev->ident.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
			dev->ident.csi = XNVME_SPEC_CSI_NVM;
			dev->ident.nsid = err;
		}

		break;

	default:
		XNVME_DEBUG("FAILED: open() : unsupported S_IFMT: %d", dev_stat.st_mode & S_IFMT);
		return -EINVAL;
	}

	state->poll_io = opts->poll_io;
	state->poll_sq = opts->poll_sq;

	XNVME_DEBUG("INFO: open() : dev->state.poll_io: %d", state->poll_io);
	XNVME_DEBUG("INFO: open() : dev->state.poll_sq: %d", state->poll_sq);

	XNVME_DEBUG("INFO: --- open() : OK ---");

	return 0;
}

void
xnvme_be_linux_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	_be_linux_state_term((void *)dev->be.state);
	memset(&dev->be, 0, sizeof(dev->be));
}

#endif

struct xnvme_be_dev g_xnvme_be_dev_linux = {
#ifdef XNVME_PLATFORM_LINUX_ENABLED
	.dev_open = xnvme_be_linux_dev_open,
	.dev_close = xnvme_be_linux_dev_close,
	.id = "linux",
#else
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
	.id = "nosys",
#endif
};
