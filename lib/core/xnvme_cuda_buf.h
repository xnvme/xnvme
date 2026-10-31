// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

bool
xnvme_buf_is_cuda(const void *buf);

int
xnvme_cuda_buf_clear(void *buf, size_t nbytes);

int
xnvme_cuda_buf_memcpy(void *dst, const void *src, size_t nbytes);

int
xnvme_cuda_buf_diff(const void *expected, const void *actual, size_t nbytes, size_t *diff,
		    bool print);

int
xnvme_cuda_buf_fill(void *buf, size_t nbytes, const char *content);
