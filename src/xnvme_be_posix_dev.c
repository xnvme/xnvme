// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_POSIX_ENABLED
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <libxnvme_adm.h>
#include <libxnvme_file.h>
#include <libxnvme_spec_fs.h>
#include <xnvme_be_posix.h>
#include <xnvme_dev.h>

int
xnvme_file_oflg_to_posix(int oflags)
{
	int flags = 0;

	if (oflags & XNVME_FILE_OFLG_CREATE) {
		flags |= O_CREAT;
	}
	if (oflags & XNVME_FILE_OFLG_DIRECT_ON) {
		XNVME_DEBUG("INFO: ignoring XNVME_FILE_OFLG_DIRECT_ON");
	}
	if (oflags & XNVME_FILE_OFLG_RDONLY) {
		flags |= O_RDONLY;
	}
	if (oflags & XNVME_FILE_OFLG_WRONLY) {
		flags |= O_WRONLY;
	}
	if (oflags & XNVME_FILE_OFLG_RDWR) {
		flags |= O_RDWR;
	}
	if (oflags & XNVME_FILE_OFLG_TRUNC) {
		flags |= O_TRUNC;
	}

	return flags;
}

int
xnvme_be_posix_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_be_posix_state *state = (void *)dev->be.state;
	const struct xnvme_be_options *opts = &dev->opts;
	const struct xnvme_ident *ident = &dev->ident;
	int flags = xnvme_file_oflg_to_posix(opts->oflags);
	struct stat dev_stat = { 0 };
	int err;

	XNVME_DEBUG("INFO: open() : opts->oflags: 0x%x, flags: 0x%x, opts->mode: 0x%x",
		    opts->oflags, flags, opts->mode);

	state->fd = open(ident->trgt, flags, opts->mode);
	if (state->fd < 0) {
		XNVME_DEBUG("FAILED: open(trgt: '%s'), state->fd: '%d', errno: %d",
			    ident->trgt, state->fd, errno);
		return -errno;
	}
	err = fstat(state->fd, &dev_stat);
	if (err < 0) {
		XNVME_DEBUG("FAILED: open() : fstat() : err: %d, errno: %d", err, errno);
		return -errno;
	}

	if (!opts->provided.admin) {
		dev->be.admin = g_xnvme_be_posix_admin_shim;
	}
	if (!opts->provided.sync) {
		dev->be.sync = g_xnvme_be_posix_sync_psync;
	}
	if (!opts->provided.async) {
		dev->be.async = g_xnvme_be_posix_async_emu;
	}

	switch (dev_stat.st_mode & S_IFMT) {
	case S_IFREG:
		XNVME_DEBUG("INFO: open() : regular file");
		dev->dtype = XNVME_DEV_TYPE_FS_FILE;
		dev->csi = XNVME_SPEC_CSI_FS;
		dev->nsid = 1;
		break;

	case S_IFBLK:
		XNVME_DEBUG("INFO: open() : block-device file");
		dev->dtype = XNVME_DEV_TYPE_BLOCK_DEVICE;
		dev->csi = XNVME_SPEC_CSI_FS;
		dev->nsid = 1;
		break;

	case S_IFCHR:
		XNVME_DEBUG("FAILED: open() : char-device-file");
		dev->dtype = XNVME_DEV_TYPE_FS_FILE;
		dev->csi = XNVME_SPEC_CSI_FS;
		dev->nsid = 1;
		break;

	default:
		XNVME_DEBUG("FAILED: open() : unsupported S_IFMT: %d", dev_stat.st_mode & S_IFMT);
		return -EINVAL;
	}

	err = xnvme_be_dev_idfy(dev);
	if (err) {
		XNVME_DEBUG("FAILED: open() : xnvme_be_dev_idfy()");
		close(state->fd);
		return -EINVAL;
	}
	err = xnvme_be_dev_derive_geometry(dev);
	if (err) {
		XNVME_DEBUG("FAILED: open() : xnvme_be_dev_derive_geometry()");
		close(state->fd);
		return err;
	}

	XNVME_DEBUG("INFO: --- open() : OK ---");

	return 0;
}

void
xnvme_be_posix_dev_close(struct xnvme_dev *dev)
{
	struct xnvme_be_posix_state *state;

	if (!dev) {
		return;
	}

	state = (void *)dev->be.state;
	close(state->fd);

	memset(&dev->be, 0, sizeof(dev->be));
}

int
xnvme_be_posix_enumerate(struct xnvme_enumeration *XNVME_UNUSED(list),
			 const char *XNVME_UNUSED(sys_uri), int XNVME_UNUSED(opts))
{
	return 0;
}
#endif

struct xnvme_be_dev g_xnvme_be_posix_dev = {
#ifdef XNVME_BE_POSIX_ENABLED
	.enumerate = xnvme_be_posix_enumerate,
	.dev_open = xnvme_be_posix_dev_open,
	.dev_close = xnvme_be_posix_dev_close,
#else
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
#endif
};
