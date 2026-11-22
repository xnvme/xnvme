// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause
#include <libxnvme.h>

int
xnvme_host_buf_clear(void *buf, size_t nbytes)
{
	memset(buf, 0, nbytes);
	return 0;
}

int
xnvme_host_buf_memcpy(void *dst, const void *src, size_t nbytes)
{
	memcpy(dst, src, nbytes);
	return 0;
}

int
xnvme_host_buf_diff(const void *expected, const void *actual, size_t nbytes, size_t *diff,
		    bool print)
{
	const uint8_t *exp = expected;
	const uint8_t *act = actual;

	if (print) {
		printf("comparison:\n");
		printf("  diffs:\n");
	}
	for (size_t i = 0; i < nbytes; ++i) {
		if (exp[i] == act[i]) {
			continue;
		}

		*diff += 1;
		if (print) {
			printf("    - {byte: '%06zu', expected: 0x%" PRIx8 ", actual: 0x%" PRIx8
			       ")\n",
			       i, exp[i], act[i]);
		}
	}
	if (print) {
		printf("  nbytes: %zu\n", nbytes);
		printf("  nbytes_diff: %zu\n", *diff);
	}

	return 0;
}

int
xnvme_host_buf_fill(void *buf, size_t nbytes, const char *content)
{
	uint8_t *cbuf = buf;

	if (strlen(content) == 1) {
		memset(cbuf, content[0], nbytes);
		return 0;
	}

	if (!strncmp(content, "anum", 4)) {
		for (size_t i = 0; i < nbytes; ++i) {
			cbuf[i] = (i % 26) + 65;
		}

		return 0;
	}

	if (!strncmp(content, "rand-t", 6)) {
		srand(time(NULL));
		for (size_t i = 0; i < nbytes; ++i) {
			cbuf[i] = (rand() % 26) + 65;
		}

		return 0;
	}

	if (!strncmp(content, "rand-k", 6)) {
		srand(0);
		for (size_t i = 0; i < nbytes; ++i) {
			cbuf[i] = (rand() % 26) + 65;
		}

		return 0;
	}

	if (!strncmp(content, "zero", 4)) {
		return xnvme_host_buf_clear(buf, nbytes);
	}

	return xnvme_buf_from_file(buf, nbytes, content);
}

#ifdef __linux__
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>

#define NUMA_QUERY_PAGES 256

/** The NUMA node of the present pages in [lo, hi); -1 when none is present or the nodes differ */
static int32_t
_numa_node(uintptr_t lo, uintptr_t hi, uintptr_t pagesize)
{
	void *pages[NUMA_QUERY_PAGES];
	int status[NUMA_QUERY_PAGES];
	int32_t node = -1;

	for (uintptr_t page = lo & ~(pagesize - 1); page < hi;) {
		unsigned long count = 0;

		while ((count < NUMA_QUERY_PAGES) && (page < hi)) {
			pages[count++] = (void *)page;
			page += pagesize;
		}
		// Without target nodes, move_pages() reports the node of each page and moves none
		if (syscall(SYS_move_pages, 0, count, pages, NULL, status, 0)) {
			XNVME_DEBUG("INFO: move_pages(), errno: %d", errno);
			return -1;
		}
		for (unsigned long i = 0; i < count; ++i) {
			if (status[i] < 0) {
				continue; // -ENOENT where the page is not present
			}
			if ((node >= 0) && (node != status[i])) {
				return -1;
			}
			node = status[i];
		}
	}

	return node;
}

struct smaps {
	int fd;
	size_t pos;
	size_t len;
	char buf[4096];
};

/** The next line of smaps in line, cut at linesize - 1 bytes; false at the end */
static bool
_smaps_line(struct smaps *smaps, char *line, size_t linesize)
{
	size_t n = 0;

	for (;;) {
		ssize_t got;
		char c;

		if (smaps->pos == smaps->len) {
			got = read(smaps->fd, smaps->buf, sizeof(smaps->buf));
			if ((got < 0) && (errno == EINTR)) {
				continue;
			}
			if (got <= 0) {
				line[n] = '\0';
				return n > 0;
			}
			smaps->pos = 0;
			smaps->len = (size_t)got;
		}
		c = smaps->buf[smaps->pos++];
		if (c == '\n') {
			line[n] = '\0';
			return true;
		}
		if (n < linesize - 1) {
			line[n++] = c;
		}
	}
}

/**
 * Reads /proc/self/smaps with read(), not stdio: fopen() allocates, and the allocator can map that
 * memory into the range being inspected, as musl's does into a range just unmapped. With numa
 * false, move_pages() is not called and numa_node is not set.
 */
static int
_props(const void *vaddr, size_t nbytes, struct xnvme_buf_props *props, bool numa)
{
	uintptr_t lo = (uintptr_t)vaddr, cursor = lo, hi = lo + nbytes;
	unsigned long start = 0, end = 0, kps;
	long pagesize = sysconf(_SC_PAGESIZE);
	uint64_t page_nbytes = UINT64_MAX;
	char perms[5] = {0};
	struct smaps smaps = {0};
	char line[4096];
	bool rw = true;
	int err;

	if (pagesize <= 0) {
		return -EINVAL;
	}

	smaps.fd = open("/proc/self/smaps", O_RDONLY | O_CLOEXEC);
	if (smaps.fd < 0) {
		err = -errno;
		XNVME_DEBUG("FAILED: open(/proc/self/smaps), errno: %d", errno);
		return err;
	}

	// smaps lists the mappings by address, each as a header line followed by its fields
	while (cursor < hi && _smaps_line(&smaps, line, sizeof(line))) {
		if (sscanf(line, "%lx-%lx %4s ", &start, &end, perms) == 3) {
			continue;
		}
		if (sscanf(line, "KernelPageSize: %lu kB", &kps) != 1) {
			continue;
		}
		if (end <= cursor) {
			continue;
		}
		if (start > cursor) {
			break;
		}
		rw = rw && (perms[0] == 'r') && (perms[1] == 'w');
		page_nbytes = XNVME_MIN_U64(page_nbytes, (uint64_t)kps << 10);
		cursor = end;
	}

	close(smaps.fd);

	if (cursor < hi) {
		props->cpu_access = XNVME_BUF_PROP_NO;
		return 0;
	}

	props->cpu_access = rw ? XNVME_BUF_PROP_YES : XNVME_BUF_PROP_NO;
	props->page_nbytes = page_nbytes;
	if (rw && numa) {
		props->numa_node = _numa_node(lo, hi, (uintptr_t)page_nbytes);
	}

	return 0;
}

int
xnvme_host_buf_get_props(const void *vaddr, size_t nbytes, struct xnvme_buf_props *props)
{
	return _props(vaddr, nbytes, props, true);
}

bool
xnvme_buf_is_hugetlb(const void *vaddr, size_t nbytes)
{
	struct xnvme_buf_props props = {.numa_node = -1};
	long pagesize = sysconf(_SC_PAGESIZE);

	if ((pagesize <= 0) || !nbytes || (nbytes > UINTPTR_MAX - (uintptr_t)vaddr) ||
	    _props(vaddr, nbytes, &props, false)) {
		return false;
	}

	return props.page_nbytes > (uint64_t)pagesize;
}
#else
int
xnvme_host_buf_get_props(const void *XNVME_UNUSED(vaddr), size_t XNVME_UNUSED(nbytes),
			 struct xnvme_buf_props *XNVME_UNUSED(props))
{
	return 0;
}

bool
xnvme_buf_is_hugetlb(const void *XNVME_UNUSED(vaddr), size_t XNVME_UNUSED(nbytes))
{
	return false;
}
#endif
