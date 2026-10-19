// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

bool
xnvme_buf_is_hip(const void *buf);

int
xnvme_hip_buf_clear(void *buf, size_t nbytes);

int
xnvme_hip_buf_memcpy(void *dst, const void *src, size_t nbytes);

int
xnvme_hip_buf_diff(const void *expected, const void *actual, size_t nbytes, size_t *diff,
		   bool print);

int
xnvme_hip_buf_fill(void *buf, size_t nbytes, const char *content);
