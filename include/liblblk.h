/**
 * The User space library for Logical Block Namespaces based on xNVMe, the
 * Cross-platform libraries and tools for NVMe devices
 *
 * Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * @headerfile liblblk.h
 */
#ifndef __LIBLBLK_H
#define __LIBLBLK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libxnvme.h>

#define LBLK_SCOPY_NENTRY_MAX 128

/**
 * Logical Block Command Set opcodes
 *
 * @see Specification Section 6, figure 346
 *
 * @enum lbkl_cmd_opc
 */
enum lbkl_cmd_opc {
	LBLK_CMD_OPC_SCOPY	= 0x19,	///< LBLK_CMD_OPC_SCOPY
};

/**
 * Produces a string representation of the given ::lblk_cmd_opc
 *
 * @param opc the enum value to produce a string representation of
 *
 * @return On success, a string representation is returned. On error, the string
 * "LBLK_CMD_OPC_ENOSYS" is returned.
 */
const char *
lblk_cmd_opc_str(enum lbkl_cmd_opc opc);

/**
 * Command-set specific status codes related to Logical Block Namespaces
 *
 * @see Specification Section 6.TBD.1, figure 355
 */
enum lblk_status_code {
	/// Copy Command Specific Status Values
	LBLK_SC_WRITE_TO_RONLY	= 0x82,	///< LBLK_SC_WRITE_TO_RONLY
};

/**
 * Produces a string representation of the given ::lblk_status_code
 *
 * @param sc the enum value to produce a string representation of
 *
 * @return On success, a string representation is returned. On error, the string
 * "LBLK_SC_ENOSYS" is returned.
 */
const char *
lblk_status_code_str(enum lblk_status_code sc);

/**
 * @struct lblk_source_range_entry
 */
struct lblk_source_range_entry {
	uint8_t rsvd0[8];

	uint64_t slba;		///< Start LBA

	uint32_t nlb	: 16;	///< Number of logical blocks, zero-based

	uint32_t rsvd20	: 16;

	uint32_t eilbrt;	///< Expected Initial Logical Block Ref. Tag
	uint32_t elbatm;	///< Expected Logical Block App. Tag Mask
	uint32_t elbat;		///< Expected Logical Block App. Tag
};
XNVME_STATIC_ASSERT(sizeof(struct lblk_source_range_entry) == 32, "Incorrect size")

/**
 * Prints the given ::lblk_source_range_entry to the given output stream
 *
 * @param stream output stream used for printing
 * @param entry pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
lblk_source_range_entry_fpr(FILE *stream,
			    const struct lblk_source_range_entry *entry,
			    int opts);

/**
 * Prints the given ::lblk_source_range_entry to stdout
 *
 * @param entry pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
lblk_source_range_entry_pr(const struct lblk_source_range_entry *entry,
			   int opts);

/**
 * @see Specification Section 6.TBD.1
 */
struct lblk_source_range {
	struct lblk_source_range_entry entry[LBLK_SCOPY_NENTRY_MAX];
};
XNVME_STATIC_ASSERT(sizeof(struct lblk_source_range) == 4096, "Incorrect size")

/**
 * Prints the given ::lblk_source_range to the given output stream
 *
 * @param stream output stream used for printing
 * @param srange pointer to structure to print
 * @param nr nr=0: print ::LBLK_SCOPY_NENTRY_MAX entries from the given , nr>0:
 * print no more than nr
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
lblk_source_range_fpr(FILE *stream, const struct lblk_source_range *srange,
		      uint8_t nr, int opts);

/**
 * Prints the given ::lblk_source_range to stdout
 *
 * @param srange pointer to structure to print
 * @param nr nr=0: print ::LBLK_SCOPY_NENTRY_MAX entries from the given , nr>0:
 * print no more than nr
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
lblk_source_range_pr(const struct lblk_source_range *srange, uint8_t nr,
		     int opts);

/**
 * NVMe Command Accessor for command with opcode LBLK_CMD_OPC_COPY
 *
 * @struct lblk_cmd_scopy
 */
struct lblk_cmd_scopy {
	uint32_t cdw00_09[10];		///< Command dword 0 to 9

	/* cdw 10-11 */
	uint64_t sdlba;			///< Start Destination LBA

	/* cdw 12 */
	uint32_t nr		: 8;	///< Number of Ranges

	uint32_t rsvd1		: 4;

	uint32_t prinfor	: 4;	///< Protection Info. Field Read
	uint32_t df		: 4;	///< Descriptor Format
	uint32_t dtype		: 4;	///< Directive Type
	uint32_t rsvd2		: 2;
	uint32_t prinfow	: 4;	///< Protection Info. Field Write
	uint32_t fua		: 1;	///< Force Unit Access
	uint32_t lr		: 1;	///< Limited Retry

	/* cdw 13 */
	uint32_t rsvd3		: 16;
	uint32_t dspec		: 16;	///< Directive Specific

	/* cdw 14 */
	uint32_t ilbrt;			///< Initial Logical Block Ref. Tag

	/* cdw 15 */
	uint32_t lbat		: 16;	///< Logical Block App. Tag
	uint32_t lbatm		: 16;	///< Logical Block App. Tag Mask

};
XNVME_STATIC_ASSERT(sizeof(struct lblk_cmd_scopy) == 64, "Incorrect size")

/**
 * NVMe Command Accessors for the Logical Block Command Set
 *
 * @struct znd_cmd
 */
struct lblk_cmd {
	union {
		struct xnvme_spec_cmd base;
		struct xnvme_spec_cmd_common common;
		struct lblk_cmd_scopy copy;
		uint32_t cdw[16];
	};
};
XNVME_STATIC_ASSERT(sizeof(struct lblk_cmd) == 64, "Incorrect size")

struct lblk_idfy_ctrlr {
	uint8_t byte0_519[520];

	/** optional nvm command support */
	union {
		struct {
			uint16_t compare		: 1;
			uint16_t write_unc		: 1;
			uint16_t dsm			: 1;
			uint16_t write_zeroes		: 1;
			uint16_t set_features_save	: 1;
			uint16_t reservations		: 1;
			uint16_t timestamp		: 1;
			uint16_t verify			: 1;
			uint16_t copy			: 1;
			uint16_t reserved		: 7;
		};
		uint16_t val;
	} oncs;

	uint8_t byte522_543[22];

	union {
		struct {
			uint16_t copy_fmt0	: 1;
			uint16_t rsvd		: 15;
		};
		uint16_t val;
	} ocfs;	///< Optional Copy Format Supported

	uint8_t byte546_4095[3549];
};
XNVME_STATIC_ASSERT(sizeof(struct lblk_idfy_ctrlr) == 4096, "Incorrect size")

/**
 * Prints the given ::lblk_idfy_ctrlr to the given output stream
 *
 * Only fields specific to Logical Block Namespaces are printed by this function
 *
 * @param stream output stream used for printing
 * @param idfy pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
lblk_idfy_ctrlr_fpr(FILE *stream, struct lblk_idfy_ctrlr *idfy, int opts);

/**
 * Prints the given ::lblk_idfy_ctrlr to stdout
 *
 * @param idfy pointer to ::lblk_idfy_ctrlr
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
lblk_idfy_ctrlr_pr(struct lblk_idfy_ctrlr *idfy, int opts);

struct lblk_idfy_ns {
	uint8_t byte0_75[76];

	uint32_t mcl;	///< Maximum Copy Length
	uint16_t mssrl;	///< Maximum Single Source Range Length
	uint8_t msrc;	///< Maximum Source Range Count

	uint8_t byte83_4095[4013];
};
XNVME_STATIC_ASSERT(sizeof(struct lblk_idfy_ns) == 4096, "Incorrect size")

/**
 * Prints the given ::lblk_idfy_ns to the given output stream
 *
 * Only fields specific to Logical Block Namespaces are printed by this function
 *
 * @param stream output stream used for printing
 * @param idfy pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
lblk_idfy_ns_fpr(FILE *stream, struct lblk_idfy_ns *idfy, int opts);

/**
 * Prints the given ::lblk_idfy_ns to stdout
 *
 * @param idfy pointer to ::lblk_idfy_ns
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
lblk_idfy_ns_pr(struct lblk_idfy_ns *idfy, int opts);

/*
 * Representation of the NVMe Identify Namespace command completion result
 *
 * @struct znd_idfy
 */
struct lblk_idfy {
	union {
		struct xnvme_spec_idfy base;
		struct lblk_idfy_ctrlr ctrlr;
		struct lblk_idfy_ns ns;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct lblk_idfy) == 4096, "Incorrect size")

/**
 * Submit, and optionally wait for completion of, a NVMe Copy Command
 *
 * @see xnvme_cmd_opts
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace Identifier
 * @param sdlba The Starting Destination LBA to start copying to
 * @param ranges Pointer to ranges-buffer, see ::lblk_source_range_entry
 * @param nr Number of ranges in the given ranges-buffer, zero-based value
 * @param opts command options, see ::xnvme_cmd_opts
 * @param ret Pointer to structure for NVMe completion and async. context
 *
 * @return On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate and `ret` filled with lower-level status codes
 */
int
lblk_cmd_scopy(struct xnvme_dev *dev, uint32_t nsid, uint64_t sdlba,
	       struct lblk_source_range_entry *ranges, uint8_t nr,
	       int opts, struct xnvme_req *ret);

#ifdef __cplusplus
}
#endif

#endif /* __LIBLBLK_H */
