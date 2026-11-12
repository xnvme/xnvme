/**
 * What the slm specification defines, by xnvme_spec
 *
 * Written from the data model of senfd 0.19.0, by toolbox/xnvme_spec_headers.py;
 * each name and mask is as the figure it cites gives it. Do not
 * edit: see the contributor docs on producing the spec headers.
 *
 * @file libxnvme_spec_slm.h
 */
#ifndef LIBXNVME_SPEC_SLM_H
#define LIBXNVME_SPEC_SLM_H

#include <stdint.h>

/**
 * The opcodes of the SLM commands
 */
enum xnvme_spec_slm_opcode {
	XNVME_SPEC_SLM_MEMORY_COPY_OPCODE = 0x1, ///< Memory Copy (Subsystem Local Memory 1.3, 3.2, Figure 7, page 15, PDF page 15)
	XNVME_SPEC_SLM_MEMORY_READ_OPCODE = 0x2, ///< Memory Read (Subsystem Local Memory 1.3, 3.2, Figure 7, page 15, PDF page 15)
	XNVME_SPEC_SLM_MEMORY_FILL_OPCODE = 0x4, ///< Memory Fill (Subsystem Local Memory 1.3, 3.2, Figure 7, page 15, PDF page 15)
	XNVME_SPEC_SLM_MEMORY_WRITE_OPCODE = 0x5, ///< Memory Write (Subsystem Local Memory 1.3, 3.2, Figure 7, page 15, PDF page 15)
};

/**
 * The status codes of the type Command Specific
 */
enum xnvme_spec_slm_command_specific_status {
	XNVME_SPEC_SLM_COMMAND_SPECIFIC_STATUS_NAMESPACE_NOT_REACHABLE = 0x88, ///< Namespace Not Reachable (Subsystem Local Memory 1.3, 3.1.2, Figure 6, page 15, PDF page 15)
};

/**
 * Memory Copy - Source Range Entries for Copy Descriptor Format 4h (Subsystem Local Memory 1.3, 3.2.1, Figure 13, page 18, PDF page 18)
 *
 * One entry of 32 bytes; the figure shows them at bytes 0, 32, 4064
 */
struct __attribute__((packed)) xnvme_spec_slm_source_range_entries_for_copy_descriptor_format_4h {
	uint32_t snsid; ///< Source Namespace Identifier, bytes 3:0
	uint32_t rsvd4; ///< Reserved, bytes 7:4
	uint64_t saddr; ///< Starting Address, bytes 15:8
	uint32_t nbyte; ///< Number of Bytes, bytes 19:16
	uint16_t rsvd20; ///< Reserved, bytes 21:20
	uint16_t sopt; ///< Source Options, bytes 23:22, see XNVME_SPEC_SLM_SOURCE_RANGE_ENTRIES_FOR_COPY_DESCRIPTOR_FORMAT_4H_SOPT_*
	uint64_t rsvd24; ///< Reserved, bytes 31:24
};

/** Fast Copy Only (Subsystem Local Memory 1.3, 3.2.1, Figure 13, page 18, PDF page 18); bit 15 of 'sopt' */
#define XNVME_SPEC_SLM_SOURCE_RANGE_ENTRIES_FOR_COPY_DESCRIPTOR_FORMAT_4H_SOPT_FCO 0x8000

/**
 * Addressable Namespaces (Subsystem Local Memory 1.3, 4.1.5.1, Figure 27, page 25, PDF page 25)
 */
struct __attribute__((packed)) xnvme_spec_slm_addressable_namespaces {
	uint32_t nsid; ///< Namespace Identifier, bytes 3:0
	uint8_t mat; ///< Memory Access Type, byte 4, see XNVME_SPEC_SLM_ADDRESSABLE_NAMESPACES_MAT_*
	uint8_t rsvd5[3]; ///< Reserved, bytes 7:5
	uint64_t memo; ///< Memory Offset, bytes 15:8
};

/** CXL Access (Subsystem Local Memory 1.3, 4.1.5.1, Figure 27, page 25, PDF page 25); bit 1 of 'mat' */
#define XNVME_SPEC_SLM_ADDRESSABLE_NAMESPACES_MAT_CXLA 0x2
/** BAR Access (Subsystem Local Memory 1.3, 4.1.5.1, Figure 27, page 25, PDF page 25); bit 0 of 'mat' */
#define XNVME_SPEC_SLM_ADDRESSABLE_NAMESPACES_MAT_BARA 0x1

/**
 * Host Addressable Namespaces (Subsystem Local Memory 1.3, 4.1.5.1, Figure 26, page 25, PDF page 25)
 *
 * Followed, from byte 16, by as many Addressable Namespace as the data says, each a 'struct xnvme_spec_slm_addressable_namespaces'
 */
struct __attribute__((packed)) xnvme_spec_slm_log_host_addressable_namespaces {
	uint16_t n; ///< Number of Entries, bytes 1:0
	uint8_t rsvd2[14]; ///< Reserved, bytes 15:2
};

/**
 * Format (Subsystem Local Memory 1.3, 4.1.6.1, Figure 30, page 28, PDF page 28), one entry of 16 bytes
 */
struct __attribute__((packed)) xnvme_spec_slm_format {
	uint8_t ds; ///< Data Size, byte 0
	uint8_t rsvd1[14]; ///< Reserved
	uint8_t val; ///< Valid, bit 7 of byte 15
};

/**
 * I/O Command Set Specific Identify Namespace (Subsystem Local Memory 1.3, 4.1.6.1, Figure 29, page 27, PDF page 27)
 */
struct __attribute__((packed)) xnvme_spec_slm_io_command_set_specific_identify_namespace {
	uint64_t nsze; ///< Namespace Size, bytes 7:0
	uint8_t nf; ///< Number of Formats, byte 8
	uint32_t nowg; ///< Namespace Optimal Write Granularity, bytes 12:9
	uint64_t mcmcl; ///< Memory Copy Maximum Copy Length, bytes 20:13
	uint32_t mcmssrl; ///< Memory Copy Maximum Single Source Range Length, bytes 24:21
	uint8_t mcmsrc; ///< Memory Copy Maximum Source Range Count, byte 25
	uint8_t rsvd26[486]; ///< Reserved, bytes 511:26
	struct xnvme_spec_slm_format f[32]; ///< SLM Format Support, 32 of 16 bytes, bytes 1023:512
	uint8_t rsvd1024[3072]; ///< Reserved, bytes 4095:1024
};

/**
 * Identify Controller (Subsystem Local Memory 1.3, 4.1.6.2, Figure 31, page 28, PDF page 28)
 */
struct __attribute__((packed)) xnvme_spec_slm_identify_controller {
	uint32_t ver; ///< Version, bytes 3:0
	uint8_t rsvd4[4092]; ///< Reserved, bytes 4095:4
};

/*
 * The fields of Memory Copy, by the dwords of the command
 */
/** Length (Subsystem Local Memory 1.3, 3.2.1, Figure 9, page 16, PDF page 16); bits 63:0 of Command Dwords 2 and 3, as one 64-bit value */
#define XNVME_SPEC_SLM_MEMORY_COPY_CDW2_3_LEN 0xffffffffffffffff
/** Starting Destination Address (Subsystem Local Memory 1.3, 3.2.1, Figure 10, page 17, PDF page 17); bits 63:0 of Command Dwords 10 and 11, as one 64-bit value */
#define XNVME_SPEC_SLM_MEMORY_COPY_CDW10_11_SDADDR 0xffffffffffffffff
/** Descriptor Format (Subsystem Local Memory 1.3, 3.2.1, Figure 11, page 17, PDF page 17); bits 11:8 of Command Dword 12 */
#define XNVME_SPEC_SLM_MEMORY_COPY_CDW12_DESFMT 0xf00
/** Number of Ranges (Subsystem Local Memory 1.3, 3.2.1, Figure 11, page 17, PDF page 17); bits 7:0 of Command Dword 12 */
#define XNVME_SPEC_SLM_MEMORY_COPY_CDW12_NR 0xff

/*
 * The fields of Memory Read, by the dwords of the command
 */
/** Starting Byte (Subsystem Local Memory 1.3, 3.2.3, Figure 21, page 23, PDF page 23); bits 63:0 of Command Dwords 10 and 11, as one 64-bit value */
#define XNVME_SPEC_SLM_MEMORY_READ_CDW10_11_SB 0xffffffffffffffff
/** Read Length (Subsystem Local Memory 1.3, 3.2.3, Figure 22, page 23, PDF page 23); bits 31:0 of Command Dword 12 */
#define XNVME_SPEC_SLM_MEMORY_READ_CDW12_RL 0xffffffff

/*
 * The fields of Memory Fill, by the dwords of the command
 */
/** Starting Byte (Subsystem Local Memory 1.3, 3.2.2, Figure 18, page 22, PDF page 22); bits 63:0 of Command Dwords 10 and 11, as one 64-bit value */
#define XNVME_SPEC_SLM_MEMORY_FILL_CDW10_11_SB 0xffffffffffffffff
/** Fill Length (Subsystem Local Memory 1.3, 3.2.2, Figure 19, page 23, PDF page 23); bits 31:0 of Command Dword 12 */
#define XNVME_SPEC_SLM_MEMORY_FILL_CDW12_FL 0xffffffff

/*
 * The fields of Memory Write, by the dwords of the command
 */
/** Starting Byte (Subsystem Local Memory 1.3, 3.2.4, Figure 24, page 24, PDF page 24); bits 63:0 of Command Dwords 10 and 11, as one 64-bit value */
#define XNVME_SPEC_SLM_MEMORY_WRITE_CDW10_11_SB 0xffffffffffffffff
/** Write Length (Subsystem Local Memory 1.3, 3.2.4, Figure 25, page 24, PDF page 24); bits 31:0 of Command Dword 12 */
#define XNVME_SPEC_SLM_MEMORY_WRITE_CDW12_WL 0xffffffff

#endif /* LIBXNVME_SPEC_SLM_H */
