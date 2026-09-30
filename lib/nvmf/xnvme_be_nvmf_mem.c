#include <unistd.h>

#include <xnvme_be_nvmf.h>

static void
xnvme_be_nvmf_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *buf)
{
	long sz = sysconf(_SC_PAGESIZE);

	if (sz == -1) {
		XNVME_DEBUG("FAILED: sysconf(), errno: %d", errno);
		return NULL;
	}

	return xnvme_buf_virt_alloc(sz, nbytes);
}

static void
xnvme_be_nvmf_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	xnvme_buf_virt_free(buf);
}

static void *
xnvme_be_nvmf_buf_realloc(const struct xnvme_dev *XNVME_UNUSED(dev), void *XNVME_UNUSED(buf),
			  size_t XNVME_UNUSED(nbytes), uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: _posix: does not support realloc");
	errno = ENOSYS;
	return NULL;
}

static int
xnvme_be_nvmf_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev), void *XNVME_UNUSED(buf),
			  uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: _posix: does not support phys/DMA alloc");
	return -ENOSYS;
}

struct xnvme_be_mem g_xnvme_be_nvmf_mem = {
	.id = "nvmf",
	.buf_alloc = xnvme_be_nvmf_buf_alloc,
	.buf_free = xnvme_be_nvmf_buf_free,
	.buf_realloc = xnvme_be_nvmf_buf_realloc,
	.buf_vtophys = xnvme_be_nvmf_buf_vtophys,
};