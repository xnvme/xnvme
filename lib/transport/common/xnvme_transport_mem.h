// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_TRANSPORT_MEM_H
#define __INTERNAL_XNVME_TRANSPORT_MEM_H

#include <stddef.h>
#include <stdint.h>

struct xnvme_dev;

struct xnvme_transport_memory_handle {
	size_t nbytes;
	uintptr_t buf;
	void *handle;
	uint32_t key;
};

/**
 * Look up the registration key of the buffer containing [addr, addr + nbytes).
 *
 * Implemented by the backend, since registration is backend specific.
 */
int
xnvme_transport_registration_lookup_key(struct xnvme_dev *dev, void *addr, size_t nbytes,
					uint32_t *key);

#endif /* __INTERNAL_XNVME_TRANSPORT_MEM_H */
