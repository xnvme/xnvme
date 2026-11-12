/**
 * What the nvm specification defines, by xnvme_spec
 *
 * Written from the data model of senfd 0.19.0, by toolbox/xnvme_spec_headers.py;
 * each name and mask is as the figure it cites gives it. Do not
 * edit: see the contributor docs on producing the spec headers.
 *
 * @file libxnvme_spec_nvm.h
 */
#ifndef LIBXNVME_SPEC_NVM_H
#define LIBXNVME_SPEC_NVM_H

#include <stdint.h>

/**
 * The opcodes of the NVM commands
 */
enum xnvme_spec_nvm_opcode {
	XNVME_SPEC_NVM_WRITE_OPCODE = 0x1, ///< Write (NVM 1.3, 3.3, Figure 22, page 27, PDF page 27)
	XNVME_SPEC_NVM_READ_OPCODE = 0x2, ///< Read (NVM 1.3, 3.3, Figure 22, page 27, PDF page 27)
	XNVME_SPEC_NVM_WRITE_UNCORRECTABLE_OPCODE = 0x4, ///< Write Uncorrectable (NVM 1.3, 3.3, Figure 22, page 27, PDF page 27)
	XNVME_SPEC_NVM_COMPARE_OPCODE = 0x5, ///< Compare (NVM 1.3, 3.3, Figure 22, page 27, PDF page 27)
	XNVME_SPEC_NVM_WRITE_ZEROES_OPCODE = 0x8, ///< Write Zeroes (NVM 1.3, 3.3, Figure 22, page 27, PDF page 27)
	XNVME_SPEC_NVM_DATASET_MANAGEMENT_OPCODE = 0x9, ///< Dataset Management (NVM 1.3, 3.3, Figure 22, page 27, PDF page 27)
	XNVME_SPEC_NVM_VERIFY_OPCODE = 0xc, ///< Verify (NVM 1.3, 3.3, Figure 22, page 27, PDF page 27)
	XNVME_SPEC_NVM_COPY_OPCODE = 0x19, ///< Copy (NVM 1.3, 3.3, Figure 22, page 27, PDF page 27)
};

/**
 * The identifiers of the log pages
 */
enum xnvme_spec_nvm_log_page_identifier {
	XNVME_SPEC_NVM_LOG_PAGE_FLEXIBLE_DATA_PLACEMENT_FDP_STATISTICS = 0x22, ///< Flexible Data Placement (FDP) Statistics (NVM 1.3, 4.1.4, Figure 109, page 75, PDF page 75)
	XNVME_SPEC_NVM_LOG_PAGE_FLEXIBLE_DATA_PLACEMENT_FDP_EVENTS = 0x23, ///< Flexible Data Placement (FDP) Events (NVM 1.3, 4.1.4, Figure 109, page 75, PDF page 75)
};

/**
 * The status codes of the type Generic Command
 */
enum xnvme_spec_nvm_generic_command_status {
	XNVME_SPEC_NVM_GENERIC_COMMAND_STATUS_ATOMIC_WRITE_UNIT_EXCEEDED = 0x14, ///< Atomic Write Unit Exceeded (NVM 1.3, 3.1.2, Figure 18, page 25, PDF page 25)
	XNVME_SPEC_NVM_GENERIC_COMMAND_STATUS_SGL_DATA_BLOCK_GRANULARITY_INVALID = 0x1e, ///< SGL Data Block Granularity Invalid (NVM 1.3, 3.1.2, Figure 18, page 25, PDF page 25)
	XNVME_SPEC_NVM_GENERIC_COMMAND_STATUS_INVALID_KEY_TAG = 0x25, ///< Invalid Key Tag (NVM 1.3, 3.1.2, Figure 18, page 25, PDF page 25)
	XNVME_SPEC_NVM_GENERIC_COMMAND_STATUS_LBA_OUT_OF_RANGE = 0x80, ///< LBA Out of Range (NVM 1.3, 3.1.2, Figure 18, page 25, PDF page 25)
	XNVME_SPEC_NVM_GENERIC_COMMAND_STATUS_CAPACITY_EXCEEDED = 0x81, ///< Capacity Exceeded (NVM 1.3, 3.1.2, Figure 18, page 25, PDF page 25)
};

/**
 * The status codes of the type Command Specific
 */
enum xnvme_spec_nvm_command_specific_status {
	XNVME_SPEC_NVM_COMMAND_SPECIFIC_STATUS_CONFLICTING_ATTRIBUTES = 0x80, ///< Conflicting Attributes (NVM 1.3, 3.1.2, Figure 19, page 25, PDF page 25)
	XNVME_SPEC_NVM_COMMAND_SPECIFIC_STATUS_INVALID_PROTECTION_INFORMATION = 0x81, ///< Invalid Protection Information (NVM 1.3, 3.1.2, Figure 19, page 25, PDF page 25)
	XNVME_SPEC_NVM_COMMAND_SPECIFIC_STATUS_COMMAND_SIZE_LIMIT_EXCEEDED = 0x83, ///< Command Size Limit Exceeded (NVM 1.3, 3.1.2, Figure 19, page 25, PDF page 25)
	XNVME_SPEC_NVM_COMMAND_SPECIFIC_STATUS_INCOMPATIBLE_NAMESPACE_OR_FORMAT = 0x85, ///< Incompatible Namespace or Format (NVM 1.3, 3.1.2, Figure 19, page 25, PDF page 25)
	XNVME_SPEC_NVM_COMMAND_SPECIFIC_STATUS_FAST_COPY_NOT_POSSIBLE = 0x86, ///< Fast Copy Not Possible (NVM 1.3, 3.1.2, Figure 19, page 25, PDF page 25)
	XNVME_SPEC_NVM_COMMAND_SPECIFIC_STATUS_OVERLAPPING_IO_RANGE = 0x87, ///< Overlapping I/O Range (NVM 1.3, 3.1.2, Figure 19, page 25, PDF page 25)
	XNVME_SPEC_NVM_COMMAND_SPECIFIC_STATUS_INSUFFICIENT_RESOURCES = 0x89, ///< Insufficient Resources (NVM 1.3, 3.1.2, Figure 19, page 25, PDF page 25)
};

/**
 * The status codes of the type Media and Data Integrity Error
 */
enum xnvme_spec_nvm_media_and_data_integrity_error_status {
	XNVME_SPEC_NVM_MEDIA_AND_DATA_INTEGRITY_ERROR_STATUS_COMPARE_FAILURE = 0x85, ///< Compare Failure (NVM 1.3, 3.1.2, Figure 20, page 26, PDF page 26)
	XNVME_SPEC_NVM_MEDIA_AND_DATA_INTEGRITY_ERROR_STATUS_DEALLOCATED_OR_UNWRITTEN_LOGICAL_BLOCK = 0x87, ///< Deallocated or Unwritten Logical Block (NVM 1.3, 3.1.2, Figure 20, page 26, PDF page 26)
};

/**
 * Reclaim Unit Handle Status Descriptor (NVM 1.3, 3.2.1.1, Figure 21, page 26, PDF page 26)
 */
struct __attribute__((packed)) xnvme_spec_nvm_reclaim_unit_handle_status_descriptor {
	uint16_t pid; ///< Placement Identifier, bytes 1:0
	uint16_t ruhid; ///< Reclaim Unit Handle Identifier, bytes 3:2
	uint32_t earutr; ///< Estimated Active Reclaim Unit Time Remaining, bytes 7:4
	uint64_t ruamw; ///< Reclaim Unit Available Media Writes, bytes 15:8
	uint8_t rsvd16[16]; ///< Reserved, bytes 31:16
};

/**
 * Copy - Source Range Entries Copy Descriptor Format 0h and Format 2h (NVM 1.3, 3.3.2, Figure 40, page 33, PDF page 33)
 *
 * One entry of 32 bytes; the figure shows them at bytes 0, 32, 4064
 */
struct __attribute__((packed)) xnvme_spec_nvm_source_range_entries_copy_descriptor_format_0h_and_format_2h {
	uint32_t spars; ///< Source Parameters, bytes 3:0
	uint32_t rsvd4; ///< Reserved, bytes 7:4
	uint64_t slba; ///< Starting LBA, bytes 15:8
	uint32_t rpars; ///< Read Parameters, bytes 19:16, see XNVME_SPEC_NVM_SOURCE_RANGE_ENTRIES_COPY_DESCRIPTOR_FORMAT_0H_AND_FORMAT_2H_RPARS_*
	uint16_t cev; ///< Command Extension Value, bytes 21:20
	uint16_t sopt; ///< Source Options, bytes 23:22
	uint32_t elbt; ///< Expected Logical Block Tags, bytes 27:24
	uint16_t elbat; ///< Expected Logical Block Application Tag, bytes 29:28
	uint16_t elbatm; ///< Expected Logical Block Application Tag Mask, bytes 31:30
};

/** Command Extension Type (NVM 1.3, 3.3.2, Figure 40, page 33, PDF page 33); bits 19:16 of 'rpars' */
#define XNVME_SPEC_NVM_SOURCE_RANGE_ENTRIES_COPY_DESCRIPTOR_FORMAT_0H_AND_FORMAT_2H_RPARS_CETYPE 0xf0000
/** Number of Logical Blocks (NVM 1.3, 3.3.2, Figure 40, page 33, PDF page 33); bits 15:0 of 'rpars' */
#define XNVME_SPEC_NVM_SOURCE_RANGE_ENTRIES_COPY_DESCRIPTOR_FORMAT_0H_AND_FORMAT_2H_RPARS_NLB 0xffff

/**
 * Copy - Source Range Entries Copy Descriptor Format 1h and Format 3h (NVM 1.3, 3.3.2, Figure 41, page 35, PDF page 35)
 *
 * One entry of 40 bytes; the figure shows them at bytes 0, 40, 4040
 */
struct __attribute__((packed)) xnvme_spec_nvm_source_range_entries_copy_descriptor_format_1h_and_format_3h {
	uint32_t spars; ///< Source Parameters, bytes 3:0
	uint32_t rsvd4; ///< Reserved, bytes 7:4
	uint64_t slba; ///< Starting LBA, bytes 15:8
	uint32_t rpars; ///< Read Parameters, bytes 19:16, see XNVME_SPEC_NVM_SOURCE_RANGE_ENTRIES_COPY_DESCRIPTOR_FORMAT_1H_AND_FORMAT_3H_RPARS_*
	uint16_t cev; ///< Command Extension Value, bytes 21:20
	uint16_t sopt; ///< Source Options, bytes 23:22
	uint16_t rsvd24; ///< Reserved, bytes 25:24
	uint8_t elbt[10]; ///< Expected Logical Block Tags, bytes 35:26
	uint16_t elbat; ///< Expected Logical Block Application Tag, bytes 37:36
	uint16_t elbatm; ///< Expected Logical Block Application Tag Mask, bytes 39:38
};

/** Command Extension Type (NVM 1.3, 3.3.2, Figure 41, page 35, PDF page 35); bits 19:16 of 'rpars' */
#define XNVME_SPEC_NVM_SOURCE_RANGE_ENTRIES_COPY_DESCRIPTOR_FORMAT_1H_AND_FORMAT_3H_RPARS_CETYPE 0xf0000
/** Number of Logical Blocks (NVM 1.3, 3.3.2, Figure 41, page 35, PDF page 35); bits 15:0 of 'rpars' */
#define XNVME_SPEC_NVM_SOURCE_RANGE_ENTRIES_COPY_DESCRIPTOR_FORMAT_1H_AND_FORMAT_3H_RPARS_NLB 0xffff

/**
 * LBA Range Type (NVM 1.3, 4.1.3.2, Figure 96, page 65, PDF page 65)
 */
struct __attribute__((packed)) xnvme_spec_nvm_lba_range_type {
	uint8_t type; ///< Type, byte 0
	uint8_t attrb; ///< Attributes, byte 1, see XNVME_SPEC_NVM_LBA_RANGE_TYPE_ATTRB_*
	uint8_t rsvd2[14]; ///< Reserved, bytes 15:2
	uint64_t slba; ///< Starting LBA, bytes 23:16
	uint64_t nlb; ///< Number of Logical Blocks, bytes 31:24
	uint8_t guid[16]; ///< Unique Identifier, bytes 47:32
	uint8_t rsvd48[16]; ///< Reserved, bytes 63:48
};

/** Hide LBA Range (NVM 1.3, 4.1.3.2, Figure 96, page 65, PDF page 65); bit 1 of 'attrb' */
#define XNVME_SPEC_NVM_LBA_RANGE_TYPE_ATTRB_HLBAR 0x2
/** LBA Range Overwriteable (NVM 1.3, 4.1.3.2, Figure 96, page 65, PDF page 65); bit 0 of 'attrb' */
#define XNVME_SPEC_NVM_LBA_RANGE_TYPE_ATTRB_LBARO 0x1

/**
 * The values of Type (NVM 1.3, 4.1.3.2, Figure 96, page 65, PDF page 65)
 */
enum xnvme_spec_nvm_lba_range_type_type {
	XNVME_SPEC_NVM_LBA_RANGE_TYPE_TYPE_GENERAL_PURPOSE = 0x0, ///< General Purpose (NVM 1.3, 4.1.3.2, Figure 96, page 65, PDF page 65)
	XNVME_SPEC_NVM_LBA_RANGE_TYPE_TYPE_FILESYSTEM = 0x1, ///< Filesystem (NVM 1.3, 4.1.3.2, Figure 96, page 65, PDF page 65)
	XNVME_SPEC_NVM_LBA_RANGE_TYPE_TYPE_RAID = 0x2, ///< RAID (NVM 1.3, 4.1.3.2, Figure 96, page 65, PDF page 65)
	XNVME_SPEC_NVM_LBA_RANGE_TYPE_TYPE_CACHE = 0x3, ///< Cache (NVM 1.3, 4.1.3.2, Figure 96, page 65, PDF page 65)
};

/**
 * Host Behavior Support (NVM 1.3, 4.1.3.7, Figure 101, page 68, PDF page 68)
 */
struct __attribute__((packed)) xnvme_spec_nvm_host_behavior_support {
	uint8_t rsvd0[2]; ///< Reserved, bytes 1:0
	uint8_t lbafee; ///< LBA Format Extension Enable, byte 2
};

/**
 * Performance Characteristics - Standard Performance Attribute (NVM 1.3, 4.1.3.8.1, Figure 103, page 71, PDF page 71)
 */
struct __attribute__((packed)) xnvme_spec_nvm_standard_performance_attribute {
	uint32_t rsvd0; ///< Reserved, bytes 3:0
	uint8_t r4karl; ///< Random 4 KiB Average Read Latency, byte 4
	uint8_t rsvd5[4091]; ///< Reserved, bytes 4095:5
};

/**
 * The values of Random 4 KiB Average Read Latency (NVM 1.3, 4.1.3.8.1, Figure 103, page 71, PDF page 71)
 */
enum xnvme_spec_nvm_standard_performance_attribute_r4karl {
	XNVME_SPEC_NVM_STANDARD_PERFORMANCE_ATTRIBUTE_R4KARL_NOT_REPORTED = 0x0, ///< Not Reported (NVM 1.3, 4.1.3.8.1, Figure 103, page 71, PDF page 71)
};

/**
 * Performance Characteristics - Performance Attribute Identifier List (NVM 1.3, 4.1.3.8.2, Figure 104, page 72, PDF page 72)
 */
struct __attribute__((packed)) xnvme_spec_nvm_performance_attribute_identifier_list {
	uint8_t byte_00; ///< Byte 00, byte 0, see XNVME_SPEC_NVM_PERFORMANCE_ATTRIBUTE_IDENTIFIER_LIST_BYTE_00_*
	uint8_t msvspa; ///< Maximum Saveable Vendor Specific Performance Attributes, byte 1
	uint8_t usvspa; ///< Unused Saveable Vendor Specific Performance Attributes, byte 2
	uint8_t rsvd3[13]; ///< Reserved, bytes 15:3
	uint8_t pac1hi[16]; ///< Performance Attribute C1h Identifier, bytes 31:16
	uint8_t pac2hi[16]; ///< Performance Attribute C2h Identifier, bytes 47:32
	uint8_t rsvd48[960]; ///< Reserved, bytes 1007:48
	uint8_t pacffhi[16]; ///< Performance Attribute FFh Identifier, bytes 1023:1008
	uint8_t rsvd1024[3072]; ///< Reserved, bytes 4095:1024
};

/** Attribute Type (NVM 1.3, 4.1.3.8.2, Figure 104, page 72, PDF page 72); bits 2:0 of 'byte_00' */
#define XNVME_SPEC_NVM_PERFORMANCE_ATTRIBUTE_IDENTIFIER_LIST_BYTE_00_ATTRTYP 0x7

/**
 * Performance Characteristics - Vendor Specific Performance Attribute (NVM 1.3, 4.1.3.8.3, Figure 105, page 73, PDF page 73)
 */
struct __attribute__((packed)) xnvme_spec_nvm_vendor_specific_performance_attribute {
	uint8_t paid[16]; ///< Performance Attribute Identifier, bytes 15:0
	uint8_t rsvd16[14]; ///< Reserved, bytes 29:16
	uint16_t attrl; ///< Attribute Length, bytes 31:30
	uint8_t vs[4064]; ///< Vendor Specific, bytes 4095:32
};

/**
 * Error Information Log Entry Data Structure - User Data (NVM 1.3, 4.1.4.1, Figure 110, page 76, PDF page 76)
 */
struct __attribute__((packed)) xnvme_spec_nvm_user_data {
	uint8_t rsvd0[16]; ///< Reserved, bytes 15:0
	uint64_t lba; ///< Logical Block Address, bytes 23:16
};

/**
 * Self-test Results (NVM 1.3, 4.1.4.3, Figure 111, page 76, PDF page 76)
 */
struct __attribute__((packed)) xnvme_spec_nvm_self_test_results {
	uint8_t rsvd0[16]; ///< Reserved, bytes 15:0
	uint64_t flba; ///< Failing LBA, bytes 23:16
};

/**
 * Change Namespace Event Data Format (Event Type 06h) (NVM 1.3, 4.1.4.4, Figure 112, page 77, PDF page 77)
 */
struct __attribute__((packed)) xnvme_spec_nvm_change_namespace_event_data_format {
	uint8_t rsvd0[32]; ///< Reserved, bytes 31:0
	uint8_t flbas; ///< Formatted LBA Size, byte 32
	uint8_t dps; ///< End-to-end Data Protection Type Settings, byte 33
};

/**
 * LBA Status Information (NVM 1.3, 4.1.4.5, Figure 113, page 77, PDF page 77)
 *
 * Followed, from byte 16, by as many LBA Status Log Namespace Element List as the data says
 */
struct __attribute__((packed)) xnvme_spec_nvm_log_lba_status_information {
	uint32_t lslplen; ///< LBA Status Log Page Length, bytes 3:0
	uint32_t nlslne; ///< Number of LBA Status Log Namespace Elements, bytes 7:4
	uint32_t estulb; ///< Estimate of Unrecoverable Logical Blocks, bytes 11:8
	uint16_t rsvd12; ///< Reserved, bytes 13:12
	uint16_t lsgc; ///< LBA Status Generation Counter, bytes 15:14
};

/**
 * LBA Range Descriptor (NVM 1.3, 4.1.4.5, Figure 115, page 78, PDF page 78)
 */
struct __attribute__((packed)) xnvme_spec_nvm_lba_range_descriptor {
	uint64_t rslba; ///< Range Starting LBA, bytes 7:0
	uint32_t rnlb; ///< Range Number of Logical Blocks, bytes 11:8
	uint32_t rsvd12; ///< Reserved, bytes 15:12
};

/**
 * LBA Status Log Namespace Element (NVM 1.3, 4.1.4.5, Figure 114, page 78, PDF page 78)
 *
 * Followed, from byte 16, by as many LBA Range Descriptor as the data says, each a 'struct xnvme_spec_nvm_lba_range_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_nvm_lba_status_log_namespace_element {
	uint32_t neid; ///< Namespace Element Identifier, bytes 3:0
	uint32_t nlrd; ///< Number of LBA Range Descriptors, bytes 7:4
	uint8_t ratype; ///< Recommended Action Type, byte 8
	uint8_t rsvd9[7]; ///< Reserved, bytes 15:9
};

/**
 * Media Reallocated - Event Type Specific (NVM 1.3, 4.1.4.7.1.1, Figure 116, page 79, PDF page 79)
 */
struct __attribute__((packed)) xnvme_spec_nvm_event_type_specific {
	uint8_t sef; ///< Specific Event Flags, byte 0, see XNVME_SPEC_NVM_EVENT_TYPE_SPECIFIC_SEF_*
	uint8_t rsvd1; ///< Reserved, byte 1
	uint16_t nlbam; ///< Number of LBAs Moved, bytes 3:2
	uint64_t lba; ///< Logical Block Address, bytes 11:4
	uint32_t rsvd12; ///< Reserved, bytes 15:12
};

/** LBA Valid (NVM 1.3, 4.1.4.7.1.1, Figure 116, page 79, PDF page 79); bit 0 of 'sef' */
#define XNVME_SPEC_NVM_EVENT_TYPE_SPECIFIC_SEF_LBAV 0x1

/**
 * Rate Limiting (NVM 1.3, 4.1.4.8, Figure 117, page 80, PDF page 80)
 *
 * Followed, from byte 16, by as many Rate Limiting Port as the data says
 *
 * Not members:
 * Rate Limiting Data: at an offset the data gives, in 'rate_limiting_port'
 * Supported Target 0: at an offset the data gives, in 'rate_limiting_port'
 * Supported Target 1: at an offset the data gives, in 'rate_limiting_port'
 * Supported Target NST: at an offset the data gives, in 'rate_limiting_port'
 * Padding: at an offset the data gives, in 'rate_limiting_port'
 */
struct __attribute__((packed)) xnvme_spec_nvm_log_rate_limiting {
	uint16_t rsvd0; ///< Reserved, bytes 1:0
	uint16_t np; ///< Number of Ports, bytes 3:2
	uint32_t lpl; ///< Log Page Length, bytes 7:4
	uint32_t gc; ///< Generation Count, bytes 11:8
	uint16_t nst; ///< Number of Supported Targets, bytes 13:12
	uint16_t rsvd14; ///< Reserved, bytes 15:14
};

/**
 * Rate Limiting Port Descriptor (NVM 1.3, 4.1.4.8, Figure 118, page 80, PDF page 80)
 *
 * Followed, from byte 1024, by as many Rate Limiting Controller as the data says
 */
struct __attribute__((packed)) xnvme_spec_nvm_rate_limiting_port_descriptor {
	uint16_t portid; ///< Port Identifier, bytes 1:0
	uint16_t nc; ///< Number of Controllers, bytes 3:2
	uint8_t rsvd4[3]; ///< Reserved, bytes 6:4
	uint8_t abwsf; ///< Available Bandwidth Scale Factor, byte 7
	uint8_t rlma[32]; ///< Rate Limiting Maximum Access, bytes 39:8
	uint64_t arbwv; ///< Available Read Bandwidth Value, bytes 47:40
	uint64_t awbwv; ///< Available Write Bandwidth Value, bytes 55:48
	uint32_t ariops; ///< Available Read IOPS, bytes 59:56
	uint32_t awiops; ///< Available Write IOPS, bytes 63:60
	uint8_t rsvd64[448]; ///< Reserved, bytes 511:64
	uint8_t vs[512]; ///< Vendor Specific, bytes 1023:512
};

/**
 * The values of Available Bandwidth Scale Factor (NVM 1.3, 4.1.3.9, Figure 108, page 75, PDF page 75)
 */
enum xnvme_spec_nvm_rate_limiting_port_descriptor_abwsf {
	XNVME_SPEC_NVM_RATE_LIMITING_PORT_DESCRIPTOR_ABWSF_1_MIB_SECOND = 0x0, ///< 1 MiB/second (NVM 1.3, 4.1.3.9, Figure 108, page 75, PDF page 75)
	XNVME_SPEC_NVM_RATE_LIMITING_PORT_DESCRIPTOR_ABWSF_10_MIB_SECOND = 0x1, ///< 10 MiB/second (NVM 1.3, 4.1.3.9, Figure 108, page 75, PDF page 75)
	XNVME_SPEC_NVM_RATE_LIMITING_PORT_DESCRIPTOR_ABWSF_100_MIB_SECOND = 0x2, ///< 100 MiB/second (NVM 1.3, 4.1.3.9, Figure 108, page 75, PDF page 75)
	XNVME_SPEC_NVM_RATE_LIMITING_PORT_DESCRIPTOR_ABWSF_1_GIB_SECOND = 0x3, ///< 1 GiB/second (NVM 1.3, 4.1.3.9, Figure 108, page 75, PDF page 75)
	XNVME_SPEC_NVM_RATE_LIMITING_PORT_DESCRIPTOR_ABWSF_10_GIB_SECOND = 0x4, ///< 10 GiB/second (NVM 1.3, 4.1.3.9, Figure 108, page 75, PDF page 75)
	XNVME_SPEC_NVM_RATE_LIMITING_PORT_DESCRIPTOR_ABWSF_100_GIB_SECOND = 0x5, ///< 100 GiB/second (NVM 1.3, 4.1.3.9, Figure 108, page 75, PDF page 75)
};

/**
 * Rate Limiting Controller Descriptor (NVM 1.3, 4.1.4.8, Figure 119, page 81, PDF page 81)
 *
 * Followed, from byte 320, by as many Non-Volatile Storage Medium Access as the data says
 */
struct __attribute__((packed)) xnvme_spec_nvm_rate_limiting_controller_descriptor {
	uint16_t cntlid; ///< Controller Identifier, bytes 1:0
	uint16_t nnsmad; ///< Number of Non-Volatile Storage Medium Access Descriptors, bytes 3:2
	uint32_t rsvd4; ///< Reserved, bytes 7:4
	uint8_t rlma[32]; ///< Rate Limiting Maximum Access, bytes 39:8
	uint8_t rsvd40[24]; ///< Reserved, bytes 63:40
	uint8_t vs[256]; ///< Vendor Specific, bytes 319:64
};

/**
 * Non-Volatile Storage Medium Access Descriptor (NVM 1.3, 4.1.4.8, Figure 120, page 81, PDF page 81)
 *
 * Followed, from byte 64, by as many Non-Volatile Storage Medium Access as the data says
 */
struct __attribute__((packed)) xnvme_spec_nvm_non_volatile_storage_medium_access_descriptor {
	uint8_t sc; ///< Scope, byte 0
	uint8_t rsvd1; ///< Reserved, byte 1
	uint16_t nnsmad; ///< Number of Non-Volatile Storage Medium Access Descriptors, bytes 3:2
	uint32_t si; ///< Scope Identifier, bytes 7:4
	uint8_t rlma[32]; ///< Rate Limiting Maximum Access, bytes 39:8
	uint8_t rsvd40[24]; ///< Reserved, bytes 63:40
};

/**
 * The values of Scope (NVM 1.3, 4.1.4.8, Figure 120, page 81, PDF page 81)
 */
enum xnvme_spec_nvm_non_volatile_storage_medium_access_descriptor_sc {
	XNVME_SPEC_NVM_NON_VOLATILE_STORAGE_MEDIUM_ACCESS_DESCRIPTOR_SC_DOMAIN = 0x1, ///< Domain (NVM 1.3, 4.1.4.8, Figure 120, page 81, PDF page 81)
	XNVME_SPEC_NVM_NON_VOLATILE_STORAGE_MEDIUM_ACCESS_DESCRIPTOR_SC_ENDURANCE_GROUP = 0x2, ///< Endurance Group (NVM 1.3, 4.1.4.8, Figure 120, page 81, PDF page 81)
	XNVME_SPEC_NVM_NON_VOLATILE_STORAGE_MEDIUM_ACCESS_DESCRIPTOR_SC_NAMESPACE = 0x3, ///< Namespace (NVM 1.3, 4.1.4.8, Figure 120, page 81, PDF page 81)
};

/**
 * Rate Limiting Maximum Access Descriptor (NVM 1.3, 4.1.4.8, Figure 121, page 82, PDF page 82)
 */
struct __attribute__((packed)) xnvme_spec_nvm_rate_limiting_maximum_access_descriptor {
	uint8_t mbsf; ///< Maximum Bandwidth Scale Factor, byte 0
	uint8_t rsvd1[7]; ///< Reserved, bytes 7:1
	uint64_t mrbwv; ///< Maximum Read Bandwidth Value, bytes 15:8
	uint64_t mwbwv; ///< Maximum Write Bandwidth Value, bytes 23:16
	uint32_t mriops; ///< Maximum Random Read IOPS, bytes 27:24
	uint32_t mwiops; ///< Maximum Random Write IOPS, bytes 31:28
};

/**
 * The values of Maximum Bandwidth Scale Factor (NVM 1.3, 4.1.3.9, Figure 108, page 75, PDF page 75)
 */
enum xnvme_spec_nvm_rate_limiting_maximum_access_descriptor_mbsf {
	XNVME_SPEC_NVM_RATE_LIMITING_MAXIMUM_ACCESS_DESCRIPTOR_MBSF_1_MIB_SECOND = 0x0, ///< 1 MiB/second (NVM 1.3, 4.1.3.9, Figure 108, page 75, PDF page 75)
	XNVME_SPEC_NVM_RATE_LIMITING_MAXIMUM_ACCESS_DESCRIPTOR_MBSF_10_MIB_SECOND = 0x1, ///< 10 MiB/second (NVM 1.3, 4.1.3.9, Figure 108, page 75, PDF page 75)
	XNVME_SPEC_NVM_RATE_LIMITING_MAXIMUM_ACCESS_DESCRIPTOR_MBSF_100_MIB_SECOND = 0x2, ///< 100 MiB/second (NVM 1.3, 4.1.3.9, Figure 108, page 75, PDF page 75)
	XNVME_SPEC_NVM_RATE_LIMITING_MAXIMUM_ACCESS_DESCRIPTOR_MBSF_1_GIB_SECOND = 0x3, ///< 1 GiB/second (NVM 1.3, 4.1.3.9, Figure 108, page 75, PDF page 75)
	XNVME_SPEC_NVM_RATE_LIMITING_MAXIMUM_ACCESS_DESCRIPTOR_MBSF_10_GIB_SECOND = 0x4, ///< 10 GiB/second (NVM 1.3, 4.1.3.9, Figure 108, page 75, PDF page 75)
	XNVME_SPEC_NVM_RATE_LIMITING_MAXIMUM_ACCESS_DESCRIPTOR_MBSF_100_GIB_SECOND = 0x5, ///< 100 GiB/second (NVM 1.3, 4.1.3.9, Figure 108, page 75, PDF page 75)
};

/**
 * Identify - Identify Namespace (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85)
 */
struct __attribute__((packed)) xnvme_spec_nvm_identify_namespace {
	uint64_t nsze; ///< Namespace Size, bytes 7:0
	uint64_t ncap; ///< Namespace Capacity, bytes 15:8
	uint64_t nuse; ///< Namespace Utilization, bytes 23:16
	uint8_t nsfeat; ///< Namespace Features, byte 24, see XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_NSFEAT_*
	uint8_t nlbaf; ///< Number of LBA Formats, byte 25
	uint8_t flbas; ///< Formatted LBA Size, byte 26, see XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_FLBAS_*
	uint8_t mc; ///< Metadata Capabilities, byte 27, see XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_MC_*
	uint8_t dpc; ///< End-to-end Data Protection Capabilities, byte 28, see XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_DPC_*
	uint8_t dps; ///< End-to-end Data Protection Type Settings, byte 29, see XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_DPS_*
	uint8_t nmic; ///< Namespace Multi-path I/O and Namespace Sharing Capabilities, byte 30
	uint8_t rescap; ///< Reservation Capabilities, byte 31
	uint8_t fpi; ///< Format Progress Indicator, byte 32
	uint8_t dlfeat; ///< Deallocate Logical Block Features, byte 33, see XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_DLFEAT_*
	uint16_t nawun; ///< Namespace Atomic Write Unit Normal, bytes 35:34
	uint16_t nawupf; ///< Namespace Atomic Write Unit Power Fail, bytes 37:36
	uint16_t nacwu; ///< Namespace Atomic Compare & Write Unit, bytes 39:38
	uint16_t nabsn; ///< Namespace Atomic Boundary Size Normal, bytes 41:40
	uint16_t nabo; ///< Namespace Atomic Boundary Offset, bytes 43:42
	uint16_t nabspf; ///< Namespace Atomic Boundary Size Power Fail, bytes 45:44
	uint16_t noiob; ///< Namespace Optimal I/O Boundary, bytes 47:46
	uint8_t nvmcap[16]; ///< NVM Capacity, bytes 63:48
	uint16_t npwg; ///< Namespace Preferred Write Granularity, bytes 65:64
	uint16_t npwa; ///< Namespace Preferred Write Alignment, bytes 67:66
	uint16_t npdg; ///< Namespace Preferred Deallocate Granularity, bytes 69:68
	uint16_t npda; ///< Namespace Preferred Deallocate Alignment, bytes 71:70
	uint16_t nows; ///< Namespace Optimal Write Size, bytes 73:72
	uint16_t mssrl; ///< Maximum Single Source Range Length, bytes 75:74
	uint32_t mcl; ///< Maximum Copy Length, bytes 79:76
	uint8_t msrc; ///< Maximum Source Range Count, byte 80
	uint8_t kpios; ///< Key Per I/O Status, byte 81, see XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_KPIOS_*
	uint8_t nulbaf; ///< Number of Unique Attribute LBA Formats, byte 82
	uint8_t rsvd83; ///< Reserved, byte 83
	uint32_t kpiodaag; ///< Key Per I/O Data Access Alignment and Granularity, bytes 87:84
	uint32_t rsvd88; ///< Reserved, bytes 91:88
	uint32_t anagrpid; ///< ANA Group Identifier, bytes 95:92
	uint8_t rsvd96[3]; ///< Reserved, bytes 98:96
	uint8_t nsattr; ///< Namespace Attributes, byte 99
	uint16_t nvmsetid; ///< NVM Set Identifier, bytes 101:100
	uint16_t endgid; ///< Endurance Group Identifier, bytes 103:102
	uint8_t nguid[16]; ///< Namespace Globally Unique Identifier, bytes 119:104
	uint64_t eui64; ///< IEEE Extended Unique Identifier, bytes 127:120
	uint32_t lbaf[64]; ///< LBA Format Support, 64 of 4 bytes of XNVME_SPEC_NVM_LBA_FORMAT_*, bytes 383:128
	uint8_t vs[3712]; ///< Vendor Specific, bytes 4095:384
};

/** Optional Read Performance (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 7 of 'nsfeat' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_NSFEAT_OPTRPERF 0x80
/** Multiple Atomicity Mode (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 6 of 'nsfeat' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_NSFEAT_MAM 0x40
/** Optional Write Performance (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bits 5:4 of 'nsfeat' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_NSFEAT_OPTPERF 0x30
/** UID Reuse (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 3 of 'nsfeat' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_NSFEAT_UIDREUSE 0x8
/** Deallocated Error (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 2 of 'nsfeat' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_NSFEAT_DAE 0x4
/** Namespace Supported Atomic Boundary & Power (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 1 of 'nsfeat' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_NSFEAT_NSABP 0x2
/** Thin Provisioning (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 0 of 'nsfeat' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_NSFEAT_THINP 0x1
/** Format Index Upper (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bits 6:5 of 'flbas' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_FLBAS_FIDXU 0x60
/** Metadata Transferred as Extended LBA (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 4 of 'flbas' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_FLBAS_MTELBA 0x10
/** Format Index Lower (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bits 3:0 of 'flbas' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_FLBAS_FIDXL 0xf
/** Metadata Transferred as Separate Buffer Support (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 1 of 'mc' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_MC_MTSBS 0x2
/** Metadata Transferred as Extended LBA Support (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 0 of 'mc' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_MC_MTELBAS 0x1
/** Protection Information In Last Bytes (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 4 of 'dpc' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_DPC_PIILB 0x10
/** Protection Information In First Bytes (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 3 of 'dpc' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_DPC_PIIFB 0x8
/** Protection Information Type 3 Supported (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 2 of 'dpc' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_DPC_PIT3S 0x4
/** Protection Information Type 2 Supported (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 1 of 'dpc' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_DPC_PIT2S 0x2
/** Protection Information Type 1 Supported (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 0 of 'dpc' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_DPC_PIT1S 0x1
/** Protection Information Position (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 3 of 'dps' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_DPS_PIP 0x8
/** Protection Information Type (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bits 2:0 of 'dps' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_DPS_PIT 0x7
/** Guard Deallocation Status (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 4 of 'dlfeat' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_DLFEAT_GDS 0x10
/** Write Zeroes Deallocation Support (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 3 of 'dlfeat' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_DLFEAT_WZDS 0x8
/** Deallocation Read Behavior (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bits 2:0 of 'dlfeat' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_DLFEAT_DRB 0x7
/** Key Per I/O Supported in Namespace (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 1 of 'kpios' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_KPIOS_KPIOSNS 0x2
/** Key Per I/O Enabled in Namespace (NVM 1.3, 4.1.5.1, Figure 123, page 85, PDF page 85); bit 0 of 'kpios' */
#define XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_KPIOS_KPIOENS 0x1
/** Relative Performance (NVM 1.3, 4.1.5.1, Figure 125, page 94, PDF page 94); bits 25:24 of 'LBA Format' */
#define XNVME_SPEC_NVM_LBA_FORMAT_RP 0x3000000
/** LBA Data Size (NVM 1.3, 4.1.5.1, Figure 125, page 94, PDF page 94); bits 23:16 of 'LBA Format' */
#define XNVME_SPEC_NVM_LBA_FORMAT_LBADS 0xff0000
/** Metadata Size (NVM 1.3, 4.1.5.1, Figure 125, page 94, PDF page 94); bits 15:0 of 'LBA Format' */
#define XNVME_SPEC_NVM_LBA_FORMAT_MS 0xffff

/**
 * Identify - Identify Controller (NVM 1.3, 4.1.5.2, Figure 126, page 94, PDF page 94)
 */
struct __attribute__((packed)) xnvme_spec_nvm_identify_controller {
	uint8_t rsvd0[526]; ///< Reserved, bytes 525:0
	uint16_t awun; ///< Atomic Write Unit Normal, bytes 527:526
	uint16_t awupf; ///< Atomic Write Unit Power Fail, bytes 529:528
	uint8_t rsvd530[2]; ///< Reserved, bytes 531:530
	uint16_t acwu; ///< Atomic Compare & Write Unit, bytes 533:532
};

/**
 * NVM Command Set I/O Command Set Specific Identify Namespace Data Structure (CSI 00h) (NVM 1.3, 4.1.5.3, Figure 127, page 97, PDF page 97)
 */
struct __attribute__((packed)) xnvme_spec_nvm_nvm_command_set_io_command_set_specific_identify_namespace_data_structure {
	uint64_t lbstm; ///< Logical Block Storage Tag Mask, bytes 7:0
	uint8_t pic; ///< Protection Information Capabilities, byte 8, see XNVME_SPEC_NVM_NVM_COMMAND_SET_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_DATA_STRUCTURE_PIC_*
	uint8_t pifa; ///< Protection Information Format Attribute, byte 9, see XNVME_SPEC_NVM_NVM_COMMAND_SET_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_DATA_STRUCTURE_PIFA_*
	uint16_t rsvd10; ///< Reserved, bytes 11:10
	uint32_t elbaf[64]; ///< Extended LBA Format Support, 64 of 4 bytes of XNVME_SPEC_NVM_EXTENDED_LBA_FORMAT_*, bytes 267:12
	uint32_t npdgl; ///< Namespace Preferred Deallocate Granularity Large, bytes 271:268
	uint32_t nprg; ///< Namespace Preferred Read Granularity, bytes 275:272
	uint32_t npra; ///< Namespace Preferred Read Alignment, bytes 279:276
	uint32_t nors; ///< Namespace Optimal Read Size, bytes 283:280
	uint32_t npdal; ///< Namespace Preferred Deallocate Alignment Large, bytes 287:284
	uint32_t lbapss; ///< LBA Format Placement Shard Size, bytes 291:288
	uint32_t tlbaag; ///< Tracked LBA Allocation Granularity, bytes 295:292
	uint8_t rsvd296[3800]; ///< Reserved, bytes 4095:296
};

/** Qualified Protection Information Format Support (NVM 1.3, 4.1.5.3, Figure 127, page 97, PDF page 97); bit 3 of 'pic' */
#define XNVME_SPEC_NVM_NVM_COMMAND_SET_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_DATA_STRUCTURE_PIC_QPIFS 0x8
/** Storage Tag Check Read Support (NVM 1.3, 4.1.5.3, Figure 127, page 97, PDF page 97); bit 2 of 'pic' */
#define XNVME_SPEC_NVM_NVM_COMMAND_SET_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_DATA_STRUCTURE_PIC_STCRS 0x4
/** 16b Guard Protection Information Storage Tag Mask (NVM 1.3, 4.1.5.3, Figure 127, page 97, PDF page 97); bit 1 of 'pic' */
#define XNVME_SPEC_NVM_NVM_COMMAND_SET_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_DATA_STRUCTURE_PIC_PISTM16B 0x2
/** 16b Guard Protection Information Storage Tag Support (NVM 1.3, 4.1.5.3, Figure 127, page 97, PDF page 97); bit 0 of 'pic' */
#define XNVME_SPEC_NVM_NVM_COMMAND_SET_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_DATA_STRUCTURE_PIC_PISTS16B 0x1
/** Storage Tag Masking Level Attribute (NVM 1.3, 4.1.5.3, Figure 127, page 97, PDF page 97); bits 2:0 of 'pifa' */
#define XNVME_SPEC_NVM_NVM_COMMAND_SET_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_DATA_STRUCTURE_PIFA_STMLA 0x7
/** Qualified Protection Information Format (NVM 1.3, 4.1.5.3, Figure 128, page 101, PDF page 101); bits 12:9 of 'Extended LBA Format' */
#define XNVME_SPEC_NVM_EXTENDED_LBA_FORMAT_QPIF 0x1e00
/** Protection Information Format (NVM 1.3, 4.1.5.3, Figure 128, page 101, PDF page 101); bits 8:7 of 'Extended LBA Format' */
#define XNVME_SPEC_NVM_EXTENDED_LBA_FORMAT_PIF 0x180
/** Storage Tag Size (NVM 1.3, 4.1.5.3, Figure 128, page 101, PDF page 101); bits 6:0 of 'Extended LBA Format' */
#define XNVME_SPEC_NVM_EXTENDED_LBA_FORMAT_STS 0x7f

/**
 * The values of Storage Tag Masking Level Attribute (NVM 1.3, 4.1.5.3, Figure 127, page 97, PDF page 97)
 */
enum xnvme_spec_nvm_nvm_command_set_io_command_set_specific_identify_namespace_data_structure_pifa_stmla {
	XNVME_SPEC_NVM_NVM_COMMAND_SET_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_DATA_STRUCTURE_PIFA_STMLA_BIT_GRANULARITY_MASKING = 0x0, ///< Bit Granularity Masking (NVM 1.3, 4.1.5.3, Figure 127, page 97, PDF page 97)
	XNVME_SPEC_NVM_NVM_COMMAND_SET_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_DATA_STRUCTURE_PIFA_STMLA_BYTE_GRANULARITY_MASKING = 0x1, ///< Byte Granularity Masking (NVM 1.3, 4.1.5.3, Figure 127, page 97, PDF page 97)
	XNVME_SPEC_NVM_NVM_COMMAND_SET_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_DATA_STRUCTURE_PIFA_STMLA_MASKING_NOT_SUPPORTED = 0x2, ///< Masking Not Supported (NVM 1.3, 4.1.5.3, Figure 127, page 97, PDF page 97)
};

/**
 * The values of Qualified Protection Information Format (NVM 1.3, 4.1.5.3, Figure 128, page 101, PDF page 101)
 */
enum xnvme_spec_nvm_extended_lba_format_qpif {
	XNVME_SPEC_NVM_EXTENDED_LBA_FORMAT_QPIF_16B_GUARD_PROTECTION_INFORMATION = 0x0, ///< 16b Guard Protection Information (NVM 1.3, 4.1.5.3, Figure 128, page 101, PDF page 101)
	XNVME_SPEC_NVM_EXTENDED_LBA_FORMAT_QPIF_32B_GUARD_PROTECTION_INFORMATION = 0x1, ///< 32b Guard Protection Information (NVM 1.3, 4.1.5.3, Figure 128, page 101, PDF page 101)
	XNVME_SPEC_NVM_EXTENDED_LBA_FORMAT_QPIF_64B_GUARD_PROTECTION_INFORMATION = 0x2, ///< 64b Guard Protection Information (NVM 1.3, 4.1.5.3, Figure 128, page 101, PDF page 101)
};

/**
 * The values of Protection Information Format (NVM 1.3, 4.1.5.3, Figure 128, page 101, PDF page 101)
 */
enum xnvme_spec_nvm_extended_lba_format_pif {
	XNVME_SPEC_NVM_EXTENDED_LBA_FORMAT_PIF_16B_GUARD_PROTECTION_INFORMATION = 0x0, ///< 16b Guard Protection Information (NVM 1.3, 4.1.5.3, Figure 128, page 101, PDF page 101)
	XNVME_SPEC_NVM_EXTENDED_LBA_FORMAT_PIF_32B_GUARD_PROTECTION_INFORMATION = 0x1, ///< 32b Guard Protection Information (NVM 1.3, 4.1.5.3, Figure 128, page 101, PDF page 101)
	XNVME_SPEC_NVM_EXTENDED_LBA_FORMAT_PIF_64B_GUARD_PROTECTION_INFORMATION = 0x2, ///< 64b Guard Protection Information (NVM 1.3, 4.1.5.3, Figure 128, page 101, PDF page 101)
	XNVME_SPEC_NVM_EXTENDED_LBA_FORMAT_PIF_QUALIFIED_TYPE = 0x3, ///< Qualified Type (NVM 1.3, 4.1.5.3, Figure 128, page 101, PDF page 101)
};

/**
 * The values of Storage Tag Size (NVM 1.3, 4.1.5.3, Figure 128, page 101, PDF page 101)
 */
enum xnvme_spec_nvm_extended_lba_format_sts {
	XNVME_SPEC_NVM_EXTENDED_LBA_FORMAT_STS_16B_GUARD_PROTECTION_INFORMATION = 0x0, ///< 16b Guard Protection Information (NVM 1.3, 4.1.5.3, Figure 128, page 101, PDF page 101)
	XNVME_SPEC_NVM_EXTENDED_LBA_FORMAT_STS_32B_GUARD_PROTECTION_INFORMATION = 0x10, ///< 32b Guard Protection Information (NVM 1.3, 4.1.5.3, Figure 128, page 101, PDF page 101)
	XNVME_SPEC_NVM_EXTENDED_LBA_FORMAT_STS_64B_GUARD_PROTECTION_INFORMATION = 0x0, ///< 64b Guard Protection Information (NVM 1.3, 4.1.5.3, Figure 128, page 101, PDF page 101)
};

/**
 * I/O Command Set Specific Identify Controller (NVM 1.3, 4.1.5.4, Figure 129, page 103, PDF page 103)
 */
struct __attribute__((packed)) xnvme_spec_nvm_io_command_set_specific_identify_controller {
	uint8_t vsl; ///< Verify Size Limit, byte 0
	uint8_t wzsl; ///< Write Zeroes Size Limit, byte 1
	uint8_t wusl; ///< Write Uncorrectable Size Limit, byte 2
	uint8_t dmrl; ///< Dataset Management Ranges Limit, byte 3
	uint32_t dmrsl; ///< Dataset Management Range Size Limit, bytes 7:4
	uint64_t dmsl; ///< Dataset Management Size Limit, bytes 15:8
	uint8_t kpiocap; ///< Key Per I/O Capabilities, byte 16, see XNVME_SPEC_NVM_IO_COMMAND_SET_SPECIFIC_IDENTIFY_CONTROLLER_KPIOCAP_*
	uint8_t wzdsl; ///< Write Zeroes With Deallocate Size Limit, byte 17
	uint16_t aocs; ///< Admin Optional Command Support, bytes 19:18, see XNVME_SPEC_NVM_IO_COMMAND_SET_SPECIFIC_IDENTIFY_CONTROLLER_AOCS_*
	uint32_t ver; ///< Version, bytes 23:20
	uint8_t lbamqf; ///< LBA Migration Queue Format, byte 24
	uint8_t rla; ///< Rate Limiting Attributes, byte 25, see XNVME_SPEC_NVM_IO_COMMAND_SET_SPECIFIC_IDENTIFY_CONTROLLER_RLA_*
	uint16_t slmc; ///< Soft Limit Maximum Controllers, bytes 27:26
	uint8_t rsvd28[4068]; ///< Reserved, bytes 4095:28
};

/** Key Per I/O Scope (NVM 1.3, 4.1.5.4, Figure 129, page 103, PDF page 103); bit 1 of 'kpiocap' */
#define XNVME_SPEC_NVM_IO_COMMAND_SET_SPECIFIC_IDENTIFY_CONTROLLER_KPIOCAP_KPIOSC 0x2
/** Key Per I/O Supported (NVM 1.3, 4.1.5.4, Figure 129, page 103, PDF page 103); bit 0 of 'kpiocap' */
#define XNVME_SPEC_NVM_IO_COMMAND_SET_SPECIFIC_IDENTIFY_CONTROLLER_KPIOCAP_KPIOS 0x1
/** Reporting Allocated LBA Supported (NVM 1.3, 4.1.5.4, Figure 129, page 103, PDF page 103); bit 0 of 'aocs' */
#define XNVME_SPEC_NVM_IO_COMMAND_SET_SPECIFIC_IDENTIFY_CONTROLLER_AOCS_RALBAS 0x1
/** Soft Limit Support (NVM 1.3, 4.1.5.4, Figure 129, page 103, PDF page 103); bit 1 of 'rla' */
#define XNVME_SPEC_NVM_IO_COMMAND_SET_SPECIFIC_IDENTIFY_CONTROLLER_RLA_SLS 0x2
/** Hard Limit Support (NVM 1.3, 4.1.5.4, Figure 129, page 103, PDF page 103); bit 0 of 'rla' */
#define XNVME_SPEC_NVM_IO_COMMAND_SET_SPECIFIC_IDENTIFY_CONTROLLER_RLA_HLS 0x1

/**
 * Namespace Granularity Descriptor (NVM 1.3, 4.1.5.8, Figure 133, page 108, PDF page 108)
 */
struct __attribute__((packed)) xnvme_spec_nvm_namespace_granularity_descriptor {
	uint64_t nsg; ///< Namespace Size Granularity, bytes 7:0
	uint64_t ncg; ///< Namespace Capacity Granularity, bytes 15:8
};

/**
 * Namespace Granularity List (NVM 1.3, 4.1.5.8, Figure 132, page 108, PDF page 108)
 */
struct __attribute__((packed)) xnvme_spec_nvm_namespace_granularity_list {
	uint32_t nga; ///< Namespace Granularity Attributes, bytes 3:0, see XNVME_SPEC_NVM_NAMESPACE_GRANULARITY_LIST_NGA_*
	uint8_t nd; ///< Number of Descriptors, byte 4
	uint8_t rsvd5[27]; ///< Reserved, bytes 31:5
	struct xnvme_spec_nvm_namespace_granularity_descriptor ngd[64]; ///< NG Descriptor, 64 of 16 bytes, bytes 1055:32
};

/** Granularity Descriptor Mapping (NVM 1.3, 4.1.5.8, Figure 132, page 108, PDF page 108); bit 0 of 'nga' */
#define XNVME_SPEC_NVM_NAMESPACE_GRANULARITY_LIST_NGA_GDM 0x1

/**
 * Namespace Management - Host Specified Fields (NVM 1.3, 4.1.6.4, Figure 134, page 112, PDF page 112)
 */
struct __attribute__((packed)) xnvme_spec_nvm_host_specified_fields {
	uint64_t nsze; ///< Namespace Size, bytes 7:0
	uint64_t ncap; ///< Namespace Capacity, bytes 15:8
	uint8_t rsvd16[10]; ///< Reserved, bytes 25:16
	uint8_t flbas; ///< Formatted LBA Size, byte 26
	uint16_t rsvd27; ///< Reserved, bytes 28:27
	uint8_t dps; ///< End-to-end Data Protection Type Settings, byte 29
	uint8_t nmic; ///< Namespace Multi-path I/O and Namespace Sharing Capabilities, byte 30
	uint8_t rsvd31[61]; ///< Reserved, bytes 91:31
	uint32_t anagrpid; ///< ANA Group Identifier, bytes 95:92
	uint32_t rsvd96; ///< Reserved, bytes 99:96
	uint16_t nvmsetid; ///< NVM Set Identifier, bytes 101:100
	uint16_t endgid; ///< Endurance Group Identifier, bytes 103:102
	uint8_t rsvd104[280]; ///< Reserved, bytes 383:104
	uint64_t lbstm; ///< Logical Block Storage Tag Mask, bytes 391:384
	uint16_t nphndls; ///< Number of Placement Handles 2, bytes 393:392
	uint8_t rsvd394[105]; ///< Reserved, bytes 498:394
	uint8_t rsvd499[13]; ///< Reserved, bytes 511:499
	uint16_t placement_handle_associated_ruh2[128]; ///< Placement Handle Associated RUH2, 128 of 2 bytes, bytes 767:512
	uint8_t rsvd768[3329]; ///< Reserved, bytes 4096:768
};

/**
 * LBA Status Descriptor Entry (NVM 1.3, 4.2.1.2, Figure 140, page 117, PDF page 117)
 */
struct __attribute__((packed)) xnvme_spec_nvm_lba_status_descriptor_entry {
	uint64_t dslba; ///< Descriptor Starting LBA, bytes 7:0
	uint32_t nlb; ///< Number of Logical Blocks, bytes 11:8
	uint8_t rsvd12; ///< Reserved, byte 12
	uint8_t stas; ///< Status, byte 13, see XNVME_SPEC_NVM_LBA_STATUS_DESCRIPTOR_ENTRY_STAS_*
	uint16_t rsvd14; ///< Reserved, bytes 15:14
};

/** LBA Range Status (NVM 1.3, 4.2.1.2, Figure 140, page 117, PDF page 117); bits 2:0 of 'stas' */
#define XNVME_SPEC_NVM_LBA_STATUS_DESCRIPTOR_ENTRY_STAS_LBARS 0x7

/**
 * The values of LBA Range Status (NVM 1.3, 4.2.1.2, Figure 140, page 117, PDF page 117)
 */
enum xnvme_spec_nvm_lba_status_descriptor_entry_stas_lbars {
	XNVME_SPEC_NVM_LBA_STATUS_DESCRIPTOR_ENTRY_STAS_LBARS_EACH_LOGICAL_BLOCK_MAY = 0x0, ///< Each logical block may (NVM 1.3, 4.2.1.2, Figure 140, page 117, PDF page 117)
	XNVME_SPEC_NVM_LBA_STATUS_DESCRIPTOR_ENTRY_STAS_LBARS_EACH_LOGICAL_BLOCK_MAY_1H = 0x1, ///< Each logical block may (NVM 1.3, 4.2.1.2, Figure 140, page 117, PDF page 117)
	XNVME_SPEC_NVM_LBA_STATUS_DESCRIPTOR_ENTRY_STAS_LBARS_EACH_LOGICAL_BLOCK_IS_A = 0x3, ///< Each logical block is a (NVM 1.3, 4.2.1.2, Figure 140, page 117, PDF page 117)
};

/**
 * LBA Status Descriptor List (NVM 1.3, 4.2.1.2, Figure 139, page 116, PDF page 116)
 *
 * Followed, from byte 8, by as many LBA Status Descriptor Entry as the data says, each a 'struct xnvme_spec_nvm_lba_status_descriptor_entry'
 */
struct __attribute__((packed)) xnvme_spec_nvm_lba_status_descriptor_list {
	uint32_t nlsd; ///< Number of LBA Status Descriptors, bytes 3:0
	uint8_t cmpc; ///< Completion Condition, byte 4
	uint8_t rsvd5[3]; ///< Reserved, bytes 7:5
};

/**
 * The values of Completion Condition (NVM 1.3, 4.2.1.2, Figure 139, page 116, PDF page 116)
 */
enum xnvme_spec_nvm_lba_status_descriptor_list_cmpc {
	XNVME_SPEC_NVM_LBA_STATUS_DESCRIPTOR_LIST_CMPC_INCOMPLETE = 0x1, ///< INCOMPLETE (NVM 1.3, 4.2.1.2, Figure 139, page 116, PDF page 116)
	XNVME_SPEC_NVM_LBA_STATUS_DESCRIPTOR_LIST_CMPC_COMPLETE = 0x2, ///< COMPLETE (NVM 1.3, 4.2.1.2, Figure 139, page 116, PDF page 116)
};

/**
 * 16b Guard Protection Information Format when STS field is cleared to 0h (NVM 1.3, 5.3.1.1, Figure 155, page 131, PDF page 131)
 */
struct __attribute__((packed)) xnvme_spec_nvm_16b_guard_protection_information_format_when_sts_field_is_cleared_to_0h {
	uint8_t guard[2]; ///< Guard, big-endian, bytes 1:0
	uint8_t application_tag[2]; ///< Application Tag, big-endian, bytes 3:2
	uint8_t reference_tag[4]; ///< Reference Tag, big-endian, bytes 7:4
};

/**
 * 16b Guard Protection Information Format when STS field is non - zero (NVM 1.3, 5.3.1.1, Figure 156, page 132, PDF page 132)
 */
struct __attribute__((packed)) xnvme_spec_nvm_16b_guard_protection_information_format_when_sts_field_is_non_zero {
	uint8_t guard[2]; ///< Guard, big-endian, bytes 1:0
	uint8_t application_tag[2]; ///< Application Tag, big-endian, bytes 3:2
	uint8_t storage_and_reference_space[4]; ///< Storage and Reference Space, big-endian, bytes 7:4
};

/**
 * 32b Guard Protection Information Format (NVM 1.3, 5.3.1.2, Figure 157, page 133, PDF page 133)
 */
struct __attribute__((packed)) xnvme_spec_nvm_32b_guard_protection_information_format {
	uint8_t guard[4]; ///< Guard, big-endian, bytes 3:0
	uint8_t application_tag[2]; ///< Application Tag, big-endian, bytes 5:4
	uint8_t storage_and_reference_space[10]; ///< Storage and Reference Space, big-endian, bytes 15:6
};

/**
 * 64b Guard Protection Information Format (NVM 1.3, 5.3.1.3, Figure 159, page 134, PDF page 134)
 */
struct __attribute__((packed)) xnvme_spec_nvm_64b_guard_protection_information_format {
	uint8_t guard[8]; ///< Guard, big-endian, bytes 7:0
	uint8_t application_tag[2]; ///< Application Tag, big-endian, bytes 9:8
	uint8_t storage_and_reference_space[6]; ///< Storage and Reference Space, big-endian, bytes 15:10
};

/**
 * Reference Exported Configuration State (NVM 1.3, 5.4.1.1, Figure 183, page 153, PDF page 153)
 */
struct __attribute__((packed)) xnvme_spec_nvm_reference_exported_configuration_state {
	uint8_t reccs[364]; ///< Reference Exported Controller Configuration State, bytes 363:0
	uint8_t renscs[48]; ///< Reference Exported Namespace Configuration State, bytes 411:364
};

/**
 * Reference Firmware Slot Information (NVM 1.3, 5.4.1.1, Figure 186, page 154, PDF page 154)
 */
struct __attribute__((packed)) xnvme_spec_nvm_log_reference_firmware_slot_information {
	uint8_t afi; ///< Active Firmware Info, byte 0
};

/**
 * Reference Exported Controller Configuration State (NVM 1.3, 5.4.1.1, Figure 189, page 157, PDF page 157)
 */
struct __attribute__((packed)) xnvme_spec_nvm_reference_exported_controller_configuration_state {
	uint16_t ecntlid; ///< Exported Controller Identifier, bytes 1:0
	uint16_t vid; ///< PCI Vendor ID, bytes 3:2
	uint16_t ssvid; ///< PCI Subsystem Vendor ID, bytes 5:4
	uint8_t sn[20]; ///< Serial Number, bytes 25:6
	uint8_t mn[40]; ///< Model Number, bytes 65:26
	uint64_t fr; ///< Firmware Revision, bytes 73:66
	uint8_t ieee[3]; ///< IEEE OUI Identifier, bytes 76:74
	uint8_t mdts; ///< Maximum Data Transfer Size, byte 77
	uint8_t rab; ///< Recommended Arbitration Burst, byte 78
	uint8_t rsvd79; ///< Reserved, byte 79
	uint16_t ncqs; ///< Number of I/O Completion Queues Supported, bytes 81:80
	uint16_t nsqs; ///< Number of I/O Submission Queues Supported, bytes 83:82
	uint16_t mqes; ///< Maximum Queue Entries Supported, bytes 85:84
	uint16_t oncs; ///< Optional NVM Commands Supported, bytes 87:86, see XNVME_SPEC_NVM_REFERENCE_EXPORTED_CONTROLLER_CONFIGURATION_STATE_ONCS_*
	uint16_t awun; ///< Atomic Write Unit Normal, bytes 89:88
	uint16_t awupf; ///< Atomic Write Unit Power Fail, bytes 91:90
	uint8_t fguid[16]; ///< FRU Globally Unique Identifier, bytes 107:92
	uint8_t subnqn[256]; ///< NVM Subsystem NVMe Qualified Name, bytes 363:108
};

/** Write Zeroes Supported (NVM 1.3, 5.4.1.1, Figure 189, page 157, PDF page 157); bit 3 of 'oncs' */
#define XNVME_SPEC_NVM_REFERENCE_EXPORTED_CONTROLLER_CONFIGURATION_STATE_ONCS_WZS 0x8
/** Dataset Management Supported (NVM 1.3, 5.4.1.1, Figure 189, page 157, PDF page 157); bit 2 of 'oncs' */
#define XNVME_SPEC_NVM_REFERENCE_EXPORTED_CONTROLLER_CONFIGURATION_STATE_ONCS_DSMS 0x4

/**
 * Reference Exported Controller Configuration State (NVM 1.3, 5.4.1.1, Figure 190, page 158, PDF page 158)
 */
struct __attribute__((packed)) xnvme_spec_nvm_reference_exported_controller_configuration_state_figure190 {
	uint32_t ensid; ///< Exported Namespace Identifier, bytes 3:0
	uint32_t lbaf0; ///< LBA Format 0 Support, bytes 7:4, see XNVME_SPEC_NVM_REFERENCE_EXPORTED_CONTROLLER_CONFIGURATION_STATE_FIGURE190_LBAF0_*
	uint16_t nawun; ///< Namespace Atomic Write Unit Normal, bytes 9:8
	uint16_t nawupf; ///< Namespace Atomic Write Unit Power Fail, bytes 11:10
	uint32_t rsvd12; ///< Reserved, bytes 15:12
	uint8_t nguid[16]; ///< Namespace Globally Unique Identifier, bytes 31:16
	uint8_t nuuid[16]; ///< Namespace UUID, bytes 47:32
};

/** Relative Performance (NVM 1.3, 5.4.1.1, Figure 190, page 158, PDF page 158); bits 25:24 of 'lbaf0' */
#define XNVME_SPEC_NVM_REFERENCE_EXPORTED_CONTROLLER_CONFIGURATION_STATE_FIGURE190_LBAF0_RP 0x3000000
/** LBA Data Size (NVM 1.3, 5.4.1.1, Figure 190, page 158, PDF page 158); bits 23:16 of 'lbaf0' */
#define XNVME_SPEC_NVM_REFERENCE_EXPORTED_CONTROLLER_CONFIGURATION_STATE_FIGURE190_LBAF0_LBADS 0xff0000
/** Metadata Size (NVM 1.3, 5.4.1.1, Figure 190, page 158, PDF page 158); bits 15:0 of 'lbaf0' */
#define XNVME_SPEC_NVM_REFERENCE_EXPORTED_CONTROLLER_CONFIGURATION_STATE_FIGURE190_LBAF0_MS 0xffff

/**
 * Reference Exported NVM Subsystem State (NVM 1.3, 5.4.1.2, Figure 191, page 159, PDF page 159)
 *
 * Followed, from byte 64, by as many NVMe Controller State as the data says
 */
struct __attribute__((packed)) xnvme_spec_nvm_reference_exported_nvm_subsystem_state {
	uint32_t arbfv; ///< Arbitration Feature Value, bytes 3:0
	uint32_t pmfv; ///< Power Management Feature Value, bytes 7:4
	uint16_t ctotfv; ///< Composite Temperature Over Threshold Feature Value, bytes 9:8
	uint16_t ctutfv; ///< Composite Temperature Under Threshold Feature Value, bytes 11:10
	uint32_t ns1erfv; ///< Namespace 1 Error Recovery Feature Value, bytes 15:12
	uint32_t nqfv; ///< Number of Queues Feature Value, bytes 19:16
	uint32_t icfv; ///< Interrupt Coalescing Feature Value, bytes 23:20
	uint32_t wanfv; ///< Write Atomicity Normal Feature Value, bytes 27:24
	uint32_t aecfv; ///< Asynchronous Event Configuration, bytes 31:28
	uint8_t csattr; ///< Controller State Attributes, byte 32, see XNVME_SPEC_NVM_REFERENCE_EXPORTED_NVM_SUBSYSTEM_STATE_CSATTR_*
	uint8_t rsvd33[15]; ///< Reserved, bytes 47:33
	uint8_t nvmecss[16]; ///< NVMe Controller State Size, bytes 63:48
};

/** Controller Suspended (NVM 1.3, 5.4.1.2, Figure 191, page 159, PDF page 159); bit 0 of 'csattr' */
#define XNVME_SPEC_NVM_REFERENCE_EXPORTED_NVM_SUBSYSTEM_STATE_CSATTR_CP 0x1

/**
 * LBA Migration Queue Entry Type 0 (NVM 1.3, 5.7.1, Figure 194, page 163, PDF page 163)
 */
struct __attribute__((packed)) xnvme_spec_nvm_lba_migration_queue_entry_type_0 {
	uint32_t nsid; ///< Namespace Identifier, bytes 3:0
	uint32_t nlb; ///< Number of Logical Blocks, bytes 7:4
	uint64_t slba; ///< Starting LBA, bytes 15:8
	uint8_t rsvd16[15]; ///< Reserved, bytes 30:16
	uint8_t lbamqa; ///< LBA Migration Queue Attributes, byte 31, see XNVME_SPEC_NVM_LBA_MIGRATION_QUEUE_ENTRY_TYPE_0_LBAMQA_*
};

/** LBA Change Information Attribute (NVM 1.3, 5.7.1, Figure 194, page 163, PDF page 163); bits 7:6 of 'lbamqa' */
#define XNVME_SPEC_NVM_LBA_MIGRATION_QUEUE_ENTRY_TYPE_0_LBAMQA_LBACIR 0xc0
/** Deallocated LBAs (NVM 1.3, 5.7.1, Figure 194, page 163, PDF page 163); bit 5 of 'lbamqa' */
#define XNVME_SPEC_NVM_LBA_MIGRATION_QUEUE_ENTRY_TYPE_0_LBAMQA_DLBA 0x20
/** Entry Sequence Attribute (NVM 1.3, 5.7.1, Figure 194, page 163, PDF page 163); bits 3:1 of 'lbamqa' */
#define XNVME_SPEC_NVM_LBA_MIGRATION_QUEUE_ENTRY_TYPE_0_LBAMQA_ESA 0xe
/** Controller Data Queue Phase Tag (NVM 1.3, 5.7.1, Figure 194, page 163, PDF page 163); bit 0 of 'lbamqa' */
#define XNVME_SPEC_NVM_LBA_MIGRATION_QUEUE_ENTRY_TYPE_0_LBAMQA_CDQP 0x1

/**
 * The values of Entry Sequence Attribute (NVM 1.3, 5.7.1, Figure 194, page 163, PDF page 163)
 */
enum xnvme_spec_nvm_lba_migration_queue_entry_type_0_lbamqa_esa {
	XNVME_SPEC_NVM_LBA_MIGRATION_QUEUE_ENTRY_TYPE_0_LBAMQA_ESA_THIS_ENTRY_IS_NOT_THE = 0x0, ///< This entry is not the (NVM 1.3, 5.7.1, Figure 194, page 163, PDF page 163)
};

/*
 * The fields of Write, by the dwords of the command
 *
 * Command Dword 13 is described otherwise if CETYPE is non-zero
 */
/** Logical Block Tags Upper (NVM 1.3, 3.3.6, Figure 69, page 54, PDF page 54); bits 47:0 of Command Dwords 2 and 3, as one 64-bit value */
#define XNVME_SPEC_NVM_WRITE_CDW2_3_LBTU 0xffffffffffff
/** Metadata Pointer (NVM 1.3, 3.3.6, Figure 67, page 53, PDF page 53); bits 63:0 of Command Dwords 4 and 5, as one 64-bit value */
#define XNVME_SPEC_NVM_WRITE_CDW4_5_MPTR 0xffffffffffffffff
/** Starting LBA (NVM 1.3, 3.3.6, Figure 70, page 54, PDF page 54); bits 63:0 of Command Dwords 10 and 11, as one 64-bit value */
#define XNVME_SPEC_NVM_WRITE_CDW10_11_SLBA 0xffffffffffffffff
/** Limited Retry (NVM 1.3, 3.3.6, Figure 71, page 54, PDF page 54); bit 31 of Command Dword 12 */
#define XNVME_SPEC_NVM_WRITE_CDW12_LR 0x80000000
/** Force Unit Access (NVM 1.3, 3.3.6, Figure 71, page 54, PDF page 54); bit 30 of Command Dword 12 */
#define XNVME_SPEC_NVM_WRITE_CDW12_FUA 0x40000000
/** Protection Information (NVM 1.3, 3.3.6, Figure 71, page 54, PDF page 54); bits 29:26 of Command Dword 12 */
#define XNVME_SPEC_NVM_WRITE_CDW12_PRINFO 0x3c000000
/** Storage Tag Check (NVM 1.3, 3.3.6, Figure 71, page 54, PDF page 54); bit 24 of Command Dword 12 */
#define XNVME_SPEC_NVM_WRITE_CDW12_STC 0x1000000
/** Directive Type (NVM 1.3, 3.3.6, Figure 71, page 54, PDF page 54); bits 23:20 of Command Dword 12 */
#define XNVME_SPEC_NVM_WRITE_CDW12_DTYPE 0xf00000
/** Command Extension Type (NVM 1.3, 3.3.6, Figure 71, page 54, PDF page 54); bits 19:16 of Command Dword 12 */
#define XNVME_SPEC_NVM_WRITE_CDW12_CETYPE 0xf0000
/** Number of Logical Blocks (NVM 1.3, 3.3.6, Figure 71, page 54, PDF page 54); bits 15:0 of Command Dword 12 */
#define XNVME_SPEC_NVM_WRITE_CDW12_NLB 0xffff
/** Directive Specific (NVM 1.3, 3.3.6, Figure 72, page 54, PDF page 54); bits 31:16 of Command Dword 13 */
#define XNVME_SPEC_NVM_WRITE_CDW13_DSPEC 0xffff0000
/** Dataset Management (NVM 1.3, 3.3.6, Figure 72, page 54, PDF page 54); bits 7:0 of Command Dword 13 */
#define XNVME_SPEC_NVM_WRITE_CDW13_DSM 0xff
/** Logical Block Tags Lower (NVM 1.3, 3.3.6, Figure 74, page 55, PDF page 55); bits 31:0 of Command Dword 14 */
#define XNVME_SPEC_NVM_WRITE_CDW14_LBTL 0xffffffff
/** Logical Block Application Tag Mask (NVM 1.3, 3.3.6, Figure 75, page 55, PDF page 55); bits 31:16 of Command Dword 15 */
#define XNVME_SPEC_NVM_WRITE_CDW15_LBATM 0xffff0000
/** Logical Block Application Tag (NVM 1.3, 3.3.6, Figure 75, page 55, PDF page 55); bits 15:0 of Command Dword 15 */
#define XNVME_SPEC_NVM_WRITE_CDW15_LBAT 0xffff

/*
 * The fields of Read, by the dwords of the command
 *
 * Command Dword 13 is described otherwise if CETYPE is non-zero
 */
/** Expected Logical Block Tags Upper (NVM 1.3, 3.3.4, Figure 52, page 49, PDF page 49); bits 47:0 of Command Dwords 2 and 3, as one 64-bit value */
#define XNVME_SPEC_NVM_READ_CDW2_3_ELBTU 0xffffffffffff
/** Metadata Pointer (NVM 1.3, 3.3.4, Figure 50, page 49, PDF page 49); bits 63:0 of Command Dwords 4 and 5, as one 64-bit value */
#define XNVME_SPEC_NVM_READ_CDW4_5_MPTR 0xffffffffffffffff
/** Starting LBA (NVM 1.3, 3.3.4, Figure 53, page 49, PDF page 49); bits 63:0 of Command Dwords 10 and 11, as one 64-bit value */
#define XNVME_SPEC_NVM_READ_CDW10_11_SLBA 0xffffffffffffffff
/** Limited Retry (NVM 1.3, 3.3.4, Figure 54, page 49, PDF page 49); bit 31 of Command Dword 12 */
#define XNVME_SPEC_NVM_READ_CDW12_LR 0x80000000
/** Force Unit Access (NVM 1.3, 3.3.4, Figure 54, page 49, PDF page 49); bit 30 of Command Dword 12 */
#define XNVME_SPEC_NVM_READ_CDW12_FUA 0x40000000
/** Protection Information (NVM 1.3, 3.3.4, Figure 54, page 49, PDF page 49); bits 29:26 of Command Dword 12 */
#define XNVME_SPEC_NVM_READ_CDW12_PRINFO 0x3c000000
/** Storage Tag Check (NVM 1.3, 3.3.4, Figure 54, page 49, PDF page 49); bit 24 of Command Dword 12 */
#define XNVME_SPEC_NVM_READ_CDW12_STC 0x1000000
/** Command Extension Type (NVM 1.3, 3.3.4, Figure 54, page 49, PDF page 49); bits 19:16 of Command Dword 12 */
#define XNVME_SPEC_NVM_READ_CDW12_CETYPE 0xf0000
/** Number of Logical Blocks (NVM 1.3, 3.3.4, Figure 54, page 49, PDF page 49); bits 15:0 of Command Dword 12 */
#define XNVME_SPEC_NVM_READ_CDW12_NLB 0xffff
/** Dataset Management (NVM 1.3, 3.3.4, Figure 55, page 50, PDF page 50); bits 7:0 of Command Dword 13 */
#define XNVME_SPEC_NVM_READ_CDW13_DSM 0xff
/** Expected Logical Block Tags Lower (NVM 1.3, 3.3.4, Figure 57, page 50, PDF page 50); bits 31:0 of Command Dword 14 */
#define XNVME_SPEC_NVM_READ_CDW14_ELBTL 0xffffffff
/** Expected Logical Block Application Tag Mask (NVM 1.3, 3.3.4, Figure 58, page 51, PDF page 51); bits 31:16 of Command Dword 15 */
#define XNVME_SPEC_NVM_READ_CDW15_ELBATM 0xffff0000
/** Expected Logical Block Application Tag (NVM 1.3, 3.3.4, Figure 58, page 51, PDF page 51); bits 15:0 of Command Dword 15 */
#define XNVME_SPEC_NVM_READ_CDW15_ELBAT 0xffff

/*
 * The fields of Write Uncorrectable, by the dwords of the command
 */
/** Starting LBA (NVM 1.3, 3.3.7, Figure 77, page 56, PDF page 56); bits 63:0 of Command Dwords 10 and 11, as one 64-bit value */
#define XNVME_SPEC_NVM_WRITE_UNCORRECTABLE_CDW10_11_SLBA 0xffffffffffffffff
/** Directive Type (NVM 1.3, 3.3.7, Figure 78, page 56, PDF page 56); bits 23:20 of Command Dword 12 */
#define XNVME_SPEC_NVM_WRITE_UNCORRECTABLE_CDW12_DTYPE 0xf00000
/** Number of Logical Blocks (NVM 1.3, 3.3.7, Figure 78, page 56, PDF page 56); bits 15:0 of Command Dword 12 */
#define XNVME_SPEC_NVM_WRITE_UNCORRECTABLE_CDW12_NLB 0xffff
/** Directive Specific (NVM 1.3, 3.3.7, Figure 79, page 57, PDF page 57); bits 31:16 of Command Dword 13 */
#define XNVME_SPEC_NVM_WRITE_UNCORRECTABLE_CDW13_DSPEC 0xffff0000

/*
 * The fields of Compare, by the dwords of the command
 */
/** Expected Logical Block Tags Upper (NVM 1.3, 3.3.1, Figure 25, page 28, PDF page 28); bits 47:0 of Command Dwords 2 and 3, as one 64-bit value */
#define XNVME_SPEC_NVM_COMPARE_CDW2_3_ELBTU 0xffffffffffff
/** Metadata Pointer (NVM 1.3, 3.3.1, Figure 23, page 28, PDF page 28); bits 63:0 of Command Dwords 4 and 5, as one 64-bit value */
#define XNVME_SPEC_NVM_COMPARE_CDW4_5_MPTR 0xffffffffffffffff
/** Starting LBA (NVM 1.3, 3.3.1, Figure 26, page 28, PDF page 28); bits 63:0 of Command Dwords 10 and 11, as one 64-bit value */
#define XNVME_SPEC_NVM_COMPARE_CDW10_11_SLBA 0xffffffffffffffff
/** Limited Retry (NVM 1.3, 3.3.1, Figure 27, page 28, PDF page 28); bit 31 of Command Dword 12 */
#define XNVME_SPEC_NVM_COMPARE_CDW12_LR 0x80000000
/** Force Unit Access (NVM 1.3, 3.3.1, Figure 27, page 28, PDF page 28); bit 30 of Command Dword 12 */
#define XNVME_SPEC_NVM_COMPARE_CDW12_FUA 0x40000000
/** Protection Information (NVM 1.3, 3.3.1, Figure 27, page 28, PDF page 28); bits 29:26 of Command Dword 12 */
#define XNVME_SPEC_NVM_COMPARE_CDW12_PRINFO 0x3c000000
/** Storage Tag Check (NVM 1.3, 3.3.1, Figure 27, page 28, PDF page 28); bit 24 of Command Dword 12 */
#define XNVME_SPEC_NVM_COMPARE_CDW12_STC 0x1000000
/** Command Extension Type (NVM 1.3, 3.3.1, Figure 27, page 28, PDF page 28); bits 19:16 of Command Dword 12 */
#define XNVME_SPEC_NVM_COMPARE_CDW12_CETYPE 0xf0000
/** Number of Logical Blocks (NVM 1.3, 3.3.1, Figure 27, page 28, PDF page 28); bits 15:0 of Command Dword 12 */
#define XNVME_SPEC_NVM_COMPARE_CDW12_NLB 0xffff
/** Command Extension Value (NVM 1.3, 3.3.1, Figure 28, page 29, PDF page 29); bits 15:0 of Command Dword 13 */
#define XNVME_SPEC_NVM_COMPARE_CDW13_CEV 0xffff
/** Expected Logical Block Tags Lower (NVM 1.3, 3.3.1, Figure 29, page 29, PDF page 29); bits 31:0 of Command Dword 14 */
#define XNVME_SPEC_NVM_COMPARE_CDW14_ELBTL 0xffffffff
/** Expected Logical Block Application Tag Mask (NVM 1.3, 3.3.1, Figure 30, page 29, PDF page 29); bits 31:16 of Command Dword 15 */
#define XNVME_SPEC_NVM_COMPARE_CDW15_ELBATM 0xffff0000
/** Expected Logical Block Application Tag (NVM 1.3, 3.3.1, Figure 30, page 29, PDF page 29); bits 15:0 of Command Dword 15 */
#define XNVME_SPEC_NVM_COMPARE_CDW15_ELBAT 0xffff

/*
 * The fields of Write Zeroes, by the dwords of the command
 *
 * Command Dword 13 is described otherwise if CETYPE is non-zero
 */
/** Logical Block Tags Upper (NVM 1.3, 3.3.8, Figure 81, page 59, PDF page 59); bits 47:0 of Command Dwords 2 and 3, as one 64-bit value */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW2_3_LBTU 0xffffffffffff
/** Starting LBA (NVM 1.3, 3.3.8, Figure 82, page 59, PDF page 59); bits 63:0 of Command Dwords 10 and 11, as one 64-bit value */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW10_11_SLBA 0xffffffffffffffff
/** Limited Retry (NVM 1.3, 3.3.8, Figure 83, page 59, PDF page 59); bit 31 of Command Dword 12 */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_LR 0x80000000
/** Force Unit Access (NVM 1.3, 3.3.8, Figure 83, page 59, PDF page 59); bit 30 of Command Dword 12 */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_FUA 0x40000000
/** Protection Information (NVM 1.3, 3.3.8, Figure 83, page 59, PDF page 59); bits 29:26 of Command Dword 12 */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_PRINFO 0x3c000000
/** Deallocate (NVM 1.3, 3.3.8, Figure 83, page 59, PDF page 59); bit 25 of Command Dword 12 */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_DEAC 0x2000000
/** Storage Tag Check (NVM 1.3, 3.3.8, Figure 83, page 59, PDF page 59); bit 24 of Command Dword 12 */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_STC 0x1000000
/** Namespace Zeroes (NVM 1.3, 3.3.8, Figure 83, page 59, PDF page 59); bit 23 of Command Dword 12 */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_NSZ 0x800000
/** Directive Type (NVM 1.3, 3.3.8, Figure 83, page 59, PDF page 59); bits 22:20 of Command Dword 12 */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_DTYPE 0x700000
/** Command Extension Type (NVM 1.3, 3.3.8, Figure 83, page 59, PDF page 59); bits 19:16 of Command Dword 12 */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_CETYPE 0xf0000
/** Number of Logical Blocks (NVM 1.3, 3.3.8, Figure 83, page 59, PDF page 59); bits 15:0 of Command Dword 12 */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_NLB 0xffff
/** Directive Specific (NVM 1.3, 3.3.8, Figure 84, page 60, PDF page 60); bits 31:16 of Command Dword 13 */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW13_DSPEC 0xffff0000
/** Logical Block Tags Lower (NVM 1.3, 3.3.8, Figure 86, page 60, PDF page 60); bits 31:0 of Command Dword 14 */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW14_LBTL 0xffffffff
/** Logical Block Application Tag Mask (NVM 1.3, 3.3.8, Figure 87, page 60, PDF page 60); bits 31:16 of Command Dword 15 */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW15_LBATM 0xffff0000
/** Logical Block Application Tag (NVM 1.3, 3.3.8, Figure 87, page 60, PDF page 60); bits 15:0 of Command Dword 15 */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW15_LBAT 0xffff

/*
 * The fields of Dataset Management, by the dwords of the command
 */
/** Number of Ranges (NVM 1.3, 3.3.3, Figure 45, page 44, PDF page 44); bits 7:0 of Command Dword 10 */
#define XNVME_SPEC_NVM_DATASET_MANAGEMENT_CDW10_NR 0xff
/** Attribute - Deallocate (NVM 1.3, 3.3.3, Figure 46, page 44, PDF page 44); bit 2 of Command Dword 11 */
#define XNVME_SPEC_NVM_DATASET_MANAGEMENT_CDW11_AD 0x4
/** Attribute - Integral Dataset for Write (NVM 1.3, 3.3.3, Figure 46, page 44, PDF page 44); bit 1 of Command Dword 11 */
#define XNVME_SPEC_NVM_DATASET_MANAGEMENT_CDW11_IDW 0x2
/** Attribute - Integral Dataset for Read (NVM 1.3, 3.3.3, Figure 46, page 44, PDF page 44); bit 0 of Command Dword 11 */
#define XNVME_SPEC_NVM_DATASET_MANAGEMENT_CDW11_IDR 0x1

/*
 * The fields of Verify, by the dwords of the command
 */
/** Expected Logical Block Tags Upper (NVM 1.3, 3.3.5, Figure 60, page 52, PDF page 52); bits 47:0 of Command Dwords 2 and 3, as one 64-bit value */
#define XNVME_SPEC_NVM_VERIFY_CDW2_3_ELBTU 0xffffffffffff
/** Starting LBA (NVM 1.3, 3.3.5, Figure 61, page 52, PDF page 52); bits 63:0 of Command Dwords 10 and 11, as one 64-bit value */
#define XNVME_SPEC_NVM_VERIFY_CDW10_11_SLBA 0xffffffffffffffff
/** Limited Retry (NVM 1.3, 3.3.5, Figure 62, page 52, PDF page 52); bit 31 of Command Dword 12 */
#define XNVME_SPEC_NVM_VERIFY_CDW12_LR 0x80000000
/** Force Unit Access (NVM 1.3, 3.3.5, Figure 62, page 52, PDF page 52); bit 30 of Command Dword 12 */
#define XNVME_SPEC_NVM_VERIFY_CDW12_FUA 0x40000000
/** Protection Information (NVM 1.3, 3.3.5, Figure 62, page 52, PDF page 52); bits 29:26 of Command Dword 12 */
#define XNVME_SPEC_NVM_VERIFY_CDW12_PRINFO 0x3c000000
/** Storage Tag Check (NVM 1.3, 3.3.5, Figure 62, page 52, PDF page 52); bit 24 of Command Dword 12 */
#define XNVME_SPEC_NVM_VERIFY_CDW12_STC 0x1000000
/** Command Extension Type (NVM 1.3, 3.3.5, Figure 62, page 52, PDF page 52); bits 19:16 of Command Dword 12 */
#define XNVME_SPEC_NVM_VERIFY_CDW12_CETYPE 0xf0000
/** Number of Logical Blocks (NVM 1.3, 3.3.5, Figure 62, page 52, PDF page 52); bits 15:0 of Command Dword 12 */
#define XNVME_SPEC_NVM_VERIFY_CDW12_NLB 0xffff
/** Command Extension Value (NVM 1.3, 3.3.5, Figure 63, page 52, PDF page 52); bits 15:0 of Command Dword 13 */
#define XNVME_SPEC_NVM_VERIFY_CDW13_CEV 0xffff
/** Expected Logical Block Tags Lower (NVM 1.3, 3.3.5, Figure 64, page 53, PDF page 53); bits 31:0 of Command Dword 14 */
#define XNVME_SPEC_NVM_VERIFY_CDW14_ELBTL 0xffffffff
/** Expected Logical Block Application Tag Mask (NVM 1.3, 3.3.5, Figure 65, page 53, PDF page 53); bits 31:16 of Command Dword 15 */
#define XNVME_SPEC_NVM_VERIFY_CDW15_ELBATM 0xffff0000
/** Expected Logical Block Application Tag (NVM 1.3, 3.3.5, Figure 65, page 53, PDF page 53); bits 15:0 of Command Dword 15 */
#define XNVME_SPEC_NVM_VERIFY_CDW15_ELBAT 0xffff

/*
 * The fields of Copy, by the dwords of the command
 */
/** Logical Block Tags Upper (NVM 1.3, 3.3.2, Figure 33, page 30, PDF page 30); bits 47:0 of Command Dwords 2 and 3, as one 64-bit value */
#define XNVME_SPEC_NVM_COPY_CDW2_3_LBTU 0xffffffffffff
/** Starting Destination LBA (NVM 1.3, 3.3.2, Figure 34, page 30, PDF page 30); bits 63:0 of Command Dwords 10 and 11, as one 64-bit value */
#define XNVME_SPEC_NVM_COPY_CDW10_11_SDLBA 0xffffffffffffffff
/** Limited Retry (NVM 1.3, 3.3.2, Figure 35, page 30, PDF page 30); bit 31 of Command Dword 12 */
#define XNVME_SPEC_NVM_COPY_CDW12_LR 0x80000000
/** Force Unit Access (NVM 1.3, 3.3.2, Figure 35, page 30, PDF page 30); bit 30 of Command Dword 12 */
#define XNVME_SPEC_NVM_COPY_CDW12_FUA 0x40000000
/** Protection Information Write (NVM 1.3, 3.3.2, Figure 35, page 30, PDF page 30); bits 29:26 of Command Dword 12 */
#define XNVME_SPEC_NVM_COPY_CDW12_PRINFOW 0x3c000000
/** Storage Tag Check Read (NVM 1.3, 3.3.2, Figure 35, page 30, PDF page 30); bit 25 of Command Dword 12 */
#define XNVME_SPEC_NVM_COPY_CDW12_STCR 0x2000000
/** Storage Tag Check Write (NVM 1.3, 3.3.2, Figure 35, page 30, PDF page 30); bit 24 of Command Dword 12 */
#define XNVME_SPEC_NVM_COPY_CDW12_STCW 0x1000000
/** Directive Type (NVM 1.3, 3.3.2, Figure 35, page 30, PDF page 30); bits 23:20 of Command Dword 12 */
#define XNVME_SPEC_NVM_COPY_CDW12_DTYPE 0xf00000
/** Command Extension Type (NVM 1.3, 3.3.2, Figure 35, page 30, PDF page 30); bits 19:16 of Command Dword 12 */
#define XNVME_SPEC_NVM_COPY_CDW12_CETYPE 0xf0000
/** Protection Information Read (NVM 1.3, 3.3.2, Figure 35, page 30, PDF page 30); bits 15:12 of Command Dword 12 */
#define XNVME_SPEC_NVM_COPY_CDW12_PRINFOR 0xf000
/** Descriptor Format (NVM 1.3, 3.3.2, Figure 35, page 30, PDF page 30); bits 11:8 of Command Dword 12 */
#define XNVME_SPEC_NVM_COPY_CDW12_DESFMT 0xf00
/** Number of Ranges (NVM 1.3, 3.3.2, Figure 35, page 30, PDF page 30); bits 7:0 of Command Dword 12 */
#define XNVME_SPEC_NVM_COPY_CDW12_NR 0xff
/** Directive Specific (NVM 1.3, 3.3.2, Figure 36, page 31, PDF page 31); bits 31:16 of Command Dword 13 */
#define XNVME_SPEC_NVM_COPY_CDW13_DSPEC 0xffff0000
/** Command Extension Value (NVM 1.3, 3.3.2, Figure 36, page 31, PDF page 31); bits 15:0 of Command Dword 13 */
#define XNVME_SPEC_NVM_COPY_CDW13_CEV 0xffff
/** Logical Block Tags Lower (NVM 1.3, 3.3.2, Figure 37, page 31, PDF page 31); bits 31:0 of Command Dword 14 */
#define XNVME_SPEC_NVM_COPY_CDW14_LBTL 0xffffffff
/** Logical Block Application Tag Mask (NVM 1.3, 3.3.2, Figure 38, page 32, PDF page 32); bits 31:16 of Command Dword 15 */
#define XNVME_SPEC_NVM_COPY_CDW15_LBATM 0xffff0000
/** Logical Block Application Tag (NVM 1.3, 3.3.2, Figure 38, page 32, PDF page 32); bits 15:0 of Command Dword 15 */
#define XNVME_SPEC_NVM_COPY_CDW15_LBAT 0xffff

#endif /* LIBXNVME_SPEC_NVM_H */
