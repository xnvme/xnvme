// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_POSIX_STATE_H
#define __INTERNAL_XNVME_BE_POSIX_STATE_H
#include <stddef.h>
#include <libxnvme.h>

/**
 * The file descriptor that the posix and shim implementations use
 *
 * They reach it through dev->be.state, so the state of every dev implementation
 * they are combined with embeds this as its first member, asserted with
 * XNVME_BE_POSIX_STATE_FIRST().
 */
struct xnvme_be_posix_state {
	int fd;
};

#define XNVME_BE_POSIX_STATE_FIRST(type)                       \
	XNVME_STATIC_ASSERT(offsetof(struct type, posix) == 0, \
			    "struct " #type " must start with its posix state")

#endif /* __INTERNAL_XNVME_BE_POSIX_STATE_H */
