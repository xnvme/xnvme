/**
 * What the kv specification defines, by xnvme_spec
 *
 * Written from the data model of senfd 0.26.0, by toolbox/xnvme_spec_headers.py;
 * each name and mask is as the figure it cites gives it. Do not
 * edit: see the contributor docs on producing the spec headers.
 *
 * @file libxnvme_spec_kv.h
 */
#ifndef LIBXNVME_SPEC_KV_H
#define LIBXNVME_SPEC_KV_H

#include <stdint.h>

/**
 * The opcodes of the commands of the Key Value Command Set
 */
enum xnvme_spec_kv_opcode {
	XNVME_SPEC_KV_STORE_OPCODE = 0x1, ///< Store (Key Value 1.4, 3.2, Figure 6, page 14, PDF page 14)
	XNVME_SPEC_KV_RETRIEVE_OPCODE = 0x2, ///< Retrieve (Key Value 1.4, 3.2, Figure 6, page 14, PDF page 14)
	XNVME_SPEC_KV_DELETE_OPCODE = 0x10, ///< Delete (Key Value 1.4, 3.2, Figure 6, page 14, PDF page 14)
	XNVME_SPEC_KV_EXIST_OPCODE = 0x14, ///< Exist (Key Value 1.4, 3.2, Figure 6, page 14, PDF page 14)
	XNVME_SPEC_KV_LIST_OPCODE = 0x6, ///< List (Key Value 1.4, 3.2, Figure 6, page 14, PDF page 14)
};

/**
 * The status codes of the type Generic Command
 */
enum xnvme_spec_kv_generic_command_status {
	XNVME_SPEC_KV_GENERIC_COMMAND_STATUS_CAPACITY_EXCEEDED = 0x81, ///< Capacity Exceeded (Key Value 1.4, 3.1.2, Figure 5, page 13, PDF page 13)
	XNVME_SPEC_KV_GENERIC_COMMAND_STATUS_NAMESPACE_NOT_READY = 0x82, ///< Namespace Not Ready (Key Value 1.4, 3.1.2, Figure 5, page 13, PDF page 13)
	XNVME_SPEC_KV_GENERIC_COMMAND_STATUS_RESERVATION_CONFLICT = 0x83, ///< Reservation Conflict (Key Value 1.4, 3.1.2, Figure 5, page 13, PDF page 13)
	XNVME_SPEC_KV_GENERIC_COMMAND_STATUS_FORMAT_IN_PROGRESS = 0x84, ///< Format In Progress (Key Value 1.4, 3.1.2, Figure 5, page 13, PDF page 13)
	XNVME_SPEC_KV_GENERIC_COMMAND_STATUS_INVALID_VALUE_SIZE = 0x85, ///< Invalid Value Size (Key Value 1.4, 3.1.2, Figure 5, page 13, PDF page 13)
	XNVME_SPEC_KV_GENERIC_COMMAND_STATUS_INVALID_KEY_SIZE = 0x86, ///< Invalid Key Size (Key Value 1.4, 3.1.2, Figure 5, page 13, PDF page 13)
	XNVME_SPEC_KV_GENERIC_COMMAND_STATUS_KV_KEY_DOES_NOT_EXIST = 0x87, ///< KV Key Does Not Exist (Key Value 1.4, 3.1.2, Figure 5, page 13, PDF page 13)
	XNVME_SPEC_KV_GENERIC_COMMAND_STATUS_UNRECOVERED_ERROR = 0x88, ///< Unrecovered Error (Key Value 1.4, 3.1.2, Figure 5, page 13, PDF page 13)
	XNVME_SPEC_KV_GENERIC_COMMAND_STATUS_KEY_EXISTS = 0x89, ///< Key Exists (Key Value 1.4, 3.1.2, Figure 5, page 13, PDF page 13)
};

/**
 * Key (Key Value 1.4, 3.2.2.2, Figure 17, page 16, PDF page 16)
 *
 * Followed, from byte 2, by KV Key, at bytes n:02
 *
 * Not members:
 * Pad: bytes m:n+1, after the KV Key
 */
struct __attribute__((packed)) xnvme_spec_kv_key {
	uint16_t kl; ///< Key Length, bytes 1:0
};

/**
 * List - Return (Key Value 1.4, 3.2.2.2, Figure 16, page 16, PDF page 16)
 *
 * Followed, from byte 4, by Key Data Structure entries, the first at bytes Variable:04, counted by NRK, each a 'struct xnvme_spec_kv_key'
 *
 * Not members:
 * Key Data Structure 2: at an offset the data gives, one of the Key Data Structure entries
 * Key Data Structure NRK: at an offset the data gives, one of the Key Data Structure entries
 */
struct __attribute__((packed)) xnvme_spec_kv_return {
	uint32_t nrk; ///< Number of Returned Keys, bytes 3:0
};

/**
 * Error Information Log Entry (Key Value 1.4, 4.1.4.1, Figure 39, page 22, PDF page 22)
 *
 * Extends Error Information Log Entry (Base 2.4, 5.2.13.1.2, Figure 212, page 218, PDF page 244)
 */
struct __attribute__((packed)) xnvme_spec_kv_error_information_log_entry {
	uint8_t rsvd0[16]; ///< bytes 15:0, see Error Information Log Entry (Base 2.4, 5.2.13.1.2, Figure 212, page 218, PDF page 244)
	uint64_t lba; ///< Logical Block Address, bytes 23:16
};

/**
 * Self-test Results (Key Value 1.4, 4.1.4.2, Figure 40, page 22, PDF page 22)
 *
 * Extends Self-test Result (Base 2.4, 5.2.13.1.7, Figure 219, page 230, PDF page 257)
 */
struct __attribute__((packed)) xnvme_spec_kv_self_test_results {
	uint8_t rsvd0[16]; ///< bytes 15:0, see Self-test Result (Base 2.4, 5.2.13.1.7, Figure 219, page 230, PDF page 257)
	uint64_t flba; ///< Failing LBA, bytes 23:16
};

/**
 * KV Format (Key Value 1.4, 4.1.5.1, Figure 43, page 25, PDF page 25)
 */
struct __attribute__((packed)) xnvme_spec_kv_kv_format {
	uint16_t kvkml; ///< KV Key Max Length, bytes 1:0
	uint8_t rsvd2; ///< Reserved, byte 2
	uint8_t afo; ///< Additional Format Options, byte 3, see XNVME_SPEC_KV_KV_FORMAT_AFO_*
	uint32_t kvvml; ///< KV Value Max Length, bytes 7:4
	uint32_t mnks; ///< Max Num Keys, bytes 11:8
	uint32_t rsvd12; ///< Reserved, bytes 15:12
};

/** Relative Performance (Key Value 1.4, 4.1.5.1, Figure 43, page 25, PDF page 25); bits 1:0 of 'afo' */
#define XNVME_SPEC_KV_KV_FORMAT_AFO_RP 0x3

/**
 * The values of Relative Performance (Key Value 1.4, 4.1.5.1, Figure 43, page 25, PDF page 25)
 */
enum xnvme_spec_kv_rp {
	XNVME_SPEC_KV_RP_BEST_PERFORMANCE = 0x0, ///< Best performance (Key Value 1.4, 4.1.5.1, Figure 43, page 25, PDF page 25)
	XNVME_SPEC_KV_RP_BETTER_PERFORMANCE = 0x1, ///< Better performance (Key Value 1.4, 4.1.5.1, Figure 43, page 25, PDF page 25)
	XNVME_SPEC_KV_RP_GOOD_PERFORMANCE = 0x2, ///< Good performance (Key Value 1.4, 4.1.5.1, Figure 43, page 25, PDF page 25)
	XNVME_SPEC_KV_RP_DEGRADED_PERFORMANCE = 0x3, ///< Degraded performance (Key Value 1.4, 4.1.5.1, Figure 43, page 25, PDF page 25)
};

/**
 * Identify - I/O Command Set Specific Identify Namespace (Key Value 1.4, 4.1.5.1, Figure 42, page 23, PDF page 23)
 */
struct __attribute__((packed)) xnvme_spec_kv_identify_namespace {
	uint64_t nsze; ///< Namespace Size, bytes 7:0
	uint64_t rsvd8; ///< Reserved, bytes 15:8
	uint64_t nuse; ///< Namespace Utilization, bytes 23:16
	uint8_t nsfeat; ///< Namespace Features, byte 24, see XNVME_SPEC_KV_IDENTIFY_NAMESPACE_NSFEAT_*
	uint8_t nkvf; ///< Number of KV Formats, byte 25
	uint8_t nmic; ///< Namespace Multi-path I/O and Namespace Sharing Capabilities, byte 26
	uint8_t rescap; ///< Reservation Capabilities, byte 27
	uint8_t fpi; ///< Format Progress Indicator, byte 28
	uint8_t kvfc; ///< KV Format Capabilities, byte 29, see XNVME_SPEC_KV_IDENTIFY_NAMESPACE_KVFC_*
	uint16_t rsvd30; ///< Reserved, bytes 31:30
	uint32_t novg; ///< Namespace Optimal Value Granularity, bytes 35:32
	uint32_t anagrpid; ///< ANA Group Identifier, bytes 39:36
	uint8_t rsvd40[3]; ///< Reserved, bytes 42:40
	uint8_t nsattr; ///< Namespace Attributes, byte 43
	uint16_t nvmsetid; ///< NVM Set Identifier, bytes 45:44
	uint16_t endgid; ///< Endurance Group Identifier, bytes 47:46
	uint8_t nguid[16]; ///< Namespace Globally Unique Identifier, big-endian, bytes 63:48
	uint8_t eui64[8]; ///< IEEE Extended Unique Identifier, big-endian, bytes 71:64
	struct xnvme_spec_kv_kv_format kvf[16]; ///< KV Format Support, 16 of 16 bytes, bytes 327:72
	uint8_t rsvd328[3512]; ///< Reserved, bytes 3839:328
	uint8_t vs[256]; ///< Vendor Specific, bytes 4095:3840
};

/** Unique ID Reuse (Key Value 1.4, 4.1.5.1, Figure 42, page 23, PDF page 23); bit 3 of 'nsfeat' */
#define XNVME_SPEC_KV_IDENTIFY_NAMESPACE_NSFEAT_UIDR 0x8
/** KV Format Index (Key Value 1.4, 4.1.5.1, Figure 42, page 23, PDF page 23); bits 3:0 of 'kvfc' */
#define XNVME_SPEC_KV_IDENTIFY_NAMESPACE_KVFC_KVFI 0xf

/**
 * I/O Command Set Specific Identify Controller (Key Value 1.4, 4.1.5.2, Figure 44, page 26, PDF page 26)
 */
struct __attribute__((packed)) xnvme_spec_kv_identify_controller {
	uint32_t ver; ///< Version, bytes 3:0
	uint8_t rsvd4[4092]; ///< Reserved, bytes 4095:4
};

/**
 * Namespace Management - Host Specified Fields (Key Value 1.4, 4.1.6, Figure 47, page 28, PDF page 28)
 */
struct __attribute__((packed)) xnvme_spec_kv_host_specified_fields {
	uint64_t nsze; ///< Namespace Size, bytes 7:0
	uint8_t rsvd8[22]; ///< Reserved, bytes 29:8
	uint8_t nmic; ///< Namespace Multi-path I/O and Namespace Sharing Capabilities, byte 30
	uint8_t kvfc; ///< KV Format Capabilities, byte 31
	uint8_t rsvd32[60]; ///< Reserved, bytes 91:32
	uint32_t anagrpid; ///< ANA Group Identifier, bytes 95:92
	uint32_t rsvd96; ///< Reserved, bytes 99:96
	uint16_t nvmsetid; ///< NVM Set Identifier, bytes 101:100
	uint16_t endgid; ///< Endurance Group Identifier, bytes 103:102
	uint8_t rsvd104[408]; ///< Reserved, bytes 511:104
};

/*
 * The fields of Store, by the dwords of the command
 *
 * NOCOMP (bit 10), SINKE (bit 9) and SIKE (bit 8) within SO of Command Dword 11 have no masks: the data model gives them bits beyond the 8 bits of SO
 *
 * Without masks, as fields without an acronym:
 * KV Key[63:00] (Key Value 1.4, 3.2.5, Figure 33, page 20, PDF page 20); bits 63:0 of Command Dwords 2 and 3, as one 64-bit value
 * KV Key[127:64] (Key Value 1.4, 3.2.5, Figure 34, page 20, PDF page 20); bits 63:0 of Command Dwords 14 and 15, as one 64-bit value
 */
/** Value Size (Key Value 1.4, 3.2.5, Figure 30, page 19, PDF page 19); bits 31:0 of Command Dword 10 */
#define XNVME_SPEC_KV_STORE_CDW10_VS 0xffffffff
/** Store Option (Key Value 1.4, 3.2.5, Figure 31, page 19, PDF page 19); bits 15:8 of Command Dword 11 */
#define XNVME_SPEC_KV_STORE_CDW11_SO 0xff00
/** Key Length (Key Value 1.4, 3.2.5, Figure 31, page 19, PDF page 19); bits 7:0 of Command Dword 11 */
#define XNVME_SPEC_KV_STORE_CDW11_KL 0xff
/** Command Extension Type (Key Value 1.4, 3.2.5, Figure 32, page 19, PDF page 19); bits 19:16 of Command Dword 13 */
#define XNVME_SPEC_KV_STORE_CDW13_CETYPE 0xf0000
/** Command Extension Value (Key Value 1.4, 3.2.5, Figure 32, page 19, PDF page 19); bits 15:0 of Command Dword 13 */
#define XNVME_SPEC_KV_STORE_CDW13_CEV 0xffff

/*
 * The fields of Retrieve, by the dwords of the command
 *
 * RRD (bit 8) within RO of Command Dword 11 has no mask: the data model gives it bits beyond the 8 bits of RO
 *
 * Without masks, as fields without an acronym:
 * KV Key[63:00] (Key Value 1.4, 3.2.3, Figure 22, page 17, PDF page 17); bits 63:0 of Command Dwords 2 and 3, as one 64-bit value
 * KV Key[127:64] (Key Value 1.4, 3.2.3, Figure 23, page 18, PDF page 18); bits 63:0 of Command Dwords 14 and 15, as one 64-bit value
 */
/** Host Buffer Size (Key Value 1.4, 3.2.3, Figure 19, page 17, PDF page 17); bits 31:0 of Command Dword 10 */
#define XNVME_SPEC_KV_RETRIEVE_CDW10_HBS 0xffffffff
/** Retrieve Option (Key Value 1.4, 3.2.3, Figure 20, page 17, PDF page 17); bits 15:8 of Command Dword 11 */
#define XNVME_SPEC_KV_RETRIEVE_CDW11_RO 0xff00
/** Key Length (Key Value 1.4, 3.2.3, Figure 20, page 17, PDF page 17); bits 7:0 of Command Dword 11 */
#define XNVME_SPEC_KV_RETRIEVE_CDW11_KL 0xff
/** Command Extension Type (Key Value 1.4, 3.2.3, Figure 21, page 17, PDF page 17); bits 19:16 of Command Dword 13 */
#define XNVME_SPEC_KV_RETRIEVE_CDW13_CETYPE 0xf0000
/** Command Extension Value (Key Value 1.4, 3.2.3, Figure 21, page 17, PDF page 17); bits 15:0 of Command Dword 13 */
#define XNVME_SPEC_KV_RETRIEVE_CDW13_CEV 0xffff

/*
 * The fields of Delete, by the dwords of the command
 *
 * Without masks, as fields without an acronym:
 * KV Key[63:00] (Key Value 1.4, 3.2.1, Figure 8, page 15, PDF page 15); bits 63:0 of Command Dwords 2 and 3, as one 64-bit value
 * KV Key[127:64] (Key Value 1.4, 3.2.1, Figure 9, page 15, PDF page 15); bits 63:0 of Command Dwords 14 and 15, as one 64-bit value
 */
/** Key Length (Key Value 1.4, 3.2.1, Figure 7, page 15, PDF page 15); bits 7:0 of Command Dword 11 */
#define XNVME_SPEC_KV_DELETE_CDW11_KL 0xff

/*
 * The fields of Exist, by the dwords of the command
 *
 * Without masks, as fields without an acronym:
 * KV Key[63:00] (Key Value 1.4, 3.2.4, Figure 26, page 18, PDF page 18); bits 63:0 of Command Dwords 2 and 3, as one 64-bit value
 * KV Key[127:64] (Key Value 1.4, 3.2.4, Figure 27, page 18, PDF page 18); bits 63:0 of Command Dwords 14 and 15, as one 64-bit value
 */
/** Key Length (Key Value 1.4, 3.2.4, Figure 25, page 18, PDF page 18); bits 7:0 of Command Dword 11 */
#define XNVME_SPEC_KV_EXIST_CDW11_KL 0xff

/*
 * The fields of List, by the dwords of the command
 *
 * Without masks, as fields without an acronym:
 * KV Key[63:00] (Key Value 1.4, 3.2.2, Figure 13, page 16, PDF page 16); bits 63:0 of Command Dwords 2 and 3, as one 64-bit value
 * KV Key[127:64] (Key Value 1.4, 3.2.2, Figure 14, page 16, PDF page 16); bits 63:0 of Command Dwords 14 and 15, as one 64-bit value
 */
/** Host Buffer Size (Key Value 1.4, 3.2.2, Figure 11, page 15, PDF page 15); bits 31:0 of Command Dword 10 */
#define XNVME_SPEC_KV_LIST_CDW10_HBS 0xffffffff
/** Key Length (Key Value 1.4, 3.2.2, Figure 12, page 16, PDF page 16); bits 7:0 of Command Dword 11 */
#define XNVME_SPEC_KV_LIST_CDW11_KL 0xff

#endif /* LIBXNVME_SPEC_KV_H */
