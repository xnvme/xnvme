// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause
#ifndef SYS_UIO_H
#define SYS_UIO_H
#include <sys/types.h>

struct iovec
{
	void *iov_base; /* Base address. */
	size_t iov_len; /* Length. */
};

#endif /* SYS_UIO_H */
