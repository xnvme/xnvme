#ifndef __LIBXNVME_SPEC_FS_H
#define __LIBXNVME_SPEC_FS_H
#include <assert.h>
#include <stdint.h>
#include <sys/types.h>
#include <libxnvme_util.h>

/**
 * Encapsulate file-system properties as Identify Controller
 */
struct xnvme_spec_fs_idfy_ctrlr {
	uint8_t byte0_519[520];

	struct {
		uint64_t direct : 1; ///< Direct I/O possible, e.g. by-passing OS caching

		uint64_t rsvd   : 63;
	} caps; ///< File System Capabilities

	struct {
		uint64_t file_data_size;  ///< Maximum size of file content, in bytes
		uint64_t file_name_len;   ///< Maximum length of file-names, in bytes
		uint64_t path_name_len;   ///< Maximum length of file-paths, in bytes
		uint64_t number_of_files; ///< Maximum number of files
	} limits;

	struct {
		uint64_t permissions_posix : 1; ///< Supports POSIX permissions
		uint64_t permissions_acl   : 1; ///< Supports Access-control-lists

		uint64_t stamp_creation    : 1;
		uint64_t stamp_access      : 1;
		uint64_t stamp_change      : 1;
		uint64_t stamp_archive     : 1;

		uint64_t hardlinks         : 1;
		uint64_t symlinks          : 1;

		uint64_t case_sensitive    : 1;
		uint64_t case_preserving   : 1;

		uint64_t journaling_block  : 1;
		uint64_t journaling_meta   : 1;

		uint64_t snapshotting      : 1;
		uint64_t compressed        : 1;
		uint64_t encrypted         : 1;

		uint64_t rsvd              : 48;
	} properties;

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

enum xnvme_spec_fs_opcs {
	XNVME_SPEC_FS_OPC_FLUSH = 0xAD,
	XNVME_SPEC_FS_OPC_WRITE = 0xAC,
	XNVME_SPEC_FS_OPC_READ  = 0xDC,
};

#define XNVME_SPEC_CSI_FS 0x1F

#endif /* __LIBXNVME_SPEC_FS_H */
