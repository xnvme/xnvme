/**
 * What the zns specification defines, by xnvme_spec
 *
 * Written from the data model of senfd 0.19.0, by toolbox/xnvme_spec_headers.py;
 * each name and mask is as the figure it cites gives it. Do not
 * edit: see the contributor docs on producing the spec headers.
 *
 * @file libxnvme_spec_zns.h
 */
#ifndef LIBXNVME_SPEC_ZNS_H
#define LIBXNVME_SPEC_ZNS_H

#include <stdint.h>

/**
 * The opcodes of the Zoned Namespace commands
 */
enum xnvme_spec_zns_opcode {
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_OPCODE = 0x79, ///< Zone Management Send (Zoned Namespace 1.5, 3.2, Figure 12, page 20, PDF page 20)
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_OPCODE = 0x7a, ///< Zone Management Receive (Zoned Namespace 1.5, 3.2, Figure 12, page 20, PDF page 20)
	XNVME_SPEC_ZNS_ZONE_APPEND_OPCODE = 0x7d, ///< Zone Append (Zoned Namespace 1.5, 3.2, Figure 12, page 20, PDF page 20)
};

/**
 * The status codes of the type Command Specific
 */
enum xnvme_spec_zns_command_specific_status {
	XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_INVALID_ZONE_OPERATION_REQUEST = 0xb6, ///< Invalid Zone Operation Request (Zoned Namespace 1.5, 3.1.2, Figure 11, page 20, PDF page 20)
	XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_ZRWA_RESOURCES_UNAVAILABLE = 0xb7, ///< ZRWA Resources Unavailable (Zoned Namespace 1.5, 3.1.2, Figure 11, page 20, PDF page 20)
	XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_ZONE_BOUNDARY_ERROR = 0xb8, ///< Zone Boundary Error (Zoned Namespace 1.5, 3.1.2, Figure 11, page 20, PDF page 20)
	XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_ZONE_IS_FULL = 0xb9, ///< Zone Is Full (Zoned Namespace 1.5, 3.1.2, Figure 11, page 20, PDF page 20)
	XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_ZONE_IS_READ_ONLY = 0xba, ///< Zone Is Read Only (Zoned Namespace 1.5, 3.1.2, Figure 11, page 20, PDF page 20)
	XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_ZONE_IS_OFFLINE = 0xbb, ///< Zone Is Offline (Zoned Namespace 1.5, 3.1.2, Figure 11, page 20, PDF page 20)
	XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_ZONE_INVALID_WRITE = 0xbc, ///< Zone Invalid Write (Zoned Namespace 1.5, 3.1.2, Figure 11, page 20, PDF page 20)
	XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_TOO_MANY_ACTIVE_ZONES = 0xbd, ///< Too Many Active Zones (Zoned Namespace 1.5, 3.1.2, Figure 11, page 20, PDF page 20)
	XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_TOO_MANY_OPEN_ZONES = 0xbe, ///< Too Many Open Zones (Zoned Namespace 1.5, 3.1.2, Figure 11, page 20, PDF page 20)
	XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_INVALID_ZONE_STATE_TRANSITION = 0xbf, ///< Invalid Zone State Transition (Zoned Namespace 1.5, 3.1.2, Figure 11, page 20, PDF page 20)
};

/**
 * Zone Descriptor (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
 */
struct __attribute__((packed)) xnvme_spec_zns_zone_descriptor {
	uint8_t ztattr; ///< Zone Type Attributes, byte 0, see XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZTATTR_*
	uint8_t zsattr; ///< Zone State Attributes, byte 1, see XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZSATTR_*
	uint8_t za; ///< Zone Attributes, byte 2, see XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZA_*
	uint8_t zai; ///< Zone Attributes Information, byte 3, see XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZAI_*
	uint32_t rsvd4; ///< Reserved, bytes 7:4
	uint64_t zcap; ///< Zone Capacity, bytes 15:8
	uint64_t zslba; ///< Zone Start Logical Block Address, bytes 23:16
	uint64_t wp; ///< Write Pointer, bytes 31:24
	uint8_t rsvd32[32]; ///< Reserved, bytes 63:32
};

/** Zone Type (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30); bits 3:0 of 'ztattr' */
#define XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZTATTR_ZT 0xf
/** Zone State (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30); bits 7:4 of 'zsattr' */
#define XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZSATTR_ZS 0xf0
/** Zone Descriptor Extension Valid (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30); bit 7 of 'za' */
#define XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZA_ZDEV 0x80
/** Zone Random Write Area Valid (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30); bit 3 of 'za' */
#define XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZA_ZRWAV 0x8
/** Reset Zone Recommended (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30); bit 2 of 'za' */
#define XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZA_RZR 0x4
/** Finish Zone Recommended (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30); bit 1 of 'za' */
#define XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZA_FZR 0x2
/** Zone Finished by Controller (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30); bit 0 of 'za' */
#define XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZA_ZFC 0x1
/** Reset Zone Recommended Time Limit (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30); bits 3:2 of 'zai' */
#define XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZAI_RZRTL 0xc
/** Finish Zone Recommended Time Limit (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30); bits 1:0 of 'zai' */
#define XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZAI_FZRTL 0x3

/**
 * The values of Zone Type (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
 */
enum xnvme_spec_zns_zone_descriptor_ztattr_zt {
	XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZTATTR_ZT_SEQUENTIAL_WRITE_REQUIRED = 0x2, ///< Sequential Write Required (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
};

/**
 * The values of Zone State (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
 */
enum xnvme_spec_zns_zone_descriptor_zsattr_zs {
	XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZSATTR_ZS_EMPTY = 0x1, ///< Empty (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
	XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZSATTR_ZS_IMPLICITLY_OPENED = 0x2, ///< Implicitly Opened (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
	XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZSATTR_ZS_EXPLICITLY_OPENED = 0x3, ///< Explicitly Opened (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
	XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZSATTR_ZS_CLOSED = 0x4, ///< Closed (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
	XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZSATTR_ZS_READ_ONLY = 0xd, ///< Read Only (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
	XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZSATTR_ZS_FULL = 0xe, ///< Full (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
	XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZSATTR_ZS_OFFLINE = 0xf, ///< Offline (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
};

/**
 * The values of Reset Zone Recommended Time Limit (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
 */
enum xnvme_spec_zns_zone_descriptor_zai_rzrtl {
	XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZAI_RZRTL_RESET_RECOMMENDED_LIMIT = 0x0, ///< Reset Recommended Limit (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
	XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZAI_RZRTL_RESET_RECOMMENDED_LIMIT_1 = 0x1, ///< Reset Recommended Limit 1 (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
	XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZAI_RZRTL_RESET_RECOMMENDED_LIMIT_2 = 0x2, ///< Reset Recommended Limit 2 (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
	XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZAI_RZRTL_RESET_RECOMMENDED_LIMIT_3 = 0x3, ///< Reset Recommended Limit 3 (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
};

/**
 * The values of Finish Zone Recommended Time Limit (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
 */
enum xnvme_spec_zns_zone_descriptor_zai_fzrtl {
	XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZAI_FZRTL_FINISH_RECOMMENDED_LIMIT = 0x0, ///< Finish Recommended Limit (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
	XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZAI_FZRTL_FINISH_RECOMMENDED_LIMIT_1 = 0x1, ///< Finish Recommended Limit 1 (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
	XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZAI_FZRTL_FINISH_RECOMMENDED_LIMIT_2 = 0x2, ///< Finish Recommended Limit 2 (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
	XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZAI_FZRTL_FINISH_RECOMMENDED_LIMIT_3 = 0x3, ///< Finish Recommended Limit 3 (Zoned Namespace 1.5, 3.4.2.2.3, Figure 38, page 30, PDF page 30)
};

/**
 * Report Zones (Zoned Namespace 1.5, 3.4.2.2.1, Figure 36, page 29, PDF page 29)
 *
 * Followed, from byte 64, by as many Zone Descriptor as the data says, each a 'struct xnvme_spec_zns_zone_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_zns_report_zones {
	uint64_t nz; ///< Number of Zones, bytes 7:0
	uint8_t rsvd8[56]; ///< Reserved, bytes 63:8
};

/**
 * Extended Report Zones (Zoned Namespace 1.5, 3.4.2.2.2, Figure 37, page 30, PDF page 30)
 *
 * Followed, from byte 64, by as many Zone Descriptor and Zone Descriptor Extension as the data says
 *
 * Not members:
 * Zone Descriptor 1: at an offset the data gives, in 'zone_descriptor'
 * Zone Descriptor Extension 1: at an offset the data gives, in 'zone_descriptor'
 * Zone Descriptor NZ-1: at an offset the data gives, in 'zone_descriptor'
 * Zone Descriptor Extension NZ-1: at an offset the data gives, in 'zone_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_zns_extended_report_zones {
	uint64_t nz; ///< Number of Zones, bytes 7:0
	uint8_t rsvd8[56]; ///< Reserved, bytes 63:8
};

/**
 * Zone Descriptor Changed - Event Specific Parameter (Zoned Namespace 1.5, 4.1.1.1, Figure 44, page 39, PDF page 39)
 */
struct __attribute__((packed)) xnvme_spec_zns_event_specific_parameter {
	uint32_t nsid; ///< Namespace Identifier, bytes 3:0
};

/**
 * Changed Zone List (Zoned Namespace 1.5, 4.1.4.1, Figure 47, page 41, PDF page 41)
 *
 * Followed, from byte 8, by as many Zone Identifier as the data says
 */
struct __attribute__((packed)) xnvme_spec_zns_log_changed_zone_list {
	uint16_t nzid; ///< Number of Zone Identifiers, bytes 1:0
	uint8_t rsvd2[6]; ///< Reserved, bytes 7:2
};

/**
 * LBA Format Extension (Zoned Namespace 1.5, 4.1.5.1, Figure 50, page 45, PDF page 45), one entry of 16 bytes
 */
struct __attribute__((packed)) xnvme_spec_zns_lba_format_extension {
	uint64_t zsze; ///< Zone Size, bytes 7:0
	uint8_t zdes; ///< Zone Descriptor Extension Size, byte 8
	uint8_t rsvd9[7]; ///< Reserved
};

/**
 * I/O Command Set Specific Identify Namespace (Zoned Namespace 1.5, 4.1.5.1, Figure 49, page 43, PDF page 43)
 */
struct __attribute__((packed)) xnvme_spec_zns_io_command_set_specific_identify_namespace {
	uint16_t zoc; ///< Zone Operation Characteristics, bytes 1:0, see XNVME_SPEC_ZNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_ZOC_*
	uint16_t ozcs; ///< Optional Zoned Command Support, bytes 3:2, see XNVME_SPEC_ZNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_OZCS_*
	uint32_t mar; ///< Maximum Active Resources, bytes 7:4
	uint32_t mor; ///< Maximum Open Resources, bytes 11:8
	uint32_t rrl; ///< Reset Recommended Limit, bytes 15:12
	uint32_t frl; ///< Finish Recommended Limit, bytes 19:16
	uint32_t rrl_20[3]; ///< Reset Recommended Limit, 3 of 4 bytes, bytes 31:20
	uint32_t frl_32[3]; ///< Finish Recommended Limit, 3 of 4 bytes, bytes 43:32
	uint32_t numzrwa; ///< Number of ZRWA Resources, bytes 47:44
	uint16_t zrwafg; ///< ZRWA Flush Granularity, bytes 49:48
	uint16_t zrwasz; ///< ZRWA Size, bytes 51:50
	uint8_t zrwacap; ///< ZRWA Capability, byte 52, see XNVME_SPEC_ZNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_ZRWACAP_*
	uint8_t rsvd53[2763]; ///< Reserved, bytes 2815:53
	struct xnvme_spec_zns_lba_format_extension lbafe[64]; ///< LBA Format Extension, 64 of 16 bytes, bytes 3839:2816
	uint8_t vs[256]; ///< Vendor Specific, bytes 4095:3840
};

/** Zone Active Excursions (Zoned Namespace 1.5, 4.1.5.1, Figure 49, page 43, PDF page 43); bit 1 of 'zoc' */
#define XNVME_SPEC_ZNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_ZOC_ZAE 0x2
/** Variable Zone Capacity (Zoned Namespace 1.5, 4.1.5.1, Figure 49, page 43, PDF page 43); bit 0 of 'zoc' */
#define XNVME_SPEC_ZNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_ZOC_VZC 0x1
/** ZRWA Supported (Zoned Namespace 1.5, 4.1.5.1, Figure 49, page 43, PDF page 43); bit 1 of 'ozcs' */
#define XNVME_SPEC_ZNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_OZCS_ZRWASUP 0x2
/** Read Across Zone Boundaries (Zoned Namespace 1.5, 4.1.5.1, Figure 49, page 43, PDF page 43); bit 0 of 'ozcs' */
#define XNVME_SPEC_ZNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_OZCS_RAZB 0x1
/** Explicit Flush Support (Zoned Namespace 1.5, 4.1.5.1, Figure 49, page 43, PDF page 43); bit 0 of 'zrwacap' */
#define XNVME_SPEC_ZNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_ZRWACAP_EXPFLUSHSUP 0x1

/**
 * I/O Command Set Specific Identify Controller (Zoned Namespace 1.5, 4.1.5.2, Figure 51, page 46, PDF page 46)
 */
struct __attribute__((packed)) xnvme_spec_zns_io_command_set_specific_identify_controller {
	uint8_t zasl; ///< Zone Append Size Limit, byte 0
	uint8_t rsvd1[3]; ///< Reserved, bytes 3:1
	uint32_t zctratt; ///< Zoned Controller Attributes, bytes 7:4, see XNVME_SPEC_ZNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_CONTROLLER_ZCTRATT_*
	uint32_t tar; ///< Total Active Resources, bytes 11:8
	uint32_t uar; ///< Unallocated Active Resources, bytes 15:12
	uint32_t tor; ///< Total Open Resources, bytes 19:16
	uint32_t uor; ///< Unallocated Open Resources, bytes 23:20
	uint32_t tzrwar; ///< Total ZRWA Resources, bytes 27:24
	uint32_t uzrwar; ///< Unallocated ZRWA Resources, bytes 31:28
	uint32_t ver; ///< Version, bytes 35:32
	uint8_t rsvd36[4060]; ///< Reserved, bytes 4095:36
};

/** Zoned Namespace Resource Management (Zoned Namespace 1.5, 4.1.5.2, Figure 51, page 46, PDF page 46); bit 0 of 'zctratt' */
#define XNVME_SPEC_ZNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_CONTROLLER_ZCTRATT_ZNRM 0x1

/**
 * I/O Namespace Management - Host Specific Fields (Zoned Namespace 1.5, 4.1.6, Figure 54, page 49, PDF page 49)
 */
struct __attribute__((packed)) xnvme_spec_zns_host_specific_fields {
	uint8_t rsvd0[499]; ///< Reserved, bytes 498:0
	uint8_t znsco; ///< Zoned Namespace Create Options, byte 499, see XNVME_SPEC_ZNS_HOST_SPECIFIC_FIELDS_ZNSCO_*
	uint32_t rar; ///< Requested Active Resources, bytes 503:500
	uint32_t ror; ///< Requested Open Resources, bytes 507:504
	uint32_t rnumzrwa; ///< Requested Number of ZRWA Resources, bytes 511:508
	uint8_t rsvd512[3584]; ///< Reserved, bytes 4095:512
};

/** Allocate ZRWA Resources (Zoned Namespace 1.5, 4.1.6, Figure 54, page 49, PDF page 49); bit 0 of 'znsco' */
#define XNVME_SPEC_ZNS_HOST_SPECIFIC_FIELDS_ZNSCO_AZR 0x1

/*
 * The fields of Zone Management Send, by the dwords of the command
 */
/** Starting LBA (Zoned Namespace 1.5, 3.4.3, Figure 39, page 32, PDF page 32); bits 63:0 of Command Dwords 10 and 11, as one 64-bit value */
#define XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW10_11_SLBA 0xffffffffffffffff
/** Zone Management (Zoned Namespace 1.5, 3.4.3, Figure 40, page 32, PDF page 32); bits 23:16 of Command Dword 13 */
#define XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZM 0xff0000
/** Data Lifetime (Zoned Namespace 1.5, 3.4.3, Figure 40, page 32, PDF page 32), when ZSA is Open Zone or Set Zone Descriptor Extension; bits 21:16 of Command Dword 13 */
#define XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZM_DL 0x3f0000
/** Zone Send Action Specific Option (Zoned Namespace 1.5, 3.4.3, Figure 40, page 32, PDF page 32); bit 9 of Command Dword 13; ZRWA Allocation (ZRWAA) when ZSA is Open Zone or Set Zone Descriptor Extension */
#define XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZSASO 0x200
/** Select All (Zoned Namespace 1.5, 3.4.3, Figure 40, page 32, PDF page 32); bit 8 of Command Dword 13 */
#define XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_SELALL 0x100
/** Zone Send Action (Zoned Namespace 1.5, 3.4.3, Figure 40, page 32, PDF page 32); bits 7:0 of Command Dword 13 */
#define XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZSA 0xff

/**
 * The values of Zone Send Action (Zoned Namespace 1.5, 3.4.3, Figure 40, page 32, PDF page 32)
 */
enum xnvme_spec_zns_zone_management_send_cdw13_zsa {
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZSA_CLOSE_ZONE = 0x1, ///< Close Zone (Zoned Namespace 1.5, 3.4.3, Figure 40, page 32, PDF page 32)
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZSA_FINISH_ZONE = 0x2, ///< Finish Zone (Zoned Namespace 1.5, 3.4.3, Figure 40, page 32, PDF page 32)
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZSA_OPEN_ZONE = 0x3, ///< Open Zone (Zoned Namespace 1.5, 3.4.3, Figure 40, page 32, PDF page 32)
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZSA_RESET_ZONE = 0x4, ///< Reset Zone (Zoned Namespace 1.5, 3.4.3, Figure 40, page 32, PDF page 32)
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZSA_OFFLINE_ZONE = 0x5, ///< Offline Zone (Zoned Namespace 1.5, 3.4.3, Figure 40, page 32, PDF page 32)
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZSA_SET_ZONE_DESCRIPTOR_EXTENSION = 0x10, ///< Set Zone Descriptor Extension (Zoned Namespace 1.5, 3.4.3, Figure 40, page 32, PDF page 32)
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZSA_FLUSH_EXPLICIT_ZRWA_RANGE = 0x11, ///< Flush Explicit ZRWA Range (Zoned Namespace 1.5, 3.4.3, Figure 40, page 32, PDF page 32)
};

/**
 * The values of Data Lifetime (Zoned Namespace 1.5, 3.4.3, Figure 40, page 32, PDF page 32)
 */
enum xnvme_spec_zns_zone_management_send_cdw13_zm_dl {
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZM_DL_SHORTEST_DATA_LIFETIME = 0x1, ///< Shortest Data Lifetime (Zoned Namespace 1.5, 3.4.3, Figure 40, page 32, PDF page 32)
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZM_DL_LONGEST_DATA_LIFETIME = 0x3f, ///< Longest Data Lifetime (Zoned Namespace 1.5, 3.4.3, Figure 40, page 32, PDF page 32)
};

/*
 * The fields of Zone Management Receive, by the dwords of the command
 */
/** Starting LBA (Zoned Namespace 1.5, 3.4.2, Figure 33, page 28, PDF page 28); bits 63:0 of Command Dwords 10 and 11, as one 64-bit value */
#define XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW10_11_SLBA 0xffffffffffffffff
/** Number of Dwords (Zoned Namespace 1.5, 3.4.2, Figure 34, page 28, PDF page 28); bits 31:0 of Command Dword 12 */
#define XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW12_NDW 0xffffffff
/** Zone Receive Action Specific Features (Zoned Namespace 1.5, 3.4.2, Figure 35, page 28, PDF page 28); bit 16 of Command Dword 13; Partial Report when ZRA is Report Zones or Extended Report Zones */
#define XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRASPF 0x10000
/** Zone Receive Action Specific (Zoned Namespace 1.5, 3.4.2, Figure 35, page 28, PDF page 28); bits 15:8 of Command Dword 13; Reporting Options when ZRA is Report Zones or Extended Report Zones */
#define XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRAS 0xff00
/** Zone Receive Action (Zoned Namespace 1.5, 3.4.2, Figure 35, page 28, PDF page 28); bits 7:0 of Command Dword 13 */
#define XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRA 0xff

/**
 * The values of Reporting Options when ZRA is Report Zones or Extended Report Zones (Zoned Namespace 1.5, 3.4.2, Figure 35, page 28, PDF page 28)
 */
enum xnvme_spec_zns_zone_management_receive_cdw13_zras {
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRAS_LIST_THE_ZONES_IN_THE_ZSE = 0x1, ///< List the zones in the ZSE (Zoned Namespace 1.5, 3.4.2, Figure 35, page 28, PDF page 28)
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRAS_LIST_THE_ZONES_IN_THE_ZSIO = 0x2, ///< List the zones in the ZSIO (Zoned Namespace 1.5, 3.4.2, Figure 35, page 28, PDF page 28)
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRAS_LIST_THE_ZONES_IN_THE_ZSEO = 0x3, ///< List the zones in the ZSEO (Zoned Namespace 1.5, 3.4.2, Figure 35, page 28, PDF page 28)
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRAS_LIST_THE_ZONES_IN_THE_ZSC = 0x4, ///< List the zones in the ZSC (Zoned Namespace 1.5, 3.4.2, Figure 35, page 28, PDF page 28)
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRAS_LIST_THE_ZONES_IN_THE_ZSF = 0x5, ///< List the zones in the ZSF (Zoned Namespace 1.5, 3.4.2, Figure 35, page 28, PDF page 28)
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRAS_LIST_THE_ZONES_IN_THE_ZSRO = 0x6, ///< List the zones in the ZSRO (Zoned Namespace 1.5, 3.4.2, Figure 35, page 28, PDF page 28)
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRAS_LIST_THE_ZONES_IN_THE_ZSO = 0x7, ///< List the zones in the ZSO (Zoned Namespace 1.5, 3.4.2, Figure 35, page 28, PDF page 28)
};

/**
 * The values of Zone Receive Action (Zoned Namespace 1.5, 3.4.2, Figure 35, page 28, PDF page 28)
 */
enum xnvme_spec_zns_zone_management_receive_cdw13_zra {
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRA_REPORT_ZONES = 0x0, ///< Report Zones (Zoned Namespace 1.5, 3.4.2, Figure 35, page 28, PDF page 28)
	XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRA_EXTENDED_REPORT_ZONES = 0x1, ///< Extended Report Zones (Zoned Namespace 1.5, 3.4.2, Figure 35, page 28, PDF page 28)
};

/*
 * The fields of Zone Append, by the dwords of the command
 *
 * Command Dword 13 is described otherwise if CETYPE is non-zero
 */
/** Logical Block Tags Upper (Zoned Namespace 1.5, 3.4.1, Figure 24, page 25, PDF page 25); bits 47:0 of Command Dwords 2 and 3, as one 64-bit value */
#define XNVME_SPEC_ZNS_ZONE_APPEND_CDW2_3_LBTU 0xffffffffffff
/** Metadata Pointer (Zoned Namespace 1.5, 3.4.1, Figure 22, page 25, PDF page 25); bits 63:0 of Command Dwords 4 and 5, as one 64-bit value */
#define XNVME_SPEC_ZNS_ZONE_APPEND_CDW4_5_MPTR 0xffffffffffffffff
/** Zone Start Logical Block Address (Zoned Namespace 1.5, 3.4.1, Figure 25, page 25, PDF page 25); bits 63:0 of Command Dwords 10 and 11, as one 64-bit value */
#define XNVME_SPEC_ZNS_ZONE_APPEND_CDW10_11_ZSLBA 0xffffffffffffffff
/** Limited Retry (Zoned Namespace 1.5, 3.4.1, Figure 26, page 25, PDF page 25); bit 31 of Command Dword 12 */
#define XNVME_SPEC_ZNS_ZONE_APPEND_CDW12_LR 0x80000000
/** Force Unit Access (Zoned Namespace 1.5, 3.4.1, Figure 26, page 25, PDF page 25); bit 30 of Command Dword 12 */
#define XNVME_SPEC_ZNS_ZONE_APPEND_CDW12_FUA 0x40000000
/** Protection Information (Zoned Namespace 1.5, 3.4.1, Figure 26, page 25, PDF page 25); bits 29:26 of Command Dword 12 */
#define XNVME_SPEC_ZNS_ZONE_APPEND_CDW12_PRINFO 0x3c000000
/** Protection Information Remap (Zoned Namespace 1.5, 3.4.1, Figure 26, page 25, PDF page 25); bit 25 of Command Dword 12 */
#define XNVME_SPEC_ZNS_ZONE_APPEND_CDW12_PIREMAP 0x2000000
/** Storage Tag Check (Zoned Namespace 1.5, 3.4.1, Figure 26, page 25, PDF page 25); bit 24 of Command Dword 12 */
#define XNVME_SPEC_ZNS_ZONE_APPEND_CDW12_STC 0x1000000
/** Directive Type (Zoned Namespace 1.5, 3.4.1, Figure 26, page 25, PDF page 25); bits 23:20 of Command Dword 12 */
#define XNVME_SPEC_ZNS_ZONE_APPEND_CDW12_DTYPE 0xf00000
/** Command Extension Type (Zoned Namespace 1.5, 3.4.1, Figure 26, page 25, PDF page 25); bits 19:16 of Command Dword 12 */
#define XNVME_SPEC_ZNS_ZONE_APPEND_CDW12_CETYPE 0xf0000
/** Number of Logical Blocks (Zoned Namespace 1.5, 3.4.1, Figure 26, page 25, PDF page 25); bits 15:0 of Command Dword 12 */
#define XNVME_SPEC_ZNS_ZONE_APPEND_CDW12_NLB 0xffff
/** Directive Specific (Zoned Namespace 1.5, 3.4.1, Figure 27, page 26, PDF page 26); bits 31:16 of Command Dword 13 */
#define XNVME_SPEC_ZNS_ZONE_APPEND_CDW13_DSPEC 0xffff0000
/** Logical Block Tags Lower (Zoned Namespace 1.5, 3.4.1, Figure 29, page 26, PDF page 26); bits 31:0 of Command Dword 14 */
#define XNVME_SPEC_ZNS_ZONE_APPEND_CDW14_LBTL 0xffffffff
/** Logical Block Application Tag Mask (Zoned Namespace 1.5, 3.4.1, Figure 30, page 26, PDF page 26); bits 31:16 of Command Dword 15 */
#define XNVME_SPEC_ZNS_ZONE_APPEND_CDW15_LBATM 0xffff0000
/** Logical Block Application Tag (Zoned Namespace 1.5, 3.4.1, Figure 30, page 26, PDF page 26); bits 15:0 of Command Dword 15 */
#define XNVME_SPEC_ZNS_ZONE_APPEND_CDW15_LBAT 0xffff

#endif /* LIBXNVME_SPEC_ZNS_H */
