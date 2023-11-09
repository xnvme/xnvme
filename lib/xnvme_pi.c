/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (C) 2022 Intel Corporation.
 * Copyright (C) Samsung Electronics Co., Ltd
 * All rights reserved.
 */

#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_cmd.h>
#include <xnvme_dev.h>
#include <xnvme_geo.h>
#include <xnvme_endian.h>
#include <xnvme_crc.h>

static bool
xnvme_pi_type_is_valid(enum xnvme_pi_type pi_type, uint32_t pi_flags)
{
	switch (pi_type) {
	case XNVME_PI_TYPE1:
	case XNVME_PI_TYPE2:
	case XNVME_PI_DISABLE:
		break;
	case XNVME_PI_TYPE3:
		if (pi_flags & XNVME_PI_FLAGS_REFTAG_CHECK) {
			XNVME_DEBUG("Reference Tag should not be checked for Type 3\n");
			return false;
		}
		break;
	default:
		XNVME_DEBUG("Unknown PI Type: %d\n", pi_type);
		return false;
	}

	return true;
}

size_t
xnvme_pi_size(enum xnvme_spec_nvm_ns_pif pi_format)
{
	uint8_t size;

	if (pi_format == XNVME_SPEC_NVM_NS_16B_GUARD) {
		size = XNVME_SIZEOF_MEMBER(struct xnvme_pif, g16);
	} else {
		size = XNVME_SIZEOF_MEMBER(struct xnvme_pif, g64);
	}

	return size;
}

static uint32_t
xnvme_get_guard_interval(uint32_t block_size, uint32_t md_size, bool pi_loc, bool md_interleave,
			 size_t dif_size)
{
	if (!pi_loc) {
		/* If PI is contained in the last bytes of metadata then the
		 * CRC covers all metadata up to but excluding the last bytes.
		 */
		if (md_interleave) {
			return block_size - dif_size;
		} else {
			return md_size - dif_size;
		}
	} else {
		/* If PI is contained in the first bytes of metadata, then the
		 * CRC does not cover any metadata.
		 */
		if (md_interleave) {
			return block_size - md_size;
		} else {
			return 0;
		}
	}
}

static inline void
xnvme_pi_set_guard(struct xnvme_pif *pif, uint64_t guard, enum xnvme_spec_nvm_ns_pif pi_format)
{
	if (pi_format == XNVME_SPEC_NVM_NS_16B_GUARD) {
		xnvme_to_be16(&(pif->g16.guard), (uint16_t)guard);
	} else {
		xnvme_to_be64(&(pif->g64.guard), guard);
	}
}

static inline uint64_t
xnvme_pi_get_guard(struct xnvme_pif *pif, enum xnvme_spec_nvm_ns_pif pi_format)
{
	uint64_t guard;

	if (pi_format == XNVME_SPEC_NVM_NS_16B_GUARD) {
		guard = (uint64_t)xnvme_from_be16(&(pif->g16.guard));
	} else {
		guard = xnvme_from_be64(&(pif->g64.guard));
	}

	return guard;
}

static inline uint64_t
xnvme_pi_generate_guard(uint64_t guard_seed, void *buf, size_t buf_len,
			enum xnvme_spec_nvm_ns_pif pi_format)
{
	uint64_t guard;

	if (pi_format == XNVME_SPEC_NVM_NS_16B_GUARD) {
		guard = (uint64_t)xnvme_crc16_t10dif((uint16_t)guard_seed, buf, buf_len);
	} else {
		guard = xnvme_crc64_nvme(buf, buf_len, guard_seed);
	}

	return guard;
}

static inline void
xnvme_pi_set_apptag(struct xnvme_pif *pif, uint16_t app_tag, enum xnvme_spec_nvm_ns_pif pi_format)
{
	if (pi_format == XNVME_SPEC_NVM_NS_16B_GUARD) {
		xnvme_to_be16(&(pif->g16.app_tag), app_tag);
	} else {
		xnvme_to_be16(&(pif->g64.app_tag), app_tag);
	}
}

static inline uint16_t
xnvme_pi_get_apptag(struct xnvme_pif *pif, enum xnvme_spec_nvm_ns_pif pi_format)
{
	uint16_t app_tag;

	if (pi_format == XNVME_SPEC_NVM_NS_16B_GUARD) {
		app_tag = xnvme_from_be16(&(pif->g16.app_tag));
	} else {
		app_tag = xnvme_from_be16(&(pif->g64.app_tag));
	}

	return app_tag;
}

static inline bool
xnvme_pi_apptag_ignore(struct xnvme_pif *pif, enum xnvme_spec_nvm_ns_pif pi_format)
{
	return xnvme_pi_get_apptag(pif, pi_format) == XNVME_APPTAG_IGNORE;
}

static inline void
xnvme_pi_set_reftag(struct xnvme_pif *pif, uint64_t ref_tag, enum xnvme_spec_nvm_ns_pif pi_format)
{
	if (pi_format == XNVME_SPEC_NVM_NS_16B_GUARD) {
		xnvme_to_be32(&(pif->g16.stor_ref_space), (uint32_t)ref_tag);
	} else {
		xnvme_to_be16(&(pif->g64.stor_ref_space_p1), (uint16_t)(ref_tag >> 32));
		xnvme_to_be32(&(pif->g64.stor_ref_space_p2), (uint32_t)ref_tag);
	}
}

static inline uint64_t
xnvme_pi_get_reftag(struct xnvme_pif *pif, enum xnvme_spec_nvm_ns_pif pi_format)
{
	uint64_t ref_tag;

	if (pi_format == XNVME_SPEC_NVM_NS_16B_GUARD) {
		ref_tag = (uint64_t)xnvme_from_be32(&(pif->g16.stor_ref_space));
	} else {
		ref_tag = (uint64_t)xnvme_from_be16(&(pif->g64.stor_ref_space_p1));
		ref_tag <<= 32;
		ref_tag |= (uint64_t)xnvme_from_be32(&(pif->g64.stor_ref_space_p2));
	}

	return ref_tag;
}

static inline bool
xnvme_pi_reftag_match(struct xnvme_pif *pif, uint64_t ref_tag,
		      enum xnvme_spec_nvm_ns_pif pi_format)
{
	uint64_t _ref_tag;
	bool match;

	_ref_tag = xnvme_pi_get_reftag(pif, pi_format);

	if (pi_format == XNVME_SPEC_NVM_NS_16B_GUARD) {
		match = (_ref_tag == (ref_tag & XNVME_REFTAG_MASK_16));
	} else {
		match = (_ref_tag == (ref_tag & XNVME_REFTAG_MASK_64));
	}

	return match;
}

static inline bool
xnvme_pi_reftag_ignore(struct xnvme_pif *pif, enum xnvme_spec_nvm_ns_pif pi_format)
{
	return xnvme_pi_reftag_match(pif, XNVME_REFTAG_MASK_32, pi_format);
}

int
xnvme_pi_ctx_init(struct xnvme_pi_ctx *ctx, uint32_t block_size, uint32_t md_size,
		  bool md_interleave, bool pi_loc, enum xnvme_pi_type pi_type, uint32_t pi_flags,
		  uint32_t init_ref_tag, uint16_t apptag_mask, uint16_t app_tag,
		  enum xnvme_spec_nvm_ns_pif pi_format)
{
	if (pi_format != XNVME_SPEC_NVM_NS_16B_GUARD && pi_format != XNVME_SPEC_NVM_NS_64B_GUARD) {
		XNVME_DEBUG("No valid PI format provided.\n");
		return -EINVAL;
	}

	if (md_size < xnvme_pi_size(pi_format)) {
		XNVME_DEBUG("Metadata size is smaller than PI size.\n");
		return -EINVAL;
	}

	if (md_interleave) {
		if (block_size < md_size) {
			XNVME_DEBUG("Block size is smaller than metadata size.\n");
			return -EINVAL;
		}
	} else {
		if (pi_format == XNVME_SPEC_NVM_NS_16B_GUARD) {
			if (block_size == 0 || (block_size % 512) != 0) {
				XNVME_DEBUG("Zero block size is not allowed and should be a "
					    "multiple of 512B\n");
				return -EINVAL;
			}
		} else {
			if (block_size == 0 || (block_size % 4096) != 0) {
				XNVME_DEBUG("Zero block size is not allowed and should be a "
					    "multiple of 4kB\n");
				return -EINVAL;
			}
		}
	}

	if (!xnvme_pi_type_is_valid(pi_type, pi_flags)) {
		XNVME_DEBUG("PI type is invalid.\n");
		return -EINVAL;
	}

	ctx->block_size = block_size;
	ctx->md_size = md_size;
	ctx->md_interleave = md_interleave;
	ctx->pi_format = pi_format;
	ctx->guard_interval = xnvme_get_guard_interval(block_size, md_size, pi_loc, md_interleave,
						       xnvme_pi_size(ctx->pi_format));
	ctx->pi_type = pi_type;
	ctx->pi_flags = pi_flags;
	ctx->init_ref_tag = init_ref_tag;
	ctx->apptag_mask = apptag_mask;
	ctx->app_tag = app_tag;

	return 0;
}

void
xnvme_pi_generate(struct xnvme_pi_ctx *ctx, uint8_t *data_buf, uint8_t *md_buf,
		  uint32_t num_blocks)
{
	struct xnvme_pif *pif;
	uint32_t offset_blocks = 0;
	uint64_t guard;
	uint64_t ref_tag;

	while (offset_blocks < num_blocks) {
		guard = 0;

		if (ctx->pi_flags & XNVME_PI_FLAGS_GUARD_CHECK) {
			if (ctx->md_interleave) {
				guard = xnvme_pi_generate_guard(0, data_buf, ctx->guard_interval,
								ctx->pi_format);
			} else {
				guard = xnvme_pi_generate_guard(0, data_buf, ctx->block_size,
								ctx->pi_format);
				guard = xnvme_pi_generate_guard(guard, md_buf, ctx->guard_interval,
								ctx->pi_format);
			}
		}

		if (ctx->md_interleave) {
			pif = (struct xnvme_pif *)(data_buf + ctx->guard_interval);
		} else {
			pif = (struct xnvme_pif *)(md_buf + ctx->guard_interval);
		}

		if (ctx->pi_flags & XNVME_PI_FLAGS_GUARD_CHECK) {
			xnvme_pi_set_guard(pif, guard, ctx->pi_format);
		}

		if (ctx->pi_flags & XNVME_PI_FLAGS_APPTAG_CHECK) {
			xnvme_pi_set_apptag(pif, ctx->app_tag, ctx->pi_format);
		}

		if (ctx->pi_flags & XNVME_PI_FLAGS_REFTAG_CHECK) {
			if (ctx->pi_type != XNVME_PI_TYPE3) {
				ref_tag = ctx->init_ref_tag + offset_blocks;
			} else {
				ref_tag = ctx->init_ref_tag;
			}

			xnvme_pi_set_reftag(pif, ref_tag, ctx->pi_format);
		}

		data_buf += ctx->block_size;
		if (!ctx->md_interleave) {
			md_buf += ctx->md_size;
		}
		offset_blocks++;
	}
}

int
xnvme_pi_verify(struct xnvme_pi_ctx *ctx, uint8_t *data_buf, uint8_t *md_buf, uint32_t num_blocks)
{
	struct xnvme_pif *pif;
	uint32_t offset_blocks = 0;
	uint64_t _guard, guard = 0;
	uint16_t _app_tag;
	uint64_t ref_tag;

	while (offset_blocks < num_blocks) {
		if (ctx->pi_flags & XNVME_PI_FLAGS_GUARD_CHECK) {
			if (ctx->md_interleave) {
				guard = xnvme_pi_generate_guard(0, data_buf, ctx->guard_interval,
								ctx->pi_format);
			} else {
				guard = xnvme_pi_generate_guard(0, data_buf, ctx->block_size,
								ctx->pi_format);
				guard = xnvme_pi_generate_guard(guard, md_buf, ctx->guard_interval,
								ctx->pi_format);
			}
		}

		if (ctx->md_interleave) {
			pif = (struct xnvme_pif *)(data_buf + ctx->guard_interval);
		} else {
			pif = (struct xnvme_pif *)(md_buf + ctx->guard_interval);
		}

		switch (ctx->pi_type) {
		case XNVME_PI_TYPE1:
		case XNVME_PI_TYPE2:
			/* If Type 1 or 2 is used, then all PI checks are disabled when
			 * the Application Tag is 0xFFFF.
			 */
			if (xnvme_pi_apptag_ignore(pif, ctx->pi_format)) {
				return 0;
			}
			break;
		case XNVME_PI_TYPE3:
			/* If Type 3 is used, then all PI checks are disabled when the
			 * Application Tag is 0xFFFF and all Reference Tag bits set.
			 */
			if (xnvme_pi_apptag_ignore(pif, ctx->pi_format) &&
			    xnvme_pi_reftag_ignore(pif, ctx->pi_format)) {
				return 0;
			}
			break;
		default:
			break;
		}

		/* For type 1 and 2, the reference tag is incremented for each
		 * subsequent logical block. For type 3, the reference tag
		 * remains the same as the initial reference tag.
		 */
		if (ctx->pi_type != XNVME_PI_TYPE3) {
			ref_tag = ctx->init_ref_tag + offset_blocks;
		} else {
			ref_tag = ctx->init_ref_tag;
		}

		if (ctx->pi_flags & XNVME_PI_FLAGS_GUARD_CHECK) {
			/* Compare the PI Guard field to the CRC computed over the logical
			 * block data.
			 */
			_guard = xnvme_pi_get_guard(pif, ctx->pi_format);
			if (_guard != guard) {
				XNVME_DEBUG("Failed to compare Guard: LBA=%" PRIu64 ","
					    "  Expected=%lx, Actual=%lx\n",
					    ref_tag, _guard, guard);
				return -1;
			}
		}

		if (ctx->pi_flags & XNVME_PI_FLAGS_APPTAG_CHECK) {
			/* Compare unmasked bits in the PI Application Tag field to the
			 * passed Application Tag.
			 */
			_app_tag = xnvme_pi_get_apptag(pif, ctx->pi_format);
			if ((_app_tag & ctx->apptag_mask) != (ctx->app_tag & ctx->apptag_mask)) {
				XNVME_DEBUG("Failed to compare App Tag: LBA=%" PRIu64 ","
					    "  Expected=%x, Actual=%x\n",
					    ref_tag, (ctx->app_tag & ctx->apptag_mask),
					    (_app_tag & ctx->apptag_mask));
				return -1;
			}
		}

		if (ctx->pi_flags & XNVME_PI_FLAGS_REFTAG_CHECK) {
			switch (ctx->pi_type) {
			case XNVME_PI_TYPE1:
			case XNVME_PI_TYPE2:
				/* Compare the PI Reference Tag field to the passed Reference Tag.
				 * The passed Reference Tag will be the least significant 4 bytes
				 * or 8 bytes (depending on the PI format)
				 * of the LBA when Type 1 is used, and application specific value
				 * if Type 2 is used.
				 */
				if (!xnvme_pi_reftag_match(pif, ref_tag, ctx->pi_format)) {
					XNVME_DEBUG("Failed to compare Ref Tag: LBA=%" PRIu64 ","
						    " Expected=%lx, Actual=%lx\n",
						    ref_tag, ref_tag,
						    xnvme_pi_get_reftag(pif, ctx->pi_format));
					return -1;
				}
				break;
			case XNVME_PI_TYPE3:
				/* For Type 3, computed Reference Tag remains unchanged.
				 * Hence ignore the Reference Tag field.
				 */
				break;
			default:
				break;
			}
		}

		data_buf += ctx->block_size;
		if (!ctx->md_interleave) {
			md_buf += ctx->md_size;
		}
		offset_blocks++;
	}

	return 0;
}
