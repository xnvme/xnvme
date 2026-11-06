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
#include <unistd.h>

bool
xnvme_buf_is_hugetlb(const void *vaddr, size_t nbytes)
{
	unsigned long start = 0, end = 0, kps;
	uintptr_t cursor = (uintptr_t)vaddr, hi = cursor + nbytes;
	long pagesize = sysconf(_SC_PAGESIZE);
	char line[4096];
	FILE *fp;

	if (pagesize <= 0) {
		return false;
	}

	fp = fopen("/proc/self/smaps", "r");
	if (!fp) {
		XNVME_DEBUG("FAILED: fopen(/proc/self/smaps), errno: %d", errno);
		return false;
	}

	while (cursor < hi && fgets(line, sizeof(line), fp)) {
		if (sscanf(line, "%lx-%lx ", &start, &end) == 2) {
			continue;
		}
		if (sscanf(line, "KernelPageSize: %lu kB", &kps) != 1) {
			continue;
		}
		if (end <= cursor) {
			continue;
		}
		if ((start > cursor) || ((kps << 10) <= (unsigned long)pagesize)) {
			break;
		}
		cursor = end;
	}

	fclose(fp);

	return cursor >= hi;
}
#else
bool
xnvme_buf_is_hugetlb(const void *XNVME_UNUSED(vaddr), size_t XNVME_UNUSED(nbytes))
{
	return false;
}
#endif
