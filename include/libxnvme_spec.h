/**
 * libxnvme_spec - the definitions that no NVMe specification has
 *
 * The definitions of the NVMe specifications are in the generated headers
 * ``libxnvme_spec_<doc>.h``, one for each specification, such as
 * ``libxnvme_spec_base.h`` and ``libxnvme_spec_nvm.h``. This header holds the
 * Data Pointer as the IOCTL of the Linux Kernel NVMe driver uses it, and the
 * Protection Information as a union of its formats.
 *
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_spec.h
 */

/**
 * Protection Information in the 16b Guard Protection Information format
 *
 * The specification gives these fields no abbreviation; the members are named by their titles.
 * Each is big-endian.
 *
 * @see NVM Command Set 1.3, Figures 155 and 156
 * @struct xnvme_pif_g16
 */
struct xnvme_pif_g16 {
	uint16_t guard;          ///< Guard, bytes 1:0
	uint16_t app_tag;        ///< Application Tag, bytes 3:2
	uint32_t stor_ref_space; ///< Storage and Reference Space, bytes 7:4
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_pif_g16) == 8, "Incorrect size")

/**
 * Protection Information in the 32b Guard Protection Information format
 *
 * @see NVM Command Set 1.3, Figure 157
 * @struct xnvme_pif_g32
 */
struct xnvme_pif_g32 {
	uint32_t guard;             ///< Guard, bytes 3:0
	uint16_t app_tag;           ///< Application Tag, bytes 5:4
	uint8_t stor_ref_space[10]; ///< Storage and Reference Space, bytes 15:6, 80 bits
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_pif_g32) == 16, "Incorrect size")

/**
 * Protection Information in the 64b Guard Protection Information format
 *
 * @see NVM Command Set 1.3, Figure 159
 * @struct xnvme_pif_g64
 */
struct xnvme_pif_g64 {
	uint64_t guard;            ///< Guard, bytes 7:0
	uint16_t app_tag;          ///< Application Tag, bytes 9:8
	uint8_t stor_ref_space[6]; ///< Storage and Reference Space, bytes 15:10, 48 bits
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_pif_g64) == 16, "Incorrect size")

/**
 * Protection Information, in the format of the namespace, see enum
 * xnvme_spec_nvm_pif
 *
 * @union xnvme_pif
 */
union xnvme_pif {
	struct xnvme_pif_g16 g16; ///< As 16b Guard Protection Information
	struct xnvme_pif_g32 g32; ///< As 32b Guard Protection Information
	struct xnvme_pif_g64 g64; ///< As 64b Guard Protection Information
};
XNVME_STATIC_ASSERT(sizeof(union xnvme_pif) == 16, "Incorrect size")

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
