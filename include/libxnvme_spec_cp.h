/**
 * What the cp specification defines, by xnvme_spec
 *
 * Written from the data model of senfd 0.19.0, by toolbox/xnvme_spec_headers.py;
 * each name and mask is as the figure it cites gives it. Do not
 * edit: see the contributor docs on producing the spec headers.
 *
 * @file libxnvme_spec_cp.h
 */
#ifndef LIBXNVME_SPEC_CP_H
#define LIBXNVME_SPEC_CP_H

#include <stdint.h>

/**
 * The opcodes of the Computational Programs commands
 */
enum xnvme_spec_cp_opcode {
	XNVME_SPEC_CP_EXECUTE_PROGRAM_OPCODE = 0x1, ///< Execute Program (Computational Programs 1.3, 3.2, Figure 15, page 26, PDF page 26)
};

/**
 * The status codes of the type Command Specific
 */
enum xnvme_spec_cp_command_specific_status {
	XNVME_SPEC_CP_COMMAND_SPECIFIC_STATUS_INSUFFICIENT_PROGRAM_RESOURCES = 0x8a, ///< Insufficient Program Resources (Computational Programs 1.3, 3.1.2, Figure 14, page 25, PDF page 25)
	XNVME_SPEC_CP_COMMAND_SPECIFIC_STATUS_INVALID_MEMORY_NAMESPACE = 0x8b, ///< Invalid Memory Namespace (Computational Programs 1.3, 3.1.2, Figure 14, page 25, PDF page 25)
	XNVME_SPEC_CP_COMMAND_SPECIFIC_STATUS_INVALID_MEMORY_RANGE_SET = 0x8c, ///< Invalid Memory Range Set (Computational Programs 1.3, 3.1.2, Figure 14, page 25, PDF page 25)
	XNVME_SPEC_CP_COMMAND_SPECIFIC_STATUS_INVALID_PROGRAM_DATA = 0x8e, ///< Invalid Program Data (Computational Programs 1.3, 3.1.2, Figure 14, page 25, PDF page 25)
	XNVME_SPEC_CP_COMMAND_SPECIFIC_STATUS_INVALID_PROGRAM_INDEX = 0x8f, ///< Invalid Program Index (Computational Programs 1.3, 3.1.2, Figure 14, page 25, PDF page 25)
	XNVME_SPEC_CP_COMMAND_SPECIFIC_STATUS_INVALID_PROGRAM_TYPE = 0x90, ///< Invalid Program Type (Computational Programs 1.3, 3.1.2, Figure 14, page 25, PDF page 25)
	XNVME_SPEC_CP_COMMAND_SPECIFIC_STATUS_MAXIMUM_MEMORY_RANGES_EXCEEDED = 0x91, ///< Maximum Memory Ranges Exceeded (Computational Programs 1.3, 3.1.2, Figure 14, page 25, PDF page 25)
	XNVME_SPEC_CP_COMMAND_SPECIFIC_STATUS_MAXIMUM_MEMORY_RANGE_SETS = 0x92, ///< Maximum Memory Range Sets (Computational Programs 1.3, 3.1.2, Figure 14, page 25, PDF page 25)
	XNVME_SPEC_CP_COMMAND_SPECIFIC_STATUS_MAXIMUM_PROGRAMS_ACTIVATED = 0x93, ///< Maximum Programs Activated (Computational Programs 1.3, 3.1.2, Figure 14, page 25, PDF page 25)
	XNVME_SPEC_CP_COMMAND_SPECIFIC_STATUS_MAXIMUM_PROGRAM_BYTES_EXCEEDED = 0x94, ///< Maximum Program Bytes Exceeded (Computational Programs 1.3, 3.1.2, Figure 14, page 25, PDF page 25)
	XNVME_SPEC_CP_COMMAND_SPECIFIC_STATUS_NO_PROGRAM = 0x96, ///< No Program (Computational Programs 1.3, 3.1.2, Figure 14, page 25, PDF page 25)
	XNVME_SPEC_CP_COMMAND_SPECIFIC_STATUS_OVERLAPPING_MEMORY_RANGES = 0x97, ///< Overlapping Memory Ranges (Computational Programs 1.3, 3.1.2, Figure 14, page 25, PDF page 25)
	XNVME_SPEC_CP_COMMAND_SPECIFIC_STATUS_PROGRAM_NOT_ACTIVATED = 0x98, ///< Program Not Activated (Computational Programs 1.3, 3.1.2, Figure 14, page 25, PDF page 25)
	XNVME_SPEC_CP_COMMAND_SPECIFIC_STATUS_PROGRAM_IN_USE = 0x99, ///< Program In Use (Computational Programs 1.3, 3.1.2, Figure 14, page 25, PDF page 25)
	XNVME_SPEC_CP_COMMAND_SPECIFIC_STATUS_PROGRAM_INDEX_NOT_DOWNLOADABLE = 0x9a, ///< Program Index Not Downloadable (Computational Programs 1.3, 3.1.2, Figure 14, page 25, PDF page 25)
	XNVME_SPEC_CP_COMMAND_SPECIFIC_STATUS_PROGRAM_TOO_BIG = 0x9b, ///< Program Too Big (Computational Programs 1.3, 3.1.2, Figure 14, page 25, PDF page 25)
};

/**
 * Program Descriptor (Computational Programs 1.3, 4.1.4.1, Figure 27, page 30, PDF page 30)
 */
struct __attribute__((packed)) xnvme_spec_cp_program_descriptor {
	uint8_t byte_00; ///< Byte 00, byte 0, see XNVME_SPEC_CP_PROGRAM_DESCRIPTOR_BYTE_00_*
	uint8_t ptype; ///< Program Type, byte 1
	uint8_t rsvd2[6]; ///< Reserved, bytes 7:2
	uint64_t pid; ///< Program ID, bytes 15:8
	uint8_t rsvd16[48]; ///< Reserved, bytes 63:16
};

/** Program Identifier Type (Computational Programs 1.3, 4.1.4.1, Figure 27, page 30, PDF page 30); bits 5:3 of 'byte_00' */
#define XNVME_SPEC_CP_PROGRAM_DESCRIPTOR_BYTE_00_PIT 0x38
/** Activation (Computational Programs 1.3, 4.1.4.1, Figure 27, page 30, PDF page 30); bit 2 of 'byte_00' */
#define XNVME_SPEC_CP_PROGRAM_DESCRIPTOR_BYTE_00_ACT 0x4
/** Program Index Occupied (Computational Programs 1.3, 4.1.4.1, Figure 27, page 30, PDF page 30); bits 1:0 of 'byte_00' */
#define XNVME_SPEC_CP_PROGRAM_DESCRIPTOR_BYTE_00_PEOCC 0x3

/**
 * The values of Program Identifier Type (Computational Programs 1.3, 4.1.4.1, Figure 27, page 30, PDF page 30)
 */
enum xnvme_spec_cp_program_descriptor_byte_00_pit {
	XNVME_SPEC_CP_PROGRAM_DESCRIPTOR_BYTE_00_PIT_PID_IS_A_PROGRAM_UNIQUE_IDENTIFIER = 0x1, ///< PID is a Program Unique Identifier (Computational Programs 1.3, 4.1.4.1, Figure 27, page 30, PDF page 30)
};

/**
 * The values of Activation (Computational Programs 1.3, 4.1.4.1, Figure 27, page 30, PDF page 30)
 */
enum xnvme_spec_cp_program_descriptor_byte_00_act {
	XNVME_SPEC_CP_PROGRAM_DESCRIPTOR_BYTE_00_ACT_ACTIVATED = 0x1, ///< Activated (Computational Programs 1.3, 4.1.4.1, Figure 27, page 30, PDF page 30)
};

/**
 * The values of Program Type (Computational Programs 1.3, 4.1.4.2, Figure 29, page 31, PDF page 31)
 */
enum xnvme_spec_cp_program_descriptor_ptype {
	XNVME_SPEC_CP_PROGRAM_DESCRIPTOR_PTYPE_DEVICE_DEFINED = 0x0, ///< Device-defined (Computational Programs 1.3, 4.1.4.2, Figure 29, page 31, PDF page 31)
};

/**
 * The values of Program ID (Computational Programs 1.3, 4.1.4.1, Figure 27, page 30, PDF page 30)
 */
enum xnvme_spec_cp_program_descriptor_pid {
	XNVME_SPEC_CP_PROGRAM_DESCRIPTOR_PID_PROGRAM_UNIQUE_ID = 0x1, ///< Program Unique ID (Computational Programs 1.3, 4.1.4.1, Figure 27, page 30, PDF page 30)
};

/**
 * Program List (Computational Programs 1.3, 4.1.4.1, Figure 26, page 30, PDF page 30)
 *
 * Followed, from byte 64, by as many Program as the data says, each a 'struct xnvme_spec_cp_program_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_cp_log_program_list {
	uint32_t numd; ///< Number of Descriptors, bytes 3:0
	uint8_t rsvd4[60]; ///< Reserved, bytes 63:4
};

/**
 * Downloadable Program Type Descriptor (Computational Programs 1.3, 4.1.4.2, Figure 30, page 32, PDF page 32)
 */
struct __attribute__((packed)) xnvme_spec_cp_downloadable_program_type_descriptor {
	uint8_t ptype; ///< Program Type, byte 0
	uint8_t ver; ///< Version, byte 1
	uint8_t rsvd2[6]; ///< Reserved, bytes 7:2
	uint8_t spt[24]; ///< Specific Program Type, bytes 31:8
};

/**
 * The values of Program Type (Computational Programs 1.3, 4.1.4.2, Figure 29, page 31, PDF page 31)
 */
enum xnvme_spec_cp_downloadable_program_type_descriptor_ptype {
	XNVME_SPEC_CP_DOWNLOADABLE_PROGRAM_TYPE_DESCRIPTOR_PTYPE_DEVICE_DEFINED = 0x0, ///< Device-defined (Computational Programs 1.3, 4.1.4.2, Figure 29, page 31, PDF page 31)
};

/**
 * Downloadable Program Types (Computational Programs 1.3, 4.1.4.2, Figure 28, page 31, PDF page 31)
 *
 * Followed, from byte 32, by as many Downloadable Program Type Descriptor as the data says, each a 'struct xnvme_spec_cp_downloadable_program_type_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_cp_log_downloadable_program_types {
	uint32_t numd; ///< Number of Descriptors, bytes 3:0
	uint8_t rsvd4[28]; ///< Reserved, bytes 31:4
};

/**
 * Memory Range Set List (Computational Programs 1.3, 4.1.4.3, Figure 32, page 32, PDF page 32)
 *
 * Followed, from byte 4, by as many Memory Range Set as the data says
 *
 * Not members:
 * Memory Range Set 1: at an offset the data gives, in 'memory_range_set'
 * Memory Range Set NUMD-1: at an offset the data gives, in 'memory_range_set'
 */
struct __attribute__((packed)) xnvme_spec_cp_log_memory_range_set_list {
	uint32_t numd; ///< Number of Descriptors, bytes 3:0
};

/**
 * Memory Range Descriptor (Computational Programs 1.3, 4.1.4.3, Figure 34, page 33, PDF page 33)
 */
struct __attribute__((packed)) xnvme_spec_cp_memory_range_descriptor {
	uint32_t mnsid; ///< Memory Namespace ID, bytes 3:0
	uint32_t len; ///< Length, bytes 7:4
	uint64_t sb; ///< Starting Byte, bytes 15:8
	uint8_t rsvd16[16]; ///< Reserved, bytes 31:16
};

/**
 * Memory Range Set Descriptor (Computational Programs 1.3, 4.1.4.3, Figure 33, page 33, PDF page 33)
 *
 * Followed, from byte 32, by as many Memory Range Descriptor as the data says, each a 'struct xnvme_spec_cp_memory_range_descriptor'
 */
struct __attribute__((packed)) xnvme_spec_cp_memory_range_set_descriptor {
	uint16_t rsid; ///< Memory Range Set ID, bytes 1:0
	uint32_t nmr; ///< Number of Memory Range Descriptors, bytes 5:2
	uint8_t rsvd6[10]; ///< Reserved, bytes 15:6
	uint8_t rsvd16[16]; ///< Reserved, bytes 31:16
};

/**
 * I/O Command Set specific Identify Namespace (Computational Programs 1.3, 4.1.5.1, Figure 36, page 34, PDF page 34)
 */
struct __attribute__((packed)) xnvme_spec_cp_io_command_set_specific_identify_namespace {
	uint16_t maxact; ///< Maximum Activated Programs, bytes 1:0
	uint16_t maxmemrs; ///< Maximum Number of Memory Range Sets, bytes 3:2
	uint16_t mrsg; ///< Memory Range Size Granularity, bytes 5:4
	uint8_t maxmemr; ///< Maximum Number of Ranges in a Memory Range Set, byte 6
	uint8_t rsvd7; ///< Reserved, byte 7
	uint64_t maxpb; ///< Maximum Bytes for All Downloaded Programs, bytes 15:8
	uint8_t lpg; ///< Load Program Granularity, byte 16
	uint8_t rsvd17[4079]; ///< Reserved, bytes 4095:17
};

/**
 * Identify Controller (Computational Programs 1.3, 4.1.5.2, Figure 37, page 34, PDF page 34)
 */
struct __attribute__((packed)) xnvme_spec_cp_identify_controller {
	uint32_t ver; ///< Version, bytes 3:0
	uint8_t rsvd4[4092]; ///< Reserved, bytes 4095:4
};

/**
 * Create operation - Memory Range Definitions (Computational Programs 1.3, 4.2.2, Figure 49, page 38, PDF page 38)
 */
struct __attribute__((packed)) xnvme_spec_cp_memory_range_definitions {
	struct xnvme_spec_cp_memory_range_descriptor mrd[128]; ///< Memory Range Descriptor, 128 of 32 bytes, bytes 4095:0
};

/*
 * The fields of Execute Program, by the dwords of the command
 */
/** Memory Range Set ID (Computational Programs 1.3, 3.2.1, Figure 16, page 27, PDF page 27); bits 31:16 of Command Dword 2 */
#define XNVME_SPEC_CP_EXECUTE_PROGRAM_CDW2_RSID 0xffff0000
/** Program Index (Computational Programs 1.3, 3.2.1, Figure 16, page 27, PDF page 27); bits 15:0 of Command Dword 2 */
#define XNVME_SPEC_CP_EXECUTE_PROGRAM_CDW2_PIND 0xffff
/** Number of Memory Ranges (Computational Programs 1.3, 3.2.1, Figure 17, page 27, PDF page 27); bits 31:0 of Command Dword 3 */
#define XNVME_SPEC_CP_EXECUTE_PROGRAM_CDW3_NUMR 0xffffffff
/** DPTR Data Length (Computational Programs 1.3, 3.2.1, Figure 18, page 27, PDF page 27); bits 31:0 of Command Dword 4 */
#define XNVME_SPEC_CP_EXECUTE_PROGRAM_CDW4_DLEN 0xffffffff
/** Parameter Data (Computational Programs 1.3, 3.2.1, Figure 20, page 27, PDF page 27); bits 63:0 of Command Dwords 10 and 11, as one 64-bit value */
#define XNVME_SPEC_CP_EXECUTE_PROGRAM_CDW10_11_CPARAM1 0xffffffffffffffff
/** Parameter Data (Computational Programs 1.3, 3.2.1, Figure 21, page 27, PDF page 27); bits 63:0 of Command Dwords 12 and 13, as one 64-bit value */
#define XNVME_SPEC_CP_EXECUTE_PROGRAM_CDW12_13_CPARAM2 0xffffffffffffffff

#endif /* LIBXNVME_SPEC_CP_H */
