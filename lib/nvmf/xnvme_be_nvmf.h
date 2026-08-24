// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_NVMF_H
#define __INTERNAL_XNVME_BE_NVMF_H

extern struct xnvme_be_admin g_xnvme_be_nvmf_admin;
extern struct xnvme_be_async g_xnvme_be_nvmf_async;
extern struct xnvme_be_dev g_xnvme_be_nvmf_dev;
extern struct xnvme_be_mem g_xnvme_be_nvmf_mem;
extern struct xnvme_be_sync g_xnvme_be_nvmf_sync;

#define _INTERNAL_NOT_IMPLEMENTED()                     \
	{                                               \
		XNVME_DEBUG("FAILED: Not implemented"); \
		return -ENOSYS;                         \
	}

#endif /* __INTERNAL_XNVME_BE_NVMF_H */
