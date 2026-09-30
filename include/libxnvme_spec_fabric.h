/**
 * libxnvme_spec_fabric - NVMe over Fabrics wire-format structs, enums and values
 *
 * This header collects everything specific to NVMe over Fabrics (NVMe-oF): the Fabrics
 * command opcode and command-type enums, the Connect, Property Get, and Property Set
 * commands, the Fabrics response CQE variants, the Connect data record, and the
 * controller property offsets. Definitions in this header are named
 * ``xnvme_spec_fabric_*`` / ``XNVME_SPEC_FABRIC_*``.
 *
 * ``struct xnvme_spec_fabric_cmd`` is a 64-byte union of the Fabrics commands, reached by
 * casting the command in the context:
 *
 * ```c
 * struct xnvme_spec_fabric_cmd *fcmd = (struct xnvme_spec_fabric_cmd *)&ctx->cmd;
 * ```
 *
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_spec_fabric.h
 */

#ifndef __LIBXNVME_SPEC_FABRIC_H
#define __LIBXNVME_SPEC_FABRIC_H

/**
 * @enum xnvme_spec_fabric_opcode
 */
enum xnvme_spec_fabric_opcode {
	XNVME_SPEC_FABRIC_OPC = 0x7f, ///< XNVME_SPEC_FABRIC_OPC
};

/**
 * @enum xnvme_spec_fabric_command_type
 */
enum xnvme_spec_fabric_command_type {
	XNVME_SPEC_FABRIC_COMMAND_PROPERTY_SET           = 0x0,
	XNVME_SPEC_FABRIC_COMMAND_CONNECT                = 0x1,
	XNVME_SPEC_FABRIC_COMMAND_PROPERTY_GET           = 0x4,
	XNVME_SPEC_FABRIC_COMMAND_AUTHENTICATION_SEND    = 0x5,
	XNVME_SPEC_FABRIC_COMMAND_AUTHENTICATION_RECEIVE = 0x6,
	XNVME_SPEC_FABRIC_COMMAND_DISCONNECT             = 0x8,
	XNVME_SPEC_FABRIC_COMMAND_VENDOR_START           = 0xc0,
	XNVME_SPEC_FABRIC_COMMAND_VENDOR_END             = 0xff,
};

/**
 * PRP or SGL for Data Transfer field, Fabrics-command specific values
 *
 * @enum xnvme_spec_fabric_psdt
 */
enum xnvme_spec_fabric_psdt {
	XNVME_SPEC_FABRIC_PSDT_NODATA = 0b00, ///< no data transferred
	XNVME_SPEC_FABRIC_PSDT_SGL    = 0b10, ///< data transferred via SGLs
};

struct xnvme_spec_fabric_connect_attr {
	union {
		struct {
			uint8_t rsvd         : 3;
			uint8_t connent      : 1;
			uint8_t indivioqdels : 1;
			uint8_t dissqfc      : 1;
			uint8_t prioclass    : 2;
		};
		uint8_t raw;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_fabric_connect_attr) == 1, "Incorrect size")

struct xnvme_spec_fabric_connect_cmd {
	uint32_t cdw0;
	uint8_t fctype;
	uint8_t rsvd[19];
	struct xnvme_spec_sgl_descriptor sgl1;
	uint16_t recfmt;
	uint16_t qid;
	uint16_t sqsize;
	struct xnvme_spec_fabric_connect_attr cattr;
	uint8_t rsvd2;
	uint32_t kato;
	uint16_t nvmsetid;
	uint8_t rsvd3[10];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_fabric_connect_cmd) == 64, "Incorrect size")

struct xnvme_spec_fabric_property_get_cmd {
	uint32_t cdw0;
	uint8_t fctype;
	uint8_t rsvd[35];
	struct {
		uint8_t prs   : 3; ///< Property size, bits 0:2
		uint8_t rsvd4 : 5; ///< Reserved bits, bits 3:7
	} attrib;
	uint8_t rsvd2[3];
	uint32_t ofst;
	uint64_t rsvd5[2];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_fabric_property_get_cmd) == 64, "Incorrect size")

struct xnvme_spec_fabric_property_set_cmd {
	uint32_t cdw0;
	uint8_t fctype;
	uint8_t rsvd[35];
	struct {
		uint8_t rsvd4 : 5;
		uint8_t pus   : 3;
	} attrib;
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
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_fabric_property_set_cmd) == 64, "Incorrect size")

/**
 * Fabrics command accessor for common use of the 64-byte NVMe-oF Fabrics command
 *
 * @struct xnvme_spec_fabric_cmd
 */
struct xnvme_spec_fabric_cmd {
	union {
		struct xnvme_spec_cmd_common common;
		struct xnvme_spec_fabric_connect_cmd connect;
		struct xnvme_spec_fabric_property_get_cmd property_get;
		struct xnvme_spec_fabric_property_set_cmd property_set;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_fabric_cmd) == 64, "Incorrect size")

/**
 * Fabrics response status, overlaid on ``struct xnvme_spec_status``
 *
 * @struct xnvme_spec_fabric_resp_status
 */
struct xnvme_spec_fabric_resp_status {
	union {
		struct {
			uint16_t rsvd2 : 1; ///< reserved, bit 0
			uint16_t sc    : 8; ///< status code, bits 8:1
			uint16_t sct   : 3; ///< status code type, bits 11:9
			uint16_t crd   : 2; ///< command retry delay, bits 13:12
			uint16_t m     : 1; ///< more, bit 14
			uint16_t dnr   : 1; ///< do not retry, bit 15
		};
		uint16_t raw;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_fabric_resp_status) ==
			    sizeof(struct xnvme_spec_status),
		    "Incorrect size")

/**
 * Fabrics Connect response CQE
 *
 * @struct xnvme_spec_fabric_connect_resp_cpl
 */
struct xnvme_spec_fabric_connect_resp_cpl {
	union {
		uint32_t scs;
		struct {
			uint16_t cntlid;
			struct {
				uint16_t obsolete : 1; ///< bit 0
				uint16_t atr  : 1; ///< Authentication Transaction Required, bit 1
				uint16_t ascr : 1; ///< Authentication and Secure Channel Required,
						   ///< bit 2
				uint16_t rsvd3 : 13; ///< bits 15:3
			} authreq;
		} success;
		struct {
			uint16_t ipo; ///< Invalid Parameter Offset
			struct {
				uint8_t ips   : 1; ///< Invalid Parameter Start, bit 0
				uint8_t rsvd4 : 7; ///< bits 7:1
			} iattr;                   ///< Invalid Attributes
			uint8_t rsvd5;
		} connect_invalid;
	};
	uint32_t rsvd1;
	uint16_t sqhd;
	uint16_t rsvd2;
	uint16_t cid;
	struct xnvme_spec_fabric_resp_status status;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_fabric_connect_resp_cpl) ==
			    sizeof(struct xnvme_spec_cpl),
		    "Incorrect size")

/**
 * Generic Fabrics response CQE
 *
 * @struct xnvme_spec_fabric_resp_cqe
 */
struct xnvme_spec_fabric_resp_cqe {
	uint64_t frts; ///< fabrics response type specific, bytes 7:0
	uint16_t sqhd; ///< Submission Queue Head, bytes 9:8
	uint16_t rsvd; ///< reserved, bytes 11:10
	uint16_t cid;  ///< Command Identifier, bytes 13:12
	struct xnvme_spec_fabric_resp_status sts;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_fabric_resp_cqe) == sizeof(struct xnvme_spec_cpl),
		    "Incorrect size")

/**
 * Property Get response CQE
 *
 * @struct xnvme_spec_fabric_property_get_resp_cpl
 */
struct xnvme_spec_fabric_property_get_resp_cpl {
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
	struct xnvme_spec_fabric_resp_status status;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_fabric_property_get_resp_cpl) ==
			    sizeof(struct xnvme_spec_cpl),
		    "Incorrect size")

/**
 * Property Set response CQE
 *
 * @struct xnvme_spec_fabric_property_set_resp_cpl
 */
struct xnvme_spec_fabric_property_set_resp_cpl {
	uint64_t rsvd1;
	uint16_t sqhd;
	uint16_t rsvd2;
	uint16_t cid;
	struct xnvme_spec_fabric_resp_status status;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_fabric_property_set_resp_cpl) ==
			    sizeof(struct xnvme_spec_cpl),
		    "Incorrect size")

/**
 * Union of the Fabrics response CQE variants, overlaid on ``struct xnvme_spec_cpl``
 *
 * @struct xnvme_spec_fabric_generic_cpl
 */
struct xnvme_spec_fabric_generic_cpl {
	union {
		struct xnvme_spec_cpl generic;
		struct xnvme_spec_fabric_resp_cqe fabric_resp;
		struct xnvme_spec_fabric_property_get_resp_cpl prop_get;
		struct xnvme_spec_fabric_property_set_resp_cpl prop_set;
		struct xnvme_spec_fabric_connect_resp_cpl connect;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_fabric_generic_cpl) == sizeof(struct xnvme_spec_cpl),
		    "Incorrect size")

/**
 * Fabrics Connect data record
 *
 * @struct xnvme_spec_fabric_connect_data
 */
struct xnvme_spec_fabric_connect_data {
	uint8_t hostid[16];
	uint16_t cntlid;
	uint8_t rsvd[238];
	uint8_t subnqn[256];
	uint8_t hostnqn[256];
	uint8_t rsvd2[256];
};

/**
 * Controller property offsets, reached over Fabrics Property Get / Property Set
 *
 * @enum xnvme_spec_fabric_property
 */
enum xnvme_spec_fabric_property {
	XNVME_SPEC_FABRIC_PROP_CAP           = 0x0000,
	XNVME_SPEC_FABRIC_PROP_VS            = 0x0008,
	XNVME_SPEC_FABRIC_PROP_CC            = 0x0014,
	XNVME_SPEC_FABRIC_PROP_CSTS          = 0x001C,
	XNVME_SPEC_FABRIC_PROP_NSSR          = 0x0020,
	XNVME_SPEC_FABRIC_PROP_NSSD          = 0x0064,
	XNVME_SPEC_FABRIC_PROP_CRTO          = 0x0068,
	XNVME_SPEC_FABRIC_PROP_TRANSPORT     = 0x1000,
	XNVME_SPEC_FABRIC_PROP_FABRIC_VENDOR = 0x1300,
};

#endif /* __LIBXNVME_SPEC_FABRIC_H */
