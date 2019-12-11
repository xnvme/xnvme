/**
 * The User space library for Zoned Namespaces based on xNVMe, the
 * Cross-platform libraries and tools for NVMe devices
 *
 * Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * @headerfile libznd.h
 */
#ifndef __LIBZND_H
#define __LIBZND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libxnvme.h>

/**
 * Zoned Command Set opcodes
 *
 * @see Specification Section 6, figure ZONEDOPCODES
 *
 * @struct znd_cmd_opc
 */
enum znd_cmd_opc {
	ZND_CMD_OPC_MGMT_SEND	= 0x79,	///< ZND_CMD_OPC_MGMT_SEND
	ZND_CMD_OPC_MGMT_RECV	= 0x7A,	///< ZND_CMD_OPC_MGMT_RECV
	ZND_CMD_OPC_APPEND	= 0x7D,	///< ZND_CMD_OPC_APPEND
};

/**
 * Log identifiers for Zoned Namespaces
 *
 * @enum znd_cmd_log_lid
 */
enum znd_cmd_log_lid {
	ZND_CMD_LOG_CHANGES = 0xBF,	///< ZND_CMD_LOG_CHANGES
};

/**
 * NVMe Command Accessor for command with opcode ZND_CMD_OPC_MGMT_SEND
 *
 * @struct znd_cmd_mgmt_send
 */
struct znd_cmd_mgmt_send {
	uint32_t cdw00_09[10];		///< Command dword 0 to 9

	/* cdw 10-11 */
	uint64_t slba;			///< Start LBA

	uint32_t cdw12;

	/* cdw 13 */
	uint32_t zsa		: 8;	///< Zone Send Action
	uint32_t zsasf		: 1;	///< Select All
	uint32_t rsvd		: 23;

	uint32_t cdw14_15[2];		///< Command dword 14 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct znd_cmd_mgmt_send) == 64, "Incorrect size")

/**
 * NVMe Command Accessor for command with opcode ZND_CMD_OPC_MGMT_RECV
 *
 * @struct znd_cmd_mgmt_recv
 */
struct znd_cmd_mgmt_recv {
	uint32_t cdw00_09[10];	///< Command dword 0 to 9

	/* cdw 10-11 */
	uint64_t slba;			///< Start LBA

	/* cdw 12 */
	uint32_t ndwords;	///< Number of dwords in data-payload

	/* cdw 13 */
	uint32_t zra		: 8;	///< Zone Receive Action
	uint32_t zrasf		: 8;	///< Zone Receive Action Specific Field
	uint32_t partial	: 1;	///< Partial

	uint32_t rsvd		: 15;

	/* cdw 14-15 */
	uint64_t addrs_dst;		///< destination addresses
};
XNVME_STATIC_ASSERT(sizeof(struct znd_cmd_mgmt_recv) == 64, "Incorrect size")

/**
 * NVMe Command Accessor for command with opcode ZND_CMD_OPC_APPEND
 *
 * @struct znd_cmd_append
 */
struct znd_cmd_append {
	uint32_t cdw00_09[10];	///< Command dword 0 to 9

	uint64_t zslba;		///< SLBA: Start Logical Block Address

	uint32_t nlb	: 16;	///< NLB: Number of logical blocks
	uint32_t rsvd	:  4;
	uint32_t dtype	:  4;	///< DT: Directive Type
	uint32_t prinfo	:  4;	///< PI: Protection Information Field
	uint32_t rsvd2	:  2;
	uint32_t fua	:  1;	///< FUA: Force unit access
	uint32_t lr	:  1;	///< LR: Limited retry

	uint32_t cdw13_15[3];	///< Command dword 13 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct znd_cmd_append) == 64, "Incorrect size")

/**
 * NVMe Command Accessors for the Zoned Command Set
 *
 * @struct znd_cmd
 */
struct znd_cmd {
	union {
		struct xnvme_spec_cmd base;
		struct xnvme_spec_cmd_common common;
		struct znd_cmd_mgmt_send mgmt_send;
		struct znd_cmd_mgmt_recv mgmt_recv;
		struct znd_cmd_append append;
		uint32_t cdw[16];
	};
};
XNVME_STATIC_ASSERT(sizeof(struct znd_cmd) == 64, "Incorrect size")

/**
 * Command-set specific status codes related to Zoned Namespaces
 *
 * @see Specification Section 4.1.1.1.1, figures GENERICSTATUSCODES and
 * STATUSCODES
 */
enum znd_status_code {
	// Namespace Management
	ZND_SC_INVALID_FORMAT	= 0x7F,	///< ZND_SC_INVALID_FORMAT

	/// Zoned Command Set
	ZND_SC_BOUNDARY_ERROR	= 0xB8,	///< ZND_SC_BOUNDARY_ERROR
	ZND_SC_IS_FULL		= 0xB9,	///< ZND_SC_IS_FULL
	ZND_SC_IS_READONLY	= 0xBA,	///< ZND_SC_IS_READONLY
	ZND_SC_IS_OFFLINE	= 0xBB,	///< ZND_SC_IS_OFFLINE
	ZND_SC_INVALID_WRITE	= 0xBC,	///< ZND_SC_INVALID_WRITE
	ZND_SC_TOO_MANY_ACTIVE	= 0xBD,	///< ZND_SC_TOO_MANY_ACTIVE
	ZND_SC_TOO_MANY_OPEN	= 0xBE,	///< ZND_SC_TOO_MANY_OPEN
	ZND_SC_INVALID_TRANS	= 0xBF,	///< ZND_SC_INVALID_TRANS
};

/**
 * Produces a string representation of the given ::znd_status_code
 *
 * @param sc the enum value to produce a string representation of
 *
 * @return On success, a string representation is returned. On error, the string
 * "ZND_SC_ENOSYS" is returned.
 */
const char *
znd_status_code_str(enum znd_status_code sc);

/**
 * This is not defined in any spec... it would just seem sane if it was...
 */
enum znd_send_action_sf {
	ZND_SEND_SF_SALL	= 0x1,		///< ZND_SEND_SF_SALL
};

/**
 * Produces a string representation of the given ::znd_send_action_sf
 *
 * @param sf The send action specific field value to produce a string
 * representation of
 *
 * @return On success, a string representation is returned. On error, the string
 * "ZND_SEND_SF_ENOSYS" is returned.
 */
const char *
znd_send_action_sf_str(enum znd_send_action_sf sf);

/**
 * Actions for the Zone Management Send command
 *
 * @see Specification Section 6.1, figure TBDZMDW13
 *
 * @enum znd_send_action
 */
enum znd_send_action {
	ZND_SEND_CLOSE		= 0x1,	///< ZND_SEND_CLOSE
	ZND_SEND_FINISH		= 0x2,	///< ZND_SEND_FINISH
	ZND_SEND_OPEN		= 0x3,	///< ZND_SEND_OPEN
	ZND_SEND_RESET		= 0x4,	///< ZND_SEND_RESET
	ZND_SEND_OFFLINE	= 0x5,	///< ZND_SEND_OFFLINE
	ZND_SEND_DESCRIPTOR	= 0x10,	///< ZND_SEND_DESCRIPTOR
};

/**
 * Produces a string representation of the given ::znd_send_action
 *
 * @param action The action to produce a string representation of
 *
 * @return On success, a string representation is returned. On error, the string
 * "ZND_SEND_ENOSYS" is returned.
 */
const char *
znd_send_action_str(enum znd_send_action action);

/**
 * Zone Receive Action (::znd_send_action) specific field values
 *
 * @see Specification Section 6.2, figure TBDZMRDW13
 *
 * @enum znd_recv_action_sf
 */
enum znd_recv_action_sf {
	ZND_RECV_SF_ALL		= 0x0,	///< ZND_RECV_SF_ALL
	ZND_RECV_SF_EMPTY	= 0x1,	///< ZND_RECV_SF_EMPTY
	ZND_RECV_SF_IOPEN	= 0x2,	///< ZND_RECV_SF_IOPEN
	ZND_RECV_SF_EOPEN	= 0x3,	///< ZND_RECV_SF_EOPEN
	ZND_RECV_SF_CLOSED	= 0x4,	///< ZND_RECV_SF_CLOSED
	ZND_RECV_SF_FULL	= 0x5,	///< ZND_RECV_SF_FULL
	ZND_RECV_SF_RONLY	= 0x6,	///< ZND_RECV_SF_RONLY
	ZND_RECV_SF_OFFLINE	= 0x7,	///< ZND_RECV_SF_OFFLINE
};

/**
 * Produces a string representation of the given ::znd_recv_action_sf
 *
 * @param sf The send action specific field value to produce a string
 * representation of
 *
 * @return On success, a string representation is returned. On error, the string
 * "ZND_RECV_SF_ENOSYS" is returned.
 */
const char *
znd_recv_action_sf_str(enum znd_recv_action_sf sf);

/**
 * Zoned Receive Action
 *
 * @see Specification Section 6.2, figure TBDZMRDW13
 * @enum znd_recv_action
 */
enum znd_recv_action {
	ZND_RECV_REPORT		= 0x0,	///< ZND_RECV_REPORT
	ZND_RECV_EREPORT	= 0x1,	///< ZND_RECV_EREPORT
};

/**
 * Produces a string representation of the given ::znd_recv_action
 *
 * @param action The receive action to produce a string representation of
 *
 * @return On success, a string representation is returned. On error, the string
 * "ZND_RECV_ENOSYS" is returned.
 */
const char *
znd_recv_action_str(enum znd_recv_action action);

/**
 * Zone Type
 *
 * @see Specification Section 6.2.2.3, figure TBDZMRD
 * @enum znd_type
 */
enum znd_type {
	ZND_TYPE_SEQWR = 0x2,	///< ZND_TYPE_SEQWR
};

/**
 * Produces a string representation of the given ::znd_type
 *
 * @param zt the ::znd_type to produce a string representation of
 *
 * @return On success, a string representation is returned. On error, the string
 * "ZND_TYPE_ENOSYS" is returned.
 */
const char *
znd_type_str(enum znd_type zt);

/**
 * Zone State as reported by Zone Management Receive
 *
 * @see Specification Section 6.2.2.3, figure TBDZMRZD
 * @enum znd_state
 */
enum znd_state {
	ZND_STATE_EMPTY		= 0x1,	///< ZND_STATE_EMPTY
	ZND_STATE_IOPEN		= 0x2,	///< ZND_STATE_IOPEN
	ZND_STATE_EOPEN		= 0x3,	///< ZND_STATE_EOPEN
	ZND_STATE_CLOSED	= 0x4,	///< ZND_STATE_CLOSED
	ZND_STATE_RONLY		= 0xD,	///< ZND_STATE_RONLY
	ZND_STATE_FULL		= 0xE,	///< ZND_STATE_FULL
	ZND_STATE_OFFLINE	= 0xF,	///< ZND_STATE_OFFLINE
};

/**
 * Produces a string representation of the given ::znd_state
 *
 * @param state the enum value produce a string representation of
 *
 * @return On success, a string representation is returned. On error, the string
 * "ZND_STATE_ENOSYS" is returned.
 */
const char *
znd_state_str(enum znd_state state);

/**
 * Identify controller accessor only for Zoned specific fields
 *
 * @see Specification, section 3.1.2
 * @struct znd_idfy_ctrlr
 */
struct znd_idfy_ctrlr {
	uint8_t zasl;		///< Zone Append Size Limit

	uint8_t rsvd8[4095];
};
XNVME_STATIC_ASSERT(sizeof(struct znd_idfy_ctrlr) == 4096, "Incorrect size")

/**
 * Prints the given ::znd_idfy_ctrlr to the given output stream
 *
 * Only fields specific to Zoned Namespaces are printed by this function
 *
 * @param stream output stream used for printing
 * @param zctrlr pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
znd_idfy_ctrlr_fpr(FILE *stream, struct znd_idfy_ctrlr *zctrlr, int opts);

/**
 * Prints the given ::znd_idfy_ctrlr to the given output stream
 *
 * Only fields specific to Zoned Namespaces are printed by this function
 *
 * @param zctrlr pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
znd_idfy_ctrlr_pr(struct znd_idfy_ctrlr *zctrlr, int opts);

/**
 * LBA Format Extension
 *
 * @see Specification Section 5.1.2, figure LBAFE
 *
 * @struct znd_idfy_lbafe
 */
struct znd_idfy_lbafe {
	uint64_t zsze;		///< Zone Size in number of logical blocks
	uint8_t zdes;		///< Zone Descriptor Extensions Size

	uint8_t rsvd[7];
};
XNVME_STATIC_ASSERT(sizeof(struct znd_idfy_lbafe) == 16, "Incorrect size")

/**
 * Prints the given ::znd_idfy_lbafe to the given output stream
 *
 * @param stream output stream used for printing
 * @param zonef pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
znd_idfy_lbafe_fpr(FILE *stream, struct znd_idfy_lbafe *zonef, int opts);

/**
 * Zoned Command Set specific identify namespace data structure
 *
 * @see TP4053, section 3
 * @struct znd_idfy_ns
 */
struct znd_idfy_ns {
	struct {
		uint16_t vzcap	: 1;	///< Variable Zone Capacity
		uint16_t zae	: 1;	///< Zone Active Excursions

		uint16_t rsvd	: 14;
	} zoc; ///< Zone Operation Characteristics

	struct {
		uint16_t razb	: 1;	///< Read Across zone boundaries

		uint16_t rsvd	: 15;
	} ozcs; ///< Optional Zoned Command Support

	uint32_t mar;		///< Maximum Active Resources
	uint32_t mor;		///< Maximum Open Resources

	uint32_t rrl;		///< Reset Recommended Limit
	uint32_t frl;		///< Finish Recommended Limit

	uint8_t rsvd20_2815[2796];

	struct znd_idfy_lbafe lbafe[16];

	uint8_t rsvd3072_3839[768];

	uint8_t vs[256];	///< Vendor Specific
};
XNVME_STATIC_ASSERT(sizeof(struct znd_idfy_ns) == 4096, "Incorrect size")

/**
 * Prints the given ::znd_idfy_ns to the given output stream
 *
 * Only fields specific to Zoned Namespaces are printed by this function
 *
 * @param stream output stream used for printing
 * @param zns pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
znd_idfy_ns_fpr(FILE *stream, struct znd_idfy_ns *zns, int opts);

/**
 * Prints the given ::znd_idfy_ns to stdout
 *
 * @param zns pointer to ::znd_idfy_ns
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
znd_idfy_ns_pr(struct znd_idfy_ns *zns, int opts);

/**
 * Representation of the NVMe Identify Namespace command completion result
 *
 * @struct znd_idfy
 */
struct znd_idfy {
	union {
		struct xnvme_spec_idfy base;
		struct znd_idfy_ctrlr zctrlr;
		struct znd_idfy_ns zns;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct znd_idfy) == 4096, "Incorrect size")

#define ZND_CHANGES_LEN 511

/**
 * Zone Identifier List as returned by get-log-page
 *
 * @see Specification Section 5.3.1.1, figure TBDZONELIST
 *
 * @struct znd_changes
 */
struct znd_changes {
	uint16_t nidents;			///< Number of Zone Identifiers
	uint8_t rsvd2[6];
	uint64_t idents[ZND_CHANGES_LEN];	///< Zone Identifiers
};
XNVME_STATIC_ASSERT(sizeof(struct znd_changes) == 4096, "Incorrect size")

/**
 * Prints the given ::znd_changes to the given output stream
 *
 * @param stream output stream used for printing
 * @param changes pointer to subject to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
znd_changes_fpr(FILE *stream, const struct znd_changes *changes, int opts);

/**
 * Pretty-printer of ::znd_changes
 *
 * @param changes pointer to subject to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
znd_changes_pr(const struct znd_changes *changes, int opts);

/**
 * Zone Descriptor as reported by Zone Management Receive command
 *
 * @see Specification Section 6.2.2.3, figure TBDZMRZD
 */
struct znd_descr {
	uint8_t zt		: 4;	///< Zone Type
	uint8_t rsvd0		: 4;

	uint8_t rsvd1		: 4;
	uint8_t zs		: 4;	///< Zone State

	/**
	 * Zone Attributes
	 */
	union {
		struct {
			uint8_t zfc: 1;		///< Zone Finished by controller
			uint8_t zfr: 1;		///< Zone Finish Recommended
			uint8_t rzr: 1;		///< Reset Zone Recommended

			uint8_t rsvd3 : 4;

			uint8_t zdev: 1;	///< Zone Descriptor Valid
		};
		uint8_t val;
	} za;

	uint8_t rsvd7[5];

	uint64_t zcap;			///< Zone Capacity (in number of LBAs)
	uint64_t zslba;			///< Zone Start LBA
	uint64_t wp;			///< Write Pointer
	uint8_t rsvd63[32];
};
XNVME_STATIC_ASSERT(sizeof(struct znd_descr) == 64, "Incorrect size")

/**
 * Prints the given ::znd_descr to the given stream
 *
 * @param stream output stream used for printing
 * @param descr the struct to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
znd_descr_fpr(FILE *stream, const struct znd_descr *descr, int opts);

/**
 * Prints the given ::znd_descr to stdout
 *
 * @param descr the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
znd_descr_pr(const struct znd_descr *descr, int opts);

/**
 * Report Zones Data Structure header
 *
 * The first 16 bytes of the two Report Zoned data structures
 *
 * @see Specification Section 6.2.2.1, figure TBDZMRRZ
 * @see Specification Section 6.2.2.2, figure TBDZMRERZ
 * @struct znd_rprt_hdr
 */
struct znd_rprt_hdr {
	/**
	 * Number of zones in namespace when receiving with partial=0
	 * Number of zones in the data-buffer, when receiving with partial=1
	 */
	uint64_t nzones;
	uint8_t rsvd[56];
};
XNVME_STATIC_ASSERT(sizeof(struct znd_rprt_hdr) == 64, "Incorrect size")

/**
 * Print the given ::znd_rprt_hdr to the given output stream
 *
 * @param stream output stream used for printing
 * @param hdr the struct to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
znd_rprt_hdr_fpr(FILE *stream, const struct znd_rprt_hdr *hdr, int opts);

/**
 * Print the given ::znd_rprt_hdr to the stdout
 *
 * @param hdr the struct to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
znd_rprt_hdr_pr(const struct znd_rprt_hdr *hdr, int opts);

/**
 * Submit, and optionally wait for completion of, a Zone Management Receive
 *
 * @note When `opts | CMD_MODE_SYNC` then `ret` is filled with completion entry
 * upon return
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace Identifier
 * @param slba Start LBA of the Zone to receive for
 * @param action the ::znd_recv_action to perform
 * @param sf the ::znd_recv_action_sf option
 * @param partial partial request
 * @param dbuf pointer to data payload
 * @param dbuf_nbytes pointer to meta payload
 * @param opts command-options, see ::xnvme_cmd_opts
 * @param ret Pointer to structure for async. context and NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
znd_cmd_mgmt_recv(struct xnvme_dev *dev, uint32_t nsid, uint64_t slba,
		  enum znd_recv_action action, enum znd_recv_action_sf sf,
		  uint8_t partial, void *dbuf, uint32_t dbuf_nbytes,
		  int opts, struct xnvme_req *ret);

/**
 * Submit, and optionally wait for completion of, a Zone Management Send
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace Identifier
 * @param zslba Start LBA of the Zone to manage
 * @param action Management action to perform with zone at zslba
 * @param sf the ::znd_send_action_sf option
 * @param dbuf For action=ZND_SEND_DESCRIPTOR provide buffer
 * @param opts command-options, see ::xnvme_cmd_opts
 * @param ret Pointer to structure for async. context and NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
znd_cmd_mgmt_send(struct xnvme_dev *dev, uint32_t nsid, uint64_t zslba,
		  enum znd_send_action action, enum znd_send_action_sf sf,
		  void *dbuf, int opts, struct xnvme_req *ret);

/**
 * Submit, and optionally wait for completion of, a Zone Append
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace Identifier
 * @param zslba First LBA of the Zone to append to
 * @param nlb number of LBAs, this is zero-based value
 * @param dbuf pointer to data payload
 * @param mbuf pointer to meta payload
 * @param opts command-options, see ::xnvme_cmd_opts
 * @param ret Pointer to structure for async. context and NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
znd_cmd_append(struct xnvme_dev *dev, uint32_t nsid, uint64_t zslba,
	       uint16_t nlb, const void *dbuf, const void *mbuf,
	       int opts, struct xnvme_req *ret);

/**
 * Encapsulation of Zone Descriptors and Zone Descriptor Extensions
 *
 * This is not the structure from the Specification, rather, this encapsulates
 * it and provides MACRO-accessors and helper-functions to retrieve
 * zone-descriptors and when supported, extensions.
 *
 * For simplicity then only used the macros:
 *
 * ZND_REPORT_DESCR(rprt, nth)	- Access the 'nth' descriptor (zero-based)
 * ZND_REPORT_DEXT(rprt, nth)	- Access the 'nth' descr. extension (zero-based)
 *
 * However, when zdes_bytes is 0, then `descr` is accessible as a regular array
 * When zdes > 0, then the ZND_REPORT_DESCR should be used to access Zone
 * Descriptors and ZND_REPORT_DEXT should be used to access Zone Descriptor
 * extension
 *
 * @struct znd_report
 */
struct znd_report {
	uint64_t nzones;		///< Nr. of zones in device
	uint32_t zd_nbytes;		///< Zone Descriptor in 'B'
	uint32_t zdext_nbytes;		///< Zone Descriptor Extension in 'B'

	uint64_t zslba;			///< First Zone in the report
	uint64_t zelba;			///< Last Zone in the report
	uint32_t nentries;		///< Nr. of entries in report

	uint8_t extended;		///< Whether this report has extensions
	uint8_t _pad[3];
	uint64_t zrent_nbytes;		///< Zone Report Entry Size in 'B'

	uint64_t report_nbytes;		///< Size of this struct in bytes
	uint64_t entries_nbytes;	///< Size of the entries in bytes

	///< Array of structs, column format: `descr[,descr_ext]`
	uint8_t storage[];
};
XNVME_STATIC_ASSERT(sizeof(struct znd_report) == 64, "Incorrect size")

/**
 * Prints the given ::znd_report to the given stream
 *
 * @param stream output stream used for printing
 * @param report pointer to the the ::znd_report to print
 * @param flags
 *
 * @return On success, the number of characters printed is returned.
 */
int
znd_report_fpr(FILE *stream, const struct znd_report *report, int flags);

/**
 * Prints the given ::znd_report to stdout
 *
 * @param report
 * @param flags
 *
 * @return On success, the number of characters printed is returned.
 */
int
znd_report_pr(const struct znd_report *report, int flags);

/**
 * Access Zone Descriptors, by entry index, in the given znd_report
 */
#define ZND_REPORT_DESCR(rprt, nth) \
	((struct znd_descr *)&(rprt->storage[nth * rprt->zrent_nbytes]))

/**
 * Access Zone Descriptors Extensions, by entry index, in the given znd_report
 */
#define ZND_REPORT_DEXT(rprt, nth) \
	((!rprt->extended) ? \
	 NULL \
	 : \
	 ((void *)&rprt->storage[nth * rprt->zrent_nbytes + rprt->zd_nbytes]))

/**
 * Retrieves a Zone Report from the namespace associated with the given `dev`,
 * reporting starting at the given `slba`, and limited to `limit` entries
 *
 * @note
 * Caller is responsible for de-allocating the returned structure using
 * xnvme_buf_virt_free()
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param slba LBA of the first zone in the report
 * @param limit when 0 then provide a report for all zones start from slba.
 * Otherwise, provide a report for [slba, slba+limit]
 * @param extended When 0, the "regular" report is provided. When 1, then the
 * Extended Report is provided, if supported by device.
 *
 * @return On success, pointer to zoned report. On error, NULL is returned and
 * `errno` is set to indicate the error
 */
struct znd_report *
znd_report_from_dev(struct xnvme_dev *dev, uint64_t slba, size_t limit, uint8_t extended);

/**
 * Scan the 'report' for a zone in the given 'state' and store it in 'zlba'
 *
 * @param report The report to scan
 * @param state The zone-state to scan for
 * @param zlba Pointer to store zlba to
 * @param opts Optional seed random-number generator, when 0 then time(NULL) is
 * used
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
znd_report_find_arbitrary(const struct znd_report *report, enum znd_state state,
			  uint64_t *zlba, int opts);

/**
 * Fills 'zdescr' with the Zone on the given 'dev' that starts at 'slba'
 *
 * @param dev The device to receive descriptors from
 * @param slba The Zone Start LBA
 * @param zdescr Pointer to the descriptor the function should fill
 *
 * @return On succes, 0 is returned and 'zdescr' filled with matching Zone
 * Descriptor. On error, negative ``errno`` is returned to indicate the error,
 * and the content of given 'zdescr' is undefined.
 */
int
znd_descr_from_dev(struct xnvme_dev *dev, uint64_t slba,
		   struct znd_descr *zdescr);

/**
 * Fills 'zdescr' with the first Zone on the given 'dev' in the given 'state'
 *
 * @param dev The device to receive descriptors from
 * @param state The state the Zone should be in
 * @param zdescr Pointer to the descriptor the function should fill
 *
 * @return On succes, 0 is returned and 'zdescr' filled with matching Zone
 * Descriptor. On error, negative ``errno`` is returned to indicate the error,
 * and the content of given 'zdescr' is undefined.
 */
int
znd_descr_from_dev_in_state(struct xnvme_dev *dev, enum znd_state state,
			    struct znd_descr *zdescr);

/**
 * Ask the given device for how many zones in the given state via receive-action
 *
 * @param dev The device to stat
 * @param sfield Receive stats of the given ::znd_recv_action_sf
 * @param nzones Pointer to store result in
 *
 * @return On Success, 0 is returned and 'nzones' filled with stat. On error,
 * negative ``errno`` is returned to indicate the error.
 */
int
znd_stat_dev(struct xnvme_dev *dev, enum znd_recv_action_sf sfield,
	     uint64_t *nzones);

/**
 * Returns a list of changes since the last report was retrieved
 *
 * @note
 * Invoking this function clears the changed log
 *
 * @note
 * Caller is responsible for de-allocating the returned structure using
 * `xnvme_buf_free`
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 *
 * @return On success, pointer to namespace structure. On error, NULL is
 * returned and `errno` is set to indicate the error
 */
struct znd_changes *
znd_changes_from_dev(struct xnvme_dev *dev);

#ifdef __cplusplus
}
#endif

#endif /* __LIBZND_H */
