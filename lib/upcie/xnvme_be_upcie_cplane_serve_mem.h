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

#define XNVME_BE_UPCIE_CPLANE_REGIONS_MAX 32

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

struct xnvme_be_upcie_cplane_registration {
	uint64_t desc_offset;
	struct dmabuf dmabuf;
	int attached;
	int map_fd; ///< -1 where the mode needs none
	uint64_t map_handle;
	int region; ///< -1 when there is none
};

int
xnvme_be_upcie_cplane_serve_register_mem(int dmabuf_fd, uint64_t nbytes, uint32_t page_size,
					 const char *bdf,
					 struct xnvme_be_upcie_cplane_registration *out);

/** Idempotent: `reg` is reset to hold nothing */
void
xnvme_be_upcie_cplane_serve_unregister_mem(struct xnvme_be_upcie_cplane_registration *reg);

#endif /* XNVME_BE_UPCIE_ENABLED */
#endif /* __INTERNAL_XNVME_BE_UPCIE_CPLANE_SERVE_MEM_H */
