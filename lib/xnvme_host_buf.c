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