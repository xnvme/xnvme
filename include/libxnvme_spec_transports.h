/**
 * libxnvme_spec_transports - NVMe over Transports wire-format structs, enums and values
 *
 * This header collects everything specific to NVMe over Transports: the Transports
 * command opcode and command-type enums, the Connect, Property Get, and Property Set
 * commands, the Transports response CQE variants, the Connect data record, and the
 * controller property offsets. Definitions in this header are named
 * ``xnvme_spec_transports_*`` / ``XNVME_SPEC_TRANSPORTS_*``.
 *
 * ``struct xnvme_spec_transports_cmd`` is a 64-byte union of the Transports commands, reached by
 * casting the command in the context:
 *
 * ```c
 * struct xnvme_spec_transports_cmd *fcmd = (struct xnvme_spec_transports_cmd *)&ctx->cmd;
 * ```
 *
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_spec_transports.h
 */

#ifndef __LIBXNVME_SPEC_TRANSPORTS_H
#define __LIBXNVME_SPEC_TRANSPORTS_H

#define XNVME_DISCOVERY_NQN "nqn.2014-08.org.nvmexpress.discovery"

/**
 * @enum xnvme_spec_transports_opcode
 */
enum xnvme_spec_transports_opcode {
	XNVME_SPEC_TRANSPORTS_OPC = 0x7f, ///< XNVME_SPEC_TRANSPORTS_OPC
};

/**
 * @enum xnvme_spec_transports_command_type
 */
enum xnvme_spec_transports_command_type {
	XNVME_SPEC_TRANSPORTS_COMMAND_PROPERTY_SET           = 0x0,
	XNVME_SPEC_TRANSPORTS_COMMAND_CONNECT                = 0x1,
	XNVME_SPEC_TRANSPORTS_COMMAND_PROPERTY_GET           = 0x4,
	XNVME_SPEC_TRANSPORTS_COMMAND_AUTHENTICATION_SEND    = 0x5,
	XNVME_SPEC_TRANSPORTS_COMMAND_AUTHENTICATION_RECEIVE = 0x6,
	XNVME_SPEC_TRANSPORTS_COMMAND_DISCONNECT             = 0x8,
	XNVME_SPEC_TRANSPORTS_COMMAND_VENDOR_START           = 0xc0,
	XNVME_SPEC_TRANSPORTS_COMMAND_VENDOR_END             = 0xff,
};

/**
 * PRP or SGL for Data Transfer field, Transports-command specific values
 *
 * @enum xnvme_spec_transports_psdt
 */
enum xnvme_spec_transports_psdt {
	XNVME_SPEC_TRANSPORTS_PSDT_NODATA = 0b00, ///< no data transferred
	XNVME_SPEC_TRANSPORTS_PSDT_SGL    = 0b10, ///< data transferred via SGLs
};

struct xnvme_spec_transports_common_cmd {
	/** Common command dword 0 (CDW0) */
	uint8_t opcode; ///< Opcode, bits 0:7
	uint8_t flags;  ///< FUSE and PSDT, see XNVME_SPEC_TRANSPORTS_COMMON_CMD_FLAGS_*
	uint16_t cid;

	uint8_t fctype;
	uint8_t rsvd2[19];
	struct xnvme_spec_sgl_descriptor sgl1;
	uint8_t fcts[24];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_transports_common_cmd) == 64, "Incorrect size")

/** Fused Operation; bits 1:0 of 'flags' */
#define XNVME_SPEC_TRANSPORTS_COMMON_CMD_FLAGS_FUSE 0x3
/** PRP or SGL for Data Transfer; bits 7:6 of 'flags' */
#define XNVME_SPEC_TRANSPORTS_COMMON_CMD_FLAGS_PSDT 0xc0

struct xnvme_spec_transports_connect_cmd {
	uint32_t cdw0;
	uint8_t fctype;
	uint8_t rsvd[19];
	struct xnvme_spec_sgl_descriptor sgl1;
	uint16_t recfmt;
	uint16_t qid;
	uint16_t sqsize;
	uint8_t cattr; ///< Connect attributes, see XNVME_SPEC_TRANSPORTS_CONNECT_CMD_CATTR_*
	uint8_t rsvd2;
	uint32_t kato;
	uint16_t nvmsetid;
	uint8_t rsvd3[10];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_transports_connect_cmd) == 64, "Incorrect size")

/** Connect Entity; bit 3 of 'cattr' */
#define XNVME_SPEC_TRANSPORTS_CONNECT_CMD_CATTR_CONNENT 0x8
/** Individual I/O Queue Deletion Supported; bit 4 of 'cattr' */
#define XNVME_SPEC_TRANSPORTS_CONNECT_CMD_CATTR_INDIVIOQDELS 0x10
/** Disable SQ Flow Control; bit 5 of 'cattr' */
#define XNVME_SPEC_TRANSPORTS_CONNECT_CMD_CATTR_DISSQFC 0x20
/** Priority Class; bits 7:6 of 'cattr' */
#define XNVME_SPEC_TRANSPORTS_CONNECT_CMD_CATTR_PRIOCLASS 0xc0

struct xnvme_spec_transports_property_get_cmd {
	/** Common command dword 0 (CDW0) */
	uint8_t opcode; ///< Opcode, bits 0:7
	uint8_t flags;
	uint16_t cid;

	uint8_t fctype;
	uint8_t rsvd[35];
	uint8_t attrib; ///< Attributes, see XNVME_SPEC_TRANSPORTS_PROPERTY_GET_CMD_ATTRIB_*
	uint8_t rsvd2[3];
	uint32_t ofst;
	uint64_t rsvd5[2];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_transports_property_get_cmd) == 64, "Incorrect size")

/** Property Size; bits 2:0 of 'attrib' */
#define XNVME_SPEC_TRANSPORTS_PROPERTY_GET_CMD_ATTRIB_PRS 0x7

struct xnvme_spec_transports_property_set_cmd {
	/** Common command dword 0 (CDW0) */
	uint8_t opcode; ///< Opcode, bits 0:7
	uint8_t flags;
	uint16_t cid;

	uint8_t fctype;
	uint8_t rsvd[35];
	uint8_t attrib; ///< Attributes, see XNVME_SPEC_TRANSPORTS_PROPERTY_SET_CMD_ATTRIB_*
	uint8_t rsvd2[3];
	uint32_t ofst;
	union {
		uint64_t value;
		struct {
			uint32_t ddw;
			uint32_t rsvd3;
		};
	};
	uint64_t rsvd5;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_transports_property_set_cmd) == 64, "Incorrect size")

/** Property Size; bits 7:5 of 'attrib' */
#define XNVME_SPEC_TRANSPORTS_PROPERTY_SET_CMD_ATTRIB_PUS 0xe0

/**
 * Transports command accessor for common use of the 64-byte NVMe over Transports command
 *
 * @struct xnvme_spec_transports_cmd
 */
struct xnvme_spec_transports_cmd {
	union {
		struct xnvme_spec_cmd_common common;
		struct xnvme_spec_transports_common_cmd fcommon;
		struct xnvme_spec_transports_connect_cmd connect;
		struct xnvme_spec_transports_property_get_cmd property_get;
		struct xnvme_spec_transports_property_set_cmd property_set;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_transports_cmd) == 64, "Incorrect size")

/**
 * Transports Connect response CQE
 *
 * The status field has the layout of the status of ``struct xnvme_spec_cqe``.
 *
 * @struct xnvme_spec_transports_connect_resp_cpl
 */
struct xnvme_spec_transports_connect_resp_cpl {
	union {
		uint32_t scs;
		struct {
			uint16_t cntlid;
			uint16_t authreq; ///< Authentication requirements, see
					  ///< XNVME_SPEC_TRANSPORTS_CONNECT_RESP_CPL_AUTHREQ_*
		} success;
		struct {
			uint16_t ipo;  ///< Invalid Parameter Offset
			uint8_t iattr; ///< Invalid attributes, see
				       ///< XNVME_SPEC_TRANSPORTS_CONNECT_RESP_CPL_IATTR_*
			uint8_t rsvd5;
		} connect_invalid;
	};
	uint32_t rsvd1;
	uint16_t sqhd;
	uint16_t rsvd2;
	uint16_t cid;
	uint16_t status; ///< Status field, see XNVME_SPEC_CQE_STATUS_*
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_transports_connect_resp_cpl) == 16, "Incorrect size")

/** Authentication Transaction Required; bit 1 of 'authreq' */
#define XNVME_SPEC_TRANSPORTS_CONNECT_RESP_CPL_AUTHREQ_ATR 0x2
/** Authentication and Secure Channel Required; bit 2 of 'authreq' */
#define XNVME_SPEC_TRANSPORTS_CONNECT_RESP_CPL_AUTHREQ_ASCR 0x4
/** Invalid Parameter Start; bit 0 of 'iattr' */
#define XNVME_SPEC_TRANSPORTS_CONNECT_RESP_CPL_IATTR_IPS 0x1

/**
 * Generic Transports response CQE
 *
 * @struct xnvme_spec_transports_resp_cqe
 */
struct xnvme_spec_transports_resp_cqe {
	uint64_t frts;   ///< transports response type specific, bytes 7:0
	uint16_t sqhd;   ///< Submission Queue Head, bytes 9:8
	uint16_t rsvd;   ///< reserved, bytes 11:10
	uint16_t cid;    ///< Command Identifier, bytes 13:12
	uint16_t status; ///< Status field, bytes 15:14, see XNVME_SPEC_CQE_STATUS_*
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_transports_resp_cqe) == 16, "Incorrect size")

/**
 * Property Get response CQE
 *
 * @struct xnvme_spec_transports_property_get_resp_cpl
 */
struct xnvme_spec_transports_property_get_resp_cpl {
	union {
		struct {
			uint32_t ddw;
			uint32_t rsvd;
		};
		uint64_t value;
	};
	uint16_t sqhd;
	uint16_t rsvd1;
	uint16_t cid;
	uint16_t status; ///< Status field, see XNVME_SPEC_CQE_STATUS_*
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_transports_property_get_resp_cpl) == 16,
		    "Incorrect size")

/**
 * Property Set response CQE
 *
 * @struct xnvme_spec_transports_property_set_resp_cpl
 */
struct xnvme_spec_transports_property_set_resp_cpl {
	uint64_t rsvd1;
	uint16_t sqhd;
	uint16_t rsvd2;
	uint16_t cid;
	uint16_t status; ///< Status field, see XNVME_SPEC_CQE_STATUS_*
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_transports_property_set_resp_cpl) == 16,
		    "Incorrect size")

/**
 * Union of the Transports response CQE variants, overlaid on ``struct xnvme_spec_cpl``
 *
 * @struct xnvme_spec_transports_generic_cpl
 */
struct xnvme_spec_transports_generic_cpl {
	union {
		struct xnvme_spec_transports_resp_cqe transports_resp;
		struct xnvme_spec_transports_property_get_resp_cpl prop_get;
		struct xnvme_spec_transports_property_set_resp_cpl prop_set;
		struct xnvme_spec_transports_connect_resp_cpl connect;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_transports_generic_cpl) == 16, "Incorrect size")

/**
 * Transports Connect data record
 *
 * @struct xnvme_spec_transports_connect_data
 */
struct xnvme_spec_transports_connect_data {
	uint8_t hostid[16];
	uint16_t cntlid;
	uint8_t rsvd[238];
	uint8_t subnqn[256];
	uint8_t hostnqn[256];
	uint8_t rsvd2[256];
};

/**
 * Controller property offsets, reached over Transports Property Get / Property Set
 *
 * @enum xnvme_spec_transports_property
 */
enum xnvme_spec_transports_property {
	XNVME_SPEC_TRANSPORTS_PROP_CAP               = 0x0000,
	XNVME_SPEC_TRANSPORTS_PROP_VS                = 0x0008,
	XNVME_SPEC_TRANSPORTS_PROP_CC                = 0x0014,
	XNVME_SPEC_TRANSPORTS_PROP_CSTS              = 0x001C,
	XNVME_SPEC_TRANSPORTS_PROP_NSSR              = 0x0020,
	XNVME_SPEC_TRANSPORTS_PROP_NSSD              = 0x0064,
	XNVME_SPEC_TRANSPORTS_PROP_CRTO              = 0x0068,
	XNVME_SPEC_TRANSPORTS_PROP_TRANSPORT         = 0x1000,
	XNVME_SPEC_TRANSPORTS_PROP_TRANSPORTS_VENDOR = 0x1300,
};

enum xnvme_spec_transport_type {
	XNVME_SPEC_TRANSPORT_RDMA      = 1,
	XNVME_SPEC_TRANSPORT_FC        = 2,
	XNVME_SPEC_TRANSPORT_TCP       = 3,
	XNVME_SPEC_TRANSPORT_INTRAHOST = 254,
};

enum xnvme_spec_address_family {
	XNVME_SPEC_AF_IPV4       = 1,
	XNVME_SPEC_AF_IPV6       = 2,
	XNVME_SPEC_AF_IB         = 3,
	XNVME_SPEC_AF_FC         = 4,
	XNVME_SPEC_AF_INTRA_HOST = 254
};

enum xnvme_spec_subtype {
	XNVME_SPEC_SUBTYPE_REFERRAL  = 1,
	XNVME_SPEC_SUBTYPE_NVM       = 2,
	XNVME_SPEC_SUBTYPE_DISCOVERY = 3,
};

struct xnvme_spec_discovery_log_page_entry {
	uint8_t trtype;  ///< Transport type
	uint8_t adrfam;  ///< Address family
	uint8_t subtype; ///< Subsystem type
	uint8_t treq; ///< Transport requirements, see XNVME_SPEC_DISCOVERY_LOG_PAGE_ENTRY_TREQ_*
	uint16_t portid;    ///< Port ID
	uint16_t cntlid;    ///< Controller ID
	uint16_t asqsz;     ///< Admin SQ Size
	uint16_t eflags;    ///< Entry flags, see XNVME_SPEC_DISCOVERY_LOG_PAGE_ENTRY_EFLAGS_*
	uint8_t rsvd[20];   ///< reserved, bytes 12-31
	char trsvcid[32];   ///< Transport Service ID
	uint8_t rsvd2[192]; ///< reserved, bytes 64-255
	char subnqn[256];   ///< Subsystem NQN
	char traddr[256];   ///< Transport Address
	char tsas[256];     ///< Transport Specific Address Subtype
	void *next[];       ///< Pointer to the next discovery log page entry
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_discovery_log_page_entry) == 1024, "Incorrect size");

/** Transport Secure Channel; bits 1:0 of 'treq' */
#define XNVME_SPEC_DISCOVERY_LOG_PAGE_ENTRY_TREQ_TSC 0x3
/** SQ Flow Control Disable; bit 2 of 'treq' */
#define XNVME_SPEC_DISCOVERY_LOG_PAGE_ENTRY_TREQ_SQFCD 0x4
/** Zero Host ID Support; bit 3 of 'treq' */
#define XNVME_SPEC_DISCOVERY_LOG_PAGE_ENTRY_TREQ_ZHIDS 0x8
/** Transport Authentication Support Capable; bits 5:4 of 'treq' */
#define XNVME_SPEC_DISCOVERY_LOG_PAGE_ENTRY_TREQ_TASC 0x30
/** Duplicate Returned Information; bit 0 of 'eflags' */
#define XNVME_SPEC_DISCOVERY_LOG_PAGE_ENTRY_EFLAGS_DUPRETINFO 0x1
/** Explicit Persistent Connection Support for Discovery; bit 1 of 'eflags' */
#define XNVME_SPEC_DISCOVERY_LOG_PAGE_ENTRY_EFLAGS_EPCSD 0x2
/** No CDC Connectivity; bit 2 of 'eflags' */
#define XNVME_SPEC_DISCOVERY_LOG_PAGE_ENTRY_EFLAGS_NCC 0x4

struct xnvme_spec_discovery_log_page {
	uint64_t genctr;     ///< Generation counter, bytes 0-7
	uint64_t numrec;     ///< Number of records, bytes 8-15
	uint16_t recfmt;     ///< Record format, bytes 16-17
	uint8_t dlpf;        ///< Discovery Log Page Flags, byte 18, see
			     ///< XNVME_SPEC_DISCOVERY_LOG_PAGE_DLPF_*
	uint8_t rsvd;        ///< Reserved, byte 19
	uint32_t tdlpl;      ///< Total Discovery Log Page Length, bytes 20-23
	uint8_t rsvd2[1000]; ///< Reserved, bytes 24-1023
	void *first[];       ///< Pointer to the first discovery log page entry
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_discovery_log_page) == 1024, "Incorrect size");

/** Extended; bit 0 of 'dlpf' */
#define XNVME_SPEC_DISCOVERY_LOG_PAGE_DLPF_EXTEND 0x1
/** Port Local; bit 1 of 'dlpf' */
#define XNVME_SPEC_DISCOVERY_LOG_PAGE_DLPF_PORTLCL 0x2
/** All Subsystems; bit 2 of 'dlpf' */
#define XNVME_SPEC_DISCOVERY_LOG_PAGE_DLPF_ALLSUB 0x4

#endif /* __LIBXNVME_SPEC_TRANSPORTS_H */
