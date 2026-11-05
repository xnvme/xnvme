// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <libxnvme.h>
#include <xnvme_dev.h>
#include <xnvme_be.h>
#include <xnvme_host_buf.h>
#include <xnvme_cuda_buf.h>
#include <xnvme_hip_buf.h>
#include <xnvme_lock.h>
#include <xnvme_buf.h>

#ifdef WIN32
#include <windows.h>

static size_t
_pagesize(void)
{
	SYSTEM_INFO si;

	GetSystemInfo(&si);

	return si.dwPageSize;
}
#else
#include <unistd.h>

static size_t
_pagesize(void)
{
	long pagesize = sysconf(_SC_PAGESIZE);

	return (pagesize > 0) ? (size_t)pagesize : 0;
}
#endif

void *
xnvme_buf_virt_alloc(size_t alignment, size_t nbytes)
{
	if (!nbytes) {
		errno = EINVAL;
		XNVME_DEBUG("FAILED: invalid value for nbytes: '%zu')", nbytes);
		return NULL;
	}
	// nbytes has to be a multiple of alignment. Therefore, we round up to the nearest
	// multiple.
	nbytes = (1 + ((nbytes - 1) / alignment)) * (alignment);
#ifdef WIN32
	return _aligned_malloc(nbytes, alignment);
#else
	return aligned_alloc(alignment, nbytes);
#endif
}

void *
xnvme_buf_virt_realloc(void *XNVME_UNUSED(buf), size_t XNVME_UNUSED(alignment),
		       size_t XNVME_UNUSED(nbytes))
{
	errno = ENOSYS;
	return NULL;
}

void
xnvme_buf_virt_free(void *buf)
{
#ifdef WIN32
	_aligned_free(buf);
#else
	free(buf);
#endif
}

void *
xnvme_buf_phys_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys)
{
	void *buf;
	int err;

	xnvme_lock();
	buf = dev->be.mem.buf_alloc(dev, nbytes, phys);
	err = errno;
	xnvme_unlock();
	errno = err;

	return buf;
}

void *
xnvme_buf_phys_realloc(const struct xnvme_dev *dev, void *buf, size_t nbytes, uint64_t *phys)
{
	void *new_buf;
	int err;

	xnvme_lock();
	new_buf = dev->be.mem.buf_realloc(dev, buf, nbytes, phys);
	err = errno;
	xnvme_unlock();
	errno = err;

	return new_buf;
}

void
xnvme_buf_phys_free(const struct xnvme_dev *dev, void *buf)
{
	xnvme_lock();
	dev->be.mem.buf_free(dev, buf);
	xnvme_unlock();
}

int
xnvme_buf_vtophys(const struct xnvme_dev *dev, void *buf, uint64_t *phys)
{
	int err;

	xnvme_lock();
	err = dev->be.mem.buf_vtophys(dev, buf, phys);
	xnvme_unlock();

	return err;
}

void *
xnvme_buf_alloc(const struct xnvme_dev *dev, size_t nbytes)
{
	return xnvme_buf_phys_alloc(dev, nbytes, NULL);
}

void *
xnvme_buf_realloc(const struct xnvme_dev *dev, void *buf, size_t nbytes)
{
	return xnvme_buf_phys_realloc(dev, buf, nbytes, NULL);
}

void
xnvme_buf_free(const struct xnvme_dev *dev, void *buf)
{
	xnvme_buf_phys_free(dev, buf);
}

int
xnvme_buf_clear(void *buf, size_t nbytes)
{
	if (xnvme_buf_is_cuda(buf)) {
		return xnvme_cuda_buf_clear(buf, nbytes);
	}

	if (xnvme_buf_is_hip(buf)) {
		return xnvme_hip_buf_clear(buf, nbytes);
	}

	return xnvme_host_buf_clear(buf, nbytes);
}

int
xnvme_buf_memcpy(void *dst, const void *src, size_t nbytes)
{
	if (xnvme_buf_is_cuda(dst) || xnvme_buf_is_cuda(src)) {
		return xnvme_cuda_buf_memcpy(dst, src, nbytes);
	}

	if (xnvme_buf_is_hip(dst) || xnvme_buf_is_hip(src)) {
		return xnvme_hip_buf_memcpy(dst, src, nbytes);
	}

	return xnvme_host_buf_memcpy(dst, src, nbytes);
}

/**
 * Helper function writing/reading given buf to/from file. When mode has
 * O_WRONLY, the given buffer is written to file, otherwise, it is read.
 *
 * NOTE: this depends on POSIX syscalls
 */
static int
fdio_func(void *buf, size_t nbytes, const char *path, struct xnvme_opts *opts)
{
	struct xnvme_cmd_ctx ctx = {0};
	struct xnvme_dev *fh = NULL;
	size_t transferred = 0;
	int err = 0;

	fh = xnvme_file_open(path, opts);
	if (fh == NULL) {
		XNVME_DEBUG("FAILED: xnvme_file_open(), errno: %d", errno);
		return -errno;
	}

	xnvme_dev_get_cmd_ctx(fh, &ctx);

	while (transferred < nbytes) {
		const size_t remain = nbytes - transferred;
		size_t nbytes_call = remain < SSIZE_MAX ? remain : SSIZE_MAX;

		if (opts->wronly) {
			err = xnvme_file_pwrite(&ctx, buf + transferred, nbytes_call, transferred);
		} else {
			err = xnvme_file_pread(&ctx, buf + transferred, nbytes_call, transferred);
		}
		if (err) {
			XNVME_DEBUG("FAILED: opts->wronly: %d, cpl.result: %ld, errno: %d",
				    opts->wronly, ctx.cpl.result, errno);
			xnvme_file_close(fh);
			return -errno;
		}

		// when reading, break at end of file
		if (!opts->wronly && ctx.cpl.result == 0) {
			break;
		}

		transferred += ctx.cpl.result;
	}

	err = xnvme_file_close(fh);
	if (err) {
		XNVME_DEBUG("FAILED: opts->wronly: %d, err: %d, errno: %d", opts->wronly, err,
			    errno);
		return -errno;
	}

	return 0;
}

int
xnvme_buf_from_file(void *buf, size_t nbytes, const char *path)
{
	struct xnvme_opts opts = {
		.rdonly = 1,
	};

	return fdio_func(buf, nbytes, path, &opts);
}

int
xnvme_buf_to_file(void *buf, size_t nbytes, const char *path)
{
	struct xnvme_opts opts = {
		.create = 1,
		.wronly = 1,
		.create_mode = S_IWUSR | S_IRUSR,
	};

	return fdio_func(buf, nbytes, path, &opts);
}

int
xnvme_buf_fill(void *buf, size_t nbytes, const char *content)
{
	if (xnvme_buf_is_cuda(buf)) {
		return xnvme_cuda_buf_fill(buf, nbytes, content);
	}

	if (xnvme_buf_is_hip(buf)) {
		return xnvme_hip_buf_fill(buf, nbytes, content);
	}

	return xnvme_host_buf_fill(buf, nbytes, content);
}

int
xnvme_buf_diff(const void *expected, const void *actual, size_t nbytes, size_t *diff)
{
	if (xnvme_buf_is_cuda(expected) || xnvme_buf_is_cuda(actual)) {
		return xnvme_cuda_buf_diff(expected, actual, nbytes, diff, false);
	}

	if (xnvme_buf_is_hip(expected) || xnvme_buf_is_hip(actual)) {
		return xnvme_hip_buf_diff(expected, actual, nbytes, diff, false);
	}

	return xnvme_host_buf_diff(expected, actual, nbytes, diff, false);
}

int
xnvme_buf_diff_pr(const void *expected, const void *actual, size_t nbytes, int XNVME_UNUSED(opts))
{
	size_t diff = 0;
	if (xnvme_buf_is_cuda(expected) || xnvme_buf_is_cuda(actual)) {
		return xnvme_cuda_buf_diff(expected, actual, nbytes, &diff, true);
	}

	if (xnvme_buf_is_hip(expected) || xnvme_buf_is_hip(actual)) {
		return xnvme_hip_buf_diff(expected, actual, nbytes, &diff, true);
	}

	return xnvme_host_buf_diff(expected, actual, nbytes, &diff, true);
}

int
xnvme_buf_export_dmabuf(void *vaddr, void **base, size_t *nbytes, int *fd)
{
	int err;

	if (xnvme_buf_is_cuda(vaddr)) {
		err = xnvme_cuda_buf_get_range(vaddr, base, nbytes);
		return err ? err : xnvme_cuda_buf_export_dmabuf(*base, *nbytes, fd);
	}
	if (xnvme_buf_is_hip(vaddr)) {
		err = xnvme_hip_buf_get_range(vaddr, base, nbytes);
		return err ? err : xnvme_hip_buf_export_dmabuf(*base, *nbytes, fd);
	}

	XNVME_DEBUG("FAILED: vaddr(%p) is neither CUDA nor HIP device memory", vaddr);

	return -EINVAL;
}

int
xnvme_buf_register(const struct xnvme_dev *dev, void *vaddr, size_t nbytes)
{
	int err;

	xnvme_lock();
	err = dev->be.mem.buf_register(dev, vaddr, nbytes);
	xnvme_unlock();

	return err;
}

int
xnvme_buf_unregister(const struct xnvme_dev *dev, void *vaddr)
{
	int err;

	xnvme_lock();
	err = dev->be.mem.buf_unregister(dev, vaddr);
	xnvme_unlock();

	return err;
}

int
xnvme_buf_get_alignment(const struct xnvme_dev *dev, size_t *align)
{
	size_t got = 0;
	int err;

	if (!dev || !align) {
		return -EINVAL;
	}
	if (!dev->be.mem.buf_get_alignment) {
		return -ENOSYS;
	}

	err = dev->be.mem.buf_get_alignment(dev, &got);
	if (err) {
		return err;
	}

	*align = (got < sizeof(void *)) ? sizeof(void *) : got;

	return 0;
}

int
xnvme_buf_noreg_register(const struct xnvme_dev *XNVME_UNUSED(dev), void *vaddr, size_t nbytes)
{
	if (!vaddr || !nbytes) {
		XNVME_DEBUG("FAILED: invalid vaddr(%p) or nbytes(%zu)", vaddr, nbytes);
		return -EINVAL;
	}

	XNVME_DEBUG("INFO: vaddr(%p) needs no registering", vaddr);

	return 0;
}

int
xnvme_buf_noreg_unregister(const struct xnvme_dev *XNVME_UNUSED(dev), void *vaddr)
{
	if (!vaddr) {
		XNVME_DEBUG("FAILED: invalid vaddr(%p)", vaddr);
		return -EINVAL;
	}

	return 0;
}

int
xnvme_buf_noreg_get_alignment(const struct xnvme_dev *dev, size_t *align)
{
	size_t pagesize;

	// The OS copies the payload of buffered IO, so any alignment serves
	if (!dev->opts.direct) {
		*align = sizeof(void *);
		return 0;
	}

	/*
	 * The OS refuses a payload of direct IO that is not aligned to what the device or the file
	 * system needs, which a page satisfies
	 */
	pagesize = _pagesize();
	if (!pagesize) {
		XNVME_DEBUG("FAILED: retrieving the page size");
		return -EIO;
	}
	*align = pagesize;

	return 0;
}

static struct xnvme_buf_reg *g_regs;

int
xnvme_buf_reg_find(const void *owner, const void *vaddr, size_t nbytes, struct xnvme_buf_reg **reg)
{
	uintptr_t lo = (uintptr_t)vaddr, hi = lo + nbytes;

	for (struct xnvme_buf_reg *cur = g_regs; cur; cur = cur->next) {
		uintptr_t cur_lo = (uintptr_t)cur->vaddr, cur_hi = cur_lo + cur->nbytes;

		if ((cur->owner != owner) || (hi <= cur_lo) || (lo >= cur_hi)) {
			continue;
		}
		if ((lo >= cur_lo) && (hi <= cur_hi)) {
			*reg = cur;
			return 0;
		}

		XNVME_DEBUG("FAILED: vaddr(%p, %zu) overlaps registered(%p, %zu)", vaddr, nbytes,
			    cur->vaddr, cur->nbytes);
		return -EBUSY;
	}

	return -ENOENT;
}

int
xnvme_buf_reg_add(const void *owner, void *vaddr, size_t nbytes, uint64_t priv)
{
	struct xnvme_buf_reg *reg;

	reg = calloc(1, sizeof(*reg));
	if (!reg) {
		return -ENOMEM;
	}

	reg->owner = owner;
	reg->vaddr = vaddr;
	reg->nbytes = nbytes;
	reg->priv = priv;
	reg->refs = 1;
	reg->next = g_regs;
	g_regs = reg;

	return 0;
}

void
xnvme_buf_reg_remove(struct xnvme_buf_reg *reg)
{
	for (struct xnvme_buf_reg **link = &g_regs; *link; link = &(*link)->next) {
		if (*link == reg) {
			*link = reg->next;
			free(reg);
			return;
		}
	}
}
