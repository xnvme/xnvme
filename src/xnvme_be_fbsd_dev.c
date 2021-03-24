#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_FBSD_ENABLED
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <libxnvme_ident.h>
#include <libxnvme_file.h>
#include <libxnvme_spec_fs.h>
#include <xnvme_dev.h>
#include <xnvme_be_fbsd.h>
#include <xnvme_be_posix.h>

void
xnvme_be_fbsd_state_term(struct xnvme_be_fbsd_state *state)
{
	if (!state) {
		return;
	}

	close(state->fd);
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

static int
_nvme_filter(const struct dirent *d)
{
	char path[264];
	struct stat bd;
	int ctrl, ns, part;

	if (d->d_name[0] == '.') {
		return 0;
	}

	if (strstr(d->d_name, "nvme")) {
		snprintf(path, sizeof(path), "%s%s", _PATH_DEV, d->d_name);
		if (stat(path, &bd)) {
			XNVME_DEBUG("cannot stat");
			return 0;
		}
		if (sscanf(d->d_name, "nvme%dn%dp%d", &ctrl, &ns, &part) == 3) {
			// Do not want to use e.g. nvme0n1p2 etc.
			XNVME_DEBUG("matches too much");
			return 0;
		}

		return 1;
	}

	return 0;
}

int
xnvme_file_oflg_to_fbsd(int oflags)
{
	int flags = 0;

	if (oflags & XNVME_FILE_OFLG_CREATE) {
		flags |= O_CREAT;
	}
	if (oflags & XNVME_FILE_OFLG_DIRECT_ON) {
		//flags |= O_DIRECT;
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
xnvme_be_fbsd_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_be_posix_state *state = (void *)dev->be.state;
	const struct xnvme_be_options *opts = &dev->opts;
	const struct xnvme_ident *ident = &dev->ident;
	int flags = xnvme_file_oflg_to_fbsd(opts->oflags);
	struct stat dev_stat = { 0 };
	int err;

	XNVME_DEBUG("INFO: open() : opts->oflags: 0x%x, flags: 0x%x, opts->mode: 0x%x",
		    opts->oflags, flags, opts->mode);

	state->fd = open(ident->trgt, flags, opts->mode);
	if (state->fd < 0) {
		XNVME_DEBUG("FAILED: open(trgt: '%s'), state->fd: '%d', errno: %d",
			    dev->ident.trgt, state->fd, errno);
		return -errno;
	}
	err = fstat(state->fd, &dev_stat);
	if (err < 0) {
		XNVME_DEBUG("FAILED: err: %d, errno: %d", err, errno);
		return -errno;
	}

	switch (dev_stat.st_mode & S_IFMT) {
	case S_IFREG:
		XNVME_DEBUG("INFO: open() : regular file");
		dev->dtype = XNVME_DEV_TYPE_FS_FILE;
		dev->csi = XNVME_SPEC_CSI_FS;
		dev->nsid = 1;
		if (!opts->provided.admin) {
			dev->be.admin = g_xnvme_be_posix_admin_shim;
		}
		if (!opts->provided.sync) {
			dev->be.sync = g_xnvme_be_posix_sync_psync;
		}
		if (!opts->provided.async) {
			dev->be.async = g_xnvme_be_posix_async_emu;
		}
		break;

	case S_IFBLK:
		XNVME_DEBUG("INFO: open() : block-device file");
		dev->dtype = XNVME_DEV_TYPE_BLOCK_DEVICE;
		dev->csi = XNVME_SPEC_CSI_FS;
		dev->nsid = 1;
		if (!opts->provided.admin) {
			dev->be.admin = g_xnvme_be_posix_admin_shim;
		}
		if (!opts->provided.sync) {
			dev->be.sync = g_xnvme_be_posix_sync_psync;
		}
		if (!opts->provided.async) {
			dev->be.async = g_xnvme_be_posix_async_emu;
		}
		break;

	case S_IFCHR:
		XNVME_DEBUG("FAILED: open() : char-device-file: no support under POSIX");
		close(state->fd);
		return -ENOSYS;

	default:
		XNVME_DEBUG("FAILED: open() : unsupported S_IFMT: %d", dev_stat.st_mode & S_IFMT);
		return -EINVAL;
	}

	err = xnvme_be_dev_idfy(dev);
	if (err) {
		XNVME_DEBUG("FAILED: open() : xnvme_be_dev_idfy");
		return -EINVAL;
	}
	err = xnvme_be_dev_derive_geometry(dev);
	if (err) {
		XNVME_DEBUG("FAILED: open() : xnvme_be_dev_derive_geometry()");
		return err;
	}

	XNVME_DEBUG("INFO: --- open() : OK ---");

	return 0;
}

int
xnvme_be_fbsd_enumerate(struct xnvme_enumeration *list, const char *sys_uri,
			int XNVME_UNUSED(opts))
{
	struct dirent **dent = NULL;
	int ndev = 0;

	if (sys_uri) {
		XNVME_DEBUG("FAILED: sys_uri: %s is not supported", sys_uri);
		return -ENOSYS;
	}

	ndev = scandir(_PATH_DEV, &dent, _nvme_filter, alphasort);

	for (int di = 0; di < ndev; ++di) {
		char uri[XNVME_IDENT_URI_LEN] = { 0 };
		struct xnvme_ident ident = { 0 };

		snprintf(uri, XNVME_IDENT_URI_LEN - 1,
			 "file:" _PATH_DEV "%s",
			 dent[di]->d_name);
		if (xnvme_ident_from_uri(uri, &ident)) {
			XNVME_DEBUG("uri: '%s'", uri);
			continue;
		}

		if (xnvme_enumeration_append(list, &ident)) {
			XNVME_DEBUG("FAILED: adding ident");
		}
	}

	for (int di = 0; di < ndev; ++di) {
		free(dent[di]);
	}
	free(dent);

	return 0;
}
#endif

struct xnvme_be_dev g_xnvme_be_fbsd_dev = {
#ifdef XNVME_BE_FBSD_ENABLED
	.enumerate = xnvme_be_fbsd_enumerate,
	.dev_open = xnvme_be_fbsd_dev_open,
	.dev_close = xnvme_be_fbsd_dev_close,
#else
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
#endif
};
