// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_UPCIE_CPLANE_SHARE_H
#define __INTERNAL_XNVME_BE_UPCIE_CPLANE_SHARE_H
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

#ifdef XNVME_BE_UPCIE_ENABLED
#include <xnvme_be_upcie.h>

/**
 * What a server hands a client for it to attach to a controller
 *
 * The descriptors stay open in the server; a client receives copies.
 * `desc_offset` is into the heap `heap_fd` names.
 */
struct xnvme_be_upcie_cplane_share {
	int heap_fd;                      ///< The DMA heap, for the client to map
	int bar0_fd;                      ///< BAR0, since a client rings its own doorbell
	uint64_t bar0_nbytes;             ///< How much of BAR0 to map
	uint64_t heap_nbytes;             ///< How much of the heap to map
	struct nvme_controller_base base; ///< What a client builds its controller around
	uint64_t desc_offset;             ///< Where the heap's description sits
	char uri[32];                     ///< The identifier clients were given
	int live;                         ///< Whether the offsets above hold allocations
};

/**
 * Fill `out` with what a client of `dev` needs, placing the heap's description in the heap
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_share_get(struct xnvme_dev *dev, struct xnvme_be_upcie_cplane_share *out);

/**
 * Release what xnvme_be_upcie_cplane_share_get() took; safe on an all-zero `share`
 */
void
xnvme_be_upcie_cplane_share_put(struct xnvme_be_upcie_cplane_share *share);

#endif /* XNVME_BE_UPCIE_ENABLED */
#endif /* __INTERNAL_XNVME_BE_UPCIE_CPLANE_SHARE_H */
