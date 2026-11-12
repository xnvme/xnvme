/**
 * What the pcie specification defines, by xnvme_spec
 *
 * Written from the data model of senfd 0.19.0, by toolbox/xnvme_spec_headers.py;
 * each name and mask is as the figure it cites gives it. Do not
 * edit: see the contributor docs on producing the spec headers.
 *
 * @file libxnvme_spec_pcie.h
 */
#ifndef LIBXNVME_SPEC_PCIE_H
#define LIBXNVME_SPEC_PCIE_H

#include <stdint.h>

/**
 * Offset AERCAP + 1Ch: AERHL - AER Header Log Register (NVMe over PCIe Transport 1.4, 3.8.6.8, Figure 66, page 34, PDF page 34)
 */
struct __attribute__((packed)) xnvme_spec_pcie_aer_header_log_register {
	uint8_t hb3; ///< Header Byte 3, byte 0
	uint8_t hb2; ///< Header Byte 2, byte 1
	uint8_t hb1; ///< Header Byte 1, byte 2
	uint8_t hb0; ///< Header Byte 0, byte 3
	uint8_t hb7; ///< Header Byte 7, byte 4
	uint8_t hb6; ///< Header Byte 6, byte 5
	uint8_t hb5; ///< Header Byte 5, byte 6
	uint8_t hb4; ///< Header Byte 4, byte 7
	uint8_t hb11; ///< Header Byte 11, byte 8
	uint8_t hb10; ///< Header Byte 10, byte 9
	uint8_t hb9; ///< Header Byte 9, byte 10
	uint8_t hb8; ///< Header Byte 8, byte 11
	uint8_t hb15; ///< Header Byte 15, byte 12
	uint8_t hb14; ///< Header Byte 14, byte 13
	uint8_t hb13; ///< Header Byte 13, byte 14
	uint8_t hb12; ///< Header Byte 12, byte 15
};

/**
 * Offset AERCAP + 38h: AERTLP - AER TLP Prefix Log Register (Optional) (NVMe over PCIe Transport 1.4, 3.8.6.9, Figure 67, page 35, PDF page 35)
 */
struct __attribute__((packed)) xnvme_spec_pcie_aer_tlp_prefix_log_register {
	uint8_t tpl1b3; ///< First TLP Prefix Log Byte 3, byte 0
	uint8_t tpl1b2; ///< First TLP Prefix Log Byte 2, byte 1
	uint8_t tpl1b1; ///< First TLP Prefix Log Byte 1, byte 2
	uint8_t tpl1b0; ///< First TLP Prefix Log Byte 0, byte 3
	uint8_t tpl2b3; ///< Second TLP Prefix Log Byte 3, byte 4
	uint8_t tpl2b2; ///< Second TLP Prefix Log Byte 2, byte 5
	uint8_t tpl2b1; ///< Second TLP Prefix Log Byte 1, byte 6
	uint8_t tpl2b0; ///< Second TLP Prefix Log Byte 0, byte 7
	uint8_t tpl3b3; ///< Third TLP Prefix Log Byte 3, byte 8
	uint8_t tpl3b2; ///< Third TLP Prefix Log Byte 2, byte 9
	uint8_t tpl3b1; ///< Third TLP Prefix Log Byte 1, byte 10
	uint8_t tpl3b0; ///< Third TLP Prefix Log Byte 0, byte 11
	uint8_t tpl4b3; ///< Fourth TLP Prefix Log Byte 3, byte 12
	uint8_t tpl4b2; ///< Fourth TLP Prefix Log Byte 2, byte 13
	uint8_t tpl4b1; ///< Fourth TLP Prefix Log Byte 1, byte 14
	uint8_t tpl4b0; ///< Fourth TLP Prefix Log Byte 0, byte 15
};

/**
 * NVMe TDISP DEVICE_INTERFACE_REPORT Reporting (NVMe over PCIe Transport 1.4, 3.8.10, Figure 69, page 38, PDF page 38)
 */
struct __attribute__((packed)) xnvme_spec_pcie_nvme_tdisp_device_interface_report_reporting {
	uint32_t tni; ///< TDISP NVMe Identifier, bytes 3:0
	uint32_t ti; ///< TDISP Information, bytes 7:4, see XNVME_SPEC_PCIE_NVME_TDISP_DEVICE_INTERFACE_REPORT_REPORTING_TI_*
	uint8_t cu; ///< CSVL Used, byte 8
	uint8_t vu; ///< VER Used, byte 9
	uint16_t rsvd10; ///< Reserved, bytes 11:10
	uint32_t aqv; ///< AQA Value, bytes 15:12
	uint32_t asv; ///< ASQ Value, bytes 19:16
	uint32_t acv; ///< ACQ Value, bytes 23:20
	uint32_t cv; ///< CMBMSC Value, bytes 27:24
	uint32_t pmv; ///< PMRMSC Value, bytes 31:28
	uint32_t pcv; ///< PMRCTL Value, bytes 35:32
	uint8_t qh[48]; ///< Queue Hash, bytes 83:36
	uint8_t rsvd84[44]; ///< Reserved, bytes 127:84
};

/** I/O Completion Queue Entry Size Value (NVMe over PCIe Transport 1.4, 3.8.10, Figure 69, page 38, PDF page 38); bits 31:28 of 'ti' */
#define XNVME_SPEC_PCIE_NVME_TDISP_DEVICE_INTERFACE_REPORT_REPORTING_TI_IOCQESV 0xf0000000
/** I/O Submission Queue Entry Size Value (NVMe over PCIe Transport 1.4, 3.8.10, Figure 69, page 38, PDF page 38); bits 27:24 of 'ti' */
#define XNVME_SPEC_PCIE_NVME_TDISP_DEVICE_INTERFACE_REPORT_REPORTING_TI_IOSQESV 0xf000000
/** Memory Page Size Value (NVMe over PCIe Transport 1.4, 3.8.10, Figure 69, page 38, PDF page 38); bits 23:20 of 'ti' */
#define XNVME_SPEC_PCIE_NVME_TDISP_DEVICE_INTERFACE_REPORT_REPORTING_TI_MPSV 0xf00000
/** Command Sets Supported Value (NVMe over PCIe Transport 1.4, 3.8.10, Figure 69, page 38, PDF page 38); bits 18:16 of 'ti' */
#define XNVME_SPEC_PCIE_NVME_TDISP_DEVICE_INTERFACE_REPORT_REPORTING_TI_CSSV 0x70000
/** Shutdown Type Value (NVMe over PCIe Transport 1.4, 3.8.10, Figure 69, page 38, PDF page 38); bit 14 of 'ti' */
#define XNVME_SPEC_PCIE_NVME_TDISP_DEVICE_INTERFACE_REPORT_REPORTING_TI_STV 0x4000
/** Shutdown Status Value (NVMe over PCIe Transport 1.4, 3.8.10, Figure 69, page 38, PDF page 38); bits 13:12 of 'ti' */
#define XNVME_SPEC_PCIE_NVME_TDISP_DEVICE_INTERFACE_REPORT_REPORTING_TI_SHSTV 0x3000
/** Shutdown Notification Value (NVMe over PCIe Transport 1.4, 3.8.10, Figure 69, page 38, PDF page 38); bits 11:10 of 'ti' */
#define XNVME_SPEC_PCIE_NVME_TDISP_DEVICE_INTERFACE_REPORT_REPORTING_TI_SHNV 0xc00
/** Enable Value (NVMe over PCIe Transport 1.4, 3.8.10, Figure 69, page 38, PDF page 38); bit 9 of 'ti' */
#define XNVME_SPEC_PCIE_NVME_TDISP_DEVICE_INTERFACE_REPORT_REPORTING_TI_ENV 0x200
/** Admin Command Processing Status (NVMe over PCIe Transport 1.4, 3.8.10, Figure 69, page 38, PDF page 38); bit 8 of 'ti' */
#define XNVME_SPEC_PCIE_NVME_TDISP_DEVICE_INTERFACE_REPORT_REPORTING_TI_ACPS 0x100
/** Version (NVMe over PCIe Transport 1.4, 3.8.10, Figure 69, page 38, PDF page 38); bits 7:0 of 'ti' */
#define XNVME_SPEC_PCIE_NVME_TDISP_DEVICE_INTERFACE_REPORT_REPORTING_TI_VER 0xff

/**
 * EOM Header (NVMe over PCIe Transport 1.4, 3.9.1.1, Figure 75, page 42, PDF page 42)
 */
struct __attribute__((packed)) xnvme_spec_pcie_eom_header {
	uint8_t lid; ///< Log Page Identifier, byte 0
	uint8_t eomip; ///< EOM In Progress, byte 1
	uint16_t hsize; ///< Header Size, bytes 3:2
	uint32_t rsz; ///< Result Size, bytes 7:4
	uint8_t edgn; ///< EOM Data Generation Number, byte 8
	uint8_t lrev; ///< Log Revision, byte 9
	uint8_t odp; ///< Optional Data Present, byte 10, see XNVME_SPEC_PCIE_EOM_HEADER_ODP_*
	uint8_t lns; ///< Lanes, byte 11
	uint8_t epl; ///< Eyes Per Lane, byte 12
	uint8_t lspfc; ///< Log Specific Parameter Field Copy, byte 13, see XNVME_SPEC_PCIE_EOM_HEADER_LSPFC_*
	uint8_t linfo; ///< Link Information, byte 14, see XNVME_SPEC_PCIE_EOM_HEADER_LINFO_*
	uint8_t rsvd15[3]; ///< Reserved, bytes 17:15
	uint16_t lsic; ///< Log Specific Identifier Copy, bytes 19:18
	uint32_t ds; ///< Descriptor Size, bytes 23:20
	uint16_t nd; ///< Number of Descriptors, bytes 25:24
	uint16_t maxtb; ///< Maximum Top Bottom, bytes 27:26
	uint16_t maxlr; ///< Maximum Left Right, bytes 29:28
	uint16_t etgood; ///< Estimated Time for Good Quality, bytes 31:30
	uint16_t etbetter; ///< Estimated Time for Better Quality, bytes 33:32
	uint16_t etbest; ///< Estimated Time for Best Quality, bytes 35:34
	uint8_t rsvd36[28]; ///< Reserved, bytes 63:36
};

/** Eye Data Field Present (NVMe over PCIe Transport 1.4, 3.9.1.1, Figure 75, page 42, PDF page 42); bit 1 of 'odp' */
#define XNVME_SPEC_PCIE_EOM_HEADER_ODP_EDFP 0x2
/** Printable Eye Field Present (NVMe over PCIe Transport 1.4, 3.9.1.1, Figure 75, page 42, PDF page 42); bit 0 of 'odp' */
#define XNVME_SPEC_PCIE_EOM_HEADER_ODP_PEFP 0x1
/** Log Specific Parameter Field Value (NVMe over PCIe Transport 1.4, 3.9.1.1, Figure 75, page 42, PDF page 42); bits 6:0 of 'lspfc' */
#define XNVME_SPEC_PCIE_EOM_HEADER_LSPFC_LSPFV 0x7f
/** Measurement Link Speed (NVMe over PCIe Transport 1.4, 3.9.1.1, Figure 75, page 42, PDF page 42); bits 3:0 of 'linfo' */
#define XNVME_SPEC_PCIE_EOM_HEADER_LINFO_MLS 0xf

/**
 * EOM Lane Descriptor (NVMe over PCIe Transport 1.4, 3.9.1.1, Figure 76, page 43, PDF page 43)
 *
 * Followed, from byte 32, by as many Printable Eye as the data says
 *
 * Not members:
 * Eye Data: at an offset the data gives, in 'printable_eye'
 * Padding: at an offset the data gives, in 'printable_eye'
 */
struct __attribute__((packed)) xnvme_spec_pcie_eom_lane_descriptor {
	uint8_t rsvd0; ///< Reserved, byte 0
	uint8_t mstat; ///< Measurement Status, byte 1, see XNVME_SPEC_PCIE_EOM_LANE_DESCRIPTOR_MSTAT_*
	uint8_t ln; ///< Lane, byte 2
	uint8_t eye; ///< Eye, byte 3
	uint16_t top; ///< Top, bytes 5:4
	uint16_t btm; ///< Bottom, bytes 7:6
	uint16_t lft; ///< Left, bytes 9:8
	uint16_t rgt; ///< Right, bytes 11:10
	uint16_t nrows; ///< Number of Rows, bytes 13:12
	uint16_t ncols; ///< Number of Columns, bytes 15:14
	uint32_t edlen; ///< Eye Data Length, bytes 19:16
	uint8_t rsvd20[12]; ///< Reserved, bytes 31:20
};

/** Measurement Successful (NVMe over PCIe Transport 1.4, 3.9.1.1, Figure 76, page 43, PDF page 43); bit 0 of 'mstat' */
#define XNVME_SPEC_PCIE_EOM_LANE_DESCRIPTOR_MSTAT_MSCS 0x1

/**
 * The registers of the pcie specification, at their offsets
 *
 * Not members:
 * CCPTR - CardBus CIS Pointer: at no offset of a number
 */
struct __attribute__((packed)) xnvme_spec_pcie_registers {
	uint32_t id; ///< ID - Identifiers (NVMe over PCIe Transport 1.4, 3.8.1.1, Figure 11, page 17, PDF page 17), at 0h, see XNVME_SPEC_PCIE_REGISTERS_ID_*
	uint16_t cmd; ///< CMD - Command (NVMe over PCIe Transport 1.4, 3.8.1.2, Figure 12, page 17, PDF page 17), at 4h, see XNVME_SPEC_PCIE_REGISTERS_CMD_*
	uint16_t sts; ///< STS - Device Status (NVMe over PCIe Transport 1.4, 3.8.1.3, Figure 13, page 18, PDF page 18), at 6h, see XNVME_SPEC_PCIE_REGISTERS_STS_*
	uint8_t rid; ///< RID - Revision ID (NVMe over PCIe Transport 1.4, 3.8.1.4, Figure 14, page 18, PDF page 18), at 8h, see XNVME_SPEC_PCIE_REGISTERS_RID_*
	uint8_t cc[3]; ///< CC - Class Code (NVMe over PCIe Transport 1.4, 3.8.1.5, Figure 15, page 18, PDF page 18), at 9h
	uint8_t cls; ///< CLS - Cache Line Size (NVMe over PCIe Transport 1.4, 3.8.1.6, Figure 16, page 18, PDF page 18), at Ch, see XNVME_SPEC_PCIE_REGISTERS_CLS_*
	uint8_t mlt; ///< MLT - Master Latency Timer (NVMe over PCIe Transport 1.4, 3.8.1.7, Figure 17, page 18, PDF page 18), at Dh, see XNVME_SPEC_PCIE_REGISTERS_MLT_*
	uint8_t htype; ///< HTYPE - Header Type (NVMe over PCIe Transport 1.4, 3.8.1.8, Figure 18, page 19, PDF page 19), at Eh, see XNVME_SPEC_PCIE_REGISTERS_HTYPE_*
	uint8_t bist; ///< BIST - Built-In Self Test (Optional) (NVMe over PCIe Transport 1.4, 3.8.1.9, Figure 19, page 19, PDF page 19), at Fh, see XNVME_SPEC_PCIE_REGISTERS_BIST_*
	uint32_t mlbar; ///< MLBAR (BAR0) - Memory Register Base Address, lower 32-bits (NVMe over PCIe Transport 1.4, 3.8.1.10, Figure 20, page 19, PDF page 19), at 10h, see XNVME_SPEC_PCIE_REGISTERS_MLBAR_*
	uint32_t mubar; ///< MUBAR (BAR1) - Memory Register Base Address, upper 32-bits (NVMe over PCIe Transport 1.4, 3.8.1.11, Figure 21, page 19, PDF page 19), at 14h, see XNVME_SPEC_PCIE_REGISTERS_MUBAR_*
	uint32_t bar2; ///< BAR2 - Index/Data Pair Register Base Address or Vendor Specific (Optional) (NVMe over PCIe Transport 1.4, 3.8.1.12, Figure 22, page 20, PDF page 20), at 18h, see XNVME_SPEC_PCIE_REGISTERS_BAR2_*
	uint8_t rsvd1c[16]; ///< Reserved, 1Ch to 2Bh
	uint32_t ss; ///< SS - Subsystem Identifiers (NVMe over PCIe Transport 1.4, 3.8.1.17, Figure 24, page 20, PDF page 20), at 2Ch, see XNVME_SPEC_PCIE_REGISTERS_SS_*
	uint32_t erom; ///< EROM - Expansion ROM (Optional) (NVMe over PCIe Transport 1.4, 3.8.1.18, Figure 25, page 20, PDF page 20), at 30h, see XNVME_SPEC_PCIE_REGISTERS_EROM_*
	uint8_t cap; ///< CAP - Capabilities Pointer (NVMe over PCIe Transport 1.4, 3.8.1.19, Figure 26, page 21, PDF page 21), at 34h, see XNVME_SPEC_PCIE_REGISTERS_CAP_*
	uint8_t rsvd35[7]; ///< Reserved, 35h to 3Bh
	uint16_t intr; ///< INTR - Interrupt Information (NVMe over PCIe Transport 1.4, 3.8.1.20, Figure 27, page 21, PDF page 21), at 3Ch, see XNVME_SPEC_PCIE_REGISTERS_INTR_*
	uint8_t mgnt; ///< MGNT - Minimum Grant (NVMe over PCIe Transport 1.4, 3.8.1.21, Figure 28, page 21, PDF page 21), at 3Eh, see XNVME_SPEC_PCIE_REGISTERS_MGNT_*
	uint8_t mlat; ///< MLAT - Maximum Latency (NVMe over PCIe Transport 1.4, 3.8.1.22, Figure 29, page 21, PDF page 21), at 3Fh, see XNVME_SPEC_PCIE_REGISTERS_MLAT_*
};

/** Device ID (NVMe over PCIe Transport 1.4, 3.8.1.1, Figure 11, page 17, PDF page 17); bits 31:16 of 'id' */
#define XNVME_SPEC_PCIE_REGISTERS_ID_DID 0xffff0000
/** Vendor ID (NVMe over PCIe Transport 1.4, 3.8.1.1, Figure 11, page 17, PDF page 17); bits 15:0 of 'id' */
#define XNVME_SPEC_PCIE_REGISTERS_ID_VID 0xffff
/** Interrupt Disable (NVMe over PCIe Transport 1.4, 3.8.1.2, Figure 12, page 17, PDF page 17); bit 10 of 'cmd' */
#define XNVME_SPEC_PCIE_REGISTERS_CMD_ID 0x400
/** Fast Back-to-Back Enable (NVMe over PCIe Transport 1.4, 3.8.1.2, Figure 12, page 17, PDF page 17); bit 9 of 'cmd' */
#define XNVME_SPEC_PCIE_REGISTERS_CMD_FBE 0x200
/** SERR# Enable (NVMe over PCIe Transport 1.4, 3.8.1.2, Figure 12, page 17, PDF page 17); bit 8 of 'cmd' */
#define XNVME_SPEC_PCIE_REGISTERS_CMD_SEE 0x100
/** IDSEL Stepping/Wait Cycle Control (NVMe over PCIe Transport 1.4, 3.8.1.2, Figure 12, page 17, PDF page 17); bit 7 of 'cmd' */
#define XNVME_SPEC_PCIE_REGISTERS_CMD_ISWCC 0x80
/** Parity Error Response Enable (NVMe over PCIe Transport 1.4, 3.8.1.2, Figure 12, page 17, PDF page 17); bit 6 of 'cmd' */
#define XNVME_SPEC_PCIE_REGISTERS_CMD_PEE 0x40
/** VGA Palette Snooping Enable (NVMe over PCIe Transport 1.4, 3.8.1.2, Figure 12, page 17, PDF page 17); bit 5 of 'cmd' */
#define XNVME_SPEC_PCIE_REGISTERS_CMD_VGA 0x20
/** Memory Write and Invalidate Enable (NVMe over PCIe Transport 1.4, 3.8.1.2, Figure 12, page 17, PDF page 17); bit 4 of 'cmd' */
#define XNVME_SPEC_PCIE_REGISTERS_CMD_MWIE 0x10
/** Special Cycle Enable (NVMe over PCIe Transport 1.4, 3.8.1.2, Figure 12, page 17, PDF page 17); bit 3 of 'cmd' */
#define XNVME_SPEC_PCIE_REGISTERS_CMD_SCE 0x8
/** Bus Master Enable (NVMe over PCIe Transport 1.4, 3.8.1.2, Figure 12, page 17, PDF page 17); bit 2 of 'cmd' */
#define XNVME_SPEC_PCIE_REGISTERS_CMD_BME 0x4
/** Memory Space Enable (NVMe over PCIe Transport 1.4, 3.8.1.2, Figure 12, page 17, PDF page 17); bit 1 of 'cmd' */
#define XNVME_SPEC_PCIE_REGISTERS_CMD_MSE 0x2
/** I/O Space Enable (NVMe over PCIe Transport 1.4, 3.8.1.2, Figure 12, page 17, PDF page 17); bit 0 of 'cmd' */
#define XNVME_SPEC_PCIE_REGISTERS_CMD_IOSE 0x1
/** Detected Parity Error (NVMe over PCIe Transport 1.4, 3.8.1.3, Figure 13, page 18, PDF page 18); bit 15 of 'sts' */
#define XNVME_SPEC_PCIE_REGISTERS_STS_DPE 0x8000
/** Signaled System Error (NVMe over PCIe Transport 1.4, 3.8.1.3, Figure 13, page 18, PDF page 18); bit 14 of 'sts' */
#define XNVME_SPEC_PCIE_REGISTERS_STS_SSE 0x4000
/** Received Master-Abort (NVMe over PCIe Transport 1.4, 3.8.1.3, Figure 13, page 18, PDF page 18); bit 13 of 'sts' */
#define XNVME_SPEC_PCIE_REGISTERS_STS_RMA 0x2000
/** Received Target Abort (NVMe over PCIe Transport 1.4, 3.8.1.3, Figure 13, page 18, PDF page 18); bit 12 of 'sts' */
#define XNVME_SPEC_PCIE_REGISTERS_STS_RTA 0x1000
/** Signaled Target-Abort (NVMe over PCIe Transport 1.4, 3.8.1.3, Figure 13, page 18, PDF page 18); bit 11 of 'sts' */
#define XNVME_SPEC_PCIE_REGISTERS_STS_STA 0x800
/** DEVSEL# Timing (NVMe over PCIe Transport 1.4, 3.8.1.3, Figure 13, page 18, PDF page 18); bits 10:9 of 'sts' */
#define XNVME_SPEC_PCIE_REGISTERS_STS_DEVT 0x600
/** Master Data Parity Error Detected (NVMe over PCIe Transport 1.4, 3.8.1.3, Figure 13, page 18, PDF page 18); bit 8 of 'sts' */
#define XNVME_SPEC_PCIE_REGISTERS_STS_DPD 0x100
/** Fast Back-to-Back Capable (NVMe over PCIe Transport 1.4, 3.8.1.3, Figure 13, page 18, PDF page 18); bit 7 of 'sts' */
#define XNVME_SPEC_PCIE_REGISTERS_STS_FBC 0x80
/** 66 MHz Capable (NVMe over PCIe Transport 1.4, 3.8.1.3, Figure 13, page 18, PDF page 18); bit 5 of 'sts' */
#define XNVME_SPEC_PCIE_REGISTERS_STS_C66 0x20
/** Capabilities List (NVMe over PCIe Transport 1.4, 3.8.1.3, Figure 13, page 18, PDF page 18); bit 4 of 'sts' */
#define XNVME_SPEC_PCIE_REGISTERS_STS_CL 0x10
/** Interrupt Status (NVMe over PCIe Transport 1.4, 3.8.1.3, Figure 13, page 18, PDF page 18); bit 3 of 'sts' */
#define XNVME_SPEC_PCIE_REGISTERS_STS_IS 0x8
/** Immediate Readiness (NVMe over PCIe Transport 1.4, 3.8.1.3, Figure 13, page 18, PDF page 18); bit 0 of 'sts' */
#define XNVME_SPEC_PCIE_REGISTERS_STS_IR 0x1
/** Revision ID (NVMe over PCIe Transport 1.4, 3.8.1.4, Figure 14, page 18, PDF page 18); bits 7:0 of 'rid' */
#define XNVME_SPEC_PCIE_REGISTERS_RID_RID 0xff
/** Cache Line Size (NVMe over PCIe Transport 1.4, 3.8.1.6, Figure 16, page 18, PDF page 18); bits 7:0 of 'cls' */
#define XNVME_SPEC_PCIE_REGISTERS_CLS_CLS 0xff
/** Master Latency Timer (NVMe over PCIe Transport 1.4, 3.8.1.7, Figure 17, page 18, PDF page 18); bits 7:0 of 'mlt' */
#define XNVME_SPEC_PCIE_REGISTERS_MLT_MLT 0xff
/** Multi-Function Device (NVMe over PCIe Transport 1.4, 3.8.1.8, Figure 18, page 19, PDF page 19); bit 7 of 'htype' */
#define XNVME_SPEC_PCIE_REGISTERS_HTYPE_MFD 0x80
/** Header Layout (NVMe over PCIe Transport 1.4, 3.8.1.8, Figure 18, page 19, PDF page 19); bits 6:0 of 'htype' */
#define XNVME_SPEC_PCIE_REGISTERS_HTYPE_HL 0x7f
/** BIST Capable (NVMe over PCIe Transport 1.4, 3.8.1.9, Figure 19, page 19, PDF page 19); bit 7 of 'bist' */
#define XNVME_SPEC_PCIE_REGISTERS_BIST_BC 0x80
/** Start BIST (NVMe over PCIe Transport 1.4, 3.8.1.9, Figure 19, page 19, PDF page 19); bit 6 of 'bist' */
#define XNVME_SPEC_PCIE_REGISTERS_BIST_SB 0x40
/** Completion Code (NVMe over PCIe Transport 1.4, 3.8.1.9, Figure 19, page 19, PDF page 19); bits 3:0 of 'bist' */
#define XNVME_SPEC_PCIE_REGISTERS_BIST_CC 0xf
/** Base Address (NVMe over PCIe Transport 1.4, 3.8.1.10, Figure 20, page 19, PDF page 19); bits 31:14 of 'mlbar' */
#define XNVME_SPEC_PCIE_REGISTERS_MLBAR_BA 0xffffc000
/** Prefetchable (NVMe over PCIe Transport 1.4, 3.8.1.10, Figure 20, page 19, PDF page 19); bit 3 of 'mlbar' */
#define XNVME_SPEC_PCIE_REGISTERS_MLBAR_PF 0x8
/** Type (NVMe over PCIe Transport 1.4, 3.8.1.10, Figure 20, page 19, PDF page 19); bits 2:1 of 'mlbar' */
#define XNVME_SPEC_PCIE_REGISTERS_MLBAR_TP 0x6
/** Resource Type Indicator (NVMe over PCIe Transport 1.4, 3.8.1.10, Figure 20, page 19, PDF page 19); bit 0 of 'mlbar' */
#define XNVME_SPEC_PCIE_REGISTERS_MLBAR_RTE 0x1
/** Base Address (NVMe over PCIe Transport 1.4, 3.8.1.11, Figure 21, page 19, PDF page 19); bits 31:0 of 'mubar' */
#define XNVME_SPEC_PCIE_REGISTERS_MUBAR_BA 0xffffffff
/** Base Address (NVMe over PCIe Transport 1.4, 3.8.1.12, Figure 22, page 20, PDF page 20); bits 31:3 of 'bar2' */
#define XNVME_SPEC_PCIE_REGISTERS_BAR2_BA 0xfffffff8
/** Resource Type Indicator (NVMe over PCIe Transport 1.4, 3.8.1.12, Figure 22, page 20, PDF page 20); bit 0 of 'bar2' */
#define XNVME_SPEC_PCIE_REGISTERS_BAR2_RTE 0x1
/** Subsystem ID (NVMe over PCIe Transport 1.4, 3.8.1.17, Figure 24, page 20, PDF page 20); bits 31:16 of 'ss' */
#define XNVME_SPEC_PCIE_REGISTERS_SS_SSID 0xffff0000
/** Subsystem Vendor ID (NVMe over PCIe Transport 1.4, 3.8.1.17, Figure 24, page 20, PDF page 20); bits 15:0 of 'ss' */
#define XNVME_SPEC_PCIE_REGISTERS_SS_SSVID 0xffff
/** ROM Base Address (NVMe over PCIe Transport 1.4, 3.8.1.18, Figure 25, page 20, PDF page 20); bits 31:0 of 'erom' */
#define XNVME_SPEC_PCIE_REGISTERS_EROM_RBA 0xffffffff
/** Capability Pointer (NVMe over PCIe Transport 1.4, 3.8.1.19, Figure 26, page 21, PDF page 21); bits 7:0 of 'cap' */
#define XNVME_SPEC_PCIE_REGISTERS_CAP_CP 0xff
/** Interrupt Pin (NVMe over PCIe Transport 1.4, 3.8.1.20, Figure 27, page 21, PDF page 21); bits 15:8 of 'intr' */
#define XNVME_SPEC_PCIE_REGISTERS_INTR_IPIN 0xff00
/** Interrupt Line (NVMe over PCIe Transport 1.4, 3.8.1.20, Figure 27, page 21, PDF page 21); bits 7:0 of 'intr' */
#define XNVME_SPEC_PCIE_REGISTERS_INTR_ILINE 0xff
/** Grant (NVMe over PCIe Transport 1.4, 3.8.1.21, Figure 28, page 21, PDF page 21); bits 7:0 of 'mgnt' */
#define XNVME_SPEC_PCIE_REGISTERS_MGNT_GNT 0xff
/** Latency (NVMe over PCIe Transport 1.4, 3.8.1.22, Figure 29, page 21, PDF page 21); bits 7:0 of 'mlat' */
#define XNVME_SPEC_PCIE_REGISTERS_MLAT_LAT 0xff

#endif /* LIBXNVME_SPEC_PCIE_H */
