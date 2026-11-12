/**
 * What the tcp specification defines, by xnvme_spec
 *
 * Written from the data model of senfd 0.19.0, by toolbox/xnvme_spec_headers.py;
 * each name and mask is as the figure it cites gives it. Do not
 * edit: see the contributor docs on producing the spec headers.
 *
 * @file libxnvme_spec_tcp.h
 */
#ifndef LIBXNVME_SPEC_TCP_H
#define LIBXNVME_SPEC_TCP_H

#include <stdint.h>

/**
 * Transport Specific Address Subtype Definition for NVMe/TCP Transport (NVMe over TCP Transport 1.3, 3.6.1.1, Figure 17, page 23, PDF page 23)
 */
struct __attribute__((packed)) xnvme_spec_tcp_transport_specific_address_subtype_definition_for_nvme_tcp_transport {
	uint8_t sectype; ///< Security Type, byte 0
	uint8_t rsvd1[255]; ///< Reserved, bytes 255:1
};

/**
 * The values of Security Type (NVMe over TCP Transport 1.3, 3.6.1.1, Figure 17, page 23, PDF page 23)
 */
enum xnvme_spec_tcp_transport_specific_address_subtype_definition_for_nvme_tcp_transport_sectype {
	XNVME_SPEC_TCP_TRANSPORT_SPECIFIC_ADDRESS_SUBTYPE_DEFINITION_FOR_NVME_TCP_TRANSPORT_SECTYPE_NO_SECURITY = 0x0, ///< No Security (NVMe over TCP Transport 1.3, 3.6.1.1, Figure 17, page 23, PDF page 23)
};

/**
 * PDU Common Header (CH) (NVMe over TCP Transport 1.3, 3.6.2.1, Figure 25, page 32, PDF page 32)
 */
struct __attribute__((packed)) xnvme_spec_tcp_pdu_common_header {
	uint8_t pdu_type; ///< PDU Type, byte 0
	uint8_t flags; ///< Flags, byte 1
	uint8_t hlen; ///< Header Length, byte 2
	uint8_t pdo; ///< PDU Data Offset, byte 3
	uint32_t plen; ///< PDU Length, bytes 7:4
};

/**
 * The values of PDU Type (NVMe over TCP Transport 1.3, 3.3, Figure 10, page 12, PDF page 12)
 */
enum xnvme_spec_tcp_pdu_common_header_pdu_type {
	XNVME_SPEC_TCP_PDU_COMMON_HEADER_PDU_TYPE_ICREQ = 0x0, ///< ICReq (NVMe over TCP Transport 1.3, 3.3, Figure 10, page 12, PDF page 12)
	XNVME_SPEC_TCP_PDU_COMMON_HEADER_PDU_TYPE_ICRESP = 0x1, ///< ICResp (NVMe over TCP Transport 1.3, 3.3, Figure 10, page 12, PDF page 12)
	XNVME_SPEC_TCP_PDU_COMMON_HEADER_PDU_TYPE_H2CTERMREQ = 0x2, ///< H2CTermReq (NVMe over TCP Transport 1.3, 3.3, Figure 10, page 12, PDF page 12)
	XNVME_SPEC_TCP_PDU_COMMON_HEADER_PDU_TYPE_C2HTERMREQ = 0x3, ///< C2HTermReq (NVMe over TCP Transport 1.3, 3.3, Figure 10, page 12, PDF page 12)
	XNVME_SPEC_TCP_PDU_COMMON_HEADER_PDU_TYPE_CAPSULECMD = 0x4, ///< CapsuleCmd (NVMe over TCP Transport 1.3, 3.3, Figure 10, page 12, PDF page 12)
	XNVME_SPEC_TCP_PDU_COMMON_HEADER_PDU_TYPE_CAPSULERESP = 0x5, ///< CapsuleResp (NVMe over TCP Transport 1.3, 3.3, Figure 10, page 12, PDF page 12)
	XNVME_SPEC_TCP_PDU_COMMON_HEADER_PDU_TYPE_H2CDATA = 0x6, ///< H2CData (NVMe over TCP Transport 1.3, 3.3, Figure 10, page 12, PDF page 12)
	XNVME_SPEC_TCP_PDU_COMMON_HEADER_PDU_TYPE_C2HDATA = 0x7, ///< C2HData (NVMe over TCP Transport 1.3, 3.3, Figure 10, page 12, PDF page 12)
	XNVME_SPEC_TCP_PDU_COMMON_HEADER_PDU_TYPE_R2T = 0x9, ///< R2T (NVMe over TCP Transport 1.3, 3.3, Figure 10, page 12, PDF page 12)
	XNVME_SPEC_TCP_PDU_COMMON_HEADER_PDU_TYPE_KDREQ = 0xa, ///< KDReq (NVMe over TCP Transport 1.3, 3.3, Figure 10, page 12, PDF page 12)
	XNVME_SPEC_TCP_PDU_COMMON_HEADER_PDU_TYPE_KDRESP = 0xb, ///< KDResp (NVMe over TCP Transport 1.3, 3.3, Figure 10, page 12, PDF page 12)
};

/**
 * Initialize Connection Request PDU (ICReq) (NVMe over TCP Transport 1.3, 3.6.2.2, Figure 26, page 32, PDF page 32)
 */
struct __attribute__((packed)) xnvme_spec_tcp_initialize_connection_request_pdu {
	uint8_t pdu_type; ///< PDU Type, byte 0
	uint8_t flags; ///< Flags, byte 1, see XNVME_SPEC_TCP_INITIALIZE_CONNECTION_REQUEST_PDU_FLAGS_*
	uint8_t hlen; ///< Header Length, byte 2
	uint8_t pdo; ///< PDU Data Offset, byte 3
	uint32_t plen; ///< PDU Length, bytes 7:4
	uint16_t pfv; ///< PDU Format Version, bytes 9:8
	uint8_t hpda; ///< Host PDU Data Alignment, byte 10
	uint8_t dgst; ///< Digest, byte 11, see XNVME_SPEC_TCP_INITIALIZE_CONNECTION_REQUEST_PDU_DGST_*
	uint32_t maxr2t; ///< Maximum Number of Outstanding R2T, bytes 15:12
	uint8_t rsvd16[112]; ///< Reserved, bytes 127:16
};

/** Kickstart Discovery Connection (NVMe over TCP Transport 1.3, 3.6.2.2, Figure 26, page 32, PDF page 32); bit 7 of 'flags' */
#define XNVME_SPEC_TCP_INITIALIZE_CONNECTION_REQUEST_PDU_FLAGS_KDCONN 0x80
/** PDU Data Digest Enable (NVMe over TCP Transport 1.3, 3.6.2.2, Figure 26, page 32, PDF page 32); bit 1 of 'dgst' */
#define XNVME_SPEC_TCP_INITIALIZE_CONNECTION_REQUEST_PDU_DGST_DDGST_ENABLE 0x2
/** PDU Header Digest Enable (NVMe over TCP Transport 1.3, 3.6.2.2, Figure 26, page 32, PDF page 32); bit 0 of 'dgst' */
#define XNVME_SPEC_TCP_INITIALIZE_CONNECTION_REQUEST_PDU_DGST_HDGST_ENABLE 0x1

/**
 * Initialize Connection Response PDU (ICResp) (NVMe over TCP Transport 1.3, 3.6.2.3, Figure 27, page 33, PDF page 33)
 */
struct __attribute__((packed)) xnvme_spec_tcp_initialize_connection_response_pdu {
	uint8_t pdu_type; ///< PDU Type, byte 0
	uint8_t flags; ///< Flags, byte 1
	uint8_t hlen; ///< Header Length, byte 2
	uint8_t pdo; ///< PDU Data Offset, byte 3
	uint32_t plen; ///< PDU Length, bytes 7:4
	uint16_t pfv; ///< PDU Format Version, bytes 9:8
	uint8_t cpda; ///< Controller PDU Data Alignment, byte 10
	uint8_t dgst; ///< Digest, byte 11, see XNVME_SPEC_TCP_INITIALIZE_CONNECTION_RESPONSE_PDU_DGST_*
	uint32_t maxh2cdata; ///< Maximum Host to Controller Data length, bytes 15:12
	uint8_t rsvd16[112]; ///< Reserved, bytes 127:16
};

/** Data Digest Enable (NVMe over TCP Transport 1.3, 3.6.2.3, Figure 27, page 33, PDF page 33); bit 1 of 'dgst' */
#define XNVME_SPEC_TCP_INITIALIZE_CONNECTION_RESPONSE_PDU_DGST_DDGST_ENABLE 0x2
/** Header Digest Enable (NVMe over TCP Transport 1.3, 3.6.2.3, Figure 27, page 33, PDF page 33); bit 0 of 'dgst' */
#define XNVME_SPEC_TCP_INITIALIZE_CONNECTION_RESPONSE_PDU_DGST_HDGST_ENABLE 0x1

/**
 * Host to Controller Terminate Connection Request PDU (H2CTermReq) (NVMe over TCP Transport 1.3, 3.6.2.4, Figure 28, page 33, PDF page 33)
 *
 * Followed, from byte 24, by as many Data as the data says
 */
struct __attribute__((packed)) xnvme_spec_tcp_host_to_controller_terminate_connection_request_pdu {
	uint8_t pdu_type; ///< PDU Type, byte 0
	uint8_t flags; ///< Flags, byte 1
	uint8_t hlen; ///< Header Length, byte 2
	uint8_t pdo; ///< PDU Data Offset, byte 3
	uint32_t plen; ///< PDU Length, bytes 7:4
	uint16_t fes; ///< Fatal Error Status, bytes 9:8
	uint32_t fei; ///< Fatal Error Information, bytes 13:10
	uint8_t rsvd14[10]; ///< Reserved, bytes 23:14
};

/**
 * The values of Fatal Error Status (NVMe over TCP Transport 1.3, 3.6.2.4, Figure 28, page 33, PDF page 33)
 */
enum xnvme_spec_tcp_host_to_controller_terminate_connection_request_pdu_fes {
	XNVME_SPEC_TCP_HOST_TO_CONTROLLER_TERMINATE_CONNECTION_REQUEST_PDU_FES_INVALID_PDU_HEADER_FIELD = 0x1, ///< Invalid PDU Header Field (NVMe over TCP Transport 1.3, 3.6.2.4, Figure 28, page 33, PDF page 33)
	XNVME_SPEC_TCP_HOST_TO_CONTROLLER_TERMINATE_CONNECTION_REQUEST_PDU_FES_PDU_SEQUENCE_ERROR = 0x2, ///< PDU Sequence Error (NVMe over TCP Transport 1.3, 3.6.2.4, Figure 28, page 33, PDF page 33)
	XNVME_SPEC_TCP_HOST_TO_CONTROLLER_TERMINATE_CONNECTION_REQUEST_PDU_FES_HEADER_DIGEST_ERROR = 0x3, ///< Header Digest Error (NVMe over TCP Transport 1.3, 3.6.2.4, Figure 28, page 33, PDF page 33)
	XNVME_SPEC_TCP_HOST_TO_CONTROLLER_TERMINATE_CONNECTION_REQUEST_PDU_FES_DATA_TRANSFER_OUT_OF_RANGE = 0x4, ///< Data Transfer Out of Range (NVMe over TCP Transport 1.3, 3.6.2.4, Figure 28, page 33, PDF page 33)
	XNVME_SPEC_TCP_HOST_TO_CONTROLLER_TERMINATE_CONNECTION_REQUEST_PDU_FES_R2T_LIMIT_EXCEEDED = 0x5, ///< R2T Limit Exceeded (NVMe over TCP Transport 1.3, 3.6.2.4, Figure 28, page 33, PDF page 33)
	XNVME_SPEC_TCP_HOST_TO_CONTROLLER_TERMINATE_CONNECTION_REQUEST_PDU_FES_UNSUPPORTED_PARAMETER = 0x6, ///< Unsupported Parameter (NVMe over TCP Transport 1.3, 3.6.2.4, Figure 28, page 33, PDF page 33)
};

/**
 * Controller to Host Terminate Connection Request PDU (C2HTermReq) (NVMe over TCP Transport 1.3, 3.6.2.5, Figure 29, page 34, PDF page 34)
 *
 * Followed, from byte 24, by as many Data as the data says
 */
struct __attribute__((packed)) xnvme_spec_tcp_controller_to_host_terminate_connection_request_pdu {
	uint8_t pdu_type; ///< PDU Type, byte 0
	uint8_t flags; ///< Flags, byte 1
	uint8_t hlen; ///< Header Length, byte 2
	uint8_t pdo; ///< PDU Data Offset, byte 3
	uint32_t plen; ///< PDU Length, bytes 7:4
	uint16_t fes; ///< Fatal Error Status, bytes 9:8
	uint32_t fei; ///< Fatal Error Information, bytes 13:10
	uint8_t rsvd14[10]; ///< Reserved, bytes 23:14
};

/**
 * The values of Fatal Error Status (NVMe over TCP Transport 1.3, 3.6.2.5, Figure 29, page 34, PDF page 34)
 */
enum xnvme_spec_tcp_controller_to_host_terminate_connection_request_pdu_fes {
	XNVME_SPEC_TCP_CONTROLLER_TO_HOST_TERMINATE_CONNECTION_REQUEST_PDU_FES_INVALID_PDU_HEADER_FIELD = 0x1, ///< Invalid PDU Header Field (NVMe over TCP Transport 1.3, 3.6.2.5, Figure 29, page 34, PDF page 34)
	XNVME_SPEC_TCP_CONTROLLER_TO_HOST_TERMINATE_CONNECTION_REQUEST_PDU_FES_PDU_SEQUENCE_ERROR = 0x2, ///< PDU Sequence Error (NVMe over TCP Transport 1.3, 3.6.2.5, Figure 29, page 34, PDF page 34)
	XNVME_SPEC_TCP_CONTROLLER_TO_HOST_TERMINATE_CONNECTION_REQUEST_PDU_FES_HEADER_DIGEST_ERROR = 0x3, ///< Header Digest Error (NVMe over TCP Transport 1.3, 3.6.2.5, Figure 29, page 34, PDF page 34)
	XNVME_SPEC_TCP_CONTROLLER_TO_HOST_TERMINATE_CONNECTION_REQUEST_PDU_FES_DATA_TRANSFER_OUT_OF_RANGE = 0x4, ///< Data Transfer Out of Range (NVMe over TCP Transport 1.3, 3.6.2.5, Figure 29, page 34, PDF page 34)
	XNVME_SPEC_TCP_CONTROLLER_TO_HOST_TERMINATE_CONNECTION_REQUEST_PDU_FES_DATA_TRANSFER_LIMIT_EXCEEDED = 0x5, ///< Data Transfer Limit Exceeded (NVMe over TCP Transport 1.3, 3.6.2.5, Figure 29, page 34, PDF page 34)
	XNVME_SPEC_TCP_CONTROLLER_TO_HOST_TERMINATE_CONNECTION_REQUEST_PDU_FES_UNSUPPORTED_PARAMETER = 0x6, ///< Unsupported Parameter (NVMe over TCP Transport 1.3, 3.6.2.5, Figure 29, page 34, PDF page 34)
};

/**
 * Command Capsule PDU (CapsuleCmd) (NVMe over TCP Transport 1.3, 3.6.2.6, Figure 30, page 35, PDF page 35)
 *
 * Followed, from byte 72, by as many NVMe-oF In-Capsule Data as the data says
 */
struct __attribute__((packed)) xnvme_spec_tcp_command_capsule_pdu {
	uint8_t pdu_type; ///< PDU Type, byte 0
	uint8_t flags; ///< Flags, byte 1, see XNVME_SPEC_TCP_COMMAND_CAPSULE_PDU_FLAGS_*
	uint8_t hlen; ///< Header Length, byte 2
	uint8_t pdo; ///< PDU Data Offset, byte 3
	uint32_t plen; ///< PDU Length, bytes 7:4
	uint8_t ccsqe[64]; ///< NVMe-oF Command Capsule SQE, bytes 71:8
};

/** PDU Data Digest Flag (NVMe over TCP Transport 1.3, 3.6.2.6, Figure 30, page 35, PDF page 35); bit 1 of 'flags' */
#define XNVME_SPEC_TCP_COMMAND_CAPSULE_PDU_FLAGS_DDGSTF 0x2
/** PDU Header Digest Flag (NVMe over TCP Transport 1.3, 3.6.2.6, Figure 30, page 35, PDF page 35); bit 0 of 'flags' */
#define XNVME_SPEC_TCP_COMMAND_CAPSULE_PDU_FLAGS_HDGSTF 0x1

/**
 * Response Capsule PDU (CapsuleResp) (NVMe over TCP Transport 1.3, 3.6.2.7, Figure 31, page 36, PDF page 36)
 */
struct __attribute__((packed)) xnvme_spec_tcp_response_capsule_pdu {
	uint8_t pdu_type; ///< PDU Type, byte 0
	uint8_t flags; ///< Flags, byte 1, see XNVME_SPEC_TCP_RESPONSE_CAPSULE_PDU_FLAGS_*
	uint8_t hlen; ///< Header Length, byte 2
	uint8_t pdo; ///< PDU Data Offset, byte 3
	uint32_t plen; ///< PDU Length, bytes 7:4
	uint8_t rccqe[16]; ///< NVMe-oF Response Capsule CQE, bytes 23:8
	uint32_t hdgst; ///< PDU Header Digest, bytes 27:24
};

/** PDU Header Digest Flag (NVMe over TCP Transport 1.3, 3.6.2.7, Figure 31, page 36, PDF page 36); bit 0 of 'flags' */
#define XNVME_SPEC_TCP_RESPONSE_CAPSULE_PDU_FLAGS_HDGSTF 0x1

/**
 * Host To Controller Data Transfer PDU (H2CData) (NVMe over TCP Transport 1.3, 3.6.2.8, Figure 32, page 37, PDF page 37)
 *
 * Followed, from byte 28, by as many Pad as the data says
 *
 * Not members:
 * PDU Data: at an offset the data gives, in 'pad'
 * Data Digest: at an offset the data gives, in 'pad'
 */
struct __attribute__((packed)) xnvme_spec_tcp_host_to_controller_data_transfer_pdu {
	uint8_t pdu_type; ///< PDU Type, byte 0
	uint8_t flags; ///< Flags, byte 1, see XNVME_SPEC_TCP_HOST_TO_CONTROLLER_DATA_TRANSFER_PDU_FLAGS_*
	uint8_t hlen; ///< Header Length, byte 2
	uint8_t pdo; ///< PDU Data Offset, byte 3
	uint32_t plen; ///< PDU Length, bytes 7:4
	uint16_t cccid; ///< Command Capsule CID, bytes 9:8
	uint16_t ttag; ///< Transfer Tag, bytes 11:10
	uint32_t datao; ///< Data Offset, bytes 15:12
	uint32_t datal; ///< Data Length, bytes 19:16
	uint32_t rsvd20; ///< Reserved, bytes 23:20
	uint32_t hdgst; ///< PDU Header Digest, bytes 27:24
};

/** Last PDU (NVMe over TCP Transport 1.3, 3.6.2.8, Figure 32, page 37, PDF page 37); bit 2 of 'flags' */
#define XNVME_SPEC_TCP_HOST_TO_CONTROLLER_DATA_TRANSFER_PDU_FLAGS_LAST_PDU 0x4
/** PDU Data Digest Flag (NVMe over TCP Transport 1.3, 3.6.2.8, Figure 32, page 37, PDF page 37); bit 1 of 'flags' */
#define XNVME_SPEC_TCP_HOST_TO_CONTROLLER_DATA_TRANSFER_PDU_FLAGS_DDGSTF 0x2
/** PDU Header Digest Flag (NVMe over TCP Transport 1.3, 3.6.2.8, Figure 32, page 37, PDF page 37); bit 0 of 'flags' */
#define XNVME_SPEC_TCP_HOST_TO_CONTROLLER_DATA_TRANSFER_PDU_FLAGS_HDGSTF 0x1

/**
 * Controller To Host Data Transfer PDU (C2HData) (NVMe over TCP Transport 1.3, 3.6.2.9, Figure 33, page 38, PDF page 38)
 *
 * Followed, from byte 28, by as many Pad as the data says
 *
 * Not members:
 * PDU Data: at an offset the data gives, in 'pad'
 * PDU Data Digest: at an offset the data gives, in 'pad'
 */
struct __attribute__((packed)) xnvme_spec_tcp_controller_to_host_data_transfer_pdu {
	uint8_t pdu_type; ///< PDU Type, byte 0
	uint8_t flags; ///< Flags, byte 1, see XNVME_SPEC_TCP_CONTROLLER_TO_HOST_DATA_TRANSFER_PDU_FLAGS_*
	uint8_t hlen; ///< Header Length, byte 2
	uint8_t pdo; ///< PDU Data Offset, byte 3
	uint32_t plen; ///< PDU Length, bytes 7:4
	uint16_t cccid; ///< Command Capsule CID, bytes 9:8
	uint16_t rsvd10; ///< Reserved, bytes 11:10
	uint32_t datao; ///< Data Offset, bytes 15:12
	uint32_t datal; ///< Data Length, bytes 19:16
	uint32_t rsvd20; ///< Reserved, bytes 23:20
	uint32_t hdgst; ///< PDU Header Digest, bytes 27:24
};

/** SUCCESS (NVMe over TCP Transport 1.3, 3.6.2.9, Figure 33, page 38, PDF page 38); bit 3 of 'flags' */
#define XNVME_SPEC_TCP_CONTROLLER_TO_HOST_DATA_TRANSFER_PDU_FLAGS_SCSS 0x8
/** Last PDU (NVMe over TCP Transport 1.3, 3.6.2.9, Figure 33, page 38, PDF page 38); bit 2 of 'flags' */
#define XNVME_SPEC_TCP_CONTROLLER_TO_HOST_DATA_TRANSFER_PDU_FLAGS_LAST_PDU 0x4
/** PDU Data Digest Flag (NVMe over TCP Transport 1.3, 3.6.2.9, Figure 33, page 38, PDF page 38); bit 1 of 'flags' */
#define XNVME_SPEC_TCP_CONTROLLER_TO_HOST_DATA_TRANSFER_PDU_FLAGS_DDGSTF 0x2
/** PDU Header Digest Flag (NVMe over TCP Transport 1.3, 3.6.2.9, Figure 33, page 38, PDF page 38); bit 0 of 'flags' */
#define XNVME_SPEC_TCP_CONTROLLER_TO_HOST_DATA_TRANSFER_PDU_FLAGS_HDGSTF 0x1

/**
 * Ready to Transfer PDU (R2T) (NVMe over TCP Transport 1.3, 3.6.2.10, Figure 34, page 39, PDF page 39)
 */
struct __attribute__((packed)) xnvme_spec_tcp_ready_to_transfer_pdu {
	uint8_t pdu_type; ///< PDU Type, byte 0
	uint8_t flags; ///< Flags, byte 1, see XNVME_SPEC_TCP_READY_TO_TRANSFER_PDU_FLAGS_*
	uint8_t hlen; ///< Header Length, byte 2
	uint8_t pdo; ///< PDU Data Offset, byte 3
	uint32_t plen; ///< PDU Length, bytes 7:4
	uint16_t cccid; ///< Command Capsule CID, bytes 9:8
	uint16_t ttag; ///< Transfer Tag, bytes 11:10
	uint32_t r2to; ///< Requested Data Offset, bytes 15:12
	uint32_t r2tl; ///< Requested Data Length, bytes 19:16
	uint32_t rsvd20; ///< Reserved, bytes 23:20
	uint32_t hdgst; ///< PDU Header Digest, bytes 27:24
};

/** PDU Header Digest Flag (NVMe over TCP Transport 1.3, 3.6.2.10, Figure 34, page 39, PDF page 39); bit 0 of 'flags' */
#define XNVME_SPEC_TCP_READY_TO_TRANSFER_PDU_FLAGS_HDGSTF 0x1

/**
 * Kickstart Discovery Request PDU (KDReq) (NVMe over TCP Transport 1.3, 3.6.2.11, Figure 35, page 39, PDF page 39)
 */
struct __attribute__((packed)) xnvme_spec_tcp_kickstart_discovery_request_pdu {
	uint8_t pdu_type; ///< PDU Type, byte 0
	uint8_t flags; ///< Flags, byte 1, see XNVME_SPEC_TCP_KICKSTART_DISCOVERY_REQUEST_PDU_FLAGS_*
	uint8_t hlen; ///< Header Length, byte 2
	uint8_t pdo; ///< PDU Data Offset, byte 3
	uint32_t plen; ///< PDU Length, bytes 7:4
	uint16_t numkr; ///< Number of Kickstart Records, bytes 9:8
	uint16_t numdie; ///< Number of Discovery Information Entries, bytes 11:10
};

/** PDU Data Digest Flag (NVMe over TCP Transport 1.3, 3.6.2.11, Figure 35, page 39, PDF page 39); bit 1 of 'flags' */
#define XNVME_SPEC_TCP_KICKSTART_DISCOVERY_REQUEST_PDU_FLAGS_DDGSTF 0x2
/** PDU Header Digest Flag (NVMe over TCP Transport 1.3, 3.6.2.11, Figure 35, page 39, PDF page 39); bit 0 of 'flags' */
#define XNVME_SPEC_TCP_KICKSTART_DISCOVERY_REQUEST_PDU_FLAGS_HDGSTF 0x1

/**
 * Kickstart Record (NVMe over TCP Transport 1.3, 3.6.2.11, Figure 36, page 40, PDF page 40)
 */
struct __attribute__((packed)) xnvme_spec_tcp_kickstart_record {
	uint8_t trtype; ///< Transport Type, byte 0
	uint8_t adrfam; ///< Address Family, byte 1
	uint8_t trsvcid[32]; ///< Transport Service Identifier, bytes 33:2
	uint8_t traddr[256]; ///< Transport Address, bytes 289:34
};

/**
 * Kickstart Discovery Response PDU (KDResp) (NVMe over TCP Transport 1.3, 3.6.2.12, Figure 37, page 41, PDF page 41)
 */
struct __attribute__((packed)) xnvme_spec_tcp_kickstart_discovery_response_pdu {
	uint8_t pdu_type; ///< PDU Type, byte 0
	uint8_t flags; ///< Flags, byte 1, see XNVME_SPEC_TCP_KICKSTART_DISCOVERY_RESPONSE_PDU_FLAGS_*
	uint8_t hlen; ///< Header Length, byte 2
	uint8_t pdo; ///< PDU Data Offset, byte 3
	uint32_t plen; ///< PDU Length, bytes 7:4
	uint8_t ksstat; ///< Kickstart Status, byte 8, see XNVME_SPEC_TCP_KICKSTART_DISCOVERY_RESPONSE_PDU_KSSTAT_*
	uint8_t failrsn; ///< Failure Reason, byte 9, see XNVME_SPEC_TCP_KICKSTART_DISCOVERY_RESPONSE_PDU_FAILRSN_*
	uint32_t hdgst; ///< PDU Header Digest, bytes 13:10
	uint8_t cdcnqn[256]; ///< CDC NVMe Qualified Name, bytes 269:14
	uint32_t ddgst; ///< PDU Data Digest, bytes 273:270
};

/** PDU Data Digest Flag (NVMe over TCP Transport 1.3, 3.6.2.12, Figure 37, page 41, PDF page 41); bit 1 of 'flags' */
#define XNVME_SPEC_TCP_KICKSTART_DISCOVERY_RESPONSE_PDU_FLAGS_DDGSTF 0x2
/** PDU Header Digest Flag (NVMe over TCP Transport 1.3, 3.6.2.12, Figure 37, page 41, PDF page 41); bit 0 of 'flags' */
#define XNVME_SPEC_TCP_KICKSTART_DISCOVERY_RESPONSE_PDU_FLAGS_HDGSTF 0x1
/** Failure (NVMe over TCP Transport 1.3, 3.6.2.12, Figure 37, page 41, PDF page 41); bit 1 of 'ksstat' */
#define XNVME_SPEC_TCP_KICKSTART_DISCOVERY_RESPONSE_PDU_KSSTAT_FAILURE 0x2
/** Success (NVMe over TCP Transport 1.3, 3.6.2.12, Figure 37, page 41, PDF page 41); bit 0 of 'ksstat' */
#define XNVME_SPEC_TCP_KICKSTART_DISCOVERY_RESPONSE_PDU_KSSTAT_SUCESS 0x1
/** Insufficient Discovery Resources (NVMe over TCP Transport 1.3, 3.6.2.12, Figure 37, page 41, PDF page 41); bit 6 of 'failrsn' */
#define XNVME_SPEC_TCP_KICKSTART_DISCOVERY_RESPONSE_PDU_FAILRSN_IDR 0x40
/** TRSVCID does not match TRTYPE (NVMe over TCP Transport 1.3, 3.6.2.12, Figure 37, page 41, PDF page 41); bit 5 of 'failrsn' */
#define XNVME_SPEC_TCP_KICKSTART_DISCOVERY_RESPONSE_PDU_FAILRSN_TDNMT 0x20
/** TRADDR does not match ADRFAM (NVMe over TCP Transport 1.3, 3.6.2.12, Figure 37, page 41, PDF page 41); bit 4 of 'failrsn' */
#define XNVME_SPEC_TCP_KICKSTART_DISCOVERY_RESPONSE_PDU_FAILRSN_TDNMA 0x10
/** Invalid ADRFAM (NVMe over TCP Transport 1.3, 3.6.2.12, Figure 37, page 41, PDF page 41); bit 3 of 'failrsn' */
#define XNVME_SPEC_TCP_KICKSTART_DISCOVERY_RESPONSE_PDU_FAILRSN_INVA 0x8
/** Invalid TRTYPE (NVMe over TCP Transport 1.3, 3.6.2.12, Figure 37, page 41, PDF page 41); bit 2 of 'failrsn' */
#define XNVME_SPEC_TCP_KICKSTART_DISCOVERY_RESPONSE_PDU_FAILRSN_INVT 0x4
/** No additional information (NVMe over TCP Transport 1.3, 3.6.2.12, Figure 37, page 41, PDF page 41); bit 1 of 'failrsn' */
#define XNVME_SPEC_TCP_KICKSTART_DISCOVERY_RESPONSE_PDU_FAILRSN_NAI 0x2

#endif /* LIBXNVME_SPEC_TCP_H */
