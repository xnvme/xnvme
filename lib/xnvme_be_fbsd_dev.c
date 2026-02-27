// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_FREEBSD_ENABLED
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <xnvme_dev.h>
#include <xnvme_be_cbi.h>
#include <xnvme_be_fbsd.h>

void
xnvme_be_fbsd_state_term(struct xnvme_be_fbsd_state *state)
{
	if (!state) {
		return;
	}

	if (state->fd.ns >= 0) {
		close(state->fd.ns);
	}
	if ((state->fd.ctrlr >= 0) && (state->fd.ctrlr != state->fd.ns)) {
		close(state->fd.ctrlr);
	}
}

void
xnvme_be_fbsd_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	xnvme_be_fbsd_state_term((void *)dev->be.state);
	memset(&dev->be, 0, sizeof(dev->be));
}

int
xnvme_file_opts_to_fbsd(struct xnvme_opts *opts)
{
	int flags = 0;

	flags |= opts->create ? O_CREAT : 0x0;
	flags |= opts->direct ? O_DIRECT : 0x0;
	flags |= opts->rdonly ? O_RDONLY : 0x0;
	flags |= opts->wronly ? O_WRONLY : 0x0;
	flags |= opts->rdwr ? O_RDWR : 0x0;
	flags |= opts->truncate ? O_TRUNC : 0x0;

	return flags;
}

int
xnvme_be_fbsd_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_be_fbsd_state *state = (void *)dev->be.state;
	const struct xnvme_ident *ident = &dev->ident;
	struct xnvme_opts *opts = &dev->opts;
	int flags = xnvme_file_opts_to_fbsd(opts);
	struct stat dev_stat = {0};
	int err;

	XNVME_DEBUG("INFO: open() : flags: 0x%x, opts->create_mode: 0x%x", flags,
		    opts->create_mode);

	state->fd.ns = -1;
	state->fd.ctrlr = -1;

	state->fd.ns = open(ident->uri, flags, opts->create_mode);
	if (state->fd.ns < 0) {
		XNVME_DEBUG("FAILED: open(uri: '%s'), state->fd.ns: '%d', errno: %d",
			    dev->ident.uri, state->fd.ns, errno);
		return -errno;
	}
	err = fstat(state->fd.ns, &dev_stat);
	if (err < 0) {
		XNVME_DEBUG("FAILED: err: %d, errno: %d", err, errno);
		return -errno;
	}

	switch (dev_stat.st_mode & S_IFMT) {
	case S_IFREG:
		XNVME_DEBUG("INFO: open() : regular file");
		dev->ident.dtype = XNVME_DEV_TYPE_FS_FILE;
		dev->ident.csi = XNVME_SPEC_CSI_FS;
		dev->ident.nsid = 1;
		if (!opts->admin) {
			dev->be.admin = g_xnvme_be_cbi_admin_shim;
		}
		if (!opts->sync) {
			dev->be.sync = g_xnvme_be_cbi_sync_psync;
		}
		if (!opts->async) {
			dev->be.async = g_xnvme_be_cbi_async_emu;
		}
		state->fd.ctrlr = state->fd.ns;
		break;

	case S_IFBLK:
		XNVME_DEBUG("INFO: open() : block-device file");
		dev->ident.dtype = XNVME_DEV_TYPE_BLOCK_DEVICE;
		dev->ident.csi = XNVME_SPEC_CSI_FS;
		dev->ident.nsid = 1;
		if (!opts->admin) {
			dev->be.admin = g_xnvme_be_cbi_admin_shim;
		}
		if (!opts->sync) {
			dev->be.sync = g_xnvme_be_cbi_sync_psync;
		}
		if (!opts->async) {
			dev->be.async = g_xnvme_be_cbi_async_emu;
		}
		state->fd.ctrlr = state->fd.ns;
		break;

	case S_IFCHR:
		XNVME_DEBUG("INFO: open() : char-device-file assuming NVMe ctrlr. or ns.");

		if (!opts->admin) {
			dev->be.admin = g_xnvme_be_fbsd_admin_nvme;
		}
		if (!opts->sync) {
			dev->be.sync = g_xnvme_be_fbsd_sync_nvme;
		}
		if (!opts->async) {
			dev->be.async = g_xnvme_be_cbi_async_emu;
		}

		if (xnvme_be_fbsd_nvme_get_nsid_and_ctrlr_fd(state->fd.ns, &dev->ident.nsid,
							     &state->fd.ctrlr)) {
			XNVME_DEBUG("INFO: open() : assuming it is an NVMe controller");
			dev->ident.dtype = XNVME_DEV_TYPE_NVME_CONTROLLER;
			dev->ident.csi = XNVME_SPEC_CSI_NVM;
			dev->ident.nsid = 0;
			state->fd.ctrlr = state->fd.ns;
		}

		XNVME_DEBUG("INFO: open() : responds like an NVMe namespace");
		dev->ident.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
		dev->ident.csi = XNVME_SPEC_CSI_NVM;
		break;

	default:
		XNVME_DEBUG("FAILED: open() : unsupported S_IFMT: %d", dev_stat.st_mode & S_IFMT);
		return -EINVAL;
	}

	state->poll_io = opts->poll_io;

	XNVME_DEBUG("INFO: open() : dev->state.poll_io: %d", state->poll_io);
	XNVME_DEBUG("INFO: --- open() : OK ---");

	return 0;
}
#endif

struct xnvme_be_dev g_xnvme_be_fbsd_dev = {
#ifdef XNVME_PLATFORM_FREEBSD_ENABLED
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_fbsd_dev_open,
	.dev_close = xnvme_be_fbsd_dev_close,
	.id = "freebsd",
#else
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
	.id = "nosys",
#endif
};
