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

#ifdef XNVME_BE_LINUX_BLOCK_ZONED_ENABLED
#include <linux/blkzoned.h>
#endif

static int
_sysfs_path_to_buf(const char *path, char *buf, int buf_len)
{
	FILE *fp;
	int c;

	fp = fopen(path, "rb");
	if (!fp) {
		return -errno;
	}

	memset(buf, 0, sizeof(char) * buf_len);
	for (int i = 0; (((c = getc(fp)) != EOF) && i < buf_len); ++i) {
		buf[i] = c;
	}

	fclose(fp);

	return 0;
}

static const char *
xnvme_path_basename(const char *path)
{
	const char *bname = strrchr(path, '/');

	if (bname && (strlen(bname) > 1)) {
		return bname + 1;
	}

	return NULL;
}

int
xnvme_be_linux_sysfs_dev_attr_to_buf(struct xnvme_dev *dev, const char *attr, char *buf,
				     int buf_len)
{
	int path_len = 0x1000;
	char path[path_len];
	const char *bname;

	bname = xnvme_path_basename(dev->ident.uri);
	if (!bname) {
		XNVME_DEBUG("FAILED: xnvme_path_basename('%s')", dev->ident.uri);
		return -EINVAL;
	}

	sprintf(path, "/sys/block/%s/%s", bname, attr);

	return _sysfs_path_to_buf(path, buf, buf_len);
}

int
xnvme_be_linux_sysfs_dev_attr_to_num(struct xnvme_dev *dev, const char *attr, uint64_t *num)
{
	const int buf_len = 0x1000;
	char buf[0x1000] = {'\0'};
	int base = 10;
	int err;

	err = xnvme_be_linux_sysfs_dev_attr_to_buf(dev, attr, buf, buf_len);
	if (err) {
		XNVME_DEBUG("FAILED: _path_to_buf, err: %d", err);
		return err;
	}

	if ((strlen(buf) > 2) && (buf[0] == '0') && (buf[1] == 'x')) {
		base = 16;
	}

	*num = strtoll(buf, NULL, base);

	return 0;
}
#endif
