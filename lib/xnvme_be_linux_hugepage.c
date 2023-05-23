// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_LINUX_ENABLED
#include <xnvme_dev.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/queue.h>
#include <linux/limits.h>

struct huge_alloc {
	char path[PATH_MAX];
	int fd;
	void *addr;
	size_t len;

	SLIST_ENTRY(huge_alloc) link;
};

SLIST_HEAD(huge_alloc_slist, huge_alloc);
struct huge_alloc_slist huge_alloc_head;

size_t
get_hugepage_size()
{
	FILE *fp;
	char line[128] = {'\0'};
	size_t hugepage_size = 0;

	fp = fopen("/proc/meminfo", "r");

	while (fgets(line, 128, fp)) {
		if (sscanf(line, "Hugepagesize: %16lu kB", &hugepage_size)) {
			XNVME_DEBUG("Hugepage size: %lu kB", hugepage_size);
			fclose(fp);
			return hugepage_size * 1024;
		}
	}
	fclose(fp);

	XNVME_DEBUG("Unable to find hugepage size");
	return 0;
}

bool
verify_hugetlbfs_path(char *path)
{
	char line[PATH_MAX];
	char search_str[PATH_MAX];
	FILE *fp;

	fp = fopen("/proc/mounts", "r");

	strncpy(search_str, path, sizeof(search_str) - 1);
	strncat(search_str, " hugetlbfs", sizeof(search_str) - strlen(search_str) - 1);

	while (fgets(line, sizeof(line), fp)) {
		if (strstr(line, search_str)) {
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}

void *
xnvme_be_linux_mem_hugepage_buf_alloc(const struct xnvme_dev *XNVME_UNUSED(dev), size_t nbytes,
				      uint64_t *XNVME_UNUSED(phys))
{
	void *buf;
	int hugepage_fd;
	struct huge_alloc *entry;
	size_t hugepage_size;
	char *env_hugepage_path;
	char hugepage_path[PATH_MAX] = {'\0'};

	env_hugepage_path = getenv("XNVME_HUGETLB_PATH");
	if (env_hugepage_path == NULL) {
		XNVME_DEBUG("ERROR: XNVME_HUGETLB_PATH env var not defined");
		errno = ENOMEM;
		return NULL;
	}

	strncpy(hugepage_path, env_hugepage_path, sizeof(hugepage_path) - 1);

	if (!verify_hugetlbfs_path(hugepage_path)) {
		XNVME_DEBUG("WARNING: Hugetlbfs is not mounted at: %s", hugepage_path);
	}

	hugepage_size = get_hugepage_size();
	if (!hugepage_size) {
		XNVME_DEBUG("Hugepage size not valid: %lu", hugepage_size);
		errno = ENOMEM;
		return NULL;
	}

	// nbytes has to be a multiple of alignment. Therefore, we round up to the nearest
	// multiple.
	nbytes = (1 + ((nbytes - 1) / hugepage_size)) * (hugepage_size);

	// Map a Huge page
	strncat(hugepage_path, "/xnvme_XXXXXX", sizeof(hugepage_path) - strlen(hugepage_path) - 1);
	hugepage_fd = mkstemp(hugepage_path);
	if (hugepage_fd == -1) {
		XNVME_DEBUG("Failed to open hugepage file. %s", hugepage_path);
		errno = ENOMEM;
		return NULL;
	}

	if (ftruncate(hugepage_fd, nbytes)) {
		XNVME_DEBUG("Failed to truncate hugepage file. %s, %lu", hugepage_path, nbytes);
		perror("hugetlb ftruncate()");
		return NULL;
	}

	buf = mmap(NULL, nbytes, PROT_READ | PROT_WRITE, MAP_SHARED, hugepage_fd, 0);
	if (buf == MAP_FAILED) {
		XNVME_DEBUG("mmap failed on hugepage file");
		return NULL;
	}

	// Appending allocated hugepage to linked-list for use during free
	entry = malloc(sizeof(struct huge_alloc));
	entry->fd = hugepage_fd;
	strncpy(entry->path, hugepage_path, sizeof(entry->path) - 1);
	entry->path[sizeof(entry->path) - 1] = '\0';
	entry->addr = buf;
	entry->len = nbytes;
	SLIST_INSERT_HEAD(&huge_alloc_head, entry, link);

	return buf;
}

void
xnvme_be_linux_mem_hugepage_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	struct huge_alloc *entry;

	if (!buf) {
		return;
	}

	// Iterate through linked-list to find the entry from *buf
	entry = SLIST_FIRST(&huge_alloc_head);
	while (entry != NULL) {
		if (entry->addr == buf) {
			break;
		}
		entry = SLIST_NEXT(entry, link);
	}

	if (!entry) {
		XNVME_DEBUG("huge_alloc_table_reset: Entry %p not found!", buf);
	}

	// Clean up all the files, buffers etc. for the hugepage
	munmap(buf, entry->len);
	close(entry->fd);
	remove(entry->path);
	SLIST_REMOVE(&huge_alloc_head, entry, huge_alloc, link);
	free(entry);
}

#endif

struct xnvme_be_mem g_xnvme_be_linux_mem_hugepage = {
	.id = "hugepage",
#ifdef XNVME_BE_LINUX_ENABLED
	.buf_alloc = xnvme_be_linux_mem_hugepage_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_linux_mem_hugepage_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
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
