/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_spec_fs.h
 */

#ifndef __LIBXNVME_SPEC_FS_H
#define __LIBXNVME_SPEC_FS_H

/**
 * Encapsulate file-system properties as Identify Controller
 */
struct xnvme_spec_fs_idfy_ctrlr {
	uint8_t byte0_519[520];

	uint64_t caps; ///< File System Capabilities, see XNVME_SPEC_FS_IDFY_CTRLR_CAPS_*

	struct {
		uint64_t file_data_size;  ///< Maximum size of file content, in bytes
		uint64_t file_name_len;   ///< Maximum length of file-names, in bytes
		uint64_t path_name_len;   ///< Maximum length of file-paths, in bytes
		uint64_t number_of_files; ///< Maximum number of files
	} limits;

	uint64_t properties; ///< File System Properties, see XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_*

	struct {
		uint32_t min;
		uint32_t max;
		uint32_t opt;
	} iosizes;

	uint8_t rsvd[3509];

	uint8_t ac;
	uint8_t dc;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_fs_idfy_ctrlr) == 4096, "Incorrect size")

/** Direct I/O possible, e.g. by-passing OS caching; bit 0 of 'caps' */
#define XNVME_SPEC_FS_IDFY_CTRLR_CAPS_DIRECT 0x1
/** Supports POSIX permissions; bit 0 of 'properties' */
#define XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_PERMISSIONS_POSIX 0x1
/** Supports Access-control-lists; bit 1 of 'properties' */
#define XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_PERMISSIONS_ACL 0x2
/** Keeps a time stamp of creation; bit 2 of 'properties' */
#define XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_STAMP_CREATION 0x4
/** Keeps a time stamp of access; bit 3 of 'properties' */
#define XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_STAMP_ACCESS 0x8
/** Keeps a time stamp of change; bit 4 of 'properties' */
#define XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_STAMP_CHANGE 0x10
/** Keeps a time stamp of archival; bit 5 of 'properties' */
#define XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_STAMP_ARCHIVE 0x20
/** Supports hard links; bit 6 of 'properties' */
#define XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_HARDLINKS 0x40
/** Supports symbolic links; bit 7 of 'properties' */
#define XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_SYMLINKS 0x80
/** File names are case sensitive; bit 8 of 'properties' */
#define XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_CASE_SENSITIVE 0x100
/** File names preserve their case; bit 9 of 'properties' */
#define XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_CASE_PRESERVING 0x200
/** Journals blocks; bit 10 of 'properties' */
#define XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_JOURNALING_BLOCK 0x400
/** Journals metadata; bit 11 of 'properties' */
#define XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_JOURNALING_META 0x800
/** Supports snapshots; bit 12 of 'properties' */
#define XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_SNAPSHOTTING 0x1000
/** Compresses content; bit 13 of 'properties' */
#define XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_COMPRESSED 0x2000
/** Encrypts content; bit 14 of 'properties' */
#define XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_ENCRYPTED 0x4000

/**
 * Encapsulate file properties as Identify Namespace
 */
struct xnvme_spec_fs_idfy_ns {
	uint64_t nsze; ///< Total size of the underlying file-system, in bytes
	uint64_t ncap; ///< Total size that can be allocated to a file, in bytes
	uint64_t nuse; ///< The current size of file, in bytes

	uint8_t rsvd[3816];

	uint8_t vendor_specific[254];

	uint8_t ac;
	uint8_t dc;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_fs_idfy_ns) == 4096, "Incorrect size")

/**
 * Opcodes of the commands xNVMe passes to a file, or a block device, it opens through the
 * operating system; the backend carries them out with system calls rather than send them on
 *
 * @enum xnvme_spec_fs_opcs
 */
enum xnvme_spec_fs_opcs {
	XNVME_SPEC_FS_OPC_FLUSH = 0xAD, ///< Flush the file, as xnvme_file_sync() does
	XNVME_SPEC_FS_OPC_WRITE = 0xAC, ///< Write to the file, as xnvme_file_pwrite() does
	XNVME_SPEC_FS_OPC_READ  = 0xDC, ///< Read from the file, as xnvme_file_pread() does
};

/** Command Set Identifier xNVMe gives a file, or block device, opened through the system */
#define XNVME_SPEC_CSI_FS 0x1F

#endif /* __LIBXNVME_SPEC_FS_H */
