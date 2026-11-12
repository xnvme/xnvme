/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (C) 2022 Intel Corporation.
 * Copyright (C) Samsung Electronics Co., Ltd
 * All rights reserved.
 *
 * @file libxnvme_pi.h
 */

#ifndef __INTERNAL_XNVME_PI_H
#define __INTERNAL_XNVME_PI_H

/**
 * Application Tag that disables the checks of a block's protection information
 *
 * At the first block whose Application Tag is this, and, for Type 3, whose Reference Tag is all
 * ones as well, xnvme_pi_verify() returns 0 without checking that block or those after it.
 */
#define XNVME_APPTAG_IGNORE 0xFFFF

/** Bits of a Reference Tag that the 16b Guard format stores, 32 */
#define XNVME_REFTAG_MASK_16 0x00000000FFFFFFFF

/** A Reference Tag of all ones; with ::XNVME_APPTAG_IGNORE, disables the checks of Type 3 */
#define XNVME_REFTAG_MASK_32 0xFFFFFFFFFFFFFFFF

/** Bits of a Reference Tag that the 64b Guard format stores, 48 */
#define XNVME_REFTAG_MASK_64 0x0000FFFFFFFFFFFF

/**
 * Protection Information Type of a namespace, as its End-to-end Data Protection Type Settings
 * give it
 *
 * @enum xnvme_pi_type
 */
enum xnvme_pi_type {
	XNVME_PI_DISABLE = 0, ///< Protection information is not enabled
	XNVME_PI_TYPE1   = 1, ///< Type 1
	XNVME_PI_TYPE2   = 2, ///< Type 2
	XNVME_PI_TYPE3   = 3, ///< Type 3; the Reference Tag is not checked
};

/**
 * Checks of the protection information to generate and verify, the bits of 'pi_flags'
 *
 * The values are those of the Protection Information Check (PRCHK) field of a command.
 *
 * @enum xnvme_pi_check_type
 */
enum xnvme_pi_check_type {
	XNVME_PI_FLAGS_REFTAG_CHECK = 1, ///< Check the Reference Tag
	XNVME_PI_FLAGS_APPTAG_CHECK = 2, ///< Check the Application Tag
	XNVME_PI_FLAGS_GUARD_CHECK  = 4, ///< Check the Guard
};

/**
 * Context for generating and verifying protection information, filled by xnvme_pi_ctx_init()
 */
struct xnvme_pi_ctx {
	uint32_t block_size;     ///< Size of a logical block, in bytes
	uint32_t md_size;        ///< Size of the metadata of a logical block, in bytes
	uint32_t guard_interval; ///< Offset of the protection information, in bytes, in the
				 ///< block when the metadata is interleaved, else in its metadata
	uint32_t pi_flags;       ///< Checks to perform, bits of ::xnvme_pi_check_type
	bool md_interleave;      ///< Whether the metadata is interleaved with the data
	uint16_t pi_type;        ///< Protection Information Type, a value of ::xnvme_pi_type
	uint16_t pi_format; ///< Protection Information Format, a value of ::xnvme_spec_nvm_ns_pif
	uint64_t init_ref_tag; ///< Reference Tag of the first block
	uint16_t app_tag;      ///< Application Tag
	uint16_t apptag_mask;  ///< Bits of the Application Tag to check
};

/**
 * Return the protection information size
 *
 * @param pi_format Protection Information Format, a value of ::xnvme_spec_nvm_ns_pif
 * @return On success, the size for protection information format.
 */
size_t
xnvme_pi_size(uint32_t pi_format);

/**
 * Initialize the Protection Information context
 *
 * @param ctx Pointer to ::xnvme_pi_ctx
 * @param block_size Block size
 * @param md_size Metadata size
 * @param md_interleave metadata part of data buffer
 * @param pi_loc Protection Information location in data/metadata buffer
 * @param pi_type Protection Information Type, a value of ::xnvme_pi_type
 * @param pi_flags Protection Information flags
 * @param init_ref_tag initial offset
 * @param apptag_mask Application Tag mask
 * @param app_tag Application Tag
 * @param pi_format Protection Information Format, a value of ::xnvme_spec_nvm_ns_pif
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 *
 * @errno
 */
int
xnvme_pi_ctx_init(struct xnvme_pi_ctx *ctx, uint32_t block_size, uint32_t md_size,
		  bool md_interleave, bool pi_loc, uint32_t pi_type, uint32_t pi_flags,
		  uint32_t init_ref_tag, uint16_t apptag_mask, uint16_t app_tag,
		  uint32_t pi_format);

/**
 * Generate and fill Protection information part of metadata
 *
 * @param ctx Pointer to ::xnvme_pi_ctx
 * @param data_buf Pointer to data-payload
 * @param md_buf Pointer to meta-payload
 * @param num_blocks Number of logical blocks
 */
void
xnvme_pi_generate(struct xnvme_pi_ctx *ctx, uint8_t *data_buf, uint8_t *md_buf,
		  uint32_t num_blocks);

/**
 * Verify the protection information content of metadata
 *
 * @param ctx Pointer to ::xnvme_pi_ctx
 * @param data_buf Pointer to data-payload
 * @param md_buf Pointer to meta-payload
 * @param num_blocks Number of logical blocks
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 *
 * @errno
 */
int
xnvme_pi_verify(struct xnvme_pi_ctx *ctx, uint8_t *data_buf, uint8_t *md_buf, uint32_t num_blocks);

#endif /* __INTERNAL_XNVME_PI_H */
