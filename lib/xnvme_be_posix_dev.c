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
xnvme_file_opts_to_posix(struct xnvme_opts *opts)
{
	int flags = 0;

	flags |= opts->create ? O_CREAT : 0x0;
	if (opts->direct) {
		XNVME_DEBUG("INFO: ignoring opts.direct; ENOSYS");
	}
	flags |= opts->rdonly ? O_RDONLY : 0x0;
	flags |= opts->wronly ? O_WRONLY : 0x0;
	flags |= opts->rdwr ? O_RDWR : 0x0;
	flags |= opts->truncate ? O_TRUNC : 0x0;

	return flags;
}

int
xnvme_be_posix_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_be_posix_state *state = (void *)dev->be.state;
	const struct xnvme_ident *ident = &dev->ident;
	struct xnvme_opts *opts = &dev->opts;
	int flags = xnvme_file_opts_to_posix(opts);
	struct stat dev_stat = {0};
	int err;

	XNVME_DEBUG("INFO: open() : opts->oflags: 0x%x, flags: 0x%x, opts->create_mode: 0x%x",
		    opts->oflags, flags, opts->create_mode);

	state->fd = open(ident->uri, flags, opts->create_mode);
	if (state->fd < 0) {
		XNVME_DEBUG("FAILED: open(uri: '%s'), state->fd: '%d', errno: %d", ident->uri,
			    state->fd, errno);
		return -errno;
	}
	err = fstat(state->fd, &dev_stat);
	if (err < 0) {
		XNVME_DEBUG("FAILED: open() : fstat() : err: %d, errno: %d", err, errno);
		return -errno;
	}

	if (!opts->admin) {
		dev->be.admin = g_xnvme_be_posix_admin_shim;
	}
	if (!opts->sync) {
		dev->be.sync = g_xnvme_be_posix_sync_psync;
	}
	if (!opts->async) {
		dev->be.async = g_xnvme_be_posix_async_emu;
	}

	switch (dev_stat.st_mode & S_IFMT) {
	case S_IFREG:
		XNVME_DEBUG("INFO: open() : regular file");
		dev->ident.dtype = XNVME_DEV_TYPE_FS_FILE;
		dev->ident.csi = XNVME_SPEC_CSI_FS;
		dev->ident.nsid = 1;
		break;

	case S_IFBLK:
		XNVME_DEBUG("INFO: open() : block-device file");
		dev->ident.dtype = XNVME_DEV_TYPE_BLOCK_DEVICE;
		dev->ident.csi = XNVME_SPEC_CSI_FS;
		dev->ident.nsid = 1;
		break;

	case S_IFCHR:
		XNVME_DEBUG("FAILED: open() : char-device-file");
		dev->ident.dtype = XNVME_DEV_TYPE_FS_FILE;
		dev->ident.csi = XNVME_SPEC_CSI_FS;
		dev->ident.nsid = 1;
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
xnvme_be_posix_enumerate(const char *XNVME_UNUSED(sys_uri), struct xnvme_opts *XNVME_UNUSED(opts),
			 xnvme_enumerate_cb XNVME_UNUSED(cb_func), void *XNVME_UNUSED(cb_args))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
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
