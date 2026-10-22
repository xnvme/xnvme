// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_NVMF_MEM_H
#define __INTERNAL_XNVME_BE_NVMF_MEM_H

struct xnvme_be_nvmf_memory_handle {
	size_t nbytes;
	uintptr_t buf;
	void *handle;
	uint32_t key;
};

int
xnvme_be_nvmf_mem_init(struct xnvme_dev *dev);

int
xnvme_be_nvmf_mem_term(struct xnvme_dev *dev);

int
xnvme_be_nvmf_registration_lookup_key(struct xnvme_dev *dev, void *addr, size_t nbytes,
				      uint32_t *key);

#endif // __INTERNAL_XNVME_BE_NVMF_MEM_H