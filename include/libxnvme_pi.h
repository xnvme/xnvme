/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (C) 2022 Intel Corporation.
 * Copyright (C) Samsung Electronics Co., Ltd
 * All rights reserved.
 *
 * @headerfile libxnvme_pi.h
 */

#ifndef __INTERNAL_XNVME_PI_H
#define __INTERNAL_XNVME_PI_H

#define XNVME_APPTAG_IGNORE  0xFFFF
#define XNVME_REFTAG_MASK_16 0x00000000FFFFFFFF
#define XNVME_REFTAG_MASK_32 0xFFFFFFFFFFFFFFFF
#define XNVME_REFTAG_MASK_64 0x0000FFFFFFFFFFFF

enum xnvme_pi_type {
	XNVME_PI_DISABLE = 0,
	XNVME_PI_TYPE1   = 1,
	XNVME_PI_TYPE2   = 2,
	XNVME_PI_TYPE3   = 3,
};

enum xnvme_pi_check_type {
	XNVME_PI_FLAGS_REFTAG_CHECK = 1,
	XNVME_PI_FLAGS_APPTAG_CHECK = 2,
	XNVME_PI_FLAGS_GUARD_CHECK  = 4,
};

struct xnvme_pi_ctx {
	uint32_t block_size;
	uint32_t md_size;
	uint32_t guard_interval;
	uint32_t pi_flags;
	bool md_interleave;
	uint16_t pi_type;
	uint16_t pi_format;
	uint64_t init_ref_tag;
	uint16_t app_tag;
	uint16_t apptag_mask;
};

/**
 * Return the protection information size
 *
 * @param pi_format Protection Information Format
 * @return On success, the size for protection information format.
 */
size_t
xnvme_pi_size(enum xnvme_spec_nvm_ns_pif pi_format);

/**
 * Initialize the Protection Information context
 *
 * @param ctx Pointer to ::xnvme_pi_ctx
 * @param block_size Block size
 * @param md_size Metadata size
 * @param md_interleave metadata part of data buffer
 * @param pi_loc Protection Information location in data/metadata buffer
 * @param pi_type Protection Information Type
 * @param pi_flags Protection Information flags
 * @param init_ref_tag initial offset
 * @param apptag_mask Application Tag mask
 * @param app_tag Application Tag
 * @param pi_format Protection Information Format
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_pi_ctx_init(struct xnvme_pi_ctx *ctx, uint32_t block_size, uint32_t md_size,
		  bool md_interleave, bool pi_loc, enum xnvme_pi_type pi_type, uint32_t pi_flags,
		  uint32_t init_ref_tag, uint16_t apptag_mask, uint16_t app_tag,
		  enum xnvme_spec_nvm_ns_pif pi_format);

/**
 * Generate and fill Protection information part of metadata
 *
 * @param ctx Pointer to ::xnvme_pi_ctx
 * @param dbuf Pointer to data-payload
 * @param mbuf Pointer to meta-payload
 * @param num_blocks Number of logical blocks
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
void
xnvme_pi_generate(struct xnvme_pi_ctx *ctx, uint8_t *data_buf, uint8_t *md_buf,
		  uint32_t num_blocks);

/**
 * Verify the protection information content of metadata
 *
 * @param ctx Pointer to ::xnvme_pi_ctx
 * @param dbuf Pointer to data-payload
 * @param mbuf Pointer to meta-payload
 * @param num_blocks Number of logical blocks
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_pi_verify(struct xnvme_pi_ctx *ctx, uint8_t *data_buf, uint8_t *md_buf, uint32_t num_blocks);

#endif /* __INTERNAL_XNVME_PI_H */
