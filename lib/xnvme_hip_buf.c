// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifdef XNVME_BE_UPCIE_HIP_ENABLED
#include <hip/hip_runtime.h>
#include <libxnvme.h>
#include <xnvme_host_buf.h>
#include <errno.h>

bool
xnvme_buf_is_hip(const void *buf)
{
	hipError_t err;
	hipPointerAttribute_t attr;

	err = hipPointerGetAttributes(&attr, buf);
	if (err != hipSuccess) {
		XNVME_DEBUG("FAILED: hipPointerGetAttributes(buf), err: %s",
			    hipGetErrorString(err));
		return false; // If it fails we assume it is not a HIP buffer
	}

	return attr.type == hipMemoryTypeDevice;
}

int
xnvme_hip_buf_clear(void *buf, size_t nbytes)
{
	hipError_t err;

	err = hipMemset(buf, 0, nbytes);
	if (err != hipSuccess) {
		XNVME_DEBUG("FAILED: hipMemset, err: %s", hipGetErrorString(err));
		return -EIO;
	}

	return 0;
}

int
xnvme_hip_buf_memcpy(void *dst, const void *src, size_t nbytes)
{
	hipError_t err;

	err = hipMemcpy(dst, src, nbytes, hipMemcpyDefault);
	if (err != hipSuccess) {
		XNVME_DEBUG("FAILED: hipMemcpy, err: %s", hipGetErrorString(err));
		return -EIO;
	}
	return 0;
}

int
xnvme_hip_buf_diff(const void *expected, const void *actual, size_t nbytes, size_t *diff,
		   bool print)
{
	void *exp, *act;
	hipError_t hip_err;
	int err;

	exp = malloc(nbytes);
	if (!exp) {
		err = -errno;
		XNVME_DEBUG("FAILED: malloc(exp), errno: %d", err);
		return err;
	}

	act = malloc(nbytes);
	if (!act) {
		err = -errno;
		XNVME_DEBUG("FAILED: malloc(act), errno: %d", err);
		goto exit;
	}

	hip_err = hipMemcpy(exp, expected, nbytes, hipMemcpyDefault);
	if (hip_err != hipSuccess) {
		XNVME_DEBUG("FAILED: hipMemcpy(expected), err: %s", hipGetErrorString(hip_err));
		err = -EIO;
		goto exit;
	}

	hip_err = hipMemcpy(act, actual, nbytes, hipMemcpyDefault);
	if (hip_err != hipSuccess) {
		XNVME_DEBUG("FAILED: hipMemcpy(actual), err: %s", hipGetErrorString(hip_err));
		err = -EIO;
		goto exit;
	}

	err = xnvme_host_buf_diff(exp, act, nbytes, diff, print);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_host_buf_diff(), err: %d", err);
		goto exit;
	}

exit:
	free(exp);
	free(act);

	return err;
}

int
xnvme_hip_buf_fill(void *buf, size_t nbytes, const char *content)
{
	hipError_t hip_err;
	int err;
	uint8_t *cbuf;

	cbuf = malloc(nbytes);
	if (!cbuf) {
		err = -errno;
		XNVME_DEBUG("FAILED: malloc(cbuf), errno: %d", err);
		return err;
	}

	err = xnvme_host_buf_fill(cbuf, nbytes, content);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_host_buf_fill(%s), err: %d", content, err);
		goto exit;
	}

	hip_err = hipMemcpy(buf, cbuf, nbytes, hipMemcpyDefault);
	if (hip_err != hipSuccess) {
		XNVME_DEBUG("FAILED: hipMemcpy, err: %s", hipGetErrorString(hip_err));
		err = -EIO;
		goto exit;
	}
exit:
	free(cbuf);
	return err;
}
#else

#include <libxnvme.h>
#include <errno.h>

bool
xnvme_buf_is_hip(const void *XNVME_UNUSED(buf))
{
	return false;
}

int
xnvme_hip_buf_clear(void *XNVME_UNUSED(buf), size_t XNVME_UNUSED(nbytes))
{
	XNVME_DEBUG("FAILED: xNVMe was compiled without HIP");
	return -ENOTSUP;
}

int
xnvme_hip_buf_memcpy(void *XNVME_UNUSED(dst), const void *XNVME_UNUSED(src),
		     size_t XNVME_UNUSED(nbytes))
{
	XNVME_DEBUG("FAILED: xNVMe was compiled without HIP");
	return -ENOTSUP;
}

int
xnvme_hip_buf_diff(const void *XNVME_UNUSED(expected), const void *XNVME_UNUSED(actual),
		   size_t XNVME_UNUSED(nbytes), size_t *XNVME_UNUSED(diff),
		   bool XNVME_UNUSED(print))
{
	XNVME_DEBUG("FAILED: xNVMe was compiled without HIP");
	return -ENOTSUP;
}

int
xnvme_hip_buf_fill(void *XNVME_UNUSED(buf), size_t XNVME_UNUSED(nbytes),
		   const char *XNVME_UNUSED(content))
{
	XNVME_DEBUG("FAILED: xNVMe was compiled without HIP");
	return -ENOTSUP;
}
#endif
