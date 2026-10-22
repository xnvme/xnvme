// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_NVMF_REF_H
#define __INTERNAL_XNVME_BE_NVMF_REF_H

struct xnvme_be_nvmf_ref;

typedef void (*xnvme_be_nvmf_ref_destructor)(struct xnvme_be_nvmf_ref *ref);

struct xnvme_be_nvmf_ref {
	int refs; ///< Reference count, not thread-safe
	xnvme_be_nvmf_ref_destructor destructor;
};

static inline void
xnvme_be_nvmf_ref_init(struct xnvme_be_nvmf_ref *ref, xnvme_be_nvmf_ref_destructor destructor)
{
	ref->refs       = 1;
	ref->destructor = destructor;
}

static inline int
xnvme_be_nvmf_ref_get(struct xnvme_be_nvmf_ref *ref)
{
	return ++ref->refs;
}

static inline int
xnvme_be_nvmf_ref_put(struct xnvme_be_nvmf_ref *ref)
{
	if (--ref->refs == 0) {
		ref->destructor(ref);
		return 0;
	}
	return ref->refs;
}

#endif /* __INTERNAL_XNVME_BE_NVMF_REF_H */