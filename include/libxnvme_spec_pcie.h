/**
 * What the pcie specification defines, by xnvme_spec
 *
 * Written from the data model of senfd 0.26.0, by toolbox/xnvme_spec_headers.py;
 * each name and mask is as the figure it cites gives it. Do not
 * edit: see the contributor docs on producing the spec headers.
 *
 * @file libxnvme_spec_pcie.h
 */
#ifndef LIBXNVME_SPEC_PCIE_H
#define LIBXNVME_SPEC_PCIE_H

#include <stdint.h>

/**
 * The identifiers of the log pages
 */
enum xnvme_spec_pcie_log_page_identifier {
	XNVME_SPEC_PCIE_LOG_PAGE_PHYSICAL_INTERFACE_RECEIVER_EYE_OPENING_MEASUREMENT = 0x19, ///< Physical Interface Receiver Eye Opening Measurement (NVMe over PCIe Transport 1.4, 3.9.1, Figure 70, page 39, PDF page 39)
};

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
 * The values of EOM In Progress (NVMe over PCIe Transport 1.4, 3.9.1.1, Figure 75, page 42, PDF page 42)
 */
enum xnvme_spec_pcie_eomip {
	XNVME_SPEC_PCIE_EOMIP_A_MEASUREMENT_IS_IN_PROGRESS = 0x1, ///< A measurement is in progress (NVMe over PCIe Transport 1.4, 3.9.1.1, Figure 75, page 42, PDF page 42)
};

/**
 * EOM Lane Descriptor (NVMe over PCIe Transport 1.4, 3.9.1.1, Figure 76, page 43, PDF page 43)
 *
 * Followed, from byte 32, by Printable Eye, at bytes (NROWS*NCOLS+32)-1:32
 *
 * Not members:
 * Eye Data (ED): bytes (NROWS*NCOLS+32)+EDLEN-1: (NROWS*NCOLS+32), after the Printable Eye
 * Padding (PAD): bytes (DS-1): (NROWS*NCOLS+32)+EDLEN, after the Printable Eye
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
 * Offset (1000h + ((2y) * (4 << CAP.DSTRD))): SQyTDBL - Submission Queue y Tail Doorbell (NVMe over PCIe Transport 1.4, 3.1.2.1, Figure 5, page 10, PDF page 10): at (1000h + ((2y) * (4 << CAP.DSTRD)))
 * Offset (1000h + ((2y + 1) * (4 << CAP.DSTRD))): CQyHDBL - Completion Queue y Head Doorbell (NVMe over PCIe Transport 1.4, 3.1.2.2, Figure 6, page 10, PDF page 10): at (1000h + ((2y + 1) * (4 << CAP.DSTRD)))
 *
 * Without masks, as fields of an array of bytes:
 * Base Class Code (BCC) (NVMe over PCIe Transport 1.4, 3.8.1.5, Figure 15, page 18, PDF page 18); bits 23:16 of 'cc'
 * Sub Class Code (SCC) (NVMe over PCIe Transport 1.4, 3.8.1.5, Figure 15, page 18, PDF page 18); bits 15:8 of 'cc'
 * Programming Interface (PI) (NVMe over PCIe Transport 1.4, 3.8.1.5, Figure 15, page 18, PDF page 18); bits 7:0 of 'cc'
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
	uint8_t rsvd1c[12]; ///< Reserved, 1Ch to 27h
	uint32_t ccptr; ///< CCPTR - CardBus CIS Pointer (NVMe over PCIe Transport 1.4, 3.8.1.16, Figure 23, page 20, PDF page 20), at 28h, see XNVME_SPEC_PCIE_REGISTERS_CCPTR_*
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
/** CardBus CIS Pointer (NVMe over PCIe Transport 1.4, 3.8.1.16, Figure 23, page 20, PDF page 20); bits 31:0 of 'ccptr' */
#define XNVME_SPEC_PCIE_REGISTERS_CCPTR_CCPTR 0xffffffff
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

/**
 * The registers of PMCAP, at their offsets from PMCAP
 */
struct __attribute__((packed)) xnvme_spec_pcie_pmcap_registers {
	uint16_t pid; ///< Offset PMCAP: PID - PCI Power Management Capability ID (NVMe over PCIe Transport 1.4, 3.8.2.1, Figure 31, page 21, PDF page 21), at 0h, see XNVME_SPEC_PCIE_PMCAP_REGISTERS_PID_*
	uint16_t pc; ///< Offset PMCAP + 2h: PC - PCI Power Management Capabilities (NVMe over PCIe Transport 1.4, 3.8.2.2, Figure 32, page 22, PDF page 22), at 2h, see XNVME_SPEC_PCIE_PMCAP_REGISTERS_PC_*
	uint16_t pmcs; ///< Offset PMCAP + 4h: PMCS - PCI Power Management Control and Status (NVMe over PCIe Transport 1.4, 3.8.2.3, Figure 33, page 22, PDF page 22), at 4h, see XNVME_SPEC_PCIE_PMCAP_REGISTERS_PMCS_*
};

/** Next Capability (NVMe over PCIe Transport 1.4, 3.8.2.1, Figure 31, page 21, PDF page 21); bits 15:8 of 'pid' */
#define XNVME_SPEC_PCIE_PMCAP_REGISTERS_PID_NEXT 0xff00
/** Cap ID (NVMe over PCIe Transport 1.4, 3.8.2.1, Figure 31, page 21, PDF page 21); bits 7:0 of 'pid' */
#define XNVME_SPEC_PCIE_PMCAP_REGISTERS_PID_CID 0xff
/** PME_Support (NVMe over PCIe Transport 1.4, 3.8.2.2, Figure 32, page 22, PDF page 22); bits 15:11 of 'pc' */
#define XNVME_SPEC_PCIE_PMCAP_REGISTERS_PC_PSUP 0xf800
/** D2_Support (NVMe over PCIe Transport 1.4, 3.8.2.2, Figure 32, page 22, PDF page 22); bit 10 of 'pc' */
#define XNVME_SPEC_PCIE_PMCAP_REGISTERS_PC_D2S 0x400
/** D1_Support (NVMe over PCIe Transport 1.4, 3.8.2.2, Figure 32, page 22, PDF page 22); bit 9 of 'pc' */
#define XNVME_SPEC_PCIE_PMCAP_REGISTERS_PC_D1S 0x200
/** Aux_Current (NVMe over PCIe Transport 1.4, 3.8.2.2, Figure 32, page 22, PDF page 22); bits 8:6 of 'pc' */
#define XNVME_SPEC_PCIE_PMCAP_REGISTERS_PC_AUXC 0x1c0
/** Device Specific Initialization (NVMe over PCIe Transport 1.4, 3.8.2.2, Figure 32, page 22, PDF page 22); bit 5 of 'pc' */
#define XNVME_SPEC_PCIE_PMCAP_REGISTERS_PC_DSI 0x20
/** PME Clock (NVMe over PCIe Transport 1.4, 3.8.2.2, Figure 32, page 22, PDF page 22); bit 3 of 'pc' */
#define XNVME_SPEC_PCIE_PMCAP_REGISTERS_PC_PMEC 0x8
/** Version (NVMe over PCIe Transport 1.4, 3.8.2.2, Figure 32, page 22, PDF page 22); bits 2:0 of 'pc' */
#define XNVME_SPEC_PCIE_PMCAP_REGISTERS_PC_VS 0x7
/** PME Status (NVMe over PCIe Transport 1.4, 3.8.2.3, Figure 33, page 22, PDF page 22); bit 15 of 'pmcs' */
#define XNVME_SPEC_PCIE_PMCAP_REGISTERS_PMCS_PMES 0x8000
/** Data Scale (NVMe over PCIe Transport 1.4, 3.8.2.3, Figure 33, page 22, PDF page 22); bits 14:13 of 'pmcs' */
#define XNVME_SPEC_PCIE_PMCAP_REGISTERS_PMCS_DSC 0x6000
/** Data Select (NVMe over PCIe Transport 1.4, 3.8.2.3, Figure 33, page 22, PDF page 22); bits 12:9 of 'pmcs' */
#define XNVME_SPEC_PCIE_PMCAP_REGISTERS_PMCS_DSE 0x1e00
/** PME Enable (NVMe over PCIe Transport 1.4, 3.8.2.3, Figure 33, page 22, PDF page 22); bit 8 of 'pmcs' */
#define XNVME_SPEC_PCIE_PMCAP_REGISTERS_PMCS_PMEE 0x100
/** No Soft Reset (NVMe over PCIe Transport 1.4, 3.8.2.3, Figure 33, page 22, PDF page 22); bit 3 of 'pmcs' */
#define XNVME_SPEC_PCIE_PMCAP_REGISTERS_PMCS_NSFRST 0x8
/** Power State (NVMe over PCIe Transport 1.4, 3.8.2.3, Figure 33, page 22, PDF page 22); bits 1:0 of 'pmcs' */
#define XNVME_SPEC_PCIE_PMCAP_REGISTERS_PMCS_PS 0x3

/**
 * The registers of MSICAP, at their offsets from MSICAP
 */
struct __attribute__((packed)) xnvme_spec_pcie_msicap_registers {
	uint16_t mid; ///< Offset MSICAP: MID - Message Signaled Interrupt Identifiers (NVMe over PCIe Transport 1.4, 3.8.3.1, Figure 35, page 23, PDF page 23), at 0h, see XNVME_SPEC_PCIE_MSICAP_REGISTERS_MID_*
	uint16_t mc; ///< Offset MSICAP + 2h: MC - Message Signaled Interrupt Message Control (NVMe over PCIe Transport 1.4, 3.8.3.2, Figure 36, page 23, PDF page 23), at 2h, see XNVME_SPEC_PCIE_MSICAP_REGISTERS_MC_*
	uint32_t ma; ///< Offset MSICAP + 4h: MA - Message Signaled Interrupt Message Address (NVMe over PCIe Transport 1.4, 3.8.3.3, Figure 37, page 23, PDF page 23), at 4h, see XNVME_SPEC_PCIE_MSICAP_REGISTERS_MA_*
	uint32_t mua; ///< Offset MSICAP + 8h: MUA - Message Signaled Interrupt Upper Address (NVMe over PCIe Transport 1.4, 3.8.3.4, Figure 38, page 23, PDF page 23), at 8h, see XNVME_SPEC_PCIE_MSICAP_REGISTERS_MUA_*
	uint16_t md; ///< Offset MSICAP + Ch: MD - Message Signaled Interrupt Message Data (NVMe over PCIe Transport 1.4, 3.8.3.5, Figure 39, page 23, PDF page 23), at Ch, see XNVME_SPEC_PCIE_MSICAP_REGISTERS_MD_*
	uint8_t rsvde[2]; ///< Reserved, Eh to Fh
	uint32_t mmask; ///< Offset MSICAP + 10h: MMASK - Message Signaled Interrupt Mask Bits (Optional) (NVMe over PCIe Transport 1.4, 3.8.3.6, Figure 40, page 24, PDF page 24), at 10h, see XNVME_SPEC_PCIE_MSICAP_REGISTERS_MMASK_*
	uint32_t mpend; ///< Offset MSICAP + 14h: MPEND - Message Signaled Interrupt Pending Bits (Optional) (NVMe over PCIe Transport 1.4, 3.8.3.7, Figure 41, page 24, PDF page 24), at 14h, see XNVME_SPEC_PCIE_MSICAP_REGISTERS_MPEND_*
};

/** Next Pointer (NVMe over PCIe Transport 1.4, 3.8.3.1, Figure 35, page 23, PDF page 23); bits 15:8 of 'mid' */
#define XNVME_SPEC_PCIE_MSICAP_REGISTERS_MID_NEXT 0xff00
/** Capability ID (NVMe over PCIe Transport 1.4, 3.8.3.1, Figure 35, page 23, PDF page 23); bits 7:0 of 'mid' */
#define XNVME_SPEC_PCIE_MSICAP_REGISTERS_MID_CID 0xff
/** Per-Vector Masking Capable (NVMe over PCIe Transport 1.4, 3.8.3.2, Figure 36, page 23, PDF page 23); bit 8 of 'mc' */
#define XNVME_SPEC_PCIE_MSICAP_REGISTERS_MC_PVM 0x100
/** 64 Bit Address Capable (NVMe over PCIe Transport 1.4, 3.8.3.2, Figure 36, page 23, PDF page 23); bit 7 of 'mc' */
#define XNVME_SPEC_PCIE_MSICAP_REGISTERS_MC_C64 0x80
/** Multiple Message Enable (NVMe over PCIe Transport 1.4, 3.8.3.2, Figure 36, page 23, PDF page 23); bits 6:4 of 'mc' */
#define XNVME_SPEC_PCIE_MSICAP_REGISTERS_MC_MME 0x70
/** Multiple Message Capable (NVMe over PCIe Transport 1.4, 3.8.3.2, Figure 36, page 23, PDF page 23); bits 3:1 of 'mc' */
#define XNVME_SPEC_PCIE_MSICAP_REGISTERS_MC_MMC 0xe
/** MSI Enable (NVMe over PCIe Transport 1.4, 3.8.3.2, Figure 36, page 23, PDF page 23); bit 0 of 'mc' */
#define XNVME_SPEC_PCIE_MSICAP_REGISTERS_MC_MSIE 0x1
/** Address (NVMe over PCIe Transport 1.4, 3.8.3.3, Figure 37, page 23, PDF page 23); bits 31:2 of 'ma' */
#define XNVME_SPEC_PCIE_MSICAP_REGISTERS_MA_ADDR 0xfffffffc
/** Upper Address (NVMe over PCIe Transport 1.4, 3.8.3.4, Figure 38, page 23, PDF page 23); bits 31:0 of 'mua' */
#define XNVME_SPEC_PCIE_MSICAP_REGISTERS_MUA_UADDR 0xffffffff
/** Data (NVMe over PCIe Transport 1.4, 3.8.3.5, Figure 39, page 23, PDF page 23); bits 15:0 of 'md' */
#define XNVME_SPEC_PCIE_MSICAP_REGISTERS_MD_DATA 0xffff
/** Mask Bits (NVMe over PCIe Transport 1.4, 3.8.3.6, Figure 40, page 24, PDF page 24); bits 31:0 of 'mmask' */
#define XNVME_SPEC_PCIE_MSICAP_REGISTERS_MMASK_MASK 0xffffffff
/** Pending Bits (NVMe over PCIe Transport 1.4, 3.8.3.7, Figure 41, page 24, PDF page 24); bits 31:0 of 'mpend' */
#define XNVME_SPEC_PCIE_MSICAP_REGISTERS_MPEND_PEND 0xffffffff

/**
 * The registers of MSIXCAP, at their offsets from MSIXCAP
 */
struct __attribute__((packed)) xnvme_spec_pcie_msixcap_registers {
	uint16_t mxid; ///< Offset MSIXCAP: MXID - MSI-X Identifiers (NVMe over PCIe Transport 1.4, 3.8.4.1, Figure 43, page 24, PDF page 24), at 0h, see XNVME_SPEC_PCIE_MSIXCAP_REGISTERS_MXID_*
	uint16_t mxc; ///< Offset MSIXCAP + 2h: MXC - MSI-X Message Control (NVMe over PCIe Transport 1.4, 3.8.4.2, Figure 44, page 24, PDF page 24), at 2h, see XNVME_SPEC_PCIE_MSIXCAP_REGISTERS_MXC_*
	uint32_t mtab; ///< Offset MSIXCAP + 4h: MTAB - MSI-X Table Offset / Table BIR (NVMe over PCIe Transport 1.4, 3.8.4.3, Figure 45, page 25, PDF page 25), at 4h, see XNVME_SPEC_PCIE_MSIXCAP_REGISTERS_MTAB_*
	uint32_t mpba; ///< Offset MSIXCAP + 8h: MPBA - MSI-X PBA Offset / PBA BIR (NVMe over PCIe Transport 1.4, 3.8.4.4, Figure 46, page 25, PDF page 25), at 8h, see XNVME_SPEC_PCIE_MSIXCAP_REGISTERS_MPBA_*
};

/** Next Pointer (NVMe over PCIe Transport 1.4, 3.8.4.1, Figure 43, page 24, PDF page 24); bits 15:8 of 'mxid' */
#define XNVME_SPEC_PCIE_MSIXCAP_REGISTERS_MXID_NEXT 0xff00
/** Capability ID (NVMe over PCIe Transport 1.4, 3.8.4.1, Figure 43, page 24, PDF page 24); bits 7:0 of 'mxid' */
#define XNVME_SPEC_PCIE_MSIXCAP_REGISTERS_MXID_CID 0xff
/** MSI-X Enable (NVMe over PCIe Transport 1.4, 3.8.4.2, Figure 44, page 24, PDF page 24); bit 15 of 'mxc' */
#define XNVME_SPEC_PCIE_MSIXCAP_REGISTERS_MXC_MXE 0x8000
/** Function Mask (NVMe over PCIe Transport 1.4, 3.8.4.2, Figure 44, page 24, PDF page 24); bit 14 of 'mxc' */
#define XNVME_SPEC_PCIE_MSIXCAP_REGISTERS_MXC_FM 0x4000
/** Table Size (NVMe over PCIe Transport 1.4, 3.8.4.2, Figure 44, page 24, PDF page 24); bits 10:0 of 'mxc' */
#define XNVME_SPEC_PCIE_MSIXCAP_REGISTERS_MXC_TS 0x7ff
/** Table Offset (NVMe over PCIe Transport 1.4, 3.8.4.3, Figure 45, page 25, PDF page 25); bits 31:3 of 'mtab' */
#define XNVME_SPEC_PCIE_MSIXCAP_REGISTERS_MTAB_TO 0xfffffff8
/** Table BIR (NVMe over PCIe Transport 1.4, 3.8.4.3, Figure 45, page 25, PDF page 25); bits 2:0 of 'mtab' */
#define XNVME_SPEC_PCIE_MSIXCAP_REGISTERS_MTAB_TBIR 0x7
/** PBA Offset (NVMe over PCIe Transport 1.4, 3.8.4.4, Figure 46, page 25, PDF page 25); bits 31:3 of 'mpba' */
#define XNVME_SPEC_PCIE_MSIXCAP_REGISTERS_MPBA_PBAO 0xfffffff8
/** PBA BIR (NVMe over PCIe Transport 1.4, 3.8.4.4, Figure 46, page 25, PDF page 25); bits 2:0 of 'mpba' */
#define XNVME_SPEC_PCIE_MSIXCAP_REGISTERS_MPBA_PBIR 0x7

/**
 * The registers of PXCAP, at their offsets from PXCAP
 */
struct __attribute__((packed)) xnvme_spec_pcie_pxcap_registers {
	uint16_t pxid; ///< Offset PXCAP: PXID - PCI Express Capability ID (NVMe over PCIe Transport 1.4, 3.8.5.1, Figure 48, page 26, PDF page 26), at 0h, see XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXID_*
	uint16_t pxcap; ///< Offset PXCAP + 2h: PXCAP - PCI Express Capabilities (NVMe over PCIe Transport 1.4, 3.8.5.2, Figure 49, page 26, PDF page 26), at 2h, see XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXCAP_*
	uint32_t pxdcap; ///< Offset PXCAP + 4h: PXDCAP - PCI Express Device Capabilities (NVMe over PCIe Transport 1.4, 3.8.5.3, Figure 50, page 26, PDF page 26), at 4h, see XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP_*
	uint16_t pxdc; ///< Offset PXCAP + 8h: PXDC - PCI Express Device Control (NVMe over PCIe Transport 1.4, 3.8.5.4, Figure 51, page 27, PDF page 27), at 8h, see XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDC_*
	uint16_t pxds; ///< Offset PXCAP + Ah: PXDS - PCI Express Device Status (NVMe over PCIe Transport 1.4, 3.8.5.5, Figure 52, page 28, PDF page 28), at Ah, see XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDS_*
	uint32_t pxlcap; ///< Offset PXCAP + Ch: PXLCAP - PCI Express Link Capabilities (NVMe over PCIe Transport 1.4, 3.8.5.6, Figure 53, page 28, PDF page 28), at Ch, see XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLCAP_*
	uint16_t pxlc; ///< Offset PXCAP + 10h: PXLC - PCI Express Link Control (NVMe over PCIe Transport 1.4, 3.8.5.7, Figure 54, page 29, PDF page 29), at 10h, see XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLC_*
	uint16_t pxls; ///< Offset PXCAP + 12h: PXLS - PCI Express Link Status (NVMe over PCIe Transport 1.4, 3.8.5.8, Figure 55, page 29, PDF page 29), at 12h, see XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLS_*
	uint8_t rsvd14[16]; ///< Reserved, 14h to 23h
	uint32_t pxdcap2; ///< Offset PXCAP + 24h: PXDCAP2 - PCI Express Device Capabilities 2 (NVMe over PCIe Transport 1.4, 3.8.5.9, Figure 56, page 30, PDF page 30), at 24h, see XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP2_*
	uint32_t pxdc2; ///< Offset PXCAP + 28h: PXDC2 - PCI Express Device Control 2 (NVMe over PCIe Transport 1.4, 3.8.5.10, Figure 57, page 30, PDF page 30), at 28h, see XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDC2_*
};

/** Next Pointer (NVMe over PCIe Transport 1.4, 3.8.5.1, Figure 48, page 26, PDF page 26); bits 15:8 of 'pxid' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXID_NEXT 0xff00
/** Capability ID (NVMe over PCIe Transport 1.4, 3.8.5.1, Figure 48, page 26, PDF page 26); bits 7:0 of 'pxid' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXID_CID 0xff
/** Interrupt Message Number (NVMe over PCIe Transport 1.4, 3.8.5.2, Figure 49, page 26, PDF page 26); bits 13:9 of 'pxcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXCAP_IMN 0x3e00
/** Slot Implemented (NVMe over PCIe Transport 1.4, 3.8.5.2, Figure 49, page 26, PDF page 26); bit 8 of 'pxcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXCAP_SI 0x100
/** Device/Port Type (NVMe over PCIe Transport 1.4, 3.8.5.2, Figure 49, page 26, PDF page 26); bits 7:4 of 'pxcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXCAP_DPT 0xf0
/** Capability Version (NVMe over PCIe Transport 1.4, 3.8.5.2, Figure 49, page 26, PDF page 26); bits 3:0 of 'pxcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXCAP_VER 0xf
/** Function Level Reset Capability (NVMe over PCIe Transport 1.4, 3.8.5.3, Figure 50, page 26, PDF page 26); bit 28 of 'pxdcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP_FLRC 0x10000000
/** Captured Slot Power Limit Scale (NVMe over PCIe Transport 1.4, 3.8.5.3, Figure 50, page 26, PDF page 26); bits 27:26 of 'pxdcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP_CSPLS 0xc000000
/** Captured Slot Power Limit Value (NVMe over PCIe Transport 1.4, 3.8.5.3, Figure 50, page 26, PDF page 26); bits 25:18 of 'pxdcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP_CSPLV 0x3fc0000
/** Role-based Error Reporting (NVMe over PCIe Transport 1.4, 3.8.5.3, Figure 50, page 26, PDF page 26); bit 15 of 'pxdcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP_RER 0x8000
/** Endpoint L1 Acceptable Latency (NVMe over PCIe Transport 1.4, 3.8.5.3, Figure 50, page 26, PDF page 26); bits 11:9 of 'pxdcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP_L1L 0xe00
/** Endpoint L0s Acceptable Latency (NVMe over PCIe Transport 1.4, 3.8.5.3, Figure 50, page 26, PDF page 26); bits 8:6 of 'pxdcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP_L0SL 0x1c0
/** Extended Tag Field Supported (NVMe over PCIe Transport 1.4, 3.8.5.3, Figure 50, page 26, PDF page 26); bit 5 of 'pxdcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP_ETFS 0x20
/** Phantom Functions Supported (NVMe over PCIe Transport 1.4, 3.8.5.3, Figure 50, page 26, PDF page 26); bits 4:3 of 'pxdcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP_PFS 0x18
/** Max_Payload_Size Supported (NVMe over PCIe Transport 1.4, 3.8.5.3, Figure 50, page 26, PDF page 26); bits 2:0 of 'pxdcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP_MPS 0x7
/** Initiate Function Level Reset (NVMe over PCIe Transport 1.4, 3.8.5.4, Figure 51, page 27, PDF page 27); bit 15 of 'pxdc' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDC_IFLR 0x8000
/** Max_Read_Request_Size (NVMe over PCIe Transport 1.4, 3.8.5.4, Figure 51, page 27, PDF page 27); bits 14:12 of 'pxdc' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDC_MRRS 0x7000
/** Enable No Snoop (NVMe over PCIe Transport 1.4, 3.8.5.4, Figure 51, page 27, PDF page 27); bit 11 of 'pxdc' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDC_ENS 0x800
/** AUX Power PM Enable (NVMe over PCIe Transport 1.4, 3.8.5.4, Figure 51, page 27, PDF page 27); bit 10 of 'pxdc' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDC_APPME 0x400
/** Phantom Functions Enable (NVMe over PCIe Transport 1.4, 3.8.5.4, Figure 51, page 27, PDF page 27); bit 9 of 'pxdc' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDC_PFE 0x200
/** Extended Tag Enable (NVMe over PCIe Transport 1.4, 3.8.5.4, Figure 51, page 27, PDF page 27); bit 8 of 'pxdc' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDC_ETE 0x100
/** Max_Payload_Size (NVMe over PCIe Transport 1.4, 3.8.5.4, Figure 51, page 27, PDF page 27); bits 7:5 of 'pxdc' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDC_MPS 0xe0
/** Enable Relaxed Ordering (NVMe over PCIe Transport 1.4, 3.8.5.4, Figure 51, page 27, PDF page 27); bit 4 of 'pxdc' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDC_ERO 0x10
/** Unsupported Request Reporting Enable (NVMe over PCIe Transport 1.4, 3.8.5.4, Figure 51, page 27, PDF page 27); bit 3 of 'pxdc' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDC_URRE 0x8
/** Fatal Error Reporting Enable (NVMe over PCIe Transport 1.4, 3.8.5.4, Figure 51, page 27, PDF page 27); bit 2 of 'pxdc' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDC_FERE 0x4
/** Non-Fatal Error Reporting Enable (NVMe over PCIe Transport 1.4, 3.8.5.4, Figure 51, page 27, PDF page 27); bit 1 of 'pxdc' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDC_NFERE 0x2
/** Correctable Error Reporting Enable (NVMe over PCIe Transport 1.4, 3.8.5.4, Figure 51, page 27, PDF page 27); bit 0 of 'pxdc' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDC_CERE 0x1
/** Transactions Pending (NVMe over PCIe Transport 1.4, 3.8.5.5, Figure 52, page 28, PDF page 28); bit 5 of 'pxds' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDS_TP 0x20
/** AUX Power Detected (NVMe over PCIe Transport 1.4, 3.8.5.5, Figure 52, page 28, PDF page 28); bit 4 of 'pxds' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDS_APD 0x10
/** Unsupported Request Detected (NVMe over PCIe Transport 1.4, 3.8.5.5, Figure 52, page 28, PDF page 28); bit 3 of 'pxds' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDS_URD 0x8
/** Fatal Error Detected (NVMe over PCIe Transport 1.4, 3.8.5.5, Figure 52, page 28, PDF page 28); bit 2 of 'pxds' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDS_FED 0x4
/** Non-Fatal Error Detected (NVMe over PCIe Transport 1.4, 3.8.5.5, Figure 52, page 28, PDF page 28); bit 1 of 'pxds' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDS_NFED 0x2
/** Correctable Error Detected (NVMe over PCIe Transport 1.4, 3.8.5.5, Figure 52, page 28, PDF page 28); bit 0 of 'pxds' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDS_CED 0x1
/** Port Number (NVMe over PCIe Transport 1.4, 3.8.5.6, Figure 53, page 28, PDF page 28); bits 31:24 of 'pxlcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLCAP_PN 0xff000000
/** ASPM Optionality Compliance (NVMe over PCIe Transport 1.4, 3.8.5.6, Figure 53, page 28, PDF page 28); bit 22 of 'pxlcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLCAP_AOC 0x400000
/** Link Bandwidth Notification Capability (NVMe over PCIe Transport 1.4, 3.8.5.6, Figure 53, page 28, PDF page 28); bit 21 of 'pxlcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLCAP_LBNC 0x200000
/** Data Link Layer Link Active Reporting Capable (NVMe over PCIe Transport 1.4, 3.8.5.6, Figure 53, page 28, PDF page 28); bit 20 of 'pxlcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLCAP_DLLLA 0x100000
/** Surprise Down Error Reporting Capable (NVMe over PCIe Transport 1.4, 3.8.5.6, Figure 53, page 28, PDF page 28); bit 19 of 'pxlcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLCAP_SDERC 0x80000
/** Clock Power Management (NVMe over PCIe Transport 1.4, 3.8.5.6, Figure 53, page 28, PDF page 28); bit 18 of 'pxlcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLCAP_CPM 0x40000
/** L1 Exit Latency (NVMe over PCIe Transport 1.4, 3.8.5.6, Figure 53, page 28, PDF page 28); bits 17:15 of 'pxlcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLCAP_L1EL 0x38000
/** L0s Exit Latency (NVMe over PCIe Transport 1.4, 3.8.5.6, Figure 53, page 28, PDF page 28); bits 14:12 of 'pxlcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLCAP_L0SEL 0x7000
/** Active State Power Management Support (NVMe over PCIe Transport 1.4, 3.8.5.6, Figure 53, page 28, PDF page 28); bits 11:10 of 'pxlcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLCAP_ASPMS 0xc00
/** Maximum Link Width (NVMe over PCIe Transport 1.4, 3.8.5.6, Figure 53, page 28, PDF page 28); bits 9:4 of 'pxlcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLCAP_MLW 0x3f0
/** Supported Link Speeds (NVMe over PCIe Transport 1.4, 3.8.5.6, Figure 53, page 28, PDF page 28); bits 3:0 of 'pxlcap' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLCAP_SLS 0xf
/** Hardware Autonomous Width Disable (NVMe over PCIe Transport 1.4, 3.8.5.7, Figure 54, page 29, PDF page 29); bit 9 of 'pxlc' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLC_HAWD 0x200
/** Enable Clock Power Management (NVMe over PCIe Transport 1.4, 3.8.5.7, Figure 54, page 29, PDF page 29); bit 8 of 'pxlc' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLC_ECPM 0x100
/** Extended Synch (NVMe over PCIe Transport 1.4, 3.8.5.7, Figure 54, page 29, PDF page 29); bit 7 of 'pxlc' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLC_ES 0x80
/** Common Clock Configuration (NVMe over PCIe Transport 1.4, 3.8.5.7, Figure 54, page 29, PDF page 29); bit 6 of 'pxlc' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLC_CCC 0x40
/** Read Completion Boundary (NVMe over PCIe Transport 1.4, 3.8.5.7, Figure 54, page 29, PDF page 29); bit 3 of 'pxlc' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLC_RCB 0x8
/** Active State Power Management Control (NVMe over PCIe Transport 1.4, 3.8.5.7, Figure 54, page 29, PDF page 29); bits 1:0 of 'pxlc' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLC_ASPMC 0x3
/** Slot Clock Configuration (NVMe over PCIe Transport 1.4, 3.8.5.8, Figure 55, page 29, PDF page 29); bit 12 of 'pxls' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLS_SCC 0x1000
/** Negotiated Link Width (NVMe over PCIe Transport 1.4, 3.8.5.8, Figure 55, page 29, PDF page 29); bits 9:4 of 'pxls' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLS_NLW 0x3f0
/** Current Link Speed (NVMe over PCIe Transport 1.4, 3.8.5.8, Figure 55, page 29, PDF page 29); bits 3:0 of 'pxls' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXLS_CLS 0xf
/** Max End-End TLP Prefixes (NVMe over PCIe Transport 1.4, 3.8.5.9, Figure 56, page 30, PDF page 30); bits 23:22 of 'pxdcap2' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP2_MEETP 0xc00000
/** End-End TLP Prefix Supported (NVMe over PCIe Transport 1.4, 3.8.5.9, Figure 56, page 30, PDF page 30); bit 21 of 'pxdcap2' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP2_EETPS 0x200000
/** Extended Fmt Field Supported (NVMe over PCIe Transport 1.4, 3.8.5.9, Figure 56, page 30, PDF page 30); bit 20 of 'pxdcap2' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP2_EFFS 0x100000
/** OBFF Supported (NVMe over PCIe Transport 1.4, 3.8.5.9, Figure 56, page 30, PDF page 30); bits 19:18 of 'pxdcap2' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP2_OBFFS 0xc0000
/** TPH Completer Supported (NVMe over PCIe Transport 1.4, 3.8.5.9, Figure 56, page 30, PDF page 30); bits 13:12 of 'pxdcap2' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP2_TPHCS 0x3000
/** Latency Tolerance Reporting Supported (NVMe over PCIe Transport 1.4, 3.8.5.9, Figure 56, page 30, PDF page 30); bit 11 of 'pxdcap2' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP2_LTRS 0x800
/** No RO-enabled PR-PR Passing (NVMe over PCIe Transport 1.4, 3.8.5.9, Figure 56, page 30, PDF page 30); bit 10 of 'pxdcap2' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP2_NPRPR 0x400
/** 128-bit CAS Completer Supported (NVMe over PCIe Transport 1.4, 3.8.5.9, Figure 56, page 30, PDF page 30); bit 9 of 'pxdcap2' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP2_CCS128 0x200
/** 64-bit AtomicOp Completer Supported (NVMe over PCIe Transport 1.4, 3.8.5.9, Figure 56, page 30, PDF page 30); bit 8 of 'pxdcap2' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP2_AOCS64 0x100
/** 32-bit AtomicOp Completer Supported (NVMe over PCIe Transport 1.4, 3.8.5.9, Figure 56, page 30, PDF page 30); bit 7 of 'pxdcap2' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP2_AOCS32 0x80
/** AtomicOp Routing Supported (NVMe over PCIe Transport 1.4, 3.8.5.9, Figure 56, page 30, PDF page 30); bit 6 of 'pxdcap2' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP2_AORS 0x40
/** ARI Forwarding Supported (NVMe over PCIe Transport 1.4, 3.8.5.9, Figure 56, page 30, PDF page 30); bit 5 of 'pxdcap2' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP2_ARIFS 0x20
/** Completion Timeout Disable Supported (NVMe over PCIe Transport 1.4, 3.8.5.9, Figure 56, page 30, PDF page 30); bit 4 of 'pxdcap2' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP2_CTDS 0x10
/** Completion Timeout Ranges Supported (NVMe over PCIe Transport 1.4, 3.8.5.9, Figure 56, page 30, PDF page 30); bits 3:0 of 'pxdcap2' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDCAP2_CTRS 0xf
/** OBFF Enable (NVMe over PCIe Transport 1.4, 3.8.5.10, Figure 57, page 30, PDF page 30); bits 14:13 of 'pxdc2' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDC2_OBFFE 0x6000
/** Latency Tolerance Reporting Mechanism Enable (NVMe over PCIe Transport 1.4, 3.8.5.10, Figure 57, page 30, PDF page 30); bit 10 of 'pxdc2' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDC2_LTRME 0x400
/** Completion Timeout Disable (NVMe over PCIe Transport 1.4, 3.8.5.10, Figure 57, page 30, PDF page 30); bit 4 of 'pxdc2' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDC2_CTD 0x10
/** Completion Timeout Value (NVMe over PCIe Transport 1.4, 3.8.5.10, Figure 57, page 30, PDF page 30); bits 3:0 of 'pxdc2' */
#define XNVME_SPEC_PCIE_PXCAP_REGISTERS_PXDC2_CTV 0xf

/**
 * The registers of AERCAP, at their offsets from AERCAP
 */
struct __attribute__((packed)) xnvme_spec_pcie_aercap_registers {
	uint32_t aerid; ///< Offset AERCAP: AERID - AER Capability ID (NVMe over PCIe Transport 1.4, 3.8.6.1, Figure 59, page 31, PDF page 31), at 0h, see XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERID_*
	uint32_t aeruces; ///< Offset AERCAP + 4: AERUCES - AER Uncorrectable Error Status Register (NVMe over PCIe Transport 1.4, 3.8.6.2, Figure 60, page 31, PDF page 31), at 4h, see XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCES_*
	uint32_t aerucem; ///< Offset AERCAP + 8: AERUCEM - AER Uncorrectable Error Mask Register (NVMe over PCIe Transport 1.4, 3.8.6.3, Figure 61, page 32, PDF page 32), at 8h, see XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCEM_*
	uint32_t aerucesev; ///< Offset AERCAP + Ch: AERUCESEV - AER Uncorrectable Error Severity Register (NVMe over PCIe Transport 1.4, 3.8.6.4, Figure 62, page 32, PDF page 32), at Ch, see XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCESEV_*
	uint32_t aerces; ///< Offset AERCAP + 10h: AERCES - AER Correctable Error Status Register (NVMe over PCIe Transport 1.4, 3.8.6.5, Figure 63, page 33, PDF page 33), at 10h, see XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCES_*
	uint32_t aercem; ///< Offset AERCAP + 14h: AERCEM - AER Correctable Error Mask Register (NVMe over PCIe Transport 1.4, 3.8.6.6, Figure 64, page 33, PDF page 33), at 14h, see XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCEM_*
	uint32_t aercc; ///< Offset AERCAP + 18h: AERCC - AER Capabilities and Control Register (NVMe over PCIe Transport 1.4, 3.8.6.7, Figure 65, page 34, PDF page 34), at 18h, see XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCC_*
};

/** Next Pointer (NVMe over PCIe Transport 1.4, 3.8.6.1, Figure 59, page 31, PDF page 31); bits 31:20 of 'aerid' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERID_NEXT 0xfff00000
/** Capability Version (NVMe over PCIe Transport 1.4, 3.8.6.1, Figure 59, page 31, PDF page 31); bits 19:16 of 'aerid' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERID_CVER 0xf0000
/** Capability ID (NVMe over PCIe Transport 1.4, 3.8.6.1, Figure 59, page 31, PDF page 31); bits 15:0 of 'aerid' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERID_CID 0xffff
/** TLP Prefix Blocked Error Status (NVMe over PCIe Transport 1.4, 3.8.6.2, Figure 60, page 31, PDF page 31); bit 25 of 'aeruces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCES_TPBES 0x2000000
/** AtomicOp Egress Blocked Status (NVMe over PCIe Transport 1.4, 3.8.6.2, Figure 60, page 31, PDF page 31); bit 24 of 'aeruces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCES_AOEBS 0x1000000
/** MC Blocked TLP Status (NVMe over PCIe Transport 1.4, 3.8.6.2, Figure 60, page 31, PDF page 31); bit 23 of 'aeruces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCES_MCBTS 0x800000
/** Uncorrectable Internal Error Status (NVMe over PCIe Transport 1.4, 3.8.6.2, Figure 60, page 31, PDF page 31); bit 22 of 'aeruces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCES_UIES 0x400000
/** ACS Violation Status (NVMe over PCIe Transport 1.4, 3.8.6.2, Figure 60, page 31, PDF page 31); bit 21 of 'aeruces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCES_ACSVS 0x200000
/** Unsupported Request Error Status (NVMe over PCIe Transport 1.4, 3.8.6.2, Figure 60, page 31, PDF page 31); bit 20 of 'aeruces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCES_URES 0x100000
/** ECRC Error Status (NVMe over PCIe Transport 1.4, 3.8.6.2, Figure 60, page 31, PDF page 31); bit 19 of 'aeruces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCES_ECRCES 0x80000
/** Malformed TLP Status (NVMe over PCIe Transport 1.4, 3.8.6.2, Figure 60, page 31, PDF page 31); bit 18 of 'aeruces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCES_MTS 0x40000
/** Receiver Overflow Status (NVMe over PCIe Transport 1.4, 3.8.6.2, Figure 60, page 31, PDF page 31); bit 17 of 'aeruces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCES_ROS 0x20000
/** Unexpected Completion Status (NVMe over PCIe Transport 1.4, 3.8.6.2, Figure 60, page 31, PDF page 31); bit 16 of 'aeruces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCES_UCS 0x10000
/** Completer Abort Status (NVMe over PCIe Transport 1.4, 3.8.6.2, Figure 60, page 31, PDF page 31); bit 15 of 'aeruces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCES_CAS 0x8000
/** Completion Timeout Status (NVMe over PCIe Transport 1.4, 3.8.6.2, Figure 60, page 31, PDF page 31); bit 14 of 'aeruces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCES_CTS 0x4000
/** Flow Control Protocol Error Status (NVMe over PCIe Transport 1.4, 3.8.6.2, Figure 60, page 31, PDF page 31); bit 13 of 'aeruces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCES_FCPES 0x2000
/** Poisoned TLP Status (NVMe over PCIe Transport 1.4, 3.8.6.2, Figure 60, page 31, PDF page 31); bit 12 of 'aeruces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCES_PTS 0x1000
/** Data Link Protocol Error Status (NVMe over PCIe Transport 1.4, 3.8.6.2, Figure 60, page 31, PDF page 31); bit 4 of 'aeruces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCES_DLPES 0x10
/** TLP Prefix Blocked Error Mask (NVMe over PCIe Transport 1.4, 3.8.6.3, Figure 61, page 32, PDF page 32); bit 25 of 'aerucem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCEM_TPBEM 0x2000000
/** AtomicOp Egress Blocked Mask (NVMe over PCIe Transport 1.4, 3.8.6.3, Figure 61, page 32, PDF page 32); bit 24 of 'aerucem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCEM_AOEBM 0x1000000
/** MC Blocked TLP Mask (NVMe over PCIe Transport 1.4, 3.8.6.3, Figure 61, page 32, PDF page 32); bit 23 of 'aerucem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCEM_MCBTM 0x800000
/** Uncorrectable Internal Error Mask (NVMe over PCIe Transport 1.4, 3.8.6.3, Figure 61, page 32, PDF page 32); bit 22 of 'aerucem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCEM_UIEM 0x400000
/** ACS Violation Mask (NVMe over PCIe Transport 1.4, 3.8.6.3, Figure 61, page 32, PDF page 32); bit 21 of 'aerucem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCEM_ACSVM 0x200000
/** Unsupported Request Error Mask (NVMe over PCIe Transport 1.4, 3.8.6.3, Figure 61, page 32, PDF page 32); bit 20 of 'aerucem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCEM_UREM 0x100000
/** ECRC Error Mask (NVMe over PCIe Transport 1.4, 3.8.6.3, Figure 61, page 32, PDF page 32); bit 19 of 'aerucem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCEM_ECRCEM 0x80000
/** Malformed TLP Mask (NVMe over PCIe Transport 1.4, 3.8.6.3, Figure 61, page 32, PDF page 32); bit 18 of 'aerucem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCEM_MTM 0x40000
/** Receiver Overflow Mask (NVMe over PCIe Transport 1.4, 3.8.6.3, Figure 61, page 32, PDF page 32); bit 17 of 'aerucem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCEM_ROM 0x20000
/** Unexpected Completion Mask (NVMe over PCIe Transport 1.4, 3.8.6.3, Figure 61, page 32, PDF page 32); bit 16 of 'aerucem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCEM_UCM 0x10000
/** Completer Abort Mask (NVMe over PCIe Transport 1.4, 3.8.6.3, Figure 61, page 32, PDF page 32); bit 15 of 'aerucem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCEM_CAM 0x8000
/** Completion Timeout Mask (NVMe over PCIe Transport 1.4, 3.8.6.3, Figure 61, page 32, PDF page 32); bit 14 of 'aerucem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCEM_CTM 0x4000
/** Flow Control Protocol Error Mask (NVMe over PCIe Transport 1.4, 3.8.6.3, Figure 61, page 32, PDF page 32); bit 13 of 'aerucem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCEM_FCPEM 0x2000
/** Poisoned TLP Mask (NVMe over PCIe Transport 1.4, 3.8.6.3, Figure 61, page 32, PDF page 32); bit 12 of 'aerucem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCEM_PTM 0x1000
/** Data Link Protocol Error Mask (NVMe over PCIe Transport 1.4, 3.8.6.3, Figure 61, page 32, PDF page 32); bit 4 of 'aerucem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCEM_DLPEM 0x10
/** TLP Prefix Blocked Error Severity (NVMe over PCIe Transport 1.4, 3.8.6.4, Figure 62, page 32, PDF page 32); bit 25 of 'aerucesev' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCESEV_TPBESEV 0x2000000
/** AtomicOp Egress Blocked Severity (NVMe over PCIe Transport 1.4, 3.8.6.4, Figure 62, page 32, PDF page 32); bit 24 of 'aerucesev' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCESEV_AOEBSEV 0x1000000
/** MC Blocked TLP Severity (NVMe over PCIe Transport 1.4, 3.8.6.4, Figure 62, page 32, PDF page 32); bit 23 of 'aerucesev' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCESEV_MCBTSEV 0x800000
/** Uncorrectable Internal Error Severity (NVMe over PCIe Transport 1.4, 3.8.6.4, Figure 62, page 32, PDF page 32); bit 22 of 'aerucesev' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCESEV_UIESEV 0x400000
/** ACS Violation Severity (NVMe over PCIe Transport 1.4, 3.8.6.4, Figure 62, page 32, PDF page 32); bit 21 of 'aerucesev' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCESEV_ACSVSEV 0x200000
/** Unsupported Request Error Severity (NVMe over PCIe Transport 1.4, 3.8.6.4, Figure 62, page 32, PDF page 32); bit 20 of 'aerucesev' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCESEV_URESEV 0x100000
/** ECRC Error Severity (NVMe over PCIe Transport 1.4, 3.8.6.4, Figure 62, page 32, PDF page 32); bit 19 of 'aerucesev' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCESEV_ECRCESEV 0x80000
/** Malformed TLP Severity (NVMe over PCIe Transport 1.4, 3.8.6.4, Figure 62, page 32, PDF page 32); bit 18 of 'aerucesev' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCESEV_MTSEV 0x40000
/** Receiver Overflow Severity (NVMe over PCIe Transport 1.4, 3.8.6.4, Figure 62, page 32, PDF page 32); bit 17 of 'aerucesev' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCESEV_ROSEV 0x20000
/** Unexpected Completion Severity (NVMe over PCIe Transport 1.4, 3.8.6.4, Figure 62, page 32, PDF page 32); bit 16 of 'aerucesev' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCESEV_UCSEV 0x10000
/** Completer Abort Severity (NVMe over PCIe Transport 1.4, 3.8.6.4, Figure 62, page 32, PDF page 32); bit 15 of 'aerucesev' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCESEV_CASEV 0x8000
/** Completion Timeout Severity (NVMe over PCIe Transport 1.4, 3.8.6.4, Figure 62, page 32, PDF page 32); bit 14 of 'aerucesev' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCESEV_CTSEV 0x4000
/** Flow Control Protocol Error Severity (NVMe over PCIe Transport 1.4, 3.8.6.4, Figure 62, page 32, PDF page 32); bit 13 of 'aerucesev' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCESEV_FCPESEV 0x2000
/** Poisoned TLP Severity (NVMe over PCIe Transport 1.4, 3.8.6.4, Figure 62, page 32, PDF page 32); bit 12 of 'aerucesev' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCESEV_PTSEV 0x1000
/** Data Link Protocol Error Severity (NVMe over PCIe Transport 1.4, 3.8.6.4, Figure 62, page 32, PDF page 32); bit 4 of 'aerucesev' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERUCESEV_DLPESEV 0x10
/** Header Log Overflow Status (NVMe over PCIe Transport 1.4, 3.8.6.5, Figure 63, page 33, PDF page 33); bit 15 of 'aerces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCES_HLOS 0x8000
/** Corrected Internal Error Status (NVMe over PCIe Transport 1.4, 3.8.6.5, Figure 63, page 33, PDF page 33); bit 14 of 'aerces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCES_CIES 0x4000
/** Advisory Non-Fatal Error Status (NVMe over PCIe Transport 1.4, 3.8.6.5, Figure 63, page 33, PDF page 33); bit 13 of 'aerces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCES_ANFES 0x2000
/** Replay Timer Timeout Status (NVMe over PCIe Transport 1.4, 3.8.6.5, Figure 63, page 33, PDF page 33); bit 12 of 'aerces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCES_RTS 0x1000
/** REPLAY_NUM Rollover Status (NVMe over PCIe Transport 1.4, 3.8.6.5, Figure 63, page 33, PDF page 33); bit 8 of 'aerces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCES_RRS 0x100
/** Bad DLLP Status (NVMe over PCIe Transport 1.4, 3.8.6.5, Figure 63, page 33, PDF page 33); bit 7 of 'aerces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCES_BDS 0x80
/** Bad TLP Status (NVMe over PCIe Transport 1.4, 3.8.6.5, Figure 63, page 33, PDF page 33); bit 6 of 'aerces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCES_BTS 0x40
/** Receiver Error Status (NVMe over PCIe Transport 1.4, 3.8.6.5, Figure 63, page 33, PDF page 33); bit 0 of 'aerces' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCES_RES 0x1
/** Header Log Overflow Mask (NVMe over PCIe Transport 1.4, 3.8.6.6, Figure 64, page 33, PDF page 33); bit 15 of 'aercem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCEM_HLOM 0x8000
/** Corrected Internal Error Mask (NVMe over PCIe Transport 1.4, 3.8.6.6, Figure 64, page 33, PDF page 33); bit 14 of 'aercem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCEM_CIEM 0x4000
/** Advisory Non-Fatal Error Mask (NVMe over PCIe Transport 1.4, 3.8.6.6, Figure 64, page 33, PDF page 33); bit 13 of 'aercem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCEM_ANFEM 0x2000
/** Replay Timer Timeout Mask (NVMe over PCIe Transport 1.4, 3.8.6.6, Figure 64, page 33, PDF page 33); bit 12 of 'aercem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCEM_RTM 0x1000
/** REPLAY_NUM Rollover Mask (NVMe over PCIe Transport 1.4, 3.8.6.6, Figure 64, page 33, PDF page 33); bit 8 of 'aercem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCEM_RRM 0x100
/** Bad DLLP Mask (NVMe over PCIe Transport 1.4, 3.8.6.6, Figure 64, page 33, PDF page 33); bit 7 of 'aercem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCEM_BDM 0x80
/** Bad TLP Mask (NVMe over PCIe Transport 1.4, 3.8.6.6, Figure 64, page 33, PDF page 33); bit 6 of 'aercem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCEM_BTM 0x40
/** Receiver Error Mask (NVMe over PCIe Transport 1.4, 3.8.6.6, Figure 64, page 33, PDF page 33); bit 0 of 'aercem' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCEM_REM 0x1
/** TLP Prefix Log Present (NVMe over PCIe Transport 1.4, 3.8.6.7, Figure 65, page 34, PDF page 34); bit 11 of 'aercc' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCC_TPLP 0x800
/** Multiple Header Recording Enable (NVMe over PCIe Transport 1.4, 3.8.6.7, Figure 65, page 34, PDF page 34); bit 10 of 'aercc' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCC_MHRE 0x400
/** Multiple Header Recording Capable (NVMe over PCIe Transport 1.4, 3.8.6.7, Figure 65, page 34, PDF page 34); bit 9 of 'aercc' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCC_MHRC 0x200
/** ECRC Check Enable (NVMe over PCIe Transport 1.4, 3.8.6.7, Figure 65, page 34, PDF page 34); bit 8 of 'aercc' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCC_ECE 0x100
/** ECRC Check Capable (NVMe over PCIe Transport 1.4, 3.8.6.7, Figure 65, page 34, PDF page 34); bit 7 of 'aercc' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCC_ECC 0x80
/** ECRC Generation Enable (NVMe over PCIe Transport 1.4, 3.8.6.7, Figure 65, page 34, PDF page 34); bit 6 of 'aercc' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCC_EGE 0x40
/** ECRC Generation Capable (NVMe over PCIe Transport 1.4, 3.8.6.7, Figure 65, page 34, PDF page 34); bit 5 of 'aercc' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCC_EGC 0x20
/** First Error Pointer (NVMe over PCIe Transport 1.4, 3.8.6.7, Figure 65, page 34, PDF page 34); bits 4:0 of 'aercc' */
#define XNVME_SPEC_PCIE_AERCAP_REGISTERS_AERCC_FEP 0x1f

#endif /* LIBXNVME_SPEC_PCIE_H */
