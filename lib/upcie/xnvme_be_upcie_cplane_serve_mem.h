// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_UPCIE_CPLANE_SERVE_MEM_H
#define __INTERNAL_XNVME_BE_UPCIE_CPLANE_SERVE_MEM_H
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

#ifdef XNVME_BE_UPCIE_ENABLED
#include <xnvme_be_upcie.h>

/**
 * Allocate `nbytes` from the heap on a client's behalf, reporting its heap offset
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_serve_buf_alloc(size_t nbytes, uint64_t *offset);

/**
 * Free a heap allocation made by xnvme_be_upcie_cplane_serve_buf_alloc()
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_serve_buf_free(uint64_t offset);

#endif /* XNVME_BE_UPCIE_ENABLED */
#endif /* __INTERNAL_XNVME_BE_UPCIE_CPLANE_SERVE_MEM_H */
