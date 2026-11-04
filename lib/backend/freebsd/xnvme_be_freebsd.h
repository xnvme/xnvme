// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_FREEBSD_H
#define __INTERNAL_XNVME_BE_FREEBSD_H
#include <xnvme_be_posix.h>

#define XNVME_BE_FREEBSD_CTRLR_PREFIX "nvme"
#define XNVME_BE_FREEBSD_NS_PREFIX    "ns"

#include <xnvme_be_freebsd_state.h>

extern struct xnvme_be_admin g_xnvme_be_freebsd_admin;
extern struct xnvme_be_sync g_xnvme_be_freebsd_sync;
extern struct xnvme_be_dev g_xnvme_be_freebsd_dev;
extern struct xnvme_be_async g_xnvme_be_freebsd_async;

#endif /* __INTERNAL_XNVME_BE_FREEBSD */
