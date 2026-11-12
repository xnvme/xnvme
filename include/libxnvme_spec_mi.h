/**
 * What the mi specification defines, by xnvme_spec
 *
 * Written from the data model of senfd 0.19.0, by toolbox/xnvme_spec_headers.py;
 * each name and mask is as the figure it cites gives it. Do not
 * edit: see the contributor docs on producing the spec headers.
 *
 * @file libxnvme_spec_mi.h
 */
#ifndef LIBXNVME_SPEC_MI_H
#define LIBXNVME_SPEC_MI_H

#include <stdint.h>

/**
 * The opcodes of the Management Interface commands
 */
enum xnvme_spec_mi_opcode {
	XNVME_SPEC_MI_READ_NVME_MI_DATA_STRUCTURE_OPCODE = 0x0, ///< Read NVMe-MI Data Structure (Management Interface 2.2, 5, Figure 68, page 95, PDF page 95)
	XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_STATUS_POLL_OPCODE = 0x1, ///< NVM Subsystem Health Status Poll (Management Interface 2.2, 5, Figure 68, page 95, PDF page 95)
	XNVME_SPEC_MI_CONTROLLER_HEALTH_STATUS_POLL_OPCODE = 0x2, ///< Controller Health Status Poll (Management Interface 2.2, 5, Figure 68, page 95, PDF page 95)
	XNVME_SPEC_MI_CONFIGURATION_SET_OPCODE = 0x3, ///< Configuration Set (Management Interface 2.2, 5, Figure 68, page 95, PDF page 95)
	XNVME_SPEC_MI_CONFIGURATION_GET_OPCODE = 0x4, ///< Configuration Get (Management Interface 2.2, 5, Figure 68, page 95, PDF page 95)
	XNVME_SPEC_MI_VPD_READ_OPCODE = 0x5, ///< VPD Read (Management Interface 2.2, 5, Figure 68, page 95, PDF page 95)
	XNVME_SPEC_MI_VPD_WRITE_OPCODE = 0x6, ///< VPD Write (Management Interface 2.2, 5, Figure 68, page 95, PDF page 95)
	XNVME_SPEC_MI_RESET_OPCODE = 0x7, ///< Reset (Management Interface 2.2, 5, Figure 68, page 95, PDF page 95)
	XNVME_SPEC_MI_SES_RECEIVE_OPCODE = 0x8, ///< SES Receive (Management Interface 2.2, 5, Figure 68, page 95, PDF page 95)
	XNVME_SPEC_MI_SES_SEND_OPCODE = 0x9, ///< SES Send (Management Interface 2.2, 5, Figure 68, page 95, PDF page 95)
	XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_BUFFER_READ_OPCODE = 0xa, ///< Management Endpoint Buffer Read (Management Interface 2.2, 5, Figure 68, page 95, PDF page 95)
	XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_BUFFER_WRITE_OPCODE = 0xb, ///< Management Endpoint Buffer Write (Management Interface 2.2, 5, Figure 68, page 95, PDF page 95)
	XNVME_SPEC_MI_SHUTDOWN_OPCODE = 0xc, ///< Shutdown (Management Interface 2.2, 5, Figure 68, page 95, PDF page 95)
	XNVME_SPEC_MI_NVME_MI_PDA_READ_OPCODE = 0xd, ///< NVMe-MI PDA Read (Management Interface 2.2, 5, Figure 68, page 95, PDF page 95)
	XNVME_SPEC_MI_NVME_MI_PDA_WRITE_OPCODE = 0xe, ///< NVMe-MI PDA Write (Management Interface 2.2, 5, Figure 68, page 95, PDF page 95)
	XNVME_SPEC_MI_NVME_MI_PDA_WRITE_ZEROES_OPCODE = 0xf, ///< NVMe-MI PDA Write Zeroes (Management Interface 2.2, 5, Figure 68, page 95, PDF page 95)
};

/**
 * The opcodes of the PCIe Commands using an Out-of-Band Mechanism commands
 */
enum xnvme_spec_pcie_commands_opcode {
	XNVME_SPEC_PCIE_COMMANDS_PCIE_CONFIGURATION_READ_OPCODE = 0x0, ///< PCIe Configuration Read (Management Interface 2.2, 7, Figure 151, page 154, PDF page 154)
	XNVME_SPEC_PCIE_COMMANDS_PCIE_CONFIGURATION_WRITE_OPCODE = 0x1, ///< PCIe Configuration Write (Management Interface 2.2, 7, Figure 151, page 154, PDF page 154)
	XNVME_SPEC_PCIE_COMMANDS_PCIE_MEMORY_READ_OPCODE = 0x2, ///< PCIe Memory Read (Management Interface 2.2, 7, Figure 151, page 154, PDF page 154)
	XNVME_SPEC_PCIE_COMMANDS_PCIE_MEMORY_WRITE_OPCODE = 0x3, ///< PCIe Memory Write (Management Interface 2.2, 7, Figure 151, page 154, PDF page 154)
	XNVME_SPEC_PCIE_COMMANDS_PCIE_IO_READ_OPCODE = 0x4, ///< PCIe I/O Read (Management Interface 2.2, 7, Figure 151, page 154, PDF page 154)
	XNVME_SPEC_PCIE_COMMANDS_PCIE_IO_WRITE_OPCODE = 0x5, ///< PCIe I/O Write (Management Interface 2.2, 7, Figure 151, page 154, PDF page 154)
};

/**
 * The identifiers of the log pages
 */
enum xnvme_spec_mi_log_page_identifier {
	XNVME_SPEC_MI_LOG_PAGE_SMART_HEALTH_INFORMATION_NVM_SUBSYSTEM_SCOPE = 0x2, ///< SMART / Health Information (NVM Subsystem scope) (Management Interface 2.2, 6.3, Figure 144, page 148, PDF page 148)
	XNVME_SPEC_MI_LOG_PAGE_HOST_ADDRESSABLE_NAMESPACES = 0x85, ///< Host Addressable Namespaces (Management Interface 2.2, 6.3, Figure 144, page 148, PDF page 148)
};

/**
 * The status codes of the type Response Message
 */
enum xnvme_spec_mi_response_message_status {
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_SUCCESS = 0x0, ///< Success (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_MORE_PROCESSING_REQUIRED = 0x1, ///< More Processing Required (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_INTERNAL_ERROR = 0x2, ///< Internal Error (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_INVALID_COMMAND_OPCODE = 0x3, ///< Invalid Command Opcode (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_INVALID_PARAMETER = 0x4, ///< Invalid Parameter (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_INVALID_COMMAND_SIZE = 0x5, ///< Invalid Command Size (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_INVALID_COMMAND_INPUT_DATA_SIZE = 0x6, ///< Invalid Command Input Data Size (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_ACCESS_DENIED = 0x7, ///< Access Denied (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_UNABLE_TO_ABORT = 0x8, ///< Unable to Abort (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_VPD_UPDATES_EXCEEDED = 0x20, ///< VPD Updates Exceeded (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_PCIE_INACCESSIBLE = 0x21, ///< PCIe Inaccessible (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_MANAGEMENT_ENDPOINT_BUFFER_CLEARED_DUE_TO_SANITIZE = 0x22, ///< Management Endpoint Buffer Cleared Due to Sanitize (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_ENCLOSURE_SERVICES_FAILURE = 0x23, ///< Enclosure Services Failure (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_ENCLOSURE_SERVICES_TRANSFER_FAILURE = 0x24, ///< Enclosure Services Transfer Failure (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_ENCLOSURE_FAILURE = 0x25, ///< Enclosure Failure (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_ENCLOSURE_SERVICES_TRANSFER_REFUSED = 0x26, ///< Enclosure Services Transfer Refused (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_UNSUPPORTED_ENCLOSURE_FUNCTION = 0x27, ///< Unsupported Enclosure Function (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_ENCLOSURE_SERVICES_UNAVAILABLE = 0x28, ///< Enclosure Services Unavailable (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_ENCLOSURE_DEGRADED = 0x29, ///< Enclosure Degraded (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
	XNVME_SPEC_MI_RESPONSE_MESSAGE_STATUS_SANITIZE_IN_PROGRESS = 0x2a, ///< Sanitize In Progress (Management Interface 2.2, 4.1.2, Figure 29, page 51, PDF page 51)
};

/**
 * NVMe-MI Message Fields (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41)
 *
 * Followed, from byte 4, by as many Message Data as the data says
 *
 * Not members:
 * Message Integrity Check: at an offset the data gives, in 'message_data'
 */
struct __attribute__((packed)) xnvme_spec_mi_nvme_mi_message_fields {
	uint8_t mctpd; ///< MCTP Data, byte 0, see XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_MCTPD_*
	uint8_t nmp; ///< NVMe-MI Message Parameters, byte 1, see XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_NMP_*
	uint8_t byte_2; ///< Byte 2, byte 2, see XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_BYTE_2_*
	uint8_t rsvd3; ///< Reserved, byte 3
};

/** Integrity Check (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41); bit 7 of 'mctpd' */
#define XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_MCTPD_IC 0x80
/** Message Type (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41); bits 6:0 of 'mctpd' */
#define XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_MCTPD_MT 0x7f
/** Request or Response (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41); bit 7 of 'nmp' */
#define XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_NMP_ROR 0x80
/** NVMe-MI Message Type (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41); bits 6:3 of 'nmp' */
#define XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_NMP_NMIMT 0x78
/** Command Slot Identifier (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41); bit 0 of 'nmp' */
#define XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_NMP_CSI 0x1
/** Command Initiated Auto Pause (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41); bit 1 of 'byte_2' */
#define XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_BYTE_2_CIAP 0x2
/** Management Endpoint Buffer (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41); bit 0 of 'byte_2' */
#define XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_BYTE_2_MEB 0x1

/**
 * The values of NVMe-MI Message Type (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41)
 */
enum xnvme_spec_mi_nvme_mi_message_fields_nmp_nmimt {
	XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_NMP_NMIMT_CONTROL_PRIMITIVE = 0x0, ///< Control Primitive (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41)
	XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_NMP_NMIMT_NVME_MI_COMMAND = 0x1, ///< NVMe-MI Command (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41)
	XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_NMP_NMIMT_NVME_ADMIN_COMMAND = 0x2, ///< NVMe Admin Command (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41)
	XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_NMP_NMIMT_PCIE_COMMAND = 0x4, ///< PCIe Command (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41)
	XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_NMP_NMIMT_ASYNCHRONOUS_EVENT = 0x5, ///< Asynchronous Event (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41)
};

/**
 * The values of Command Slot Identifier (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41)
 */
enum xnvme_spec_mi_nvme_mi_message_fields_nmp_csi {
	XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_NMP_CSI_COMMAND_SLOT_0 = 0x0, ///< Command Slot 0 (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41)
	XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_NMP_CSI_COMMAND_SLOT_1 = 0x1, ///< Command Slot 1 (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41)
};

/**
 * The values of Management Endpoint Buffer (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41)
 */
enum xnvme_spec_mi_nvme_mi_message_fields_byte_2_meb {
	XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_BYTE_2_MEB_OUT_OF_BAND_COMMAND_MESSAGES = 0x0, ///< Out-of-band Command Messages (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41)
	XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_BYTE_2_MEB_OUT_OF_BAND_COMMAND_MESSAGES_1H = 0x1, ///< Out-of-band Command Messages (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41)
	XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_BYTE_2_MEB_IN_BAND_TUNNELING = 0x0, ///< In-band Tunneling (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41)
	XNVME_SPEC_MI_NVME_MI_MESSAGE_FIELDS_BYTE_2_MEB_IN_BAND_TUNNELING_IN_BAND_TUNNELING = 0x1, ///< In-band Tunneling In-band Tunneling (Management Interface 2.2, 3.1.1, Figure 20, page 41, PDF page 41)
};

/**
 * Response Message Fields (Management Interface 2.2, 4.1.2, Figure 28, page 50, PDF page 50)
 *
 * Followed, from byte 5, by as many Response Body as the data says
 *
 * Not members:
 * Message Integrity Check: at an offset the data gives, in 'response_body'
 */
struct __attribute__((packed)) xnvme_spec_mi_response_message_fields {
	uint32_t nmh; ///< NVMe-MI Message Header, bytes 3:0
	uint8_t status; ///< Status, byte 4
};

/**
 * Invalid Parameter Error Response Fields (Management Interface 2.2, 4.1.2.2, Figure 32, page 53, PDF page 53)
 */
struct __attribute__((packed)) xnvme_spec_mi_invalid_parameter_error_response_fields {
	uint8_t rsvd0[5]; ///< Reserved, bytes 4:0
	uint8_t pel[3]; ///< Parameter Error Location, bytes 7:5
};

/**
 * More Processing Required Response Fields (Management Interface 2.2, 4.1.2.3, Figure 34, page 54, PDF page 54)
 */
struct __attribute__((packed)) xnvme_spec_mi_more_processing_required_response_fields {
	uint8_t rsvd0[6]; ///< Reserved, bytes 5:0
	uint16_t mprt; ///< More Processing Required Time, bytes 7:6
};

/**
 * Control Primitive Fields (Management Interface 2.2, 4.2.1, Figure 37, page 57, PDF page 57)
 */
struct __attribute__((packed)) xnvme_spec_mi_control_primitive_fields {
	uint32_t nmh; ///< NVMe-MI Message Header, bytes 3:0
	uint8_t cpo; ///< Control Primitive Opcode, byte 4
	uint8_t tag; ///< Tag, byte 5
	uint16_t cpsp; ///< Control Primitive Specific Parameter, bytes 7:6
	uint32_t mic; ///< Message Integrity Check, bytes 11:8
};

/**
 * The values of Control Primitive Opcode (Management Interface 2.2, 4.2.1, Figure 38, page 58, PDF page 58)
 */
enum xnvme_spec_mi_control_primitive_fields_cpo {
	XNVME_SPEC_MI_CONTROL_PRIMITIVE_FIELDS_CPO_PAUSE = 0x0, ///< Pause (Management Interface 2.2, 4.2.1, Figure 38, page 58, PDF page 58)
	XNVME_SPEC_MI_CONTROL_PRIMITIVE_FIELDS_CPO_RESUME = 0x1, ///< Resume (Management Interface 2.2, 4.2.1, Figure 38, page 58, PDF page 58)
	XNVME_SPEC_MI_CONTROL_PRIMITIVE_FIELDS_CPO_ABORT = 0x2, ///< Abort (Management Interface 2.2, 4.2.1, Figure 38, page 58, PDF page 58)
	XNVME_SPEC_MI_CONTROL_PRIMITIVE_FIELDS_CPO_GET_STATE = 0x3, ///< Get State (Management Interface 2.2, 4.2.1, Figure 38, page 58, PDF page 58)
	XNVME_SPEC_MI_CONTROL_PRIMITIVE_FIELDS_CPO_REPLAY = 0x4, ///< Replay (Management Interface 2.2, 4.2.1, Figure 38, page 58, PDF page 58)
};

/**
 * Control Primitive Success Response Fields (Management Interface 2.2, 4.2.1, Figure 40, page 58, PDF page 58)
 */
struct __attribute__((packed)) xnvme_spec_mi_control_primitive_success_response_fields {
	uint32_t nmh; ///< NVMe-MI Message Header, bytes 3:0
	uint8_t status; ///< Status, byte 4
	uint8_t tag; ///< Tag, byte 5
	uint16_t cpsr; ///< Control Primitive Specific Response, bytes 7:6
	uint32_t mic; ///< Message Integrity Check, bytes 11:8
};

/**
 * Pause Control Primitive Success Response Fields (Management Interface 2.2, 4.2.1.1, Figure 41, page 59, PDF page 59)
 */
struct __attribute__((packed)) xnvme_spec_mi_pause_control_primitive_success_response_fields {
	uint8_t rsvd0[6]; ///< Reserved, bytes 5:0
	uint16_t cpsr; ///< Control Primitive Specific Response, bytes 7:6
};

/**
 * Abort Control Primitive Success Response Fields (Management Interface 2.2, 4.2.1.3, Figure 42, page 60, PDF page 60)
 */
struct __attribute__((packed)) xnvme_spec_mi_abort_control_primitive_success_response_fields {
	uint8_t rsvd0[6]; ///< Reserved, bytes 5:0
	uint16_t cpsr; ///< Control Primitive Specific Response, bytes 7:6, see XNVME_SPEC_MI_ABORT_CONTROL_PRIMITIVE_SUCCESS_RESPONSE_FIELDS_CPSR_*
};

/** Command Processing Abort Status (Management Interface 2.2, 4.2.1.3, Figure 42, page 60, PDF page 60); bits 1:0 of 'cpsr' */
#define XNVME_SPEC_MI_ABORT_CONTROL_PRIMITIVE_SUCCESS_RESPONSE_FIELDS_CPSR_CPAS 0x3

/**
 * Management Endpoint State (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61)
 */
struct __attribute__((packed)) xnvme_spec_mi_management_endpoint_state {
	uint16_t mes; ///< Management Endpoint State, bytes 1:0, see XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_STATE_MES_*
};

/** Pause Flag (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61); bit 15 of 'mes' */
#define XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_STATE_MES_PFLG 0x8000
/** NVM Subsystem Reset Occurred (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61); bit 14 of 'mes' */
#define XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_STATE_MES_NSSRO 0x4000
/** Bad Packet or Other Physical Layer (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61); bit 13 of 'mes' */
#define XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_STATE_MES_BPOPL 0x2000
/** Bad, Unexpected, or Expired Message Tag (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61); bit 12 of 'mes' */
#define XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_STATE_MES_BUEMT 0x1000
/** Out-of-Sequence Packet Sequence Number (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61); bit 11 of 'mes' */
#define XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_STATE_MES_OSPSN 0x800
/** Unexpected Middle or End of Packet (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61); bit 10 of 'mes' */
#define XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_STATE_MES_UMEP 0x400
/** Incorrect Transmission Unit (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61); bit 9 of 'mes' */
#define XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_STATE_MES_ITU 0x200
/** Unknown Destination ID (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61); bit 8 of 'mes' */
#define XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_STATE_MES_UDSTID 0x100
/** Bad Header Version (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61); bit 7 of 'mes' */
#define XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_STATE_MES_BHVS 0x80
/** Unsupported Transmission Unit (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61); bit 6 of 'mes' */
#define XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_STATE_MES_UTUNT 0x40
/** Bad Message Integrity Check Error (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61); bit 4 of 'mes' */
#define XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_STATE_MES_BMICE 0x10
/** Command Message to non-Idle Command Slot (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61); bit 3 of 'mes' */
#define XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_STATE_MES_CMNICS 0x8
/** Slot Command Servicing State (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61); bits 1:0 of 'mes' */
#define XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_STATE_MES_SSTA 0x3

/**
 * The values of Slot Command Servicing State (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61)
 */
enum xnvme_spec_mi_management_endpoint_state_mes_ssta {
	XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_STATE_MES_SSTA_IDLE = 0x0, ///< Idle (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61)
	XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_STATE_MES_SSTA_RECEIVE = 0x1, ///< Receive (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61)
	XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_STATE_MES_SSTA_PROCESS = 0x2, ///< Process (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61)
	XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_STATE_MES_SSTA_TRANSMIT = 0x3, ///< Transmit (Management Interface 2.2, 4.2.1.4, Figure 43, page 61, PDF page 61)
};

/**
 * Get State Control Primitive Request Message Fields (Management Interface 2.2, 4.2.1.4, Figure 44, page 62, PDF page 62)
 */
struct __attribute__((packed)) xnvme_spec_mi_get_state_control_primitive_request_message_fields {
	uint8_t rsvd0[6]; ///< Reserved, bytes 5:0
	uint16_t cpsp; ///< Control Primitive Specific Parameter, bytes 7:6, see XNVME_SPEC_MI_GET_STATE_CONTROL_PRIMITIVE_REQUEST_MESSAGE_FIELDS_CPSP_*
};

/** Clear Error State Flags (Management Interface 2.2, 4.2.1.4, Figure 44, page 62, PDF page 62); bit 0 of 'cpsp' */
#define XNVME_SPEC_MI_GET_STATE_CONTROL_PRIMITIVE_REQUEST_MESSAGE_FIELDS_CPSP_CESF 0x1

/**
 * Get State Control Primitive Success Response Fields (Management Interface 2.2, 4.2.1.4, Figure 45, page 63, PDF page 63)
 */
struct __attribute__((packed)) xnvme_spec_mi_get_state_control_primitive_success_response_fields {
	uint8_t rsvd0[6]; ///< Reserved, bytes 5:0
	uint16_t cpsr; ///< Control Primitive Specific Response, bytes 7:6
};

/**
 * Replay Control Primitive Request Fields (Management Interface 2.2, 4.2.1.5, Figure 46, page 64, PDF page 64)
 */
struct __attribute__((packed)) xnvme_spec_mi_replay_control_primitive_request_fields {
	uint8_t rsvd0[6]; ///< Reserved, bytes 5:0
	uint16_t cpsp; ///< Control Primitive Specific Parameter, bytes 7:6, see XNVME_SPEC_MI_REPLAY_CONTROL_PRIMITIVE_REQUEST_FIELDS_CPSP_*
};

/** Response Replay Offset (Management Interface 2.2, 4.2.1.5, Figure 46, page 64, PDF page 64); bits 7:0 of 'cpsp' */
#define XNVME_SPEC_MI_REPLAY_CONTROL_PRIMITIVE_REQUEST_FIELDS_CPSP_RRO 0xff

/**
 * Replay Control Primitive Success Response Fields (Management Interface 2.2, 4.2.1.5, Figure 47, page 64, PDF page 64)
 */
struct __attribute__((packed)) xnvme_spec_mi_replay_control_primitive_success_response_fields {
	uint8_t rsvd0[6]; ///< Reserved, bytes 5:0
	uint16_t cpsr; ///< Control Primitive Specific Response, bytes 7:6, see XNVME_SPEC_MI_REPLAY_CONTROL_PRIMITIVE_SUCCESS_RESPONSE_FIELDS_CPSR_*
};

/** Response Replay (Management Interface 2.2, 4.2.1.5, Figure 47, page 64, PDF page 64); bit 0 of 'cpsr' */
#define XNVME_SPEC_MI_REPLAY_CONTROL_PRIMITIVE_SUCCESS_RESPONSE_FIELDS_CPSR_RR 0x1

/**
 * NVMe-MI Send Command Response Message to NVMe Admin Command CQE Mapping Table (Management Interface 2.2, 4.3.1.1, Figure 51, page 71, PDF page 71)
 *
 * Followed, from byte 8, by as many Response Data as the data says
 *
 * Not members:
 * Message Integrity Check: at an offset the data gives, in 'response_data'
 */
struct __attribute__((packed)) xnvme_spec_mi_nvme_mi_send_command_response_message_to_nvme_admin_command_cqe_mapping_table {
	uint8_t mctpd; ///< MCTP Data, byte 0
	uint8_t nvme_mi_message_parameters; ///< NVMe-MI Message Parameters, byte 1
	uint16_t rsvd2; ///< Reserved, bytes 3:2
	uint8_t status; ///< Status, byte 4
	uint8_t nvme_management_response[3]; ///< NVMe Management Response, bytes 7:5
};

/**
 * NVMe-MI Receive Command Response Message to NVMe Admin Command CQE Mapping Table (Management Interface 2.2, 4.3.2.1, Figure 56, page 78, PDF page 78)
 *
 * Followed, from byte 8, by as many Response Data as the data says
 *
 * Not members:
 * Message Integrity Check: at an offset the data gives, in 'response_data'
 */
struct __attribute__((packed)) xnvme_spec_mi_nvme_mi_receive_command_response_message_to_nvme_admin_command_cqe_mapping_table {
	uint8_t mctpd; ///< MCTP Data, byte 0
	uint8_t nvme_mi_message; ///< NVMe-MI Message, byte 1
	uint16_t rsvd2; ///< Reserved, bytes 3:2
	uint8_t status; ///< Status, byte 4
	uint8_t nvme_management[3]; ///< NVMe Management, bytes 7:5
};

/**
 * Asynchronous Event Message (AEM) Fields (Management Interface 2.2, 4.4.4, Figure 60, page 83, PDF page 83)
 *
 * Followed, from byte 4, by as many Asynchronous Event Data as the data says
 *
 * Not members:
 * Message Integrity Check: at an offset the data gives, in 'asynchronous_event_data'
 */
struct __attribute__((packed)) xnvme_spec_mi_asynchronous_event_message_aem_fields {
	uint32_t nmh; ///< NVMe-MI Message Header, bytes 3:0
};

/**
 * AE Occurrence List (Management Interface 2.2, 4.4.4, Figure 61, page 84, PDF page 84)
 *
 * Followed, from byte 7, by as many AE Occurrence as the data says
 *
 * Not members:
 * AE Occurrence 1: at an offset the data gives, in 'ae_occurrence'
 * AE Occurrence N: at an offset the data gives, in 'ae_occurrence'
 */
struct __attribute__((packed)) xnvme_spec_mi_ae_occurrence_list {
	uint8_t numaeo; ///< Number of AE Occurrence Data Structures, byte 0
	uint8_t aelver; ///< AE Occurrence List Version Number, byte 1
	uint8_t aeolli[3]; ///< AE Occurrence List Length Info, bytes 4:2
	uint8_t aeolhl; ///< AE Occurrence List Header Length, byte 5
	uint8_t aemti; ///< AEM Transmission Info, byte 6, see XNVME_SPEC_MI_AE_OCCURRENCE_LIST_AEMTI_*
};

/** AEM Generation Number (Management Interface 2.2, 4.4.4, Figure 61, page 84, PDF page 84); bits 7:3 of 'aemti' */
#define XNVME_SPEC_MI_AE_OCCURRENCE_LIST_AEMTI_AEMGN 0xf8
/** AEM Retry Count (Management Interface 2.2, 4.4.4, Figure 61, page 84, PDF page 84); bits 2:0 of 'aemti' */
#define XNVME_SPEC_MI_AE_OCCURRENCE_LIST_AEMTI_AEMRC 0x7

/**
 * AE Occurrence (Management Interface 2.2, 4.4.4, Figure 62, page 85, PDF page 85)
 *
 * Followed, from byte 9, by as many AE Occurrence Specific Info as the data says
 *
 * Not members:
 * AE Occurrence Vendor Specific Info: at an offset the data gives, in 'ae_occurrence_specific_info'
 */
struct __attribute__((packed)) xnvme_spec_mi_ae_occurrence {
	uint8_t aelhlen; ///< AE Occurrence Header Length, byte 0
	uint8_t aeosil; ///< AE Occurrence Specific Info Length, byte 1
	uint8_t aeovsil; ///< AE Occurrence Vendor Specific Info Length, byte 2
	uint8_t aeoui[6]; ///< AE Occurrence Unique ID, bytes 8:3
};

/**
 * NVMe-MI Command Request Message Description (NCREQ) (Management Interface 2.2, 5, Figure 67, page 94, PDF page 94)
 *
 * Followed, from byte 8, by as many NVMe Management Dword as the data says
 *
 * Not members:
 * Message Integrity Check: at an offset the data gives, in 'nmd'
 */
struct __attribute__((packed)) xnvme_spec_mi_nvme_mi_command_request_message_description {
	uint32_t nmh; ///< NVMe-MI Message Header, bytes 3:0
	uint8_t opc; ///< Opcode, byte 4
	uint8_t rsvd5[3]; ///< Reserved, bytes 7:5
};

/**
 * NVMe-MI Command Response Message Description (NCRESP) (Management Interface 2.2, 5, Figure 72, page 97, PDF page 97)
 *
 * Followed, from byte 8, by as many Response Data as the data says
 *
 * Not members:
 * Message Integrity Check: at an offset the data gives, in 'response_data'
 */
struct __attribute__((packed)) xnvme_spec_mi_nvme_mi_command_response_message_description {
	uint32_t nmh; ///< NVMe-MI Message Header, bytes 3:0
	uint8_t status; ///< Status, byte 4
	uint8_t nmresp[3]; ///< NVMe Management Response, bytes 7:5
};

/**
 * AE Supported (Management Interface 2.2, 5.1.4, Figure 83, page 101, PDF page 101)
 */
struct __attribute__((packed)) xnvme_spec_mi_ae_supported {
	uint8_t aesl; ///< AE Supported Length, byte 0
	uint16_t aesi; ///< AE Supported Info, bytes 2:1, see XNVME_SPEC_MI_AE_SUPPORTED_AESI_*
};

/** AE Supported Enable (Management Interface 2.2, 5.1.4, Figure 83, page 101, PDF page 101); bit 15 of 'aesi' */
#define XNVME_SPEC_MI_AE_SUPPORTED_AESI_AESE 0x8000
/** AE Supported ID (Management Interface 2.2, 5.1.4, Figure 83, page 101, PDF page 101); bits 7:0 of 'aesi' */
#define XNVME_SPEC_MI_AE_SUPPORTED_AESI_AESI 0xff

/**
 * AE Supported List (Management Interface 2.2, 5.1.4, Figure 82, page 100, PDF page 100)
 *
 * Followed, from byte 5, by as many AE Supported as the data says, each a 'struct xnvme_spec_mi_ae_supported'
 *
 * Not members:
 * AE Supported 1: at an offset the data gives, in 'ae_supported'
 * AE Supported N: at an offset the data gives, in 'ae_supported'
 */
struct __attribute__((packed)) xnvme_spec_mi_ae_supported_list {
	uint8_t numaes; ///< Number of AE Supported Data Structures, byte 0
	uint8_t aeslver; ///< AE Supported List Version Number, byte 1
	uint16_t aestl; ///< AE Supported Total Length, bytes 3:2
	uint8_t aeslhl; ///< AE Supported List Header Length, byte 4
};

/**
 * AE Enable (Management Interface 2.2, 5.2.4, Figure 92, page 106, PDF page 106)
 */
struct __attribute__((packed)) xnvme_spec_mi_ae_enable {
	uint8_t aeel; ///< AE Enable Length, byte 0
	uint16_t aeei; ///< AE Enable Info, bytes 2:1, see XNVME_SPEC_MI_AE_ENABLE_AEEI_*
};

/** AE Enable (Management Interface 2.2, 5.2.4, Figure 92, page 106, PDF page 106); bit 15 of 'aeei' */
#define XNVME_SPEC_MI_AE_ENABLE_AEEI_AEE 0x8000
/** AE Enable ID (Management Interface 2.2, 5.2.4, Figure 92, page 106, PDF page 106); bits 7:0 of 'aeei' */
#define XNVME_SPEC_MI_AE_ENABLE_AEEI_AEEI 0xff

/**
 * AE Enable List (Management Interface 2.2, 5.2.4, Figure 91, page 106, PDF page 106)
 *
 * Followed, from byte 5, by as many AE Enable as the data says, each a 'struct xnvme_spec_mi_ae_enable'
 *
 * Not members:
 * AE Enable 1: at an offset the data gives, in 'ae_enable'
 * AE Enable N: at an offset the data gives, in 'ae_enable'
 */
struct __attribute__((packed)) xnvme_spec_mi_ae_enable_list {
	uint8_t numaee; ///< Number of AE Enable Data Structures, byte 0
	uint8_t aeelver; ///< AE Enable List Version Number, byte 1
	uint16_t aeetl; ///< AE Enable Total Length, bytes 3:2
	uint8_t aeelhl; ///< AE Enable List Header Length, byte 4
};

/**
 * Controller Health Data Structure (CHDS) (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111)
 */
struct __attribute__((packed)) xnvme_spec_mi_controller_health_data_structure {
	uint16_t ctlid; ///< Controller Identifier, bytes 1:0
	uint16_t cs; ///< Controller Status, bytes 3:2, see XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CS_*
	uint16_t ctemp; ///< Composite Temperature, bytes 5:4
	uint8_t pdlu; ///< Percentage Used, byte 6
	uint8_t spare; ///< Available Spare, byte 7
	uint8_t cwarn; ///< Critical Warning, byte 8, see XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CWARN_*
	uint16_t chscf; ///< Controller Health Status Changed Flags, bytes 10:9, see XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CHSCF_*
	uint8_t rsvd11[5]; ///< Reserved, bytes 15:11
};

/** Firmware Activated (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 7 of 'cs' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CS_FA 0x80
/** Namespace Attribute Changed (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 6 of 'cs' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CS_NAC 0x40
/** Controller Enable Change Occurred (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 5 of 'cs' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CS_CECO 0x20
/** NVM Subsystem Reset Occurred (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 4 of 'cs' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CS_NSSRO 0x10
/** Shutdown Status (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bits 3:2 of 'cs' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CS_SHST 0xc
/** Controller Fatal Status (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 1 of 'cs' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CS_CFS 0x2
/** Ready (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 0 of 'cs' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CS_RDY 0x1
/** Indeterminate Personality State (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 6 of 'cwarn' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CWARN_IPS 0x40
/** Persistent Memory Region Error (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 5 of 'cwarn' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CWARN_PMRE 0x20
/** Volatile Memory Backup Failed (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 4 of 'cwarn' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CWARN_VMBF 0x10
/** Read Only (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 3 of 'cwarn' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CWARN_RO 0x8
/** Reliability Degraded (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 2 of 'cwarn' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CWARN_RD 0x4
/** Temperature Above or Under Threshold (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 1 of 'cwarn' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CWARN_TAUT 0x2
/** Spare Threshold (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 0 of 'cwarn' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CWARN_ST 0x1
/** Telemetry Controller-Initiated Data Available Flag (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 13 of 'chscf' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CHSCF_TCIDAF 0x2000
/** Critical Warning Flag (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 12 of 'chscf' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CHSCF_CWARNF 0x1000
/** Available Spare Flag (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 11 of 'chscf' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CHSCF_SPAREF 0x800
/** Percentage Drive Life Used Flag (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 10 of 'chscf' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CHSCF_PDLUF 0x400
/** Composite Temperature Change Flag (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 9 of 'chscf' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CHSCF_CTEMPF 0x200
/** Controller Status Change Flag (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 8 of 'chscf' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CHSCF_CSF 0x100
/** Firmware Activated Flag (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 7 of 'chscf' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CHSCF_FAF 0x80
/** Namespace Attribute Changed Flag (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 6 of 'chscf' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CHSCF_NACF 0x40
/** Controller Enable Change Occurred Flag (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 5 of 'chscf' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CHSCF_CECOF 0x20
/** NVM Subsystem Reset Occurred Flag (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 4 of 'chscf' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CHSCF_NSSROF 0x10
/** Shutdown Status Flag (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 2 of 'chscf' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CHSCF_SHSTF 0x4
/** Controller Fatal Status Flag (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 1 of 'chscf' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CHSCF_CFSF 0x2
/** Ready Flag (Management Interface 2.2, 5.3, Figure 97, page 111, PDF page 111); bit 0 of 'chscf' */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_DATA_STRUCTURE_CHSCF_RDYF 0x1

/**
 * NVM Subsystem Health Data Structure (NSHDS) (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118)
 */
struct __attribute__((packed)) xnvme_spec_mi_nvm_subsystem_health_data_structure {
	uint8_t nss; ///< NVM Subsystem Status, byte 0, see XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_NSS_*
	uint8_t sw; ///< SMART Warnings, byte 1
	uint8_t ctemp; ///< Composite Temperature, byte 2
	uint8_t pdlu; ///< Percentage Drive Life Used, byte 3
	uint16_t ccsf; ///< Composite Controller Status Flags, bytes 5:4, see XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_CCSF_*
	uint16_t rsvd6; ///< Reserved, bytes 7:6
};

/** AEM Transmission Failure (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 7 of 'nss' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_NSS_ATF 0x80
/** Sanitize Failure Mode (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 6 of 'nss' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_NSS_SFM 0x40
/** Drive Functional (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 5 of 'nss' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_NSS_DF 0x20
/** Reset Not Required (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 4 of 'nss' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_NSS_RNR 0x10
/** Port 0 PCIe Link Active (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 3 of 'nss' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_NSS_P0LA 0x8
/** Port 1 PCIe Link Active (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 2 of 'nss' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_NSS_P1LA 0x4
/** Sanitize Namespace Failure Mode (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 1 of 'nss' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_NSS_SNFM 0x2
/** Telemetry Controller-Initiated Data Available Flag (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 13 of 'ccsf' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_CCSF_TCIDAF 0x2000
/** Critical Warning Flag (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 12 of 'ccsf' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_CCSF_CWARNF 0x1000
/** Available Spare Flag (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 11 of 'ccsf' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_CCSF_SPAREF 0x800
/** Percentage Drive Life Used Flag (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 10 of 'ccsf' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_CCSF_PDLUF 0x400
/** Composite Temperature Change Flag (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 9 of 'ccsf' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_CCSF_CTEMPF 0x200
/** Controller Status Change Flag (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 8 of 'ccsf' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_CCSF_CSF 0x100
/** Firmware Activated Flag (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 7 of 'ccsf' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_CCSF_FAF 0x80
/** Namespace Attribute Changed Flag (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 6 of 'ccsf' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_CCSF_NACF 0x40
/** Controller Enable Change Occurred Flag (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 5 of 'ccsf' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_CCSF_CECOF 0x20
/** NVM Subsystem Reset Occurred Flag (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 4 of 'ccsf' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_CCSF_NSSROF 0x10
/** Shutdown Status Flag (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 2 of 'ccsf' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_CCSF_SHSTF 0x4
/** Controller Fatal Status Flag (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 1 of 'ccsf' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_CCSF_CFSF 0x2
/** Ready Flag (Management Interface 2.2, 5.6, Figure 105, page 118, PDF page 118); bit 0 of 'ccsf' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_DATA_STRUCTURE_CCSF_RDYF 0x1

/**
 * NVMe-MI PDA Command Request Message Description (Management Interface 2.2, 5.7, Figure 107, page 121, PDF page 121)
 *
 * Followed, from byte 8, by as many NVMe Management Dword as the data says
 *
 * Not members:
 * Message Integrity Check: at an offset the data gives, in 'nmd'
 */
struct __attribute__((packed)) xnvme_spec_mi_nvme_mi_pda_command_request_message_description {
	uint32_t nmh; ///< NVMe-MI Message Header, bytes 3:0
	uint8_t opc; ///< Opcode, byte 4
	uint8_t dformat; ///< Data Format, byte 5
	uint16_t rsvd6; ///< Reserved, bytes 7:6
};

/**
 * The values of Data Format (Management Interface 2.2, 5.7, Figure 107, page 121, PDF page 121)
 */
enum xnvme_spec_mi_nvme_mi_pda_command_request_message_description_dformat {
	XNVME_SPEC_MI_NVME_MI_PDA_COMMAND_REQUEST_MESSAGE_DESCRIPTION_DFORMAT_4KIB = 0x1, ///< 4KiB (Management Interface 2.2, 5.7, Figure 107, page 121, PDF page 121)
	XNVME_SPEC_MI_NVME_MI_PDA_COMMAND_REQUEST_MESSAGE_DESCRIPTION_DFORMAT_512B = 0x2, ///< 512B (Management Interface 2.2, 5.7, Figure 107, page 121, PDF page 121)
	XNVME_SPEC_MI_NVME_MI_PDA_COMMAND_REQUEST_MESSAGE_DESCRIPTION_DFORMAT_BYTE_LEVEL = 0x3, ///< Byte Level (Management Interface 2.2, 5.7, Figure 107, page 121, PDF page 121)
};

/**
 * NVM Subsystem Information (Management Interface 2.2, 5.8.1, Figure 115, page 126, PDF page 126)
 */
struct __attribute__((packed)) xnvme_spec_mi_nvm_subsystem_information {
	uint8_t nump; ///< Number of Ports, byte 0
	uint8_t mjr; ///< NVMe-MI Major Version Number, byte 1
	uint8_t mnr; ///< NVMe-MI Minor Version Number, byte 2
	uint8_t nnsc; ///< NVMe-MI NVM Subsystem Capabilities, byte 3
	uint8_t rsvd4[28]; ///< Reserved, bytes 31:4
};

/**
 * Port Information (Management Interface 2.2, 5.8.2, Figure 117, page 127, PDF page 127)
 */
struct __attribute__((packed)) xnvme_spec_mi_port_information {
	uint8_t prttyp; ///< Port Type, byte 0
	uint8_t prtcap; ///< Port Capabilities, byte 1, see XNVME_SPEC_MI_PORT_INFORMATION_PRTCAP_*
	uint16_t mmtus; ///< Maximum MCTP Transmission Unit Size, bytes 3:2
	uint32_t mebs; ///< Management Endpoint Buffer Size, bytes 7:4
	uint8_t ptsp[24]; ///< Port Type Specific, bytes 31:8
};

/** Asynchronous Event Messages Supported (Management Interface 2.2, 5.8.2, Figure 117, page 127, PDF page 127); bit 1 of 'prtcap' */
#define XNVME_SPEC_MI_PORT_INFORMATION_PRTCAP_AEMS 0x2

/**
 * The values of Port Type (Management Interface 2.2, 5.8.2, Figure 117, page 127, PDF page 127)
 */
enum xnvme_spec_mi_port_information_prttyp {
	XNVME_SPEC_MI_PORT_INFORMATION_PRTTYP_INACTIVE = 0x0, ///< Inactive (Management Interface 2.2, 5.8.2, Figure 117, page 127, PDF page 127)
	XNVME_SPEC_MI_PORT_INFORMATION_PRTTYP_PCIE = 0x1, ///< PCIe (Management Interface 2.2, 5.8.2, Figure 117, page 127, PDF page 127)
	XNVME_SPEC_MI_PORT_INFORMATION_PRTTYP_2_WIRE = 0x2, ///< 2-Wire (Management Interface 2.2, 5.8.2, Figure 117, page 127, PDF page 127)
};

/**
 * PCIe Port Specific Data (Management Interface 2.2, 5.8.2, Figure 118, page 127, PDF page 127)
 */
struct __attribute__((packed)) xnvme_spec_mi_pcie_port_specific_data {
	uint8_t rsvd0[8]; ///< Reserved, bytes 7:0
	uint8_t pciemps; ///< PCIe Maximum Payload Size, byte 8
	uint8_t pcieslsv; ///< PCIe Supported Link Speeds Vector, byte 9, see XNVME_SPEC_MI_PCIE_PORT_SPECIFIC_DATA_PCIESLSV_*
	uint8_t pciecls; ///< PCIe Current Link Speed, byte 10
	uint8_t pciemlw; ///< PCIe Maximum Link Width, byte 11
	uint8_t pcienlw; ///< PCIe Negotiated Link Width, byte 12
	uint8_t pciepn; ///< PCIe Port Number, byte 13
	uint8_t rsvd14[18]; ///< Reserved, bytes 31:14
};

/** 64.0 GT/s Support (Management Interface 2.2, 5.8.2, Figure 118, page 127, PDF page 127); bit 5 of 'pcieslsv' */
#define XNVME_SPEC_MI_PCIE_PORT_SPECIFIC_DATA_PCIESLSV_GTS64 0x20
/** 32.0 GT/s Support (Management Interface 2.2, 5.8.2, Figure 118, page 127, PDF page 127); bit 4 of 'pcieslsv' */
#define XNVME_SPEC_MI_PCIE_PORT_SPECIFIC_DATA_PCIESLSV_GTS32 0x10
/** 16.0 GT/s Support (Management Interface 2.2, 5.8.2, Figure 118, page 127, PDF page 127); bit 3 of 'pcieslsv' */
#define XNVME_SPEC_MI_PCIE_PORT_SPECIFIC_DATA_PCIESLSV_GTS16 0x8
/** 8.0 GT/s Support (Management Interface 2.2, 5.8.2, Figure 118, page 127, PDF page 127); bit 2 of 'pcieslsv' */
#define XNVME_SPEC_MI_PCIE_PORT_SPECIFIC_DATA_PCIESLSV_GTS8 0x4
/** 5.0 GT/s Support (Management Interface 2.2, 5.8.2, Figure 118, page 127, PDF page 127); bit 1 of 'pcieslsv' */
#define XNVME_SPEC_MI_PCIE_PORT_SPECIFIC_DATA_PCIESLSV_GTS5 0x2
/** 2.5 GT/s Support (Management Interface 2.2, 5.8.2, Figure 118, page 127, PDF page 127); bit 0 of 'pcieslsv' */
#define XNVME_SPEC_MI_PCIE_PORT_SPECIFIC_DATA_PCIESLSV_GTS2P5 0x1

/**
 * The values of PCIe Maximum Payload Size (Management Interface 2.2, 5.8.2, Figure 118, page 127, PDF page 127)
 */
enum xnvme_spec_mi_pcie_port_specific_data_pciemps {
	XNVME_SPEC_MI_PCIE_PORT_SPECIFIC_DATA_PCIEMPS_1_KIB = 0x3, ///< 1 KiB (Management Interface 2.2, 5.8.2, Figure 118, page 127, PDF page 127)
	XNVME_SPEC_MI_PCIE_PORT_SPECIFIC_DATA_PCIEMPS_2_KIB = 0x4, ///< 2 KiB (Management Interface 2.2, 5.8.2, Figure 118, page 127, PDF page 127)
	XNVME_SPEC_MI_PCIE_PORT_SPECIFIC_DATA_PCIEMPS_4_KIB = 0x5, ///< 4 KiB (Management Interface 2.2, 5.8.2, Figure 118, page 127, PDF page 127)
};

/**
 * 2-Wire Port Specific Data (Management Interface 2.2, 5.8.2, Figure 119, page 129, PDF page 129)
 */
struct __attribute__((packed)) xnvme_spec_mi_2_wire_port_specific_data {
	uint8_t rsvd0[8]; ///< Reserved, bytes 7:0
	uint8_t cvpdaddr; ///< Current VPD Address, byte 8
	uint8_t mvpdfreq; ///< Maximum VPD Access Frequency, byte 9
	uint8_t cmeaddr; ///< Current Management Endpoint Address, byte 10
	uint8_t twprt; ///< 2-Wire Protocols Supported, byte 11, see XNVME_SPEC_MI_2_WIRE_PORT_SPECIFIC_DATA_TWPRT_*
	uint8_t nvmebm; ///< NVMe Basic Management, byte 12, see XNVME_SPEC_MI_2_WIRE_PORT_SPECIFIC_DATA_NVMEBM_*
	uint32_t pdas; ///< PDA Size, bytes 16:13
	uint8_t sdformat; ///< Supported Data Format, byte 17
	uint8_t rsvd18[14]; ///< Reserved, bytes 31:18
};

/** I3C Support (Management Interface 2.2, 5.8.2, Figure 119, page 129, PDF page 129); bit 7 of 'twprt' */
#define XNVME_SPEC_MI_2_WIRE_PORT_SPECIFIC_DATA_TWPRT_I3CSPRT 0x80
/** Maximum SMBus/I2C Frequency (Management Interface 2.2, 5.8.2, Figure 119, page 129, PDF page 129); bits 1:0 of 'twprt' */
#define XNVME_SPEC_MI_2_WIRE_PORT_SPECIFIC_DATA_TWPRT_MSMBFREQ 0x3
/** NVMe Basic Management Support (Management Interface 2.2, 5.8.2, Figure 119, page 129, PDF page 129); bit 0 of 'nvmebm' */
#define XNVME_SPEC_MI_2_WIRE_PORT_SPECIFIC_DATA_NVMEBM_NVMEBMS 0x1

/**
 * The values of Maximum VPD Access Frequency (Management Interface 2.2, 5.8.2, Figure 119, page 129, PDF page 129)
 */
enum xnvme_spec_mi_2_wire_port_specific_data_mvpdfreq {
	XNVME_SPEC_MI_2_WIRE_PORT_SPECIFIC_DATA_MVPDFREQ_1_MHZ = 0x3, ///< 1 MHz (Management Interface 2.2, 5.8.2, Figure 119, page 129, PDF page 129)
};

/**
 * The values of Maximum SMBus/I2C Frequency (Management Interface 2.2, 5.8.2, Figure 119, page 129, PDF page 129)
 */
enum xnvme_spec_mi_2_wire_port_specific_data_twprt_msmbfreq {
	XNVME_SPEC_MI_2_WIRE_PORT_SPECIFIC_DATA_TWPRT_MSMBFREQ_1_MHZ = 0x3, ///< 1 MHz (Management Interface 2.2, 5.8.2, Figure 119, page 129, PDF page 129)
};

/**
 * The values of Supported Data Format (Management Interface 2.2, 5.8.2, Figure 119, page 129, PDF page 129)
 */
enum xnvme_spec_mi_2_wire_port_specific_data_sdformat {
	XNVME_SPEC_MI_2_WIRE_PORT_SPECIFIC_DATA_SDFORMAT_4_KIB = 0x2, ///< 4 KiB (Management Interface 2.2, 5.8.2, Figure 119, page 129, PDF page 129)
	XNVME_SPEC_MI_2_WIRE_PORT_SPECIFIC_DATA_SDFORMAT_512B = 0x1, ///< 512B (Management Interface 2.2, 5.8.2, Figure 119, page 129, PDF page 129)
	XNVME_SPEC_MI_2_WIRE_PORT_SPECIFIC_DATA_SDFORMAT_BYTE_LEVEL = 0x0, ///< Byte Level (Management Interface 2.2, 5.8.2, Figure 119, page 129, PDF page 129)
};

/**
 * Controller Information (Management Interface 2.2, 5.8.4, Figure 120, page 131, PDF page 131)
 */
struct __attribute__((packed)) xnvme_spec_mi_controller_information {
	uint8_t portid; ///< Port Identifier, byte 0
	uint32_t rsvd1; ///< Reserved, bytes 4:1
	uint8_t prii; ///< PCIe Routing ID Information, byte 5, see XNVME_SPEC_MI_CONTROLLER_INFORMATION_PRII_*
	uint16_t pri; ///< PCIe Routing ID, bytes 7:6, see XNVME_SPEC_MI_CONTROLLER_INFORMATION_PRI_*
	uint16_t pcivid; ///< PCI Vendor ID, bytes 9:8
	uint16_t pcidid; ///< PCI Device ID, bytes 11:10
	uint16_t pcisvid; ///< PCI Subsystem Vendor ID, bytes 13:12
	uint16_t pcisdid; ///< PCI Subsystem Device ID, bytes 15:14
	uint8_t pciesn; ///< PCIe Segment Number, byte 16
	uint8_t rsvd17[15]; ///< Reserved, bytes 31:17
};

/** PCIe Routing ID Valid (Management Interface 2.2, 5.8.4, Figure 120, page 131, PDF page 131); bit 0 of 'prii' */
#define XNVME_SPEC_MI_CONTROLLER_INFORMATION_PRII_PCIERIV 0x1
/** PCI Bus Number (Management Interface 2.2, 5.8.4, Figure 120, page 131, PDF page 131); bits 15:8 of 'pri' */
#define XNVME_SPEC_MI_CONTROLLER_INFORMATION_PRI_PCIBN 0xff00
/** PCI Device Number (Management Interface 2.2, 5.8.4, Figure 120, page 131, PDF page 131); bits 7:3 of 'pri' */
#define XNVME_SPEC_MI_CONTROLLER_INFORMATION_PRI_PCIDN 0xf8
/** PCI Function Number (Management Interface 2.2, 5.8.4, Figure 120, page 131, PDF page 131); bits 2:0 of 'pri' */
#define XNVME_SPEC_MI_CONTROLLER_INFORMATION_PRI_PCIFN 0x7

/**
 * Optionally Supported Command (Management Interface 2.2, 5.8.5, Figure 122, page 132, PDF page 132)
 */
struct __attribute__((packed)) xnvme_spec_mi_optionally_supported_command {
	uint8_t ctyp; ///< Command Type, byte 0, see XNVME_SPEC_MI_OPTIONALLY_SUPPORTED_COMMAND_CTYP_*
	uint8_t opc; ///< Opcode, byte 1
};

/** NVMe-MI Message Type (Management Interface 2.2, 5.8.5, Figure 122, page 132, PDF page 132); bits 6:3 of 'ctyp' */
#define XNVME_SPEC_MI_OPTIONALLY_SUPPORTED_COMMAND_CTYP_NMIMT 0x78

/**
 * Optionally Supported Command List (Management Interface 2.2, 5.8.5, Figure 121, page 132, PDF page 132)
 *
 * Followed, from byte 2, by as many Command as the data says, each a 'struct xnvme_spec_mi_optionally_supported_command'
 */
struct __attribute__((packed)) xnvme_spec_mi_optionally_supported_command_list {
	uint16_t numcmd; ///< Number of Commands, bytes 1:0
};

/**
 * Management Endpoint Buffer Supported Command (Management Interface 2.2, 5.8.6, Figure 124, page 133, PDF page 133)
 */
struct __attribute__((packed)) xnvme_spec_mi_management_endpoint_buffer_supported_command {
	uint8_t ctyp; ///< Command Type, byte 0, see XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_BUFFER_SUPPORTED_COMMAND_CTYP_*
	uint8_t opc; ///< Opcode, byte 1
};

/** NVMe-MI Message Type (Management Interface 2.2, 5.8.6, Figure 124, page 133, PDF page 133); bits 6:3 of 'ctyp' */
#define XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_BUFFER_SUPPORTED_COMMAND_CTYP_NMIMT 0x78

/**
 * Management Endpoint Buffer Supported Command List (Management Interface 2.2, 5.8.6, Figure 123, page 133, PDF page 133)
 *
 * Followed, from byte 2, by as many Command as the data says, each a 'struct xnvme_spec_mi_management_endpoint_buffer_supported_command'
 */
struct __attribute__((packed)) xnvme_spec_mi_management_endpoint_buffer_supported_command_list {
	uint16_t numcmd; ///< Number of Commands, bytes 1:0
};

/**
 * NVMe Admin Command Request Description (Management Interface 2.2, 6, Figure 139, page 142, PDF page 142)
 *
 * Followed, from byte 68, by as many NVMe Request Data as the data says
 *
 * Not members:
 * Message Integrity Check: at an offset the data gives, in 'nvme_request_data'
 */
struct __attribute__((packed)) xnvme_spec_mi_nvme_admin_command_request_description {
	uint32_t nvmemh; ///< NVMe-MI Message Header, bytes 3:0
	uint8_t opc; ///< Opcode, byte 4
	uint8_t cflgs; ///< Command Flags, byte 5, see XNVME_SPEC_MI_NVME_ADMIN_COMMAND_REQUEST_DESCRIPTION_CFLGS_*
	uint16_t ctlid; ///< Controller ID, bytes 7:6
	uint32_t sqedw[5]; ///< Submission Queue Entry Dword, 5 of 4 bytes, bytes 27:8
	uint32_t dofst; ///< Data Offset, bytes 31:28
	uint32_t dlen; ///< Data Length, bytes 35:32
	uint64_t rsvd36; ///< Reserved, bytes 43:36
	uint32_t sqedw10; ///< Submission Queue Entry Dword 10, bytes 47:44
	uint32_t sqedw11; ///< Submission Queue Entry Dword 11, bytes 51:48
	uint32_t sqedw12; ///< Submission Queue Entry Dword 12, bytes 55:52
	uint32_t sqedw13; ///< Submission Queue Entry Dword 13, bytes 59:56
	uint32_t sqedw14; ///< Submission Queue Entry Dword 14, bytes 63:60
	uint32_t sqedw15; ///< Submission Queue Entry Dword 15, bytes 67:64
};

/** Ignore Shutdown (Management Interface 2.2, 6, Figure 139, page 142, PDF page 142); bit 2 of 'cflgs' */
#define XNVME_SPEC_MI_NVME_ADMIN_COMMAND_REQUEST_DESCRIPTION_CFLGS_ISH 0x4
/** DOFST Valid (Management Interface 2.2, 6, Figure 139, page 142, PDF page 142); bit 1 of 'cflgs' */
#define XNVME_SPEC_MI_NVME_ADMIN_COMMAND_REQUEST_DESCRIPTION_CFLGS_DOFSTV 0x2
/** DLEN Valid (Management Interface 2.2, 6, Figure 139, page 142, PDF page 142); bit 0 of 'cflgs' */
#define XNVME_SPEC_MI_NVME_ADMIN_COMMAND_REQUEST_DESCRIPTION_CFLGS_DLENV 0x1

/**
 * NVMe Admin Command Response Description (Management Interface 2.2, 6, Figure 141, page 145, PDF page 145)
 *
 * Followed, from byte 20, by as many NVMe Response Data as the data says
 *
 * Not members:
 * Message Integrity Check: at an offset the data gives, in 'nvme_response_data'
 */
struct __attribute__((packed)) xnvme_spec_mi_nvme_admin_command_response_description {
	uint32_t nmh; ///< NVMe-MI Message Header, bytes 3:0
	uint8_t status; ///< Status, byte 4
	uint8_t rsvd5[3]; ///< Reserved, bytes 7:5
	uint32_t cqedw[2]; ///< Completion Queue Entry Dword, 2 of 4 bytes, bytes 15:8
	uint32_t cqedw3; ///< Completion Queue Entry Dword 3, bytes 19:16
};

/**
 * PCIe Command Request Description (Management Interface 2.2, 7, Figure 150, page 153, PDF page 153)
 *
 * Followed, from byte 8, by as many PCIe Request Dword as the data says
 *
 * Not members:
 * Message Integrity Check: at an offset the data gives, in 'nmd'
 */
struct __attribute__((packed)) xnvme_spec_mi_pcie_command_request_description {
	uint32_t nmh; ///< NVMe-MI Message Header, bytes 3:0
	uint8_t opc; ///< Opcode, byte 4
	uint8_t rsvd5; ///< Reserved, byte 5
	uint16_t ctlid; ///< Controller ID, bytes 7:6
};

/**
 * PCIe Command Response Description (Management Interface 2.2, 7, Figure 153, page 155, PDF page 155)
 *
 * Followed, from byte 8, by as many Response Data as the data says
 *
 * Not members:
 * Message Integrity Check: at an offset the data gives, in 'response_data'
 */
struct __attribute__((packed)) xnvme_spec_mi_pcie_command_response_description {
	uint32_t nmh; ///< NVMe-MI Message Header, bytes 3:0
	uint8_t status; ///< Status, byte 4
	uint8_t rsvd5[3]; ///< Reserved, bytes 7:5
};

/**
 * Common Header (Management Interface 2.2, 8.2.1, Figure 172, page 167, PDF page 167)
 */
struct __attribute__((packed)) xnvme_spec_mi_common_header {
	uint8_t ipmiver; ///< IPMI Format Version Number, byte 0
	uint8_t iuaoff; ///< Internal Use Area Starting Offset, byte 1
	uint8_t ciaoff; ///< Chassis Info Area Starting Offset, byte 2
	uint8_t biaoff; ///< Board Info Area Starting Offset, byte 3
	uint8_t piaoff; ///< Product Info Area Starting Offset, byte 4
	uint8_t mrioff; ///< MultiRecord Info Area Starting Offset, byte 5
	uint8_t bfc; ///< Boot Failure Code, byte 6
	uint8_t chchk; ///< Common Header Checksum, byte 7
};

/**
 * The values of Boot Failure Code (Management Interface 2.2, 8.2.1, Figure 172, page 167, PDF page 167)
 */
enum xnvme_spec_mi_common_header_bfc {
	XNVME_SPEC_MI_COMMON_HEADER_BFC_UNRECOVERABLE_HARDWARE_ISSUE = 0x1, ///< Unrecoverable Hardware Issue (Management Interface 2.2, 8.2.1, Figure 172, page 167, PDF page 167)
	XNVME_SPEC_MI_COMMON_HEADER_BFC_SELF_TEST_FAILURE = 0x2, ///< Self-test Failure (Management Interface 2.2, 8.2.1, Figure 172, page 167, PDF page 167)
	XNVME_SPEC_MI_COMMON_HEADER_BFC_CORRUPTED_CRITICAL_DATA = 0x4, ///< Corrupted Critical Data (Management Interface 2.2, 8.2.1, Figure 172, page 167, PDF page 167)
	XNVME_SPEC_MI_COMMON_HEADER_BFC_CORRUPTED_KEY_MANIFEST = 0x6, ///< Corrupted Key Manifest (Management Interface 2.2, 8.2.1, Figure 172, page 167, PDF page 167)
	XNVME_SPEC_MI_COMMON_HEADER_BFC_CORRUPTED_FIRMWARE_IMAGE = 0x8, ///< Corrupted Firmware Image (Management Interface 2.2, 8.2.1, Figure 172, page 167, PDF page 167)
	XNVME_SPEC_MI_COMMON_HEADER_BFC_CORRUPTED_SECURITY_DATA = 0xa, ///< Corrupted Security Data (Management Interface 2.2, 8.2.1, Figure 172, page 167, PDF page 167)
	XNVME_SPEC_MI_COMMON_HEADER_BFC_CORRUPTED_RECOVERY_FIRMWARE = 0xc, ///< Corrupted Recovery Firmware (Management Interface 2.2, 8.2.1, Figure 172, page 167, PDF page 167)
};

/**
 * NVMe MultiRecord Area (Management Interface 2.2, 8.2.3, Figure 175, page 170, PDF page 170)
 */
struct __attribute__((packed)) xnvme_spec_mi_nvme_multirecord_area {
	uint8_t rti; ///< Record Type Identifier, byte 0
	uint8_t rfmt; ///< Record Format, byte 1, see XNVME_SPEC_MI_NVME_MULTIRECORD_AREA_RFMT_*
	uint8_t rlen; ///< Record Length, byte 2
	uint8_t rcsum; ///< Record Checksum, byte 3
	uint8_t hsum; ///< Header Checksum, byte 4
	uint8_t nmavn; ///< NVMe MultiRecord Area Version Number, byte 5
	uint8_t ff; ///< Form Factor, byte 6
	uint8_t rsvd7[6]; ///< Reserved, bytes 12:7
	uint8_t i1p8psr; ///< Initial 1.8 V Power Supply Requirements, byte 13
	uint8_t m1p8psr; ///< Maximum 1.8 V Power Supply Requirements, byte 14
	uint8_t i3p3psr; ///< Initial 3.3 V Power Supply Requirements, byte 15
	uint8_t m3p3psr; ///< Maximum 3.3 V Power Supply Requirements, byte 16
	uint8_t rsvd17; ///< Reserved, byte 17
	uint8_t m3p3apsr; ///< Maximum 3.3 V aux Power Supply Requirements, byte 18
	uint8_t i5psr; ///< Initial 5 V Power Supply Requirements, byte 19
	uint8_t m5psr; ///< Maximum 5 V Power Supply Requirements, byte 20
	uint8_t i12psr; ///< Initial 12 V Power Supply Requirements, byte 21
	uint8_t m12psr; ///< Maximum 12 V Power Supply Requirements, byte 22
	uint8_t mtl; ///< Maximum Thermal Load, byte 23
	uint8_t tnvmcap[13]; ///< Total NVM Capacity, bytes 36:24
	uint8_t pad[27]; ///< Pad, bytes 63:37
};

/** Last Record (Management Interface 2.2, 8.2.3, Figure 175, page 170, PDF page 170); bit 7 of 'rfmt' */
#define XNVME_SPEC_MI_NVME_MULTIRECORD_AREA_RFMT_LREC 0x80
/** Record Version (Management Interface 2.2, 8.2.3, Figure 175, page 170, PDF page 170); bits 6:0 of 'rfmt' */
#define XNVME_SPEC_MI_NVME_MULTIRECORD_AREA_RFMT_RVER 0x7f

/**
 * NVMe PCIe Port MultiRecord Area (Management Interface 2.2, 8.2.4, Figure 176, page 171, PDF page 171)
 */
struct __attribute__((packed)) xnvme_spec_mi_nvme_pcie_port_multirecord_area {
	uint8_t rti; ///< Record Type Identifier, byte 0
	uint8_t rfmt; ///< Record Format, byte 1, see XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_RFMT_*
	uint8_t rlen; ///< Record Length, byte 2
	uint8_t rcsum; ///< Record Checksum, byte 3
	uint8_t hsum; ///< Header Checksum, byte 4
	uint8_t npciepmavn; ///< NVMe PCIe Port MultiRecord Area Version Number, byte 5
	uint8_t pciepn; ///< PCIe Port Number, byte 6
	uint8_t pinfo; ///< Port Information, byte 7, see XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_PINFO_*
	uint8_t pciels; ///< PCIe Link Speed, byte 8, see XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_PCIELS_*
	uint8_t pciemlw; ///< PCIe Maximum Link Width, byte 9
	uint8_t mctps; ///< MCTP Support, byte 10, see XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_MCTPS_*
	uint8_t rccap; ///< Ref Clk Capability, byte 11, see XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_RCCAP_*
	uint8_t portid; ///< Port Identifier, byte 12
	uint8_t pad[3]; ///< Pad, bytes 15:13
};

/** Last Record (Management Interface 2.2, 8.2.4, Figure 176, page 171, PDF page 171); bit 7 of 'rfmt' */
#define XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_RFMT_LREC 0x80
/** Record Version (Management Interface 2.2, 8.2.4, Figure 176, page 171, PDF page 171); bits 6:0 of 'rfmt' */
#define XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_RFMT_RVER 0x7f
/** Common PCIe Port Capabilities (Management Interface 2.2, 8.2.4, Figure 176, page 171, PDF page 171); bit 0 of 'pinfo' */
#define XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_PINFO_CPPC 0x1
/** 64.0 GT/s Support (Management Interface 2.2, 8.2.4, Figure 176, page 171, PDF page 171); bit 5 of 'pciels' */
#define XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_PCIELS_GTS64 0x20
/** 32.0 GT/s Support (Management Interface 2.2, 8.2.4, Figure 176, page 171, PDF page 171); bit 4 of 'pciels' */
#define XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_PCIELS_GTS32 0x10
/** 16.0 GT/s Support (Management Interface 2.2, 8.2.4, Figure 176, page 171, PDF page 171); bit 3 of 'pciels' */
#define XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_PCIELS_GTS16 0x8
/** 8.0 GT/s Support (Management Interface 2.2, 8.2.4, Figure 176, page 171, PDF page 171); bit 2 of 'pciels' */
#define XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_PCIELS_GTS8 0x4
/** 5.0 GT/s Support (Management Interface 2.2, 8.2.4, Figure 176, page 171, PDF page 171); bit 1 of 'pciels' */
#define XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_PCIELS_GTS5 0x2
/** 2.5 GT/s Support (Management Interface 2.2, 8.2.4, Figure 176, page 171, PDF page 171); bit 0 of 'pciels' */
#define XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_PCIELS_GTS2P5 0x1
/** MCTP PCIe VDM Support (Management Interface 2.2, 8.2.4, Figure 176, page 171, PDF page 171); bit 0 of 'mctps' */
#define XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_MCTPS_MCTPPCIEVS 0x1
/** RefClk Support (Management Interface 2.2, 8.2.4, Figure 176, page 171, PDF page 171); bit 3 of 'rccap' */
#define XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_RCCAP_RCS 0x8
/** Separate RefClk with SSC Support (Management Interface 2.2, 8.2.4, Figure 176, page 171, PDF page 171); bit 2 of 'rccap' */
#define XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_RCCAP_SRCSS 0x4
/** Separate RefClk with No SSC Support (Management Interface 2.2, 8.2.4, Figure 176, page 171, PDF page 171); bit 1 of 'rccap' */
#define XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_RCCAP_SRCNSS 0x2
/** Common RefClk Support (Management Interface 2.2, 8.2.4, Figure 176, page 171, PDF page 171); bit 0 of 'rccap' */
#define XNVME_SPEC_MI_NVME_PCIE_PORT_MULTIRECORD_AREA_RCCAP_CRCS 0x1

/**
 * Topology MultiRecord (Management Interface 2.2, 8.2.5, Figure 177, page 174, PDF page 174)
 *
 * Followed, from byte 8, by as many Element Descriptor as the data says
 *
 * Not members:
 * Element Descriptor 1: at an offset the data gives, in 'element_descriptor'
 * Element Descriptor N: at an offset the data gives, in 'element_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_mi_topology_multirecord {
	uint8_t rti; ///< Record Type Identifier, byte 0
	uint8_t rfmt; ///< Record Format, byte 1, see XNVME_SPEC_MI_TOPOLOGY_MULTIRECORD_RFMT_*
	uint8_t rlen; ///< Record Length, byte 2
	uint8_t rcsum; ///< Record Checksum, byte 3
	uint8_t hsum; ///< Header Checksum, byte 4
	uint8_t vnum; ///< Version Number, byte 5
	uint8_t rsvd6; ///< Reserved, byte 6
	uint8_t elemc; ///< Element Count, byte 7
};

/** Last Record (Management Interface 2.2, 8.2.5, Figure 177, page 174, PDF page 174); bit 7 of 'rfmt' */
#define XNVME_SPEC_MI_TOPOLOGY_MULTIRECORD_RFMT_LREC 0x80
/** Record Version (Management Interface 2.2, 8.2.5, Figure 177, page 174, PDF page 174); bits 6:0 of 'rfmt' */
#define XNVME_SPEC_MI_TOPOLOGY_MULTIRECORD_RFMT_RVER 0x7f

/**
 * Element Descriptor (Management Interface 2.2, 8.2.5, Figure 179, page 175, PDF page 175)
 *
 * Followed, from byte 3, by as many Type-Specific Information as the data says
 */
struct __attribute__((packed)) xnvme_spec_mi_element_descriptor {
	uint8_t typ; ///< Type, byte 0
	uint8_t rev; ///< Revision, byte 1
	uint8_t len; ///< Length, byte 2
};

/**
 * Extended Element Descriptor (Management Interface 2.2, 8.2.5.1, Figure 181, page 176, PDF page 176)
 *
 * Followed, from byte 3, by as many Extended Content as the data says
 */
struct __attribute__((packed)) xnvme_spec_mi_extended_element_descriptor {
	uint8_t typ; ///< Type, byte 0
	uint8_t rev; ///< Revision, byte 1
	uint8_t len; ///< Length, byte 2
};

/**
 * Upstream Connector Element Descriptor (Management Interface 2.2, 8.2.5.2, Figure 182, page 176, PDF page 176)
 *
 * Followed, from byte 11, by as many Upstream Port Descriptor as the data says
 *
 * Not members:
 * Upstream Port Descriptor 1: at an offset the data gives, in 'upstream_port_descriptor'
 * Upstream Port Descriptor N: at an offset the data gives, in 'upstream_port_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_mi_upstream_connector_element_descriptor {
	uint8_t typ; ///< Type, byte 0
	uint8_t rev; ///< Revision, byte 1
	uint8_t len; ///< Length, byte 2
	uint8_t ff; ///< Form Factor, byte 3
	uint8_t lptr; ///< Label Pointer, byte 4
	uint16_t rsvd5; ///< Reserved, bytes 6:5
	uint8_t maxapwr; ///< Maximum Auxiliary Power, byte 7
	uint16_t maxpwr; ///< Maximum Power, bytes 9:8
	uint8_t updc; ///< Upstream Port Descriptor Count, byte 10
};

/**
 * 2-Wire Upstream Port Descriptor (Management Interface 2.2, 8.2.5.2, Figure 184, page 178, PDF page 178)
 *
 * Followed, from byte 3, by as many 2-Wire Pointer as the data says
 */
struct __attribute__((packed)) xnvme_spec_mi_2_wire_upstream_port_descriptor {
	uint8_t typ; ///< Type, byte 0
	uint8_t len; ///< Length, byte 1
	uint8_t cnt; ///< Count, byte 2
};

/**
 * PCIe Upstream Port Descriptor (Management Interface 2.2, 8.2.5.2, Figure 185, page 179, PDF page 179)
 */
struct __attribute__((packed)) xnvme_spec_mi_pcie_upstream_port_descriptor {
	uint8_t typ; ///< Type, byte 0
	uint8_t len; ///< Length, byte 1
	uint8_t sl; ///< Starting Lane, byte 2
	uint8_t el; ///< Ending Lane, byte 3
	uint8_t pcieptr; ///< PCIe Pointer, byte 4
	uint8_t dport; ///< Destination Port, byte 5
};

/**
 * Expansion Connector Element Descriptor (Management Interface 2.2, 8.2.5.3, Figure 186, page 180, PDF page 180)
 *
 * Followed, from byte 6, by as many Expansion Connector Port Descriptor as the data says
 *
 * Not members:
 * Expansion Connector Port Descriptor 1: at an offset the data gives, in 'expansion_connector_port_descriptor'
 * Expansion Connector Port Descriptor N: at an offset the data gives, in 'expansion_connector_port_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_mi_expansion_connector_element_descriptor {
	uint8_t typ; ///< Type, byte 0
	uint8_t rev; ///< Revision, byte 1
	uint8_t len; ///< Length, byte 2
	uint8_t ff; ///< Form Factor, byte 3
	uint8_t lptr; ///< Label Pointer, byte 4
	uint8_t ecpdc; ///< Expansion Connector Port Descriptor Count, byte 5
};

/**
 * Expansion Connector PCIe Port Descriptor (Management Interface 2.2, 8.2.5.3, Figure 187, page 181, PDF page 181)
 */
struct __attribute__((packed)) xnvme_spec_mi_expansion_connector_pcie_port_descriptor {
	uint8_t typ; ///< Type, byte 0
	uint8_t len; ///< Length, byte 1
	uint8_t sl; ///< Starting Lane, byte 2
	uint8_t el; ///< Ending Lane, byte 3
};

/**
 * Label Element Descriptor (Management Interface 2.2, 8.2.5.4, Figure 188, page 181, PDF page 181)
 *
 * Followed, from byte 3, by as many Label String as the data says
 */
struct __attribute__((packed)) xnvme_spec_mi_label_element_descriptor {
	uint8_t typ; ///< Type, byte 0
	uint8_t rev; ///< Revision, byte 1
	uint8_t len; ///< Length, byte 2
};

/**
 * 2-Wire Mux Element Descriptor (Management Interface 2.2, 8.2.5.5, Figure 189, page 181, PDF page 181)
 *
 * Followed, from byte 6, by as many 2-Wire Mux Channel Descriptor as the data says
 *
 * Not members:
 * 2-Wire Mux Channel Descriptor 1: at an offset the data gives, in '2_wire_mux_channel_descriptor'
 * 2-Wire Mux Channel Descriptor N: at an offset the data gives, in '2_wire_mux_channel_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_mi_2_wire_mux_element_descriptor {
	uint8_t typ; ///< Type, byte 0
	uint8_t rev; ///< Revision, byte 1
	uint8_t len; ///< Length, byte 2
	uint8_t twaddri; ///< 2-Wire Address Info, byte 3, see XNVME_SPEC_MI_2_WIRE_MUX_ELEMENT_DESCRIPTOR_TWADDRI_*
	uint8_t twcap; ///< 2-Wire Capabilities, byte 4, see XNVME_SPEC_MI_2_WIRE_MUX_ELEMENT_DESCRIPTOR_TWCAP_*
	uint8_t twmcdc; ///< 2-Wire Mux Channel Descriptor Count, byte 5
};

/** 2-Wire Address (Management Interface 2.2, 8.2.5.5, Figure 189, page 181, PDF page 181); bits 7:1 of 'twaddri' */
#define XNVME_SPEC_MI_2_WIRE_MUX_ELEMENT_DESCRIPTOR_TWADDRI_TWADDR 0xfe
/** ARP Capable (Management Interface 2.2, 8.2.5.5, Figure 189, page 181, PDF page 181); bit 0 of 'twaddri' */
#define XNVME_SPEC_MI_2_WIRE_MUX_ELEMENT_DESCRIPTOR_TWADDRI_ARPC 0x1
/** Form Factor Reset (Management Interface 2.2, 8.2.5.5, Figure 189, page 181, PDF page 181); bit 7 of 'twcap' */
#define XNVME_SPEC_MI_2_WIRE_MUX_ELEMENT_DESCRIPTOR_TWCAP_FFR 0x80
/** Packet Error Code Support (Management Interface 2.2, 8.2.5.5, Figure 189, page 181, PDF page 181); bit 6 of 'twcap' */
#define XNVME_SPEC_MI_2_WIRE_MUX_ELEMENT_DESCRIPTOR_TWCAP_PECS 0x40
/** Maximum Speed (Management Interface 2.2, 8.2.5.5, Figure 189, page 181, PDF page 181); bits 1:0 of 'twcap' */
#define XNVME_SPEC_MI_2_WIRE_MUX_ELEMENT_DESCRIPTOR_TWCAP_MSPD 0x3

/**
 * The values of Maximum Speed (Management Interface 2.2, 8.2.5.5, Figure 189, page 181, PDF page 181)
 */
enum xnvme_spec_mi_2_wire_mux_element_descriptor_twcap_mspd {
	XNVME_SPEC_MI_2_WIRE_MUX_ELEMENT_DESCRIPTOR_TWCAP_MSPD_1_MHZ = 0x2, ///< 1 MHz (Management Interface 2.2, 8.2.5.5, Figure 189, page 181, PDF page 181)
};

/**
 * 2-Wire Mux Channel Descriptor (Management Interface 2.2, 8.2.5.5, Figure 191, page 183, PDF page 183)
 *
 * Followed, from byte 3, by as many 2-Wire Pointer as the data says
 */
struct __attribute__((packed)) xnvme_spec_mi_2_wire_mux_channel_descriptor {
	uint8_t typ; ///< Type, byte 0
	uint8_t len; ///< Length, byte 1
	uint8_t cnt; ///< Count, byte 2
};

/**
 * PCIe Switch Port Descriptor (Management Interface 2.2, 8.2.5.6, Figure 193, page 184, PDF page 184)
 */
struct __attribute__((packed)) xnvme_spec_mi_pcie_switch_port_descriptor {
	uint8_t typ; ///< Type, byte 0
	uint8_t len; ///< Length, byte 1
	uint8_t pciels; ///< PCIe Link Speed, byte 2, see XNVME_SPEC_MI_PCIE_SWITCH_PORT_DESCRIPTOR_PCIELS_*
	uint8_t pciemlw; ///< PCIe Maximum Link Width, byte 3
	uint8_t rccap; ///< RefClk Capability, byte 4, see XNVME_SPEC_MI_PCIE_SWITCH_PORT_DESCRIPTOR_RCCAP_*
	uint8_t pn; ///< Port Number, byte 5
	uint8_t pcieptr; ///< PCIe Pointer, byte 6
	uint8_t dport; ///< Destination Port, byte 7
};

/** 64.0 GT/s Support (Management Interface 2.2, 8.2.5.6, Figure 193, page 184, PDF page 184); bit 5 of 'pciels' */
#define XNVME_SPEC_MI_PCIE_SWITCH_PORT_DESCRIPTOR_PCIELS_GTS64 0x20
/** 32.0 GT/s Support (Management Interface 2.2, 8.2.5.6, Figure 193, page 184, PDF page 184); bit 4 of 'pciels' */
#define XNVME_SPEC_MI_PCIE_SWITCH_PORT_DESCRIPTOR_PCIELS_GTS32 0x10
/** 16.0 GT/s Support (Management Interface 2.2, 8.2.5.6, Figure 193, page 184, PDF page 184); bit 3 of 'pciels' */
#define XNVME_SPEC_MI_PCIE_SWITCH_PORT_DESCRIPTOR_PCIELS_GTS16 0x8
/** 8.0 GT/s Support (Management Interface 2.2, 8.2.5.6, Figure 193, page 184, PDF page 184); bit 2 of 'pciels' */
#define XNVME_SPEC_MI_PCIE_SWITCH_PORT_DESCRIPTOR_PCIELS_GTS8 0x4
/** 5.0 GT/s Support (Management Interface 2.2, 8.2.5.6, Figure 193, page 184, PDF page 184); bit 1 of 'pciels' */
#define XNVME_SPEC_MI_PCIE_SWITCH_PORT_DESCRIPTOR_PCIELS_GTS5 0x2
/** 2.5 GT/s Support (Management Interface 2.2, 8.2.5.6, Figure 193, page 184, PDF page 184); bit 0 of 'pciels' */
#define XNVME_SPEC_MI_PCIE_SWITCH_PORT_DESCRIPTOR_PCIELS_GTS2P5 0x1
/** RefClk Support (Management Interface 2.2, 8.2.5.6, Figure 193, page 184, PDF page 184); bit 3 of 'rccap' */
#define XNVME_SPEC_MI_PCIE_SWITCH_PORT_DESCRIPTOR_RCCAP_RCS 0x8
/** Separate RefClk with SSC Support (Management Interface 2.2, 8.2.5.6, Figure 193, page 184, PDF page 184); bit 2 of 'rccap' */
#define XNVME_SPEC_MI_PCIE_SWITCH_PORT_DESCRIPTOR_RCCAP_SRCSS 0x4
/** Separate RefClk with No SSC Support (Management Interface 2.2, 8.2.5.6, Figure 193, page 184, PDF page 184); bit 1 of 'rccap' */
#define XNVME_SPEC_MI_PCIE_SWITCH_PORT_DESCRIPTOR_RCCAP_SRCNSS 0x2
/** Common RefClk Support (Management Interface 2.2, 8.2.5.6, Figure 193, page 184, PDF page 184); bit 0 of 'rccap' */
#define XNVME_SPEC_MI_PCIE_SWITCH_PORT_DESCRIPTOR_RCCAP_CRCS 0x1

/**
 * PCIe Switch Element Descriptor (Management Interface 2.2, 8.2.5.6, Figure 192, page 184, PDF page 184)
 *
 * Followed, from byte 5, by as many Downstream Switch Port Descriptor as the data says, each a 'struct xnvme_spec_mi_pcie_switch_port_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_mi_pcie_switch_element_descriptor {
	uint8_t typ; ///< Type, byte 0
	uint8_t rev; ///< Revision, byte 1
	uint8_t len; ///< Length, byte 2
	uint8_t uspd; ///< Upstream Switch Port Descriptor, byte 3
	uint8_t dspdc; ///< Downstream Switch Port Descriptor Count, byte 4
};

/**
 * NVM Subsystem Port Descriptor (Management Interface 2.2, 8.2.5.7, Figure 195, page 189, PDF page 189)
 */
struct __attribute__((packed)) xnvme_spec_mi_nvm_subsystem_port_descriptor {
	uint8_t typ; ///< Type, byte 0
	uint8_t len; ///< Length, byte 1
	uint8_t pciels; ///< PCIe Link Speed, byte 2, see XNVME_SPEC_MI_NVM_SUBSYSTEM_PORT_DESCRIPTOR_PCIELS_*
	uint8_t pciemlw; ///< PCIe Maximum Link Width, byte 3
	uint8_t rccap; ///< RefClk Capability, byte 4, see XNVME_SPEC_MI_NVM_SUBSYSTEM_PORT_DESCRIPTOR_RCCAP_*
	uint8_t portid; ///< Port Identifier, byte 5
	uint8_t mctps; ///< MCTP Support, byte 6, see XNVME_SPEC_MI_NVM_SUBSYSTEM_PORT_DESCRIPTOR_MCTPS_*
};

/** 64.0 GT/s Support (Management Interface 2.2, 8.2.5.7, Figure 195, page 189, PDF page 189); bit 5 of 'pciels' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_PORT_DESCRIPTOR_PCIELS_GTS64 0x20
/** 32.0 GT/s Support (Management Interface 2.2, 8.2.5.7, Figure 195, page 189, PDF page 189); bit 4 of 'pciels' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_PORT_DESCRIPTOR_PCIELS_GTS32 0x10
/** 16.0 GT/s Support (Management Interface 2.2, 8.2.5.7, Figure 195, page 189, PDF page 189); bit 3 of 'pciels' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_PORT_DESCRIPTOR_PCIELS_GTS16 0x8
/** 8.0 GT/s Support (Management Interface 2.2, 8.2.5.7, Figure 195, page 189, PDF page 189); bit 2 of 'pciels' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_PORT_DESCRIPTOR_PCIELS_GTS8 0x4
/** 5.0 GT/s Support (Management Interface 2.2, 8.2.5.7, Figure 195, page 189, PDF page 189); bit 1 of 'pciels' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_PORT_DESCRIPTOR_PCIELS_GTS5 0x2
/** 2.5 GT/s Support (Management Interface 2.2, 8.2.5.7, Figure 195, page 189, PDF page 189); bit 0 of 'pciels' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_PORT_DESCRIPTOR_PCIELS_GTS2P5 0x1
/** RefClk Support (Management Interface 2.2, 8.2.5.7, Figure 195, page 189, PDF page 189); bit 3 of 'rccap' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_PORT_DESCRIPTOR_RCCAP_RCS 0x8
/** Separate RefClk with SSC Support (Management Interface 2.2, 8.2.5.7, Figure 195, page 189, PDF page 189); bit 2 of 'rccap' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_PORT_DESCRIPTOR_RCCAP_SRCSS 0x4
/** Separate RefClk with No SSC Support (Management Interface 2.2, 8.2.5.7, Figure 195, page 189, PDF page 189); bit 1 of 'rccap' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_PORT_DESCRIPTOR_RCCAP_SRCNSS 0x2
/** Common RefClk Support (Management Interface 2.2, 8.2.5.7, Figure 195, page 189, PDF page 189); bit 0 of 'rccap' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_PORT_DESCRIPTOR_RCCAP_CRCS 0x1
/** MCTP PCIe VDM Support (Management Interface 2.2, 8.2.5.7, Figure 195, page 189, PDF page 189); bit 0 of 'mctps' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_PORT_DESCRIPTOR_MCTPS_MCTPPCIEVS 0x1

/**
 * NVM Subsystem Element Descriptor (Management Interface 2.2, 8.2.5.7, Figure 194, page 186, PDF page 186)
 *
 * Followed, from byte 6, by as many NVM Subsystem Port Descriptor as the data says, each a 'struct xnvme_spec_mi_nvm_subsystem_port_descriptor'
 *
 * Not members:
 * NVM Subsystem Port Descriptor 1: at an offset the data gives, in 'nvm_subsystem_port_descriptor'
 * NVM Subsystem Port Descriptor N: at an offset the data gives, in 'nvm_subsystem_port_descriptor'
 * Management Endpoint Ready Independent of Media Timeout: at an offset the data gives, in 'nvm_subsystem_port_descriptor'
 * Management Endpoint Ready With Media Timeout: at an offset the data gives, in 'nvm_subsystem_port_descriptor'
 * Maximum Unresponsive Time: at an offset the data gives, in 'nvm_subsystem_port_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_mi_nvm_subsystem_element_descriptor {
	uint8_t typ; ///< Type, byte 0
	uint8_t rev; ///< Revision, byte 1
	uint8_t len; ///< Length, byte 2
	uint8_t saddri; ///< SMBus/I2C Address Info, byte 3, see XNVME_SPEC_MI_NVM_SUBSYSTEM_ELEMENT_DESCRIPTOR_SADDRI_*
	uint8_t twcap; ///< 2-Wire Capabilities, byte 4, see XNVME_SPEC_MI_NVM_SUBSYSTEM_ELEMENT_DESCRIPTOR_TWCAP_*
	uint8_t nvmspdc; ///< NVM Subsystem Port Descriptor Count, byte 5
};

/** SMBus/I2C Address (Management Interface 2.2, 8.2.5.7, Figure 194, page 186, PDF page 186); bits 7:1 of 'saddri' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_ELEMENT_DESCRIPTOR_SADDRI_SADDR 0xfe
/** ARP Capable (Management Interface 2.2, 8.2.5.7, Figure 194, page 186, PDF page 186); bit 0 of 'saddri' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_ELEMENT_DESCRIPTOR_SADDRI_ARPC 0x1
/** Reset (Management Interface 2.2, 8.2.5.7, Figure 194, page 186, PDF page 186); bit 7 of 'twcap' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_ELEMENT_DESCRIPTOR_TWCAP_RST 0x80
/** MCTP over 2-Wire In Aux Power Support (Management Interface 2.2, 8.2.5.7, Figure 194, page 186, PDF page 186); bits 6:5 of 'twcap' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_ELEMENT_DESCRIPTOR_TWCAP_M2WAS 0x60
/** I3C Support (Management Interface 2.2, 8.2.5.7, Figure 194, page 186, PDF page 186); bit 4 of 'twcap' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_ELEMENT_DESCRIPTOR_TWCAP_I3CS 0x10
/** Maximum Speed (Management Interface 2.2, 8.2.5.7, Figure 194, page 186, PDF page 186); bits 1:0 of 'twcap' */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_ELEMENT_DESCRIPTOR_TWCAP_MSPD 0x3

/**
 * The values of Maximum Speed (Management Interface 2.2, 8.2.5.7, Figure 194, page 186, PDF page 186)
 */
enum xnvme_spec_mi_nvm_subsystem_element_descriptor_twcap_mspd {
	XNVME_SPEC_MI_NVM_SUBSYSTEM_ELEMENT_DESCRIPTOR_TWCAP_MSPD_1_MHZ = 0x2, ///< 1 MHz (Management Interface 2.2, 8.2.5.7, Figure 194, page 186, PDF page 186)
};

/**
 * Vendor-Specific Element Descriptors (Management Interface 2.2, 8.2.5.9, Figure 197, page 191, PDF page 191)
 *
 * Followed, from byte 5, by as many Vendor Specific as the data says
 */
struct __attribute__((packed)) xnvme_spec_mi_vendor_specific_element_descriptors {
	uint8_t typ; ///< Type, byte 0
	uint8_t rev; ///< Revision, byte 1
	uint8_t len; ///< Length, byte 2
	uint16_t pcivid; ///< PCI Vendor ID, bytes 4:3
};

/*
 * The fields of Read NVMe-MI Data Structure, by the dwords of the command
 */
/** Data Structure Type (Management Interface 2.2, 5.8, Figure 112, page 124, PDF page 124); bits 31:24 of Command Dword 0 */
#define XNVME_SPEC_MI_READ_NVME_MI_DATA_STRUCTURE_CDW0_DTYP 0xff000000
/** Port Identifier (Management Interface 2.2, 5.8, Figure 112, page 124, PDF page 124); bits 23:16 of Command Dword 0 */
#define XNVME_SPEC_MI_READ_NVME_MI_DATA_STRUCTURE_CDW0_PORTID 0xff0000
/** Controller Identifier (Management Interface 2.2, 5.8, Figure 112, page 124, PDF page 124); bits 15:0 of Command Dword 0 */
#define XNVME_SPEC_MI_READ_NVME_MI_DATA_STRUCTURE_CDW0_CTRLID 0xffff
/** I/O Command Set Identifier (Management Interface 2.2, 5.8, Figure 113, page 125, PDF page 125); bits 7:0 of Command Dword 1 */
#define XNVME_SPEC_MI_READ_NVME_MI_DATA_STRUCTURE_CDW1_IOCSI 0xff

/**
 * The values of Data Structure Type (Management Interface 2.2, 5.8, Figure 112, page 124, PDF page 124)
 */
enum xnvme_spec_mi_read_nvme_mi_data_structure_cdw0_dtyp {
	XNVME_SPEC_MI_READ_NVME_MI_DATA_STRUCTURE_CDW0_DTYP_NVM_SUBSYSTEM_INFORMATION = 0x0, ///< NVM Subsystem Information (Management Interface 2.2, 5.8, Figure 112, page 124, PDF page 124)
	XNVME_SPEC_MI_READ_NVME_MI_DATA_STRUCTURE_CDW0_DTYP_PORT_INFORMATION = 0x1, ///< Port Information (Management Interface 2.2, 5.8, Figure 112, page 124, PDF page 124)
	XNVME_SPEC_MI_READ_NVME_MI_DATA_STRUCTURE_CDW0_DTYP_CONTROLLER_LIST = 0x2, ///< Controller List (Management Interface 2.2, 5.8, Figure 112, page 124, PDF page 124)
	XNVME_SPEC_MI_READ_NVME_MI_DATA_STRUCTURE_CDW0_DTYP_CONTROLLER_INFORMATION = 0x3, ///< Controller Information (Management Interface 2.2, 5.8, Figure 112, page 124, PDF page 124)
	XNVME_SPEC_MI_READ_NVME_MI_DATA_STRUCTURE_CDW0_DTYP_OPTIONALLY_SUPPORTED_COMMAND_LIST = 0x4, ///< Optionally Supported Command List (Management Interface 2.2, 5.8, Figure 112, page 124, PDF page 124)
};

/*
 * The fields of NVM Subsystem Health Status Poll, by the dwords of the command
 */
/** Clear Change Flags (Management Interface 2.2, 5.6, Figure 104, page 117, PDF page 117); bit 31 of Command Dword 1 */
#define XNVME_SPEC_MI_NVM_SUBSYSTEM_HEALTH_STATUS_POLL_CDW1_CCF 0x80000000

/*
 * The fields of Controller Health Status Poll, by the dwords of the command
 */
/** Report All (Management Interface 2.2, 5.3, Figure 94, page 109, PDF page 109); bit 31 of Command Dword 0 */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_STATUS_POLL_CDW0_ALL 0x80000000
/** Include SR-IOV Virtual Functions (Management Interface 2.2, 5.3, Figure 94, page 109, PDF page 109); bit 26 of Command Dword 0 */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_STATUS_POLL_CDW0_INCVF 0x4000000
/** Include SR-IOV Physical Functions (Management Interface 2.2, 5.3, Figure 94, page 109, PDF page 109); bit 25 of Command Dword 0 */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_STATUS_POLL_CDW0_INCPF 0x2000000
/** Include PCI Functions (Management Interface 2.2, 5.3, Figure 94, page 109, PDF page 109); bit 24 of Command Dword 0 */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_STATUS_POLL_CDW0_INCF 0x1000000
/** Maximum Response Entries (Management Interface 2.2, 5.3, Figure 94, page 109, PDF page 109); bits 23:16 of Command Dword 0 */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_STATUS_POLL_CDW0_MAXRENT 0xff0000
/** Starting Controller ID (Management Interface 2.2, 5.3, Figure 94, page 109, PDF page 109); bits 15:0 of Command Dword 0 */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_STATUS_POLL_CDW0_SCTLID 0xffff
/** Clear Change Flags (Management Interface 2.2, 5.3, Figure 95, page 110, PDF page 110); bit 31 of Command Dword 1 */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_STATUS_POLL_CDW1_CCF 0x80000000
/** Critical Warning (Management Interface 2.2, 5.3, Figure 95, page 110, PDF page 110); bit 4 of Command Dword 1 */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_STATUS_POLL_CDW1_CWARN 0x10
/** Available Spare (Management Interface 2.2, 5.3, Figure 95, page 110, PDF page 110); bit 3 of Command Dword 1 */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_STATUS_POLL_CDW1_SPARE 0x8
/** Percentage Drive Life Used (Management Interface 2.2, 5.3, Figure 95, page 110, PDF page 110); bit 2 of Command Dword 1 */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_STATUS_POLL_CDW1_PDLU 0x4
/** Composite Temperature Change (Management Interface 2.2, 5.3, Figure 95, page 110, PDF page 110); bit 1 of Command Dword 1 */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_STATUS_POLL_CDW1_CTEMP 0x2
/** Controller Status Change (Management Interface 2.2, 5.3, Figure 95, page 110, PDF page 110); bit 0 of Command Dword 1 */
#define XNVME_SPEC_MI_CONTROLLER_HEALTH_STATUS_POLL_CDW1_CSC 0x1

/*
 * The fields of Configuration Set, by the dwords of the command
 */
/** Configuration Identifier Specific (Management Interface 2.2, 5.2, Figure 84, page 101, PDF page 101); bits 31:8 of Command Dword 0 */
#define XNVME_SPEC_MI_CONFIGURATION_SET_CDW0_CIS 0xffffff00
/** Configuration Identifier (Management Interface 2.2, 5.2, Figure 84, page 101, PDF page 101); bits 7:0 of Command Dword 0 */
#define XNVME_SPEC_MI_CONFIGURATION_SET_CDW0_CID 0xff
/** Configuration Identifier Specific (Management Interface 2.2, 5.2, Figure 85, page 101, PDF page 101); bits 31:0 of Command Dword 1 */
#define XNVME_SPEC_MI_CONFIGURATION_SET_CDW1_CIS 0xffffffff

/**
 * The values of Configuration Identifier (Management Interface 2.2, 5.1, Figure 75, page 98, PDF page 98)
 */
enum xnvme_spec_mi_configuration_set_cdw0_cid {
	XNVME_SPEC_MI_CONFIGURATION_SET_CDW0_CID_SMBUS_I2C_FREQUENCY = 0x1, ///< SMBus/I2C Frequency (Management Interface 2.2, 5.1, Figure 75, page 98, PDF page 98)
	XNVME_SPEC_MI_CONFIGURATION_SET_CDW0_CID_HEALTH_STATUS_CHANGE = 0x2, ///< Health Status Change (Management Interface 2.2, 5.1, Figure 75, page 98, PDF page 98)
	XNVME_SPEC_MI_CONFIGURATION_SET_CDW0_CID_MCTP_TRANSMISSION_UNIT_SIZE = 0x3, ///< MCTP Transmission Unit Size (Management Interface 2.2, 5.1, Figure 75, page 98, PDF page 98)
	XNVME_SPEC_MI_CONFIGURATION_SET_CDW0_CID_ASYNCHRONOUS_EVENT = 0x4, ///< Asynchronous Event (Management Interface 2.2, 5.1, Figure 75, page 98, PDF page 98)
};

/*
 * The fields of Configuration Get, by the dwords of the command
 */
/** Configuration Identifier Specific (Management Interface 2.2, 5.1, Figure 73, page 98, PDF page 98); bits 31:8 of Command Dword 0 */
#define XNVME_SPEC_MI_CONFIGURATION_GET_CDW0_CIS 0xffffff00
/** Configuration Identifier (Management Interface 2.2, 5.1, Figure 73, page 98, PDF page 98); bits 7:0 of Command Dword 0 */
#define XNVME_SPEC_MI_CONFIGURATION_GET_CDW0_CID 0xff

/**
 * The values of Configuration Identifier (Management Interface 2.2, 5.1, Figure 75, page 98, PDF page 98)
 */
enum xnvme_spec_mi_configuration_get_cdw0_cid {
	XNVME_SPEC_MI_CONFIGURATION_GET_CDW0_CID_SMBUS_I2C_FREQUENCY = 0x1, ///< SMBus/I2C Frequency (Management Interface 2.2, 5.1, Figure 75, page 98, PDF page 98)
	XNVME_SPEC_MI_CONFIGURATION_GET_CDW0_CID_HEALTH_STATUS_CHANGE = 0x2, ///< Health Status Change (Management Interface 2.2, 5.1, Figure 75, page 98, PDF page 98)
	XNVME_SPEC_MI_CONFIGURATION_GET_CDW0_CID_MCTP_TRANSMISSION_UNIT_SIZE = 0x3, ///< MCTP Transmission Unit Size (Management Interface 2.2, 5.1, Figure 75, page 98, PDF page 98)
	XNVME_SPEC_MI_CONFIGURATION_GET_CDW0_CID_ASYNCHRONOUS_EVENT = 0x4, ///< Asynchronous Event (Management Interface 2.2, 5.1, Figure 75, page 98, PDF page 98)
};

/*
 * The fields of VPD Read, by the dwords of the command
 */
/** Data Offset (Management Interface 2.2, 5.13, Figure 131, page 137, PDF page 137); bits 15:0 of Command Dword 0 */
#define XNVME_SPEC_MI_VPD_READ_CDW0_DOFST 0xffff
/** Data Length (Management Interface 2.2, 5.13, Figure 132, page 137, PDF page 137); bits 15:0 of Command Dword 1 */
#define XNVME_SPEC_MI_VPD_READ_CDW1_DLEN 0xffff

/*
 * The fields of VPD Write, by the dwords of the command
 */
/** Data Offset (Management Interface 2.2, 5.14, Figure 134, page 138, PDF page 138); bits 15:0 of Command Dword 0 */
#define XNVME_SPEC_MI_VPD_WRITE_CDW0_DOFST 0xffff
/** Data Length (Management Interface 2.2, 5.14, Figure 135, page 138, PDF page 138); bits 15:0 of Command Dword 1 */
#define XNVME_SPEC_MI_VPD_WRITE_CDW1_DLEN 0xffff

/*
 * The fields of Reset, by the dwords of the command
 */
/** Reset Type (Management Interface 2.2, 5.9, Figure 125, page 133, PDF page 133); bits 31:24 of Command Dword 0 */
#define XNVME_SPEC_MI_RESET_CDW0_RSTTYP 0xff000000

/**
 * The values of Reset Type (Management Interface 2.2, 5.9, Figure 125, page 133, PDF page 133)
 */
enum xnvme_spec_mi_reset_cdw0_rsttyp {
	XNVME_SPEC_MI_RESET_CDW0_RSTTYP_RESET_NVM_SUBSYSTEM = 0x0, ///< Reset NVM Subsystem (Management Interface 2.2, 5.9, Figure 125, page 133, PDF page 133)
};

/*
 * The fields of SES Receive, by the dwords of the command
 */
/** Page Code (Management Interface 2.2, 5.10, Figure 126, page 135, PDF page 135); bits 7:0 of Command Dword 0 */
#define XNVME_SPEC_MI_SES_RECEIVE_CDW0_PCODE 0xff
/** Allocation Length (Management Interface 2.2, 5.10, Figure 127, page 135, PDF page 135); bits 15:0 of Command Dword 1 */
#define XNVME_SPEC_MI_SES_RECEIVE_CDW1_ALENGTH 0xffff

/*
 * The fields of SES Send, by the dwords of the command
 */
/** Data Length (Management Interface 2.2, 5.11, Figure 129, page 136, PDF page 136); bits 15:0 of Command Dword 1 */
#define XNVME_SPEC_MI_SES_SEND_CDW1_DLEN 0xffff

/*
 * The fields of Management Endpoint Buffer Read, by the dwords of the command
 */
/** Data Offset (Management Interface 2.2, 5.4, Figure 99, page 115, PDF page 115); bits 31:0 of Command Dword 0 */
#define XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_BUFFER_READ_CDW0_DOFST 0xffffffff
/** Data Length (Management Interface 2.2, 5.4, Figure 100, page 115, PDF page 115); bits 15:0 of Command Dword 1 */
#define XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_BUFFER_READ_CDW1_DLEN 0xffff

/*
 * The fields of Management Endpoint Buffer Write, by the dwords of the command
 */
/** Data Offset (Management Interface 2.2, 5.5, Figure 102, page 116, PDF page 116); bits 31:0 of Command Dword 0 */
#define XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_BUFFER_WRITE_CDW0_DOFST 0xffffffff
/** Data Length (Management Interface 2.2, 5.5, Figure 103, page 116, PDF page 116); bits 15:0 of Command Dword 1 */
#define XNVME_SPEC_MI_MANAGEMENT_ENDPOINT_BUFFER_WRITE_CDW1_DLEN 0xffff

/*
 * The fields of Shutdown, by the dwords of the command
 */
/** Shutdown Type (Management Interface 2.2, 5.12, Figure 130, page 136, PDF page 136); bits 31:24 of Command Dword 0 */
#define XNVME_SPEC_MI_SHUTDOWN_CDW0_SHDNTYP 0xff000000

/**
 * The values of Shutdown Type (Management Interface 2.2, 5.12, Figure 130, page 136, PDF page 136)
 */
enum xnvme_spec_mi_shutdown_cdw0_shdntyp {
	XNVME_SPEC_MI_SHUTDOWN_CDW0_SHDNTYP_NORMAL_NVM_SUBSYSTEM_SHUTDOWN = 0x0, ///< Normal NVM Subsystem Shutdown (Management Interface 2.2, 5.12, Figure 130, page 136, PDF page 136)
	XNVME_SPEC_MI_SHUTDOWN_CDW0_SHDNTYP_ABRUPT_NVM_SUBSYSTEM_SHUTDOWN = 0x1, ///< Abrupt NVM Subsystem Shutdown (Management Interface 2.2, 5.12, Figure 130, page 136, PDF page 136)
};

/*
 * The fields of NVMe-MI PDA Read, by the dwords of the command
 */
/** Data Offset (Management Interface 2.2, 5.7.1, Figure 108, page 122, PDF page 122); bits 31:0 of Command Dword 0 */
#define XNVME_SPEC_MI_NVME_MI_PDA_READ_CDW0_DOFST 0xffffffff
/** Data Length (Management Interface 2.2, 5.7.1, Figure 109, page 123, PDF page 123); bits 31:0 of Command Dword 1 */
#define XNVME_SPEC_MI_NVME_MI_PDA_READ_CDW1_DLEN 0xffffffff

/**
 * The values of Data Length (Management Interface 2.2, 5.7.1, Figure 109, page 123, PDF page 123)
 */
enum xnvme_spec_mi_nvme_mi_pda_read_cdw1_dlen {
	XNVME_SPEC_MI_NVME_MI_PDA_READ_CDW1_DLEN_4_KIB = 0x1, ///< 4 KiB (Management Interface 2.2, 5.7.1, Figure 109, page 123, PDF page 123)
	XNVME_SPEC_MI_NVME_MI_PDA_READ_CDW1_DLEN_512B = 0x2, ///< 512B (Management Interface 2.2, 5.7.1, Figure 109, page 123, PDF page 123)
	XNVME_SPEC_MI_NVME_MI_PDA_READ_CDW1_DLEN_BYTE_LEVEL = 0x3, ///< Byte Level (Management Interface 2.2, 5.7.1, Figure 109, page 123, PDF page 123)
};

/*
 * The fields of NVMe-MI PDA Write, by the dwords of the command
 */
/** Data Offset (Management Interface 2.2, 5.7.2, Figure 110, page 123, PDF page 123); bits 31:0 of Command Dword 0 */
#define XNVME_SPEC_MI_NVME_MI_PDA_WRITE_CDW0_DOFST 0xffffffff
/** Data Length (Management Interface 2.2, 5.7.2, Figure 111, page 123, PDF page 123); bits 31:0 of Command Dword 1 */
#define XNVME_SPEC_MI_NVME_MI_PDA_WRITE_CDW1_DLEN 0xffffffff

/**
 * The values of Data Length (Management Interface 2.2, 5.7.2, Figure 111, page 123, PDF page 123)
 */
enum xnvme_spec_mi_nvme_mi_pda_write_cdw1_dlen {
	XNVME_SPEC_MI_NVME_MI_PDA_WRITE_CDW1_DLEN_4_KIB = 0x1, ///< 4 KiB (Management Interface 2.2, 5.7.2, Figure 111, page 123, PDF page 123)
	XNVME_SPEC_MI_NVME_MI_PDA_WRITE_CDW1_DLEN_512B = 0x2, ///< 512B (Management Interface 2.2, 5.7.2, Figure 111, page 123, PDF page 123)
	XNVME_SPEC_MI_NVME_MI_PDA_WRITE_CDW1_DLEN_4_096B = 0x3, ///< 4,096B (Management Interface 2.2, 5.7.2, Figure 111, page 123, PDF page 123)
};

/*
 * The fields of NVMe-MI PDA Write Zeroes, by the dwords of the command
 */

/*
 * The fields of PCIe Configuration Read, by the dwords of the command
 */
/** Length (Management Interface 2.2, 7.1, Figure 154, page 155, PDF page 155); bits 15:0 of Command Dword 0 */
#define XNVME_SPEC_PCIE_COMMANDS_PCIE_CONFIGURATION_READ_CDW0_LENGTH 0xffff
/** Offset (Management Interface 2.2, 7.1, Figure 155, page 156, PDF page 156); bits 11:0 of Command Dword 1 */
#define XNVME_SPEC_PCIE_COMMANDS_PCIE_CONFIGURATION_READ_CDW1_OFFSET 0xfff

/*
 * The fields of PCIe Configuration Write, by the dwords of the command
 */
/** Length (Management Interface 2.2, 7.2, Figure 156, page 156, PDF page 156); bits 15:0 of Command Dword 0 */
#define XNVME_SPEC_PCIE_COMMANDS_PCIE_CONFIGURATION_WRITE_CDW0_LENGTH 0xffff
/** Offset (Management Interface 2.2, 7.2, Figure 157, page 156, PDF page 156); bits 11:0 of Command Dword 1 */
#define XNVME_SPEC_PCIE_COMMANDS_PCIE_CONFIGURATION_WRITE_CDW1_OFFSET 0xfff

/*
 * The fields of PCIe Memory Read, by the dwords of the command
 */
/** Base Address Register (Management Interface 2.2, 7.5, Figure 162, page 158, PDF page 158); bits 18:16 of Command Dword 0 */
#define XNVME_SPEC_PCIE_COMMANDS_PCIE_MEMORY_READ_CDW0_BAR 0x70000
/** Length (Management Interface 2.2, 7.5, Figure 162, page 158, PDF page 158); bits 15:0 of Command Dword 0 */
#define XNVME_SPEC_PCIE_COMMANDS_PCIE_MEMORY_READ_CDW0_LENGTH 0xffff
/** Offset Lower (Management Interface 2.2, 7.5, Figure 163, page 159, PDF page 159); bits 31:0 of Command Dword 1 */
#define XNVME_SPEC_PCIE_COMMANDS_PCIE_MEMORY_READ_CDW1_OFFSETL 0xffffffff
/** Offset Upper (Management Interface 2.2, 7.5, Figure 164, page 159, PDF page 159); bits 31:0 of Command Dword 2 */
#define XNVME_SPEC_PCIE_COMMANDS_PCIE_MEMORY_READ_CDW2_OFFSETU 0xffffffff

/*
 * The fields of PCIe Memory Write, by the dwords of the command
 */
/** Base Address Register (Management Interface 2.2, 7.6, Figure 165, page 160, PDF page 160); bits 18:16 of Command Dword 0 */
#define XNVME_SPEC_PCIE_COMMANDS_PCIE_MEMORY_WRITE_CDW0_BAR 0x70000
/** Length (Management Interface 2.2, 7.6, Figure 165, page 160, PDF page 160); bits 15:0 of Command Dword 0 */
#define XNVME_SPEC_PCIE_COMMANDS_PCIE_MEMORY_WRITE_CDW0_LENGTH 0xffff
/** Offset (Management Interface 2.2, 7.6, Figure 166, page 160, PDF page 160); bits 31:0 of Command Dword 1 */
#define XNVME_SPEC_PCIE_COMMANDS_PCIE_MEMORY_WRITE_CDW1_OFFSET 0xffffffff
/** Offset (Management Interface 2.2, 7.6, Figure 167, page 160, PDF page 160); bits 31:0 of Command Dword 2 */
#define XNVME_SPEC_PCIE_COMMANDS_PCIE_MEMORY_WRITE_CDW2_OFFSET 0xffffffff

/*
 * The fields of PCIe I/O Read, by the dwords of the command
 */
/** Base Address Register (Management Interface 2.2, 7.3, Figure 158, page 157, PDF page 157); bits 18:16 of Command Dword 0 */
#define XNVME_SPEC_PCIE_COMMANDS_PCIE_IO_READ_CDW0_BAR 0x70000
/** Length (Management Interface 2.2, 7.3, Figure 158, page 157, PDF page 157); bits 15:0 of Command Dword 0 */
#define XNVME_SPEC_PCIE_COMMANDS_PCIE_IO_READ_CDW0_LENGTH 0xffff
/** Offset (Management Interface 2.2, 7.3, Figure 159, page 157, PDF page 157); bits 31:0 of Command Dword 1 */
#define XNVME_SPEC_PCIE_COMMANDS_PCIE_IO_READ_CDW1_OFFSET 0xffffffff

/*
 * The fields of PCIe I/O Write, by the dwords of the command
 */
/** Base Address Register (Management Interface 2.2, 7.4, Figure 160, page 158, PDF page 158); bits 18:16 of Command Dword 0 */
#define XNVME_SPEC_PCIE_COMMANDS_PCIE_IO_WRITE_CDW0_BAR 0x70000
/** Length (Management Interface 2.2, 7.4, Figure 160, page 158, PDF page 158); bits 15:0 of Command Dword 0 */
#define XNVME_SPEC_PCIE_COMMANDS_PCIE_IO_WRITE_CDW0_LENGTH 0xffff
/** Offset (Management Interface 2.2, 7.4, Figure 161, page 158, PDF page 158); bits 31:0 of Command Dword 1 */
#define XNVME_SPEC_PCIE_COMMANDS_PCIE_IO_WRITE_CDW1_OFFSET 0xffffffff

#endif /* LIBXNVME_SPEC_MI_H */
