// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_FREEBSD_ENABLED
#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");
#include <sys/param.h>
#include <sys/ioccom.h>
#include <sys/stat.h>

#include <paths.h>
#include <fcntl.h>
#include <unistd.h>
#include <dev/nvme/nvme.h>
#include <errno.h>
#include <xnvme_dev.h>
#include <xnvme_be_freebsd.h>

XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd) == sizeof(struct nvme_command), "Incorrect size")
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cpl) == sizeof(struct nvme_completion),
		    "Incorrect size")

int
xnvme_be_freebsd_nvme_get_nsid_and_ctrlr_fd(int fd, uint32_t *nsid, int *ctrlr_fd)
{
	struct nvme_get_nsid get_nsid = {0};
	char ctrlr_path[1024] = {0};
	int nfd;
	int err;

	err = ioctl(fd, NVME_GET_NSID, &get_nsid);
	if (err) {
		return err < 0 ? err : -EIO;
	}

	snprintf(ctrlr_path, sizeof(ctrlr_path), _PATH_DEV "%s", get_nsid.cdev);

	*nsid = get_nsid.nsid;
	nfd = open(ctrlr_path, O_RDWR);
	if (nfd < 0) {
		XNVME_DEBUG("FAILED: open(%s), errno: %d", ctrlr_path, errno);
		return -errno;
	}

	*ctrlr_fd = nfd;

	return 0;
}

#endif
