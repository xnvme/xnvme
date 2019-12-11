// Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_SGL_H
#define __INTERNAL_XNVME_SGL_H

#include <xnvme_dev.h>
#include <sys/queue.h>

struct xnvme_sgl {
	struct xnvme_spec_sgl_descriptor *indirect, *descriptors;
	int ndescr, nalloc;
	size_t len;

	SLIST_ENTRY(xnvme_sgl) free_list;
};

struct xnvme_sgl_pool {
	struct xnvme_dev *dev;

	SLIST_HEAD(, xnvme_sgl) free_list;
};

#endif /* __INTERNAL_XNVME_SGL_H */
