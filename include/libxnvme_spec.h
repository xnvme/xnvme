/**
 * libxnvme_spec - the definitions that no NVMe specification has
 *
 * The definitions of the NVMe specifications are in the generated headers
 * ``libxnvme_spec_<doc>.h``, one for each specification, such as
 * ``libxnvme_spec_base.h`` and ``libxnvme_spec_nvm.h``. This header holds the
 * Data Pointer as the IOCTL of the Linux Kernel NVMe driver uses it, and the
 * Protection Information as one structure of its formats.
 *
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_spec.h
 */

/**
 * Protection Information, in the format of the 16b Guard or the 64b Guard
 *
 * @struct xnvme_pif
 */
struct xnvme_pif {
	union {
		struct {
			uint16_t guard;
			uint16_t app_tag;
			uint32_t stor_ref_space;
		} g16;
		struct {
			uint64_t guard;
			uint16_t app_tag;
			uint16_t stor_ref_space_p1;
			uint32_t stor_ref_space_p2;
		} g64;
	};
};
XNVME_STATIC_ASSERT(XNVME_SIZEOF_MEMBER(struct xnvme_pif, g16) == 8, "Incorrect size");
XNVME_STATIC_ASSERT(XNVME_SIZEOF_MEMBER(struct xnvme_pif, g64) == 16, "Incorrect size");

/**
 * Data Pointer as the IOCTL of the Linux Kernel NVMe driver uses it
 *
 * @struct xnvme_spec_dptr_linux_ioctl
 */
struct xnvme_spec_dptr_linux_ioctl {
	uint64_t data;         ///< Address of the data
	uint32_t metadata_len; ///< Length of the metadata
	uint32_t data_len;     ///< Length of the data
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_dptr_linux_ioctl) == 16, "Incorrect size")
