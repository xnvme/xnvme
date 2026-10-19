// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_LINUX_ENABLED
#include <xnvme_dev.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <linux/memfd.h>
#include <unistd.h>

#include <upcie/debug.h>
#include <upcie/hostmem.h>
#include <upcie/hostmem_config.h>
#include <upcie/hostmem_hugepage.h>
#include <upcie/hostmem_heap.h>
#include <upcie/hostmem_dma.h>

#define XNVME_HUGEPAGE_HEAP_SIZE_DEFAULT (256 * 1024 * 1024)

static struct hostmem_config g_hugepage_config;
static struct hostmem_heap g_hugepage_heap;
static int g_hugepage_initialized;

static int
_hugepage_heap_init(void)
{
	size_t heap_size = XNVME_HUGEPAGE_HEAP_SIZE_DEFAULT;
	const char *env_val;
	int err;

	err = hostmem_config_init(&g_hugepage_config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_config_init(), err: %d", err);
		return err;
	}

	env_val = getenv("XNVME_HUGEPAGE_BACKEND");
	if (env_val) {
		if (!strcmp(env_val, "memfd")) {
			g_hugepage_config.backend = HOSTMEM_BACKEND_MEMFD;
		} else if (!strcmp(env_val, "hugetlbfs")) {
			g_hugepage_config.backend = HOSTMEM_BACKEND_HUGETLBFS;
		} else {
			XNVME_DEBUG("WARNING: unknown XNVME_HUGEPAGE_BACKEND='%s'", env_val);
		}
	}

	env_val = getenv("XNVME_HUGETLB_PATH");
	if (env_val) {
		strncpy(g_hugepage_config.hugetlb_path, env_val,
			sizeof(g_hugepage_config.hugetlb_path) - 1);
		g_hugepage_config.hugetlb_path[sizeof(g_hugepage_config.hugetlb_path) - 1] = '\0';
		g_hugepage_config.backend = HOSTMEM_BACKEND_HUGETLBFS;
	}

	env_val = getenv("XNVME_HUGEPAGE_HEAP_SIZE");
	if (env_val) {
		heap_size = strtoull(env_val, NULL, 0);
		if (!heap_size) {
			heap_size = XNVME_HUGEPAGE_HEAP_SIZE_DEFAULT;
		}
	}

	if (g_hugepage_config.hugepgsz) {
		heap_size = ((heap_size + g_hugepage_config.hugepgsz - 1) /
			     g_hugepage_config.hugepgsz) *
			    g_hugepage_config.hugepgsz;
	}

	err = hostmem_heap_init(&g_hugepage_heap, heap_size, &g_hugepage_config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_heap_init(), err: %d", err);
		return err;
	}

	g_hugepage_initialized = 1;

	return 0;
}

void *
xnvme_be_linux_mem_hugepage_buf_alloc(const struct xnvme_dev *XNVME_UNUSED(dev), size_t nbytes,
				      uint64_t *phys)
{
	void *buf;
	int err;

	if (!g_hugepage_initialized) {
		err = _hugepage_heap_init();
		if (err) {
			errno = -err;
			return NULL;
		}
	}

	buf = hostmem_dma_malloc(&g_hugepage_heap, nbytes);
	if (!buf) {
		errno = ENOMEM;
		return NULL;
	}

	if (phys) {
		err = hostmem_heap_block_virt_to_phys(&g_hugepage_heap, buf, phys);
		if (err) {
			hostmem_dma_free(&g_hugepage_heap, buf);
			errno = -err;
			return NULL;
		}
	}

	return buf;
}

void
xnvme_be_linux_mem_hugepage_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	if (!buf) {
		return;
	}

	hostmem_dma_free(&g_hugepage_heap, buf);
}

int
xnvme_be_linux_mem_hugepage_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf,
					uint64_t *phys)
{
	return hostmem_heap_block_virt_to_phys(&g_hugepage_heap, buf, phys);
}

#endif

struct xnvme_be_mem g_xnvme_be_linux_mem_hugepage = {
	.id = "hugepage",
#ifdef XNVME_PLATFORM_LINUX_ENABLED
	.buf_alloc = xnvme_be_linux_mem_hugepage_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_linux_mem_hugepage_buf_free,
	.buf_vtophys = xnvme_be_linux_mem_hugepage_buf_vtophys,
	.mem_map = xnvme_be_nosys_mem_map,
	.mem_unmap = xnvme_be_nosys_mem_unmap,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.mem_map = xnvme_be_nosys_mem_map,
	.mem_unmap = xnvme_be_nosys_mem_unmap,
#endif
};
