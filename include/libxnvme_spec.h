/**
 * libxnvme_spec - NVMe structs, enum, values and helper function declarations
 *
 * If an entity is defined in the NVMe specification, then an enum or struct
 * should exist in this header file. The purpose of this is to provide a single
 * point of entry for spec. definitions for the purpose of providing structs
 * with accessors, possibly bit-fields, and enums for values such as command
 * opcodes.
 *
 * Auxilary header files are provided for the different IO command-sets, e.g.
 * ``libxnvme_nvm.h``, providing utility functions such as ``xnvme_nvm_read``,
 * ``xnvme_nvm_write``, and similarly ``libxnvme_znd.h`` with utilities such as
 * ``xnvme_znd_append``, ``xnvme_znd_mgmt_send``.  These can also contain enums
 * and structs, however, these are not based on definitions in the NVMe
 * specification, rather, these build on the spec. definition in order to
 * provide something sligthly more convenient to the user.
 *
 * The pretty-printers of these definitions are declared in ``libxnvme_pp.h``.
 * They are named by the type, suffixed by ``_fpr`` to print to a given stream
 * and ``_pr`` to print to stdout, for example xnvme_spec_idfy_ns_fpr() and
 * xnvme_spec_idfy_ns_pr(), and for an enum by ``_str``, for example
 * xnvme_spec_adm_opc_str().
 *
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_spec.h
 */

/**
 * NVMe PCIe BAR0 as-a-struct.
 *
 * @struct xnvme_spec_ctrlr_bar
 */
struct xnvme_spec_ctrlr_bar {
	uint64_t cap;
	uint32_t vs;
	uint32_t intms;
	uint32_t intmc;
	uint32_t cc;
	uint32_t rsvd24;
	uint32_t csts;
	uint32_t nssr;
	uint32_t aqa;
	uint64_t asq;
	uint64_t acq;
	uint32_t cmbloc;
	uint32_t cmbsz;
	uint32_t bpinfo;
	uint32_t bprsel;
	uint64_t bpmbl;
	uint64_t cmbmsc;
	uint32_t cmbsts;
	uint8_t rsvd92[3492];
	uint32_t pmrcap;
	uint32_t pmrctl;
	uint32_t pmrsts;
	uint32_t pmrebs;
	uint32_t pmrswtp;
	uint32_t pmrmscl;
	uint32_t pmrmscu;
	uint8_t css[484];
} __attribute__((__packed__));

/**
 * NVMe Command Status Code Types
 *
 * @enum xnvme_spec_status_code_type
 */
enum xnvme_spec_status_code_type {
	XNVME_STATUS_CODE_TYPE_GENERIC = 0x0, ///< Generic
	XNVME_STATUS_CODE_TYPE_CMDSPEC = 0x1, ///< Command Specific
	XNVME_STATUS_CODE_TYPE_MEDIA   = 0x2, ///< Media and data integrity error
	XNVME_STATUS_CODE_TYPE_PATH    = 0x3, ///< Path related status
	XNVME_STATUS_CODE_TYPE_VENDOR  = 0x7, ///< Vendor specific
};

/**
 * NVMe Command Status Code
 *
 * @enum xnvme_spec_status_code
 */
enum xnvme_spec_status_code {
	// TODO: Add remaining status codes from spec
	XNVME_STATUS_CODE_INVALID_FIELD = 0x02, ///< Invalid Field
};

/**
 * Completion Queue Entry
 *
 * @struct xnvme_spec_cqe
 */
struct xnvme_spec_cqe {
	/* dword 0-1 */
	uint64_t result; ///< Command Specific, dwords 0 and 1, see XNVME_SPEC_CQE_RESULT_*

	/* dword 2 */
	uint16_t sqhd; ///< submission queue head pointer
	uint16_t sqid; ///< submission queue identifier

	/* dword 3 */
	uint16_t cid;    ///< command identifier
	uint16_t status; ///< Status Field and Phase Tag, see XNVME_SPEC_CQE_STATUS_*
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cqe) == 16, "Incorrect size")

/** Command Specific, dword 0; bits 31:0 of 'result' */
#define XNVME_SPEC_CQE_RESULT_CDW0 0xffffffff
/** Command Specific, dword 1; bits 63:32 of 'result' */
#define XNVME_SPEC_CQE_RESULT_CDW1 0xffffffff00000000

/** Phase Tag; bit 0 of 'status' */
#define XNVME_SPEC_CQE_STATUS_P 0x1
/** Status Code; bits 8:1 of 'status' */
#define XNVME_SPEC_CQE_STATUS_SC 0x1fe
/** Status Code Type; bits 11:9 of 'status' */
#define XNVME_SPEC_CQE_STATUS_SCT 0xe00
/** Command Retry Delay; bits 13:12 of 'status' */
#define XNVME_SPEC_CQE_STATUS_CRD 0x3000
/** More; bit 14 of 'status' */
#define XNVME_SPEC_CQE_STATUS_M 0x4000
/** Do Not Retry; bit 15 of 'status' */
#define XNVME_SPEC_CQE_STATUS_DNR 0x8000

/**
 * NVMe get-log-page entry for SMART / Health Information
 *
 * The 128-bit counters are little-endian byte arrays. The struct is packed since CTEMP, at byte
 * 1, is not aligned.
 *
 * @see Base 2.4, Figure 213
 * @struct xnvme_spec_log_health_entry
 */
struct __attribute__((packed)) xnvme_spec_log_health_entry {
	uint8_t cw;     ///< CW: Critical Warning, see XNVME_SPEC_LOG_HEALTH_ENTRY_CW_*
	uint16_t ctemp; ///< CTEMP: Composite Temperature, in Kelvin
	uint8_t avsp;   ///< AVSP: Available Spare, in percent
	uint8_t avspt;  ///< AVSPT: Available Spare Threshold, in percent
	uint8_t pused;  ///< PUSED: Percentage Used, can exceed 100
	uint8_t egcws;  ///< EGCWS: Endurance Group Critical Warning Summary, see ..._EGCWS_*
	uint8_t infw;   ///< INFW: Informative Warning, see XNVME_SPEC_LOG_HEALTH_ENTRY_INFW_*
	uint8_t rsvd8[24];

	uint8_t dur[16]; ///< DUR: Data Units Read
	uint8_t duw[16]; ///< DUW: Data Units Written

	uint8_t hrc[16]; ///< HRC: Host Read Commands
	uint8_t hwc[16]; ///< HWC: Host Write Commands

	uint8_t cbt[16];   ///< CBT: Controller Busy Time
	uint8_t pwrc[16];  ///< PWRC: Power Cycles
	uint8_t poh[16];   ///< POH: Power On Hours
	uint8_t upl[16];   ///< UPL: Unexpected Power Losses, formerly Unsafe Shutdowns
	uint8_t mdie[16];  ///< MDIE: Media and Data Integrity Errors
	uint8_t neile[16]; ///< NEILE: Number of Error Information Log Entries

	uint32_t wctt;   ///< WCTT: Warning Composite Temperature Time
	uint32_t cctt;   ///< CCTT: Critical Composite Temperature Time
	uint16_t tsen1;  ///< TSEN1: Temperature Sensor 1, in Kelvin
	uint16_t tsen2;  ///< TSEN2: Temperature Sensor 2, in Kelvin
	uint16_t tsen3;  ///< TSEN3: Temperature Sensor 3, in Kelvin
	uint16_t tsen4;  ///< TSEN4: Temperature Sensor 4, in Kelvin
	uint16_t tsen5;  ///< TSEN5: Temperature Sensor 5, in Kelvin
	uint16_t tsen6;  ///< TSEN6: Temperature Sensor 6, in Kelvin
	uint16_t tsen7;  ///< TSEN7: Temperature Sensor 7, in Kelvin
	uint16_t tsen8;  ///< TSEN8: Temperature Sensor 8, in Kelvin
	uint32_t tmt1tc; ///< TMT1TC: Thermal Management Temperature 1 Transition Count
	uint32_t tmt2tc; ///< TMT2TC: Thermal Management Temperature 2 Transition Count
	uint32_t tttmt1; ///< TTTMT1: Total Time For Thermal Management Temperature 1
	uint32_t tttmt2; ///< TTTMT2: Total Time For Thermal Management Temperature 2
	uint64_t olec;   ///< OLEC: Operational Lifetime Energy Consumed
	uint32_t ipm; ///< IPM: Interval Power Measurement, see XNVME_SPEC_LOG_HEALTH_ENTRY_IPM_*
	uint8_t rsvd244[268];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_log_health_entry) == 512, "Incorrect size")

/** Available Spare Capacity Below Threshold; bit 0 of 'cw', ASCBT (Base 2.4, Figure 213) */
#define XNVME_SPEC_LOG_HEALTH_ENTRY_CW_ASCBT 0x1
/** Temperature Threshold Condition; bit 1 of 'cw', TTC (Base 2.4, Figure 213) */
#define XNVME_SPEC_LOG_HEALTH_ENTRY_CW_TTC 0x2
/** NVM Subsystem Degraded Reliability; bit 2 of 'cw', NDR (Base 2.4, Figure 213) */
#define XNVME_SPEC_LOG_HEALTH_ENTRY_CW_NDR 0x4
/** All Media Read-Only; bit 3 of 'cw', AMRO (Base 2.4, Figure 213) */
#define XNVME_SPEC_LOG_HEALTH_ENTRY_CW_AMRO 0x8
/** Volatile Memory Backup Failed; bit 4 of 'cw', VMBF (Base 2.4, Figure 213) */
#define XNVME_SPEC_LOG_HEALTH_ENTRY_CW_VMBF 0x10
/** Persistent Memory Region Read-Only; bit 5 of 'cw', PMRRO (Base 2.4, Figure 213) */
#define XNVME_SPEC_LOG_HEALTH_ENTRY_CW_PMRRO 0x20
/** Indeterminate Personality State; bit 6 of 'cw', IPS (Base 2.4, Figure 213) */
#define XNVME_SPEC_LOG_HEALTH_ENTRY_CW_IPS 0x40
/** Endurance Group Available Spare Capacity Below Threshold; bit 0 of 'egcws', EGASCBT (Base 2.4,
 * Figure 213) */
#define XNVME_SPEC_LOG_HEALTH_ENTRY_EGCWS_EGASCBT 0x1
/** Endurance Group Degraded Reliability; bit 2 of 'egcws', EGDR (Base 2.4, Figure 213) */
#define XNVME_SPEC_LOG_HEALTH_ENTRY_EGCWS_EGDR 0x4
/** Endurance Group Read-Only; bit 3 of 'egcws', EGRO (Base 2.4, Figure 213) */
#define XNVME_SPEC_LOG_HEALTH_ENTRY_EGCWS_EGRO 0x8
/** Voltage Low Threshold Warning; bit 0 of 'infw', VLTHW (Base 2.4, Figure 213) */
#define XNVME_SPEC_LOG_HEALTH_ENTRY_INFW_VLTHW 0x1
/** Interval Power Measurement Value; bits 15:0 of 'ipm', IPMV (Base 2.4, Figure 213) */
#define XNVME_SPEC_LOG_HEALTH_ENTRY_IPM_IPMV 0xffff
/** Interval Power Measurement Scale; bits 17:16 of 'ipm', IPMS (Base 2.4, Figure 213) */
#define XNVME_SPEC_LOG_HEALTH_ENTRY_IPM_IPMS 0x30000
/** Power Measurement Type; bits 23:20 of 'ipm', PMT (Base 2.4, Figure 213) */
#define XNVME_SPEC_LOG_HEALTH_ENTRY_IPM_PMT 0xf00000

/**
 * NVMe get-log-page entry for sanitize status information
 *
 * @see Base 2.4, Figure 312
 * @struct xnvme_spec_log_sanitize_entry
 */

struct __attribute__((packed)) xnvme_spec_log_sanitize_entry {
	uint16_t sprog;   ///< SPROG: Sanitize Progress
	uint16_t sstat;   ///< SSTAT: Sanitize Status, see XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_*
	uint32_t scdw10;  ///< SCDW10: Sanitize Command Dword 10 Information
	uint32_t eto;     ///< ETO: Estimated Time For Overwrite
	uint32_t etbe;    ///< ETBE: Estimated Time For Block Erase
	uint32_t etce;    ///< ETCE: Estimated Time For Crypto Erase
	uint32_t etodmm;  ///< ETODMM: Estimated Time For Overwrite With No-Deallocate Media
			  ///< Modification
	uint32_t etbenmm; ///< ETBENMM: Estimated Time For Block Erase With No-Deallocate Media
			  ///< Modification
	uint32_t etcenmm; ///< ETCENMM: Estimated Time For Crypto Erase With No-Deallocate Media
			  ///< Modification
	uint32_t etpvds;  ///< ETPVDS: Estimated Time For Post-Verification Deallocation State
	uint8_t ssi; ///< SSI: Sanitize State Information, see XNVME_SPEC_LOG_SANITIZE_ENTRY_SSI_*
	uint8_t rsvd37[3];
	uint32_t mnsoip; ///< MNSOIP: Maximum Namespace Sanitize Operations In Progress
	uint32_t stnsid; ///< STNSID: Sanitization Target NSID
	uint8_t rsvd48[464];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_log_sanitize_entry) == 512, "Incorrect size")

/**
 * Sanitize Operation Status, 000b never started, 001b sanitized, 010b sanitizing, 011b failed,
 * 100b sanitized with unexpected deallocate; bits 2:0 of 'sstat', SOS (Base 2.4, Figure 312)
 */
#define XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_SOS 0x7
/** Overwrite Passes Completed; bits 7:3 of 'sstat', OPC (Base 2.4, Figure 312) */
#define XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_OPC 0xf8
/** Global Data Erased; bit 8 of 'sstat', GDE (Base 2.4, Figure 312) */
#define XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_GDE 0x100
/** Media Verification Canceled; bit 9 of 'sstat', MVCNCLD (Base 2.4, Figure 312) */
#define XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_MVCNCLD 0x200
/** Namespace Data Erased; bit 10 of 'sstat', NDE (Base 2.4, Figure 312) */
#define XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_NDE 0x400
/** Purged; bit 11 of 'sstat', PRGD (Base 2.4, Figure 312) */
#define XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_PRGD 0x800
/** Sanitize State; bits 3:0 of 'ssi', SANS (Base 2.4, Figure 312) */
#define XNVME_SPEC_LOG_SANITIZE_ENTRY_SSI_SANS 0xf
/** Failure State; bits 7:4 of 'ssi', FAILS (Base 2.4, Figure 312) */
#define XNVME_SPEC_LOG_SANITIZE_ENTRY_SSI_FAILS 0xf0

/**
 * NVMe get-log-page entry for error information
 *
 * @see Base 2.4, Figure 212
 * @struct xnvme_spec_log_erri_entry
 */
struct xnvme_spec_log_erri_entry {
	uint64_t ecnt;   ///< ECNT: Error Count
	uint16_t sqid;   ///< SQID: Submission Queue ID
	uint16_t cid;    ///< CID: Command ID
	uint16_t sts;    ///< STS: Status Info, STATUS and P, see XNVME_SPEC_LOG_ERRI_ENTRY_STS_*
	uint16_t pel;    ///< PEL: Parameter Error Location, see XNVME_SPEC_LOG_ERRI_ENTRY_PEL_*
	uint64_t lba;    ///< LBA: Logical Block Address
	uint32_t nsid;   ///< NSID: Namespace Identifier
	uint8_t vsia;    ///< VSIA: Vendor Specific Information Available
	uint8_t trtype;  ///< TRTYPE: Transport Type
	uint8_t csi;     ///< CSI: Command Set Identifier
	uint8_t opc;     ///< OPC: Opcode
	uint64_t csinfo; ///< CSINFO: Command Specific Information
	uint16_t ttsi;   ///< TTSI: Transport Type Specific Information
	uint8_t rsvd42[21];
	uint8_t lpver; ///< LPVER: Log Page Version
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_log_erri_entry) == 64, "Incorrect size")

/** Phase Tag; bit 0 of 'sts', P (Base 2.4, Figure 212) */
#define XNVME_SPEC_LOG_ERRI_ENTRY_STS_P 0x1
/** Status Code, of STATUS; bits 8:1 of 'sts', SC (Base 2.4, Figure 101) */
#define XNVME_SPEC_LOG_ERRI_ENTRY_STS_SC 0x1fe
/** Status Code Type, of STATUS; bits 11:9 of 'sts', SCT (Base 2.4, Figure 101) */
#define XNVME_SPEC_LOG_ERRI_ENTRY_STS_SCT 0xe00
/** Command Retry Delay, of STATUS; bits 13:12 of 'sts', CRD (Base 2.4, Figure 101) */
#define XNVME_SPEC_LOG_ERRI_ENTRY_STS_CRD 0x3000
/** More, of STATUS; bit 14 of 'sts', M (Base 2.4, Figure 101) */
#define XNVME_SPEC_LOG_ERRI_ENTRY_STS_M 0x4000
/** Do Not Retry, of STATUS; bit 15 of 'sts', DNR (Base 2.4, Figure 101) */
#define XNVME_SPEC_LOG_ERRI_ENTRY_STS_DNR 0x8000
/** Byte Location; bits 7:0 of 'pel', BYTLOC (Base 2.4, Figure 212) */
#define XNVME_SPEC_LOG_ERRI_ENTRY_PEL_BYTLOC 0xff
/** Bit Location; bits 10:8 of 'pel', BITLOC (Base 2.4, Figure 212) */
#define XNVME_SPEC_LOG_ERRI_ENTRY_PEL_BITLOC 0x700

/**
 * NVMe Reclaim unit handle descriptor
 *
 * @struct xnvme_spec_ruh_desc
 */
struct xnvme_spec_ruh_desc {
	uint8_t ruht; ///< Reclaim unit handle type
	uint8_t rsvd[3];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_ruh_desc) == 4, "Incorrect size")

/**
 * NVMe FDP configuration descriptor
 *
 * @struct xnvme_spec_fdp_conf_desc
 */
struct xnvme_spec_fdp_conf_desc {
	uint16_t ds; ///< Descriptor Size

	uint8_t fdpa; ///< FDP Attributes, see XNVME_SPEC_FDP_CONF_DESC_FDPA_*

	uint8_t vss;      ///< Vendor specific size
	uint32_t nrg;     ///< Number of reclaim groups
	uint16_t nruh;    ///< Number of reclaim unit handles
	uint16_t maxpids; ///< Max placement identifiers
	uint32_t nns;     ///< Number of namespaces supported
	uint64_t runs;    ///< Reclaim unit nominal size
	uint32_t erutl;   ///< Estimated reclaim unit time limit
	uint8_t rsvd28[36];
	struct xnvme_spec_ruh_desc ruh_desc[];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_fdp_conf_desc) == 64, "Incorrect size")

/** Reclaim Group Identifier Format; bits 3:0 of 'fdpa' */
#define XNVME_SPEC_FDP_CONF_DESC_FDPA_RGIF 0xf
/** FDP Volatile Write Cache; bit 4 of 'fdpa' */
#define XNVME_SPEC_FDP_CONF_DESC_FDPA_FDPVWC 0x10
/** FDP Configuration Valid; bit 7 of 'fdpa' */
#define XNVME_SPEC_FDP_CONF_DESC_FDPA_FDPCV 0x80

/**
 * NVMe FDP configurations log page
 *
 * @struct xnvme_spec_log_fdp_conf
 */
struct xnvme_spec_log_fdp_conf {
	uint16_t ncfg; ///< Number of FDP configurations
	uint8_t version;
	uint8_t rsvd1;
	uint32_t size;
	uint8_t rsvd2[8];
	struct xnvme_spec_fdp_conf_desc conf_desc[];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_log_fdp_conf) == 16, "Incorrect size")

/**
 * NVMe Reclaim unit handle usage descriptor
 *
 * @struct xnvme_spec_ruhu_desc
 */
struct xnvme_spec_ruhu_desc {
	uint8_t ruha; ///< Reclaim unit handle attributes
	uint8_t rsvd[7];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_ruhu_desc) == 8, "Incorrect size")

/**
 * NVMe Reclaim unit handle usage log page
 *
 * @struct xnvme_spec_log_ruhu
 */
struct xnvme_spec_log_ruhu {
	uint16_t nruh; ///< Number of Reclaim Unit Handles
	uint8_t rsvd[6];
	struct xnvme_spec_ruhu_desc ruhu_desc[];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_log_ruhu) == 8, "Incorrect size")

/**
 * NVMe FDP statistics log page
 *
 * @struct xnvme_spec_log_fdp_stats
 */
struct xnvme_spec_log_fdp_stats {
	uint64_t hbmw[2]; ///< Host bytes with metadata written
	uint64_t mbmw[2]; ///< Media bytes with metadata written
	uint64_t mbe[2];  ///< Media bytes erased
	uint8_t rsvd48[16];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_log_fdp_stats) == 64, "Incorrect size")

/**
 * Media reallocated
 *
 * @struct xnvme_spec_fdp_event_media_reallocated
 */
struct __attribute__((packed)) xnvme_spec_fdp_event_media_reallocated {
	uint8_t sef; ///< Specific Event Flags, see XNVME_SPEC_FDP_EVENT_MEDIA_REALLOCATED_SEF_*

	uint8_t rsvd1;
	uint16_t nlbam; ///< Number of LBAs moved
	uint64_t lba;   ///< Logical block address
	uint8_t rsvd2[4];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_fdp_event_media_reallocated) == 16, "Incorrect size")

/** LBA Valid; bit 0 of 'sef' */
#define XNVME_SPEC_FDP_EVENT_MEDIA_REALLOCATED_SEF_LBAV 0x1

/**
 * NVMe FDP event
 *
 * @struct xnvme_spec_fdp_event
 */
struct __attribute__((packed)) xnvme_spec_fdp_event {
	uint8_t type; ///< Event type

	uint8_t fdpef; ///< FDP Event Flags, see XNVME_SPEC_FDP_EVENT_FDPEF_*

	uint16_t pid;              ///< Placement identifier
	uint64_t timestamp;        ///< Event timestamp
	uint32_t nsid;             ///< Namespace identifier
	uint8_t type_specific[16]; ///< Event type specific
	uint16_t rgid;             ///< Reclaim group identifier
	uint16_t ruhid;            ///< Reclaim unit handle identifier
	uint8_t rsvd1[4];
	uint8_t vs[24];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_fdp_event) == 64, "Incorrect size")

/** Placement Identifier Valid; bit 0 of 'fdpef' */
#define XNVME_SPEC_FDP_EVENT_FDPEF_PIV 0x1
/** Namespace Identifier Valid; bit 1 of 'fdpef' */
#define XNVME_SPEC_FDP_EVENT_FDPEF_NSIDV 0x2
/** Location Valid; bit 2 of 'fdpef' */
#define XNVME_SPEC_FDP_EVENT_FDPEF_LV 0x4

/**
 * NVMe FDP events log page
 *
 * @struct xnvme_spec_log_fdp_events
 */
struct xnvme_spec_log_fdp_events {
	uint32_t nevents; ///< Number of FDP events
	uint8_t rsvd[60];
	struct xnvme_spec_fdp_event event[];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_log_fdp_events) == 64, "Incorrect size")

/**
 * NVMe FDP event descriptor
 *
 * @struct xnvme_spec_fdp_event_desc
 */
struct xnvme_spec_fdp_event_desc {
	uint8_t type; ///< Event type

	uint8_t fdpeta; ///< FDP Event Type Attributes, see XNVME_SPEC_FDP_EVENT_DESC_FDPETA_*
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_fdp_event_desc) == 2, "Incorrect size")

/** Event Enabled; bit 0 of 'fdpeta' */
#define XNVME_SPEC_FDP_EVENT_DESC_FDPETA_EE 0x1

/**
 * Identifiers (lpi) for NVMe get-log-page
 *
 * NVMe 1.4 - Figure ?
 *
 * @enum xnvme_spec_log_lpi
 */
enum xnvme_spec_log_lpi {
	XNVME_SPEC_LOG_RSVD      = 0x0,  ///< XNVME_SPEC_LOG_RSVD
	XNVME_SPEC_LOG_ERRI      = 0x1,  ///< XNVME_SPEC_LOG_ERRI
	XNVME_SPEC_LOG_HEALTH    = 0x2,  ///< XNVME_SPEC_LOG_HEALTH
	XNVME_SPEC_LOG_FW        = 0x3,  ///< XNVME_SPEC_LOG_FW
	XNVME_SPEC_LOG_CHNS      = 0x4,  ///< XNVME_SPEC_LOG_CHNS
	XNVME_SPEC_LOG_CSAE      = 0x5,  ///< XNVME_SPEC_LOG_CSAE
	XNVME_SPEC_LOG_SELFTEST  = 0x6,  ///< XNVME_SPEC_LOG_SELFTEST
	XNVME_SPEC_LOG_TELEHOST  = 0x7,  ///< XNVME_SPEC_LOG_TELEHOST
	XNVME_SPEC_LOG_TELECTRLR = 0x8,  ///< XNVME_SPEC_LOG_TELECTRLR
	XNVME_SPEC_LOG_FDPCONF   = 0x20, ///< XNVME_SPEC_LOG_FDPCONF
	XNVME_SPEC_LOG_FDPRUHU   = 0x21, ///< XNVME_SPEC_LOG_FDPRUHU
	XNVME_SPEC_LOG_FDPSTATS  = 0x22, ///< XNVME_SPEC_LOG_FDPSTATS
	XNVME_SPEC_LOG_FDPEVENTS = 0x23, ///< XNVME_SPEC_LOG_FDPEVENTS
	XNVME_SPEC_LOG_SANITIZE  = 0x81, ///< XNVME_SPEC_LOG_SANITIZE
};

/**
 * Management Operation (MO) of the I/O Management Receive command
 *
 * @enum xnvme_spec_io_mgmt_recv_mo
 */
enum xnvme_spec_io_mgmt_recv_mo {
	// Reclaim Unit Handle Status
	XNVME_SPEC_IO_MGMT_RECV_RUHS = 0x1,
};

/**
 * NVMe Reclaim unit handle status descriptor
 *
 * @struct xnvme_spec_ruhs_desc
 */
struct xnvme_spec_ruhs_desc {
	uint16_t pi;     ///< Placement Identifier
	uint16_t ruhi;   ///< Reclaim Unit Handle Identifier
	uint32_t earutr; ///< Estimated Active Reclaim Unit Time Remaining
	uint64_t ruamw;  ///< Reclaim Unit Available Media Writes
	uint8_t rsvd[16];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_ruhs_desc) == 32, "Incorrect size")

/**
 * NVMe Reclaim Unit Handle Status
 *
 * @struct xnvme_spec_ruhs
 */
struct xnvme_spec_ruhs {
	uint8_t rsvd[14];
	uint16_t nruhsd; ///< Number of Reclaim Unit Handle Status Descriptors
	struct xnvme_spec_ruhs_desc desc[];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_ruhs) == 16, "Incorrect size")

/**
 * Management Operation (MO) of the I/O Management Send command
 *
 * @enum xnvme_spec_io_mgmt_send_mo
 */
enum xnvme_spec_io_mgmt_send_mo {
	// Reclaim Unit Handle Update
	XNVME_SPEC_IO_MGMT_SEND_RUHU = 0x1,
};

/**
 * Controller or Namespace Structure (CNS) of the Identify command, the data it returns
 *
 * @enum xnvme_spec_idfy_cns
 */
enum xnvme_spec_idfy_cns {
	XNVME_SPEC_IDFY_NS      = 0x0, ///< XNVME_SPEC_IDFY_NS
	XNVME_SPEC_IDFY_CTRLR   = 0x1, ///< XNVME_SPEC_IDFY_CTRLR
	XNVME_SPEC_IDFY_NSLIST  = 0x2, ///< XNVME_SPEC_IDFY_NSLIST
	XNVME_SPEC_IDFY_NS_DESC = 0x3, ///< XNVME_SPEC_IDFY_NS_DESC
	XNVME_SPEC_IDFY_SETL    = 0x4, ///< XNVME_SPEC_IDFY_SETL

	XNVME_SPEC_IDFY_NS_IOCS     = 0x05, ///< XNVME_SPEC_IDFY_NS_IOCS
	XNVME_SPEC_IDFY_CTRLR_IOCS  = 0x6,  ///< XNVME_SPEC_IDFY_CTRLR_IOCS
	XNVME_SPEC_IDFY_NSLIST_IOCS = 0x7,  ///< XNVME_SPEC_IDFY_NSLIST_IOCS

	XNVME_SPEC_IDFY_NSLIST_ALLOC = 0x10, ///< XNVME_SPEC_IDFY_NSLIST_ALLOC
	XNVME_SPEC_IDFY_NS_ALLOC     = 0x11, ///< XNVME_SPEC_IDFY_NS_ALLOC
	XNVME_SPEC_IDFY_CTRLR_NS     = 0x12, ///< XNVME_SPEC_IDFY_CTRLR_NS
	XNVME_SPEC_IDFY_CTRLR_SUB    = 0x13, ///< XNVME_SPEC_IDFY_CTRLR_SUB
	XNVME_SPEC_IDFY_CTRLR_PRI    = 0x14, ///< XNVME_SPEC_IDFY_CTRLR_PRI
	XNVME_SPEC_IDFY_CTRLR_SEC    = 0x15, ///< XNVME_SPEC_IDFY_CTRLR_SEC
	XNVME_SPEC_IDFY_NSGRAN       = 0x16, ///< XNVME_SPEC_IDFY_NSGRAN
	XNVME_SPEC_IDFY_UUIDL        = 0x17, ///< XNVME_SPEC_IDFY_UUIDL

	XNVME_SPEC_IDFY_NSLIST_ALLOC_IOCS = 0x1A, ///< XNVME_SPEC_IDFY_NSLIST_ALLOC_IOCS
	XNVME_SPEC_IDFY_NS_ALLOC_IOCS     = 0x1B, ///< XNVME_SPEC_IDFY_NS_ALLOC_IOCS
	XNVME_SPEC_IDFY_IOCS              = 0x1C, ///< XNVME_SPEC_IDFY_IOCS
};

/**
 * LBA Format
 *
 * @see NVM Command Set 1.3, Figure 125
 * @struct xnvme_spec_lbaf
 */
struct xnvme_spec_lbaf {
	uint16_t ms;   ///< MS: Metadata Size, in bytes
	uint8_t lbads; ///< LBADS: LBA Data Size, as a power of two
	uint8_t rp;    ///< Bits 31:24, see XNVME_SPEC_LBAF_RP_*
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_lbaf) == 4, "Incorrect size")

/** Relative Performance, 00b best to 11b degraded; bits 25:24 of the LBA format, bits 1:0 of 'rp',
 * RP (NVM Command Set 1.3, Figure 125) */
#define XNVME_SPEC_LBAF_RP_RP 0x3

/**
 * Command Set Identifiers
 *
 * @see Base 2.4, Figure 337
 *
 * @enum xnvme_spec_csi
 */
enum xnvme_spec_csi {
	XNVME_SPEC_CSI_NVM   = 0x0, ///< XNVME_SPEC_CSI_NVM
	XNVME_SPEC_CSI_KV    = 0x1, ///< XNVME_SPEC_CSI_KV
	XNVME_SPEC_CSI_ZONED = 0x2, ///< XNVME_SPEC_CSI_ZONED
};

/**
 * Namespace Identifier Type (NIDT) of a Namespace Identification Descriptor
 *
 * @see NVM Express Base Specification 2.3, Section 5.2.13.2.3, Figure 331 (Identify - Namespace
 * Identification Descriptor)
 *
 * @enum xnvme_spec_ns_desc_nidt
 */
enum xnvme_spec_ns_desc_nidt {
	XNVME_SPEC_NS_DESC_NIDT_IEUID = 0x1, ///< IEEE Extended Unique Identifier (IEUID)
	XNVME_SPEC_NS_DESC_NIDT_NGUID = 0x2, ///< Namespace Globally Unique Identifier (NGUID)
	XNVME_SPEC_NS_DESC_NIDT_NUUID = 0x3, ///< Namespace UUID (NUUID)
	XNVME_SPEC_NS_DESC_NIDT_CSI   = 0x4, ///< Command Set Identifier (CSI)
};

/**
 * Namespace Identification Descriptor
 *
 * That is, one entry in the list returned for opcode XNVME_SPEC_OPC_IDFY(0x06) with
 * XNVME_SPEC_IDFY_NS_DESC(0x3). Only the first 'nidl' bytes of 'nid' are valid, the next
 * descriptor starts at offset 4 + 'nidl', and the list is terminated by a descriptor with
 * 'nidl' equal to zero (NVMe Base 2.3, 5.2.13.2.3).
 *
 * @see NVM Express Base Specification 2.3, Section 5.2.13.2.3, Figure 331 (Identify - Namespace
 * Identification Descriptor)
 *
 * @struct xnvme_spec_ns_desc
 */
struct xnvme_spec_ns_desc {
	uint8_t nidt;     ///< Namespace Identifier Type (NIDT)
	uint8_t nidl;     ///< Namespace Identifier Length (NIDL)
	uint8_t rsvd2[2]; ///< Reserved
	uint8_t nid[16];  ///< Namespace Identifier, 8 (IEUID), 16 (NGUID, NUUID) or 1 (CSI) bytes
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_ns_desc) == 20, "Incorrect size")

/**
 * Representation of NVMe completion result Identify Namespace
 *
 * That is, for opcode XNVME_SPEC_OPC_IDFY(0x06) with XNVME_SPEC_IDFY_NS(0x0). A field of bits
 * is an integer of the width the specification gives it; its documentation names the bits.
 *
 * @see NVM Command Set 1.3, Figure 123
 * @struct xnvme_spec_idfy_ns
 */
struct xnvme_spec_idfy_ns {
	uint64_t nsze; ///< NSZE: Namespace Size
	uint64_t ncap; ///< NCAP: Namespace Capacity
	uint64_t nuse; ///< NUSE: Namespace Utilization

	uint8_t nsfeat; ///< NSFEAT: Namespace Features, see XNVME_SPEC_IDFY_NS_NSFEAT_*

	uint8_t nlbaf; ///< NLBAF: Number of LBA Formats, zero-based

	uint8_t flbas; ///< FLBAS: Formatted LBA Size, see XNVME_SPEC_IDFY_NS_FLBAS_*

	uint8_t mc; ///< MC: Metadata Capabilities, see XNVME_SPEC_IDFY_NS_MC_*

	uint8_t dpc; ///< DPC: End-to-end Data Protection Capabilities, see
		     ///< XNVME_SPEC_IDFY_NS_DPC_*

	uint8_t dps; ///< DPS: End-to-end Data Protection Type Settings, see
		     ///< XNVME_SPEC_IDFY_NS_DPS_*

	uint8_t nmic; ///< NMIC: Namespace Multi-path I/O and Namespace Sharing Capabilities, see
		      ///< XNVME_SPEC_IDFY_NS_NMIC_*

	uint8_t rescap; ///< RESCAP: Reservation Capabilities, see XNVME_SPEC_IDFY_NS_RESCAP_*

	uint8_t fpi; ///< FPI: Format Progress Indicator, see XNVME_SPEC_IDFY_NS_FPI_*

	uint8_t dlfeat; ///< DLFEAT: Deallocate Logical Block Features, see
			///< XNVME_SPEC_IDFY_NS_DLFEAT_*

	uint16_t nawun; ///< NAWUN: Namespace Atomic Write Unit Normal

	uint16_t nawupf; ///< NAWUPF: Namespace Atomic Write Unit Power Fail

	uint16_t nacwu; ///< NACWU: Namespace Atomic Compare & Write Unit

	uint16_t nabsn; ///< NABSN: Namespace Atomic Boundary Size Normal

	uint16_t nabo; ///< NABO: Namespace Atomic Boundary Offset

	uint16_t nabspf; ///< NABSPF: Namespace Atomic Boundary Size Power Fail

	uint16_t noiob; ///< NOIOB: Namespace Optimal I/O Boundary, in logical blocks

	uint64_t nvmcap[2]; ///< NVMCAP: NVM Capacity, a 128-bit integer, low half first

	uint16_t npwg; ///< NPWG: Namespace Preferred Write Granularity

	uint16_t npwa; ///< NPWA: Namespace Preferred Write Alignment

	uint16_t npdg; ///< NPDG: Namespace Preferred Deallocate Granularity

	uint16_t npda; ///< NPDA: Namespace Preferred Deallocate Alignment

	uint16_t nows; ///< NOWS: Namespace Optimal Write Size

	uint16_t mssrl; ///< MSSRL: Maximum Single Source Range Length

	uint32_t mcl; ///< MCL: Maximum Copy Length

	uint8_t msrc; ///< MSRC: Maximum Source Range Count, zero-based

	uint8_t kpios; ///< KPIOS: Key Per I/O Status, see XNVME_SPEC_IDFY_NS_KPIOS_*

	uint8_t nulbaf; ///< NULBAF: Number of Unique Attribute LBA Formats

	uint8_t rsvd83;

	uint32_t kpiodaag; ///< KPIODAAG: Key Per I/O Data Access Alignment and Granularity

	uint8_t rsvd88[4];

	uint32_t anagrpid; ///< ANAGRPID: ANA Group Identifier

	uint8_t rsvd96[3];

	uint8_t nsattr; ///< NSATTR: Namespace Attributes, see XNVME_SPEC_IDFY_NS_NSATTR_*

	uint16_t nvmsetid; ///< NVMSETID: NVM Set Identifier

	uint16_t endgid; ///< ENDGID: Endurance Group Identifier

	uint8_t nguid[16]; ///< NGUID: Namespace Globally Unique Identifier

	uint64_t eui64; ///< EUI64: IEEE Extended Unique Identifier

	struct xnvme_spec_lbaf lbaf[64]; ///< LBAF0 to LBAF63: LBA Format Support

	uint8_t vs[3712]; ///< VS: Vendor Specific
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_idfy_ns) == 4096, "Incorrect size")

/** Thin Provisioning; bit 0 of 'nsfeat', THINP (NVM Command Set 1.3, Figure 123) */
#define XNVME_SPEC_IDFY_NS_NSFEAT_THINP 0x1
/** Namespace Supported Atomic Boundary & Power, NAWUN, NAWUPF and NACWU are defined; bit 1 of
 * 'nsfeat', NSABP (NVM Command Set 1.3, Figure 123) */
#define XNVME_SPEC_IDFY_NS_NSFEAT_NSABP 0x2
/** Deallocated Error; bit 2 of 'nsfeat', DAE (NVM Command Set 1.3, Figure 123) */
#define XNVME_SPEC_IDFY_NS_NSFEAT_DAE 0x4
/** UID Reuse, non-zero NGUID and EUI64 are never reused when cleared; bit 3 of 'nsfeat', UIDREUSE
 * (NVM Command Set 1.3, Figure 123) */
#define XNVME_SPEC_IDFY_NS_NSFEAT_UIDREUSE 0x8
/** Optional Write Performance, NPWG, NPWA, NPDG, NPDA, NOWS and, by bit 5, NPDGL and NPDAL are
 * defined; bits 5:4 of 'nsfeat', OPTPERF (NVM Command Set 1.3, Figure 123) */
#define XNVME_SPEC_IDFY_NS_NSFEAT_OPTPERF 0x30
/** Multiple Atomicity Mode; bit 6 of 'nsfeat', MAM (NVM Command Set 1.3, Figure 123) */
#define XNVME_SPEC_IDFY_NS_NSFEAT_MAM 0x40
/** Optional Read Performance, NPRG, NPRA and NORS are defined; bit 7 of 'nsfeat', OPTRPERF (NVM
 * Command Set 1.3, Figure 123) */
#define XNVME_SPEC_IDFY_NS_NSFEAT_OPTRPERF 0x80
/** Format Index Lower, the least significant bits of the index into 'lbaf'; bits 3:0 of 'flbas',
 * FIDXL (NVM Command Set 1.3, Figure 123) */
#define XNVME_SPEC_IDFY_NS_FLBAS_FIDXL 0xf
/** Metadata Transferred as Extended LBA, at the end of the data; bit 4 of 'flbas', MTELBA (NVM
 * Command Set 1.3, Figure 123) */
#define XNVME_SPEC_IDFY_NS_FLBAS_MTELBA 0x10
/** Format Index Upper, the most significant bits of the index into 'lbaf'; bits 6:5 of 'flbas',
 * FIDXU (NVM Command Set 1.3, Figure 123) */
#define XNVME_SPEC_IDFY_NS_FLBAS_FIDXU 0x60
/** Metadata Transferred as Extended LBA Support; bit 0 of 'mc', MTELBAS (NVM Command Set 1.3,
 * Figure 123) */
#define XNVME_SPEC_IDFY_NS_MC_MTELBAS 0x1
/** Metadata Transferred as Separate Buffer Support; bit 1 of 'mc', MTSBS (NVM Command Set 1.3,
 * Figure 123) */
#define XNVME_SPEC_IDFY_NS_MC_MTSBS 0x2
/** Protection Information Type 1 Supported; bit 0 of 'dpc', PIT1S (NVM Command Set 1.3, Figure
 * 123) */
#define XNVME_SPEC_IDFY_NS_DPC_PIT1S 0x1
/** Protection Information Type 2 Supported; bit 1 of 'dpc', PIT2S (NVM Command Set 1.3, Figure
 * 123) */
#define XNVME_SPEC_IDFY_NS_DPC_PIT2S 0x2
/** Protection Information Type 3 Supported; bit 2 of 'dpc', PIT3S (NVM Command Set 1.3, Figure
 * 123) */
#define XNVME_SPEC_IDFY_NS_DPC_PIT3S 0x4
/** Protection Information In First Bytes of the metadata; bit 3 of 'dpc', PIIFB (NVM Command
 * Set 1.3, Figure 123) */
#define XNVME_SPEC_IDFY_NS_DPC_PIIFB 0x8
/** Protection Information In Last Bytes of the metadata; bit 4 of 'dpc', PIILB (NVM Command
 * Set 1.3, Figure 123) */
#define XNVME_SPEC_IDFY_NS_DPC_PIILB 0x10
/** Protection Information Type, 0 when disabled; bits 2:0 of 'dps', PIT (NVM Command Set 1.3,
 * Figure 123) */
#define XNVME_SPEC_IDFY_NS_DPS_PIT 0x7
/** Protection Information Position, the start of the metadata, else its end; bit 3 of 'dps', PIP
 * (NVM Command Set 1.3, Figure 123) */
#define XNVME_SPEC_IDFY_NS_DPS_PIP 0x8
/** Shared Namespace; bit 0 of 'nmic', SHRNS (Base 2.4, Figure 346) */
#define XNVME_SPEC_IDFY_NS_NMIC_SHRNS 0x1
/** Dispersed Namespace; bit 1 of 'nmic', DISNS (Base 2.4, Figure 346) */
#define XNVME_SPEC_IDFY_NS_NMIC_DISNS 0x2
/** Persist Through Power Loss Support; bit 0 of 'rescap', PTPLS (Base 2.4, Figure 346) */
#define XNVME_SPEC_IDFY_NS_RESCAP_PTPLS 0x1
/** Write Exclusive Support; bit 1 of 'rescap', WES (Base 2.4, Figure 346) */
#define XNVME_SPEC_IDFY_NS_RESCAP_WES 0x2
/** Exclusive Access Support; bit 2 of 'rescap', EAS (Base 2.4, Figure 346) */
#define XNVME_SPEC_IDFY_NS_RESCAP_EAS 0x4
/** Write Exclusive - Registrants Only Support; bit 3 of 'rescap', WEROS (Base 2.4, Figure 346) */
#define XNVME_SPEC_IDFY_NS_RESCAP_WEROS 0x8
/** Exclusive Access - Registrants Only Support; bit 4 of 'rescap', EAROS (Base 2.4, Figure 346) */
#define XNVME_SPEC_IDFY_NS_RESCAP_EAROS 0x10
/** Write Exclusive - All Registrants Support; bit 5 of 'rescap', WEARS (Base 2.4, Figure 346) */
#define XNVME_SPEC_IDFY_NS_RESCAP_WEARS 0x20
/** Exclusive Access - All Registrants Support; bit 6 of 'rescap', EAARS (Base 2.4, Figure 346) */
#define XNVME_SPEC_IDFY_NS_RESCAP_EAARS 0x40
/** Ignore Existing Key Support; bit 7 of 'rescap', IEKS (Base 2.4, Figure 346) */
#define XNVME_SPEC_IDFY_NS_RESCAP_IEKS 0x80
/** Remaining Format NVM, in percent; bits 6:0 of 'fpi', RFNVM (Base 2.4, Figure 346) */
#define XNVME_SPEC_IDFY_NS_FPI_RFNVM 0x7f
/** Format Progress Indicator Support; bit 7 of 'fpi', FPIS (Base 2.4, Figure 346) */
#define XNVME_SPEC_IDFY_NS_FPI_FPIS 0x80
/** Deallocation Read Behavior: 000b not reported, 001b all bytes 0x00, 010b all bytes 0xFF;
 * bits 2:0 of 'dlfeat', DRB (NVM Command Set 1.3, Figure 123) */
#define XNVME_SPEC_IDFY_NS_DLFEAT_DRB 0x7
/** Write Zeroes Deallocation Support; bit 3 of 'dlfeat', WZDS (NVM Command Set 1.3, Figure 123) */
#define XNVME_SPEC_IDFY_NS_DLFEAT_WZDS 0x8
/** Guard Deallocation Status, the guard of a deallocated block is the CRC of the value read, else
 * 0xFFFF; bit 4 of 'dlfeat', GDS (NVM Command Set 1.3, Figure 123) */
#define XNVME_SPEC_IDFY_NS_DLFEAT_GDS 0x10
/** Key Per I/O Enabled in Namespace; bit 0 of 'kpios', KPIOENS (NVM Command Set 1.3, Figure 123)
 */
#define XNVME_SPEC_IDFY_NS_KPIOS_KPIOENS 0x1
/** Key Per I/O Supported in Namespace; bit 1 of 'kpios', KPIOSNS (NVM Command Set 1.3, Figure 123)
 */
#define XNVME_SPEC_IDFY_NS_KPIOS_KPIOSNS 0x2
/** Currently Write Protected; bit 0 of 'nsattr', CWP (Base 2.4, Figure 346) */
#define XNVME_SPEC_IDFY_NS_NSATTR_CWP 0x1

/**
 * Protection Information in the 16b Guard Protection Information format
 *
 * The specification gives these fields no abbreviation; the members are named by their titles.
 * Each is big-endian.
 *
 * @see NVM Command Set 1.3, Figures 155 and 156
 * @struct xnvme_pif_g16
 */
struct xnvme_pif_g16 {
	uint16_t guard;          ///< Guard, bytes 1:0
	uint16_t app_tag;        ///< Application Tag, bytes 3:2
	uint32_t stor_ref_space; ///< Storage and Reference Space, bytes 7:4
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_pif_g16) == 8, "Incorrect size")

/**
 * Protection Information in the 32b Guard Protection Information format
 *
 * @see NVM Command Set 1.3, Figure 157
 * @struct xnvme_pif_g32
 */
struct xnvme_pif_g32 {
	uint32_t guard;             ///< Guard, bytes 3:0
	uint16_t app_tag;           ///< Application Tag, bytes 5:4
	uint8_t stor_ref_space[10]; ///< Storage and Reference Space, bytes 15:6, 80 bits
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_pif_g32) == 16, "Incorrect size")

/**
 * Protection Information in the 64b Guard Protection Information format
 *
 * @see NVM Command Set 1.3, Figure 159
 * @struct xnvme_pif_g64
 */
struct xnvme_pif_g64 {
	uint64_t guard;            ///< Guard, bytes 7:0
	uint16_t app_tag;          ///< Application Tag, bytes 9:8
	uint8_t stor_ref_space[6]; ///< Storage and Reference Space, bytes 15:10, 48 bits
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_pif_g64) == 16, "Incorrect size")

/**
 * Protection Information, in the format of the namespace, see enum xnvme_spec_nvm_ns_pif
 *
 * @union xnvme_pif
 */
union xnvme_pif {
	struct xnvme_pif_g16 g16; ///< As 16b Guard Protection Information
	struct xnvme_pif_g32 g32; ///< As 32b Guard Protection Information
	struct xnvme_pif_g64 g64; ///< As 64b Guard Protection Information
};
XNVME_STATIC_ASSERT(sizeof(union xnvme_pif) == 16, "Incorrect size")

/**
 * Protection Information Format (PIF) of an Extended LBA Format, the size of the Guard
 *
 * @enum xnvme_spec_nvm_ns_pif
 */
enum xnvme_spec_nvm_ns_pif {
	// 16 bit guard protection information
	XNVME_SPEC_NVM_NS_16B_GUARD = 0x0,
	// 32 bit guard protection information
	XNVME_SPEC_NVM_NS_32B_GUARD = 0x1,
	// 64 bit guard protection information
	XNVME_SPEC_NVM_NS_64B_GUARD = 0x2,
	// Qualified Type (QTYPE), the format is given by QPIF
	XNVME_SPEC_NVM_NS_QTYPE = 0x3,
};

/**
 * Extended LBA Format
 *
 * @see NVM Command Set 1.3, Figure 128
 * @struct xnvme_spec_elbaf
 */
struct xnvme_spec_elbaf {
	uint32_t bits; ///< Extended LBA format, see XNVME_SPEC_ELBAF_BITS_*
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_elbaf) == 4, "Incorrect size")

/** Storage Tag Size; bits 6:0 of 'bits', STS (NVM Command Set 1.3, Figure 128) */
#define XNVME_SPEC_ELBAF_BITS_STS 0x7f
/** Protection Information Format, see xnvme_spec_nvm_ns_pif; bits 8:7 of 'bits', PIF (NVM Command
 * Set 1.3, Figure 128) */
#define XNVME_SPEC_ELBAF_BITS_PIF 0x180
/** Qualified Protection Information Format, if PIF is QTYPE; bits 12:9 of 'bits', QPIF (NVM
 * Command Set 1.3, Figure 128) */
#define XNVME_SPEC_ELBAF_BITS_QPIF 0x1e00

/** Size, in bytes, of the Serial Number (SN) of the Identify Controller data structure */
#define XNVME_SPEC_CTRLR_SN_LEN 20
/** Size, in bytes, of the Model Number (MN) of the Identify Controller data structure */
#define XNVME_SPEC_CTRLR_MN_LEN 40
/** Size, in bytes, of the Firmware Revision (FR) of the Identify Controller data structure */
#define XNVME_SPEC_CTRLR_FR_LEN 8

/**
 * Power State Descriptor
 *
 * @see Base 2.4, Figure 340
 * @struct xnvme_spec_power_state
 */
struct xnvme_spec_power_state {
	uint16_t mp; ///< MP: Maximum Power, bits 15:00

	uint8_t rsvd2;

	uint8_t mxps_nops; ///< Bits 31:24, see XNVME_SPEC_POWER_STATE_MXPS_NOPS_*

	uint32_t enlat; ///< ENLAT: Entry Latency, in microseconds, bits 63:32
	uint32_t exlat; ///< EXLAT: Exit Latency, in microseconds, bits 95:64

	uint8_t rrt; ///< RRT: Relative Read Throughput, see XNVME_SPEC_POWER_STATE_RRT_*
	uint8_t rrl; ///< RRL: Relative Read Latency, see XNVME_SPEC_POWER_STATE_RRL_*
	uint8_t rwt; ///< RWT: Relative Write Throughput, see XNVME_SPEC_POWER_STATE_RWT_*
	uint8_t rwl; ///< RWL: Relative Write Latency, see XNVME_SPEC_POWER_STATE_RWL_*

	uint16_t idlp; ///< IDLP: Idle Power, bits 143:128

	uint8_t ips; ///< IPS: Idle Power Scale, see XNVME_SPEC_POWER_STATE_IPS_*

	uint8_t rsvd19;

	uint16_t actp; ///< ACTP: Active Power, bits 175:160

	uint8_t apw_aps; ///< Bits 183:176, see XNVME_SPEC_POWER_STATE_APW_APS_*

	uint8_t epfrt; ///< EPFRT: Emergency Power Fail Recovery Time, bits 191:184
	uint8_t fqvt;  ///< FQVT: Forced Quiescence Vault Time, bits 199:192
	uint8_t epfvt; ///< EPFVT: Emergency Power Fail Vault Time, bits 207:200

	uint8_t epfrts_fqvts; ///< Bits 215:208, see XNVME_SPEC_POWER_STATE_EPFRTS_FQVTS_*
	uint8_t epfvts;       ///< Bits 223:216, see XNVME_SPEC_POWER_STATE_EPFVTS_*

	uint8_t mbw;  ///< MBW: Max Bandwidth, bits 231:224
	uint8_t mbws; ///< Bits 239:232, see XNVME_SPEC_POWER_STATE_MBWS_*

	uint16_t miiell; ///< MIIELL: Minimum Idle I/O Exit Latency Limit, bits 255:240
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_power_state) == 32, "Incorrect size")

/** Max Power Scale; bit 0 of 'mxps_nops', bit 24 of the descriptor, MXPS (Base 2.4, Figure 340) */
#define XNVME_SPEC_POWER_STATE_MXPS_NOPS_MXPS 0x1
/** Non-Operational State; bit 1 of 'mxps_nops', bit 25 of the descriptor, NOPS (Base 2.4, Figure
 * 340) */
#define XNVME_SPEC_POWER_STATE_MXPS_NOPS_NOPS 0x2
/** Relative Read Throughput; bits 4:0 of 'rrt', RRT (Base 2.4, Figure 340) */
#define XNVME_SPEC_POWER_STATE_RRT_RRT 0x1f
/** Relative Read Latency; bits 4:0 of 'rrl', RRL (Base 2.4, Figure 340) */
#define XNVME_SPEC_POWER_STATE_RRL_RRL 0x1f
/** Relative Write Throughput; bits 4:0 of 'rwt', RWT (Base 2.4, Figure 340) */
#define XNVME_SPEC_POWER_STATE_RWT_RWT 0x1f
/** Relative Write Latency; bits 4:0 of 'rwl', RWL (Base 2.4, Figure 340) */
#define XNVME_SPEC_POWER_STATE_RWL_RWL 0x1f
/** Idle Power Scale; bits 7:6 of 'ips', bits 151:150 of the descriptor, IPS (Base 2.4, Figure 340)
 */
#define XNVME_SPEC_POWER_STATE_IPS_IPS 0xc0
/** Active Power Workload; bits 2:0 of 'apw_aps', bits 178:176 of the descriptor, APW (Base 2.4,
 * Figure 340) */
#define XNVME_SPEC_POWER_STATE_APW_APS_APW 0x7
/** Active Power Scale; bits 7:6 of 'apw_aps', bits 183:182 of the descriptor, APS (Base 2.4,
 * Figure 340) */
#define XNVME_SPEC_POWER_STATE_APW_APS_APS 0xc0
/** Emergency Power Fail Recovery Time Scale; bits 3:0 of 'epfrts_fqvts', bits 211:208 of the
 * descriptor, EPFRTS (Base 2.4, Figure 340) */
#define XNVME_SPEC_POWER_STATE_EPFRTS_FQVTS_EPFRTS 0xf
/** Forced Quiescence Vault Time Scale; bits 7:4 of 'epfrts_fqvts', bits 215:212 of the
 * descriptor, FQVTS (Base 2.4, Figure 340) */
#define XNVME_SPEC_POWER_STATE_EPFRTS_FQVTS_FQVTS 0xf0
/** Emergency Power Fail Vault Time Scale; bits 3:0 of 'epfvts', bits 219:216 of the descriptor,
 * EPFVTS (Base 2.4, Figure 340) */
#define XNVME_SPEC_POWER_STATE_EPFVTS_EPFVTS 0xf
/** Max Bandwidth Scale; bits 2:0 of 'mbws', bits 234:232 of the descriptor, MBWS (Base 2.4, Figure
 * 340) */
#define XNVME_SPEC_POWER_STATE_MBWS_MBWS 0x7

/**
 * NVMe over Fabrics attributes of the Identify Controller data, bytes 2047:1792
 *
 * @see Base 2.4, Figure 338
 * @struct xnvme_spec_idfy_ctrlr_nvmf
 */
struct xnvme_spec_idfy_ctrlr_nvmf {
	uint32_t ioccsz; ///< IOCCSZ: I/O Queue Command Capsule Supported Size, in 16 byte units

	uint32_t iorcsz; ///< IORCSZ: I/O Queue Response Capsule Supported Size, in 16 byte units

	uint16_t icdoff; ///< ICDOFF: In Capsule Data Offset, in 16 byte units

	uint8_t fcatt; ///< FCATT: Fabrics Controller Attributes, see
		       ///< XNVME_SPEC_IDFY_CTRLR_NVMF_FCATT_*

	uint8_t msdbd; ///< MSDBD: Maximum SGL Data Block Descriptors, 0 is no limit

	uint16_t ofcs; ///< OFCS: Optional Fabrics Commands Support, see
		       ///< XNVME_SPEC_IDFY_CTRLR_NVMF_OFCS_*

	uint8_t dctype; ///< DCTYPE: Discovery Controller Type

	uint8_t ccrl; ///< CCRL: Cross-Controller Reset Limit

	uint8_t rsvd16[240];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_idfy_ctrlr_nvmf) == 256, "Incorrect size")

/** Dynamic Controller Model Support; bit 0 of 'fcatt', DCMS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_NVMF_FCATT_DCMS 0x1
/** Non-Zero NVM Set ID Support; bit 1 of 'fcatt', NZNSETIDS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_NVMF_FCATT_NZNSETIDS 0x2
/** Disconnect Command Support; bit 0 of 'ofcs', DCS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_NVMF_OFCS_DCS 0x1

/**
 * Representation of NVMe completion result Identify Controller
 *
 * A field of bits is an integer of the width the specification gives it; its documentation
 * names the bits.
 *
 * @see Base 2.4, Figure 338
 * @struct xnvme_spec_idfy_ctrlr
 */
struct xnvme_spec_idfy_ctrlr {
	/* bytes 0-255: controller capabilities and features */

	uint16_t vid;                        ///< PCI Vendor ID
	uint16_t ssvid;                      ///< PCI Subsystem Vendor ID
	int8_t sn[XNVME_SPEC_CTRLR_SN_LEN];  ///< SerialNumber
	int8_t mn[XNVME_SPEC_CTRLR_MN_LEN];  ///< Model Number
	uint8_t fr[XNVME_SPEC_CTRLR_FR_LEN]; ///< Firmware Revision
	uint8_t rab;                         ///< Recomm. Arbitration Burst
	uint8_t ieee[3];                     ///< IEEE OUI Identifier

	uint8_t cmic; ///< Controller multi-path I/O and namespace sharing capabilities, see
		      ///< XNVME_SPEC_IDFY_CTRLR_CMIC_*

	uint8_t mdts;    ///< Maximum Data Transfer Size
	uint16_t cntlid; ///< Controller ID

	uint32_t ver; ///< Version, see XNVME_SPEC_IDFY_CTRLR_VER_*

	uint32_t rtd3r; ///< RTD3R: RTD3 Resume Latency
	uint32_t rtd3e; ///< RTD3E: RTD3 Entry Latency

	uint32_t
		oaes; ///< Optional asynchronous events supported, see XNVME_SPEC_IDFY_CTRLR_OAES_*

	uint32_t ctratt; ///< Controller attributes, see XNVME_SPEC_IDFY_CTRLR_CTRATT_*

	uint16_t rrls; ///< RRLS: Read Recovery Levels Supported, see XNVME_SPEC_IDFY_CTRLR_RRLS_*

	uint8_t bpcap; ///< BPCAP: Boot Partition Capabilities
	uint8_t chsi;  ///< CHSI: CXL HDM Support Information
	uint32_t nssl; ///< NSSL: NVM Subsystem Shutdown Latency, in microseconds

	uint8_t rsvd108[2];

	uint8_t plsi; ///< PLSI: Power Loss Signaling Information

	uint8_t cntrltype; ///< Controller Type

	uint8_t fguid[16]; ///< FRU Globally Unique Ident.

	uint16_t crdt1; ///< Command Retry Delay Time1

	uint16_t crdt2; ///< Command Retry Delay Time2

	uint16_t crdt3; ///< Command Retry Delay Time3

	uint8_t crcap; ///< CRCAP: Controller Reachability Capabilities
	uint8_t ciu;   ///< CIU: Controller Instance Uniquifier
	uint64_t cirn; ///< CIRN: Controller Instance Random Number

	uint8_t rsvd144[109];

	uint8_t nvmsr; ///< NVM Subsystem Report, see XNVME_SPEC_IDFY_CTRLR_NVMSR_*

	uint8_t vwci; ///< VPD Write Cycle Information, see XNVME_SPEC_IDFY_CTRLR_VWCI_*

	uint8_t mec; ///< Management Endpoint Capabilities, see XNVME_SPEC_IDFY_CTRLR_MEC_*

	/* bytes 256-511: admin command set attributes */

	uint16_t oacs; ///< Optional admin command support, see XNVME_SPEC_IDFY_CTRLR_OACS_*

	uint8_t acl; ///< abort command limit

	uint8_t aerl; ///< asynchronous event request limit

	uint8_t frmw; ///< Firmware updates, see XNVME_SPEC_IDFY_CTRLR_FRMW_*

	uint8_t lpa; ///< Log Page Attributes, see XNVME_SPEC_IDFY_CTRLR_LPA_*

	uint8_t elpe; ///< Error Log Page Entries
	uint8_t npss; ///< Number of Power States Supported

	uint8_t avscc; ///< Admin vendor specific command configuration, see
		       ///< XNVME_SPEC_IDFY_CTRLR_AVSCC_*

	uint8_t apsta; ///< Autonomous power state transition attributes, see
		       ///< XNVME_SPEC_IDFY_CTRLR_APSTA_*

	uint16_t wctemp; ///< Warning Composite Temperature threshold
	uint16_t cctemp; ///< Critical Composite Temperature threshold
	uint16_t mtfa;   ///< Maximum Time for Firmware Activation
	uint32_t hmpre;  ///< Host Memory Buffer Preferred size
	uint32_t hmmin;  ///< Host Memory Buffer Minimum size

	uint64_t tnvmcap[2]; ///< total NVM capacity

	uint64_t unvmcap[2]; ///< unallocated NVM capacity

	uint32_t rpmbs; ///< Replay protected memory block support, see
			///< XNVME_SPEC_IDFY_CTRLR_RPMBS_*

	uint16_t edstt; ///< extended device self-test time (in minutes)

	uint8_t dsto; ///< Device self-test options, see XNVME_SPEC_IDFY_CTRLR_DSTO_*

	uint8_t fwug; ///< Firmware update granularity, in 4 KiB units; 0x00 if not reported, 0xFF
		      ///< if unrestricted

	uint16_t kas; ///< Keep Alive Support, the timer's granularity in 100 ms units; 0 if not
		      ///< supported

	uint16_t hctma; ///< Host controlled thermal management attributes, see
			///< XNVME_SPEC_IDFY_CTRLR_HCTMA_*

	uint16_t mntmt; ///< Minimum Thermal Management Temperature
	uint16_t mxtmt; ///< Maximum Thermal Management Temperature

	uint32_t sanicap; ///< Sanitize capabilities, see XNVME_SPEC_IDFY_CTRLR_SANICAP_*

	uint32_t hmminds; ///< Host Memory Buffer Minimum Descriptor Entry Size
	uint16_t hmmaxd;  ///< Host Memory Maximum Descriptor Entries

	uint16_t nsetidmax; ///< NVM Set Identifier Maximum
	uint16_t endgidmax; ///< Endurance Group Identifier Maximum

	uint8_t anatt; ///< ANA Transition Time

	uint8_t anacap; ///< Asymmetric Namespace Access capabilities, see
			///< XNVME_SPEC_IDFY_CTRLR_ANACAP_*

	uint32_t anagrpmax; ///< ANA Group Identifier Maximum
	uint32_t nanagrpid; ///< Number of ANA Group Identifiers

	uint32_t pels; ///< Persistent Event Log Size

	uint16_t did; ///< DID: Domain Identifier

	uint8_t kpioc; ///< KPIOC: Key Per I/O Capabilities

	uint8_t rsvd359;

	uint16_t mptfawr; ///< MPTFAWR: Maximum Processing Time for Firmware Activation Without
			  ///< Reset

	uint8_t rmdca; ///< RMDCA: Restore Manufacturing Configuration Attributes

	uint8_t rsvd363[5];

	uint64_t megcap[2]; ///< Max Endurance Group Capacity

	uint8_t tmpthha;   ///< TMPTHHA: Temperature Threshold Hysteresis Attributes
	uint8_t mupa;      ///< MUPA: Maximum Unlimited Power Attributes
	uint16_t cqt;      ///< CQT: Command Quiesce Time
	uint16_t cdpa;     ///< CDPA: Configurable Device Personality Attributes
	uint16_t mup;      ///< MUP: Maximum Unlimited Power
	uint16_t ipmsr;    ///< IPMSR: Interval Power Measurement Sample Rate
	uint16_t msmt;     ///< MSMT: Maximum Stop Measurement Time
	uint16_t mnens;    ///< MNENS: Maximum Number of Exported NVM Subsystems
	uint16_t mnecpens; ///< MNECPENS: Maximum Number of Exported Controllers per Exported NVM
			   ///< Subsystem
	uint32_t mensnn;   ///< MENSNN: Maximum Exported NVM Subsystem Number of Namespaces
	uint8_t ensa;      ///< ENSA: Exported NVM Subsystem Attributes
	uint8_t endsfs;    ///< ENDSFS: Exported Namespace Data Structure Formats Supported

	uint8_t vsen1[4]; ///< VSEN1: Voltage Sensor 1, a 32-bit value in the format of Figure 339
	uint8_t vsen2[4]; ///< VSEN2: Voltage Sensor 2, a 32-bit value in the format of Figure 339
	uint8_t vsen3[4]; ///< VSEN3: Voltage Sensor 3, a 32-bit value in the format of Figure 339
	uint8_t vsen4[4]; ///< VSEN4: Voltage Sensor 4, a 32-bit value in the format of Figure 339

	uint16_t msvmt; ///< MSVMT: Maximum Stop Voltage Measurement Time

	uint8_t rsvd424[88];

	/* bytes 512-703: nvm command set attributes */

	uint8_t sqes; ///< Submission queue entry size, see XNVME_SPEC_IDFY_CTRLR_SQES_*

	uint8_t cqes; ///< Completion queue entry size, see XNVME_SPEC_IDFY_CTRLR_CQES_*

	uint16_t maxcmd;

	uint32_t nn; ///< Number of Namespaces

	uint16_t oncs; ///< Optional NVM command support, see XNVME_SPEC_IDFY_CTRLR_ONCS_*

	uint16_t fuses; ///< FUSES: Fused Operation Support, see XNVME_SPEC_IDFY_CTRLR_FUSES_*

	uint8_t fna; ///< Format NVM attributes, see XNVME_SPEC_IDFY_CTRLR_FNA_*

	uint8_t vwc; ///< Volatile write cache, see XNVME_SPEC_IDFY_CTRLR_VWC_*

	uint16_t awun;   ///< Atomic Write Unit Normal
	uint16_t awupf;  ///< Atomic Write Unit Power Fail
	uint8_t icsvscc; ///< ICSVSCC: I/O Command Set Vendor Specific Command Configuration, see
			 ///< XNVME_SPEC_IDFY_CTRLR_ICSVSCC_*

	uint8_t nwpc; ///< NWPC: Namespace Write Protection Capabilities, see
		      ///< XNVME_SPEC_IDFY_CTRLR_NWPC_*

	uint16_t acwu; ///< atomic compare & write unit

	uint16_t cdfs; ///< Copy descriptor formats supported, see XNVME_SPEC_IDFY_CTRLR_CDFS_*

	uint32_t sgls; ///< SGL support, see XNVME_SPEC_IDFY_CTRLR_SGLS_*

	uint32_t mnan; ///< Maximum Number of Allowed Namespaces

	uint64_t maxdna[2]; ///< Maximum Domain Namespace Attachments
	uint32_t maxcna;    ///< Maximum I/O Controllers Namespace Attachments

	uint32_t oaqd;   ///< OAQD: Optimal Aggregated Queue Depth
	uint8_t rhiri;   ///< RHIRI: Recommended Host-Initiated Refresh Interval
	uint8_t hirt;    ///< HIRT: Host-Initiated Refresh Time
	uint16_t cmmrtd; ///< CMMRTD: Controller Maximum Memory Range Tracking Descriptors
	uint16_t nmmrtd; ///< NMMRTD: NVM Subsystem Maximum Memory Range Tracking Descriptors
	uint8_t minmrtg; ///< MINMRTG: Minimum Memory Range Tracking Granularity
	uint8_t maxmrtg; ///< MAXMRTG: Maximum Memory Range Tracking Granularity
	uint8_t trattr;  ///< TRATTR: Tracking Attributes
	uint8_t rsvd577;
	uint16_t mcudmq;  ///< MCUDMQ: Maximum Controller User Data Migration Queues
	uint16_t mnsudmq; ///< MNSUDMQ: Maximum NVM Subsystem User Data Migration Queues
	uint16_t mcmr;    ///< MCMR: Maximum CDQ Memory Ranges
	uint16_t nmcmr;   ///< NMCMR: NVM Subsystem Maximum CDQ Memory Ranges
	uint16_t mcdqpc;  ///< MCDQPC: Maximum Controller Data Queue PRP Count

	uint8_t rsvd588[180];

	uint8_t subnqn[256]; ///< NVM Subsystems NVMe Qualified Name

	uint8_t reserved_1024[768];

	struct xnvme_spec_idfy_ctrlr_nvmf nvmf_specific; ///< NVMe over Fabrics-specific fields

	/* bytes 2048-3071: power state descriptors */
	struct xnvme_spec_power_state psd[32];

	/* bytes 3072-4095: vendor specific */
	uint8_t vs[1024];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_idfy_ctrlr) == 4096, "Incorrect size")

/** Multiple Ports; bit 0 of 'cmic', MPORTS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CMIC_MPORTS 0x1
/** Multiple Controllers; bit 1 of 'cmic', MCTRS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CMIC_MCTRS 0x2
/** Function Type; bit 2 of 'cmic', FT (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CMIC_FT 0x4
/** Asymmetric Namespace Access Reporting Support; bit 3 of 'cmic', ANARS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CMIC_ANARS 0x8
/** Tertiary Version; bits 7:0 of 'ver', TER (Base 2.4, Figure 37) */
#define XNVME_SPEC_IDFY_CTRLR_VER_TER 0xff
/** Minor Version; bits 15:8 of 'ver', MNR (Base 2.4, Figure 37) */
#define XNVME_SPEC_IDFY_CTRLR_VER_MNR 0xff00
/** Major Version; bits 31:16 of 'ver', MJR (Base 2.4, Figure 37) */
#define XNVME_SPEC_IDFY_CTRLR_VER_MJR 0xffff0000
/** Attached Namespace Attribute Notices; bit 8 of 'oaes', NSAN (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OAES_NSAN 0x100
/** Firmware Activation Notices; bit 9 of 'oaes', FAN (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OAES_FAN 0x200
/** Asymmetric Namespace Access Change Notices; bit 11 of 'oaes', ANACN (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OAES_ANACN 0x800
/** Predictable Latency Event Aggregate Log Change Notices; bit 12 of 'oaes', PLEAN (Base 2.4,
 * Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OAES_PLEAN 0x1000
/** LBA Status Information Alert Notices; bit 13 of 'oaes', LSIAN (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OAES_LSIAN 0x2000
/** Endurance Group Event Aggregate Log Page Change Notices; bit 14 of 'oaes', EGEAN (Base 2.4,
 * Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OAES_EGEAN 0x4000
/** Normal NVM Subsystem Shutdown; bit 15 of 'oaes', NNSS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OAES_NNSS 0x8000
/** Temperature Threshold Hysteresis Recovery; bit 16 of 'oaes', TTHR (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OAES_TTHR 0x10000
/** Reachability Groups Change Notices Support; bit 17 of 'oaes', RGCNS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OAES_RGCNS 0x20000
/** Allocated Namespace Attribute Notices; bit 19 of 'oaes', ANSAN (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OAES_ANSAN 0x80000
/** Cross-Controller Reset Completed Notices; bit 20 of 'oaes', CCRCN (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OAES_CCRCN 0x100000
/** Lost Host Communication Notices; bit 21 of 'oaes', LHCN (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OAES_LHCN 0x200000
/** Rate Limiting Configuration Change; bit 22 of 'oaes', RLCC (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OAES_RLCC 0x400000
/** Zone Descriptor Changed Notices; bit 27 of 'oaes', ZDCN (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OAES_ZDCN 0x8000000
/** Discovery Log Page Change Notification; bit 31 of 'oaes', DLPCN (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OAES_DLPCN 0x80000000
/** Host Identifier Support; bit 0 of 'ctratt', HIDS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_HIDS 0x1
/** Non-Operational Power State Permissive Mode; bit 1 of 'ctratt', NOPSPM (Base 2.4, Figure 338)
 */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_NOPSPM 0x2
/** NVM Sets; bit 2 of 'ctratt', NSETS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_NSETS 0x4
/** Read Recovery Levels; bit 3 of 'ctratt', RRLVLS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_RRLVLS 0x8
/** Endurance Groups; bit 4 of 'ctratt', EGS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_EGS 0x10
/** Predictable Latency Mode; bit 5 of 'ctratt', PLM (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_PLM 0x20
/** Traffic Based Keep Alive Support; bit 6 of 'ctratt', TBKAS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_TBKAS 0x40
/** Namespace Granularity; bit 7 of 'ctratt', NG (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_NG 0x80
/** SQ Associations; bit 8 of 'ctratt', SQA (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_SQA 0x100
/** UUID List; bit 9 of 'ctratt', ULIST (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_ULIST 0x200
/** Multi-Domain Subsystem; bit 10 of 'ctratt', MDS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_MDS 0x400
/** Fixed Capacity Management; bit 11 of 'ctratt', FCM (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_FCM 0x800
/** Variable Capacity Management; bit 12 of 'ctratt', VCM (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_VCM 0x1000
/** Delete Endurance Group; bit 13 of 'ctratt', DEG (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_DEG 0x2000
/** Delete NVM Set; bit 14 of 'ctratt', DNVMS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_DNVMS 0x4000
/** Extended LBA Formats Supported; bit 15 of 'ctratt', ELBAS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_ELBAS 0x8000
/** MDTS and Size Limits Exclude Metadata; bit 16 of 'ctratt', MEM (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_MEM 0x10000
/** HMB Restrict Non-Operational Power State Access; bit 17 of 'ctratt', HMBR (Base 2.4, Figure
 * 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_HMBR 0x20000
/** Reservations and Host Identifier Interaction; bit 18 of 'ctratt', RHII (Base 2.4, Figure 338)
 */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_RHII 0x40000
/** Flexible Data Placement Support; bit 19 of 'ctratt', FDPS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_FDPS 0x80000
/** Power Limit Support; bit 20 of 'ctratt', PLS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_PLS 0x100000
/** Power Measurement Support; bit 21 of 'ctratt', PMS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_PMS 0x200000
/** Voltage Measurement Support; bit 22 of 'ctratt', VMS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_VMS 0x400000
/** Idle I/O Exit Latency Limit Support and Scope; bits 24:23 of 'ctratt', IIELLSS (Base 2.4,
 * Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_IIELLSS 0x1800000
/** Read Recovery Level 0; bit 0 of 'rrls', RRL0 (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RRLS_RRL0 0x1
/** Read Recovery Level 1; bit 1 of 'rrls', RRL1 (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RRLS_RRL1 0x2
/** Read Recovery Level 2; bit 2 of 'rrls', RRL2 (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RRLS_RRL2 0x4
/** Read Recovery Level 3; bit 3 of 'rrls', RRL3 (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RRLS_RRL3 0x8
/** Read Recovery Level 4; bit 4 of 'rrls', RRL4 (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RRLS_RRL4 0x10
/** Read Recovery Level 5; bit 5 of 'rrls', RRL5 (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RRLS_RRL5 0x20
/** Read Recovery Level 6; bit 6 of 'rrls', RRL6 (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RRLS_RRL6 0x40
/** Read Recovery Level 7; bit 7 of 'rrls', RRL7 (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RRLS_RRL7 0x80
/** Read Recovery Level 8; bit 8 of 'rrls', RRL8 (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RRLS_RRL8 0x100
/** Read Recovery Level 9; bit 9 of 'rrls', RRL9 (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RRLS_RRL9 0x200
/** Read Recovery Level 10; bit 10 of 'rrls', RRL10 (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RRLS_RRL10 0x400
/** Read Recovery Level 11; bit 11 of 'rrls', RRL11 (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RRLS_RRL11 0x800
/** Read Recovery Level 12; bit 12 of 'rrls', RRL12 (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RRLS_RRL12 0x1000
/** Read Recovery Level 13; bit 13 of 'rrls', RRL13 (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RRLS_RRL13 0x2000
/** Read Recovery Level 14; bit 14 of 'rrls', RRL14 (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RRLS_RRL14 0x4000
/** Read Recovery Level 15; bit 15 of 'rrls', RRL15 (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RRLS_RRL15 0x8000
/** NVMe Storage Device; bit 0 of 'nvmsr', NVMESD (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_NVMSR_NVMESD 0x1
/** NVMe Enclosure; bit 1 of 'nvmsr', NVMEE (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_NVMSR_NVMEE 0x2
/** VPD Write Cycles Remaining; bits 6:0 of 'vwci', VWCR (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_VWCI_VWCR 0x7f
/** VPD Write Cycle Remaining Valid; bit 7 of 'vwci', VWCRV (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_VWCI_VWCRV 0x80
/** 2-Wire Port Management Endpoint; bit 0 of 'mec', TWPME (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_MEC_TWPME 0x1
/** PCIe Port Management Endpoint; bit 1 of 'mec', PCIEME (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_MEC_PCIEME 0x2
/** Security Send Receive Supported; bit 0 of 'oacs', SSRS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OACS_SSRS 0x1
/** Format NVM Supported; bit 1 of 'oacs', FNVMS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OACS_FNVMS 0x2
/** Firmware Download Supported; bit 2 of 'oacs', FWDS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OACS_FWDS 0x4
/** Namespace Management Supported; bit 3 of 'oacs', NMS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OACS_NMS 0x8
/** Device Self-test Supported; bit 4 of 'oacs', DSTS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OACS_DSTS 0x10
/** Directives Supported; bit 5 of 'oacs', DIRS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OACS_DIRS 0x20
/** NVMe-MI Send Receive Supported; bit 6 of 'oacs', NSRS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OACS_NSRS 0x40
/** Virtualization Management Supported; bit 7 of 'oacs', VMS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OACS_VMS 0x80
/** Doorbell Buffer Config Supported; bit 8 of 'oacs', DBCS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OACS_DBCS 0x100
/** Get LBA Status Supported; bit 9 of 'oacs', GLSS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OACS_GLSS 0x200
/** Command and Feature Lockdown Supported; bit 10 of 'oacs', CFLS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OACS_CFLS 0x400
/** Host Managed Live Migration Support; bit 11 of 'oacs', HMLMS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OACS_HMLMS 0x800
/** Exported NVM Subsystem Support; bit 12 of 'oacs', ENSS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OACS_ENSS 0x1000
/** Controller-scoped Command and Feature Lockdown Supported; bit 13 of 'oacs', CCFLS (Base 2.4,
 * Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_OACS_CCFLS 0x2000
/** First Firmware Slot Read Only; bit 0 of 'frmw', FFSRO (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_FRMW_FFSRO 0x1
/** Number Of Firmware Slots; bits 3:1 of 'frmw', NOFS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_FRMW_NOFS 0xe
/** Firmware Activation Without Reset; bit 4 of 'frmw', FAWR (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_FRMW_FAWR 0x10
/** Support Multiple Update Detection; bit 5 of 'frmw', SMUD (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_FRMW_SMUD 0x20
/** SMART Support; bit 0 of 'lpa', SMARTS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_LPA_SMARTS 0x1
/** Commands Supported and Effects Support; bit 1 of 'lpa', CSES (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_LPA_CSES 0x2
/** Log Page Extended Data Support; bit 2 of 'lpa', LPEDS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_LPA_LPEDS 0x4
/** Telemetry Support; bit 3 of 'lpa', TS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_LPA_TS 0x8
/** Persistent Event Support; bit 4 of 'lpa', PES (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_LPA_PES 0x10
/** Miscellaneous Log Page Support; bit 5 of 'lpa', MLPS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_LPA_MLPS 0x20
/** Data Area 4 Support; bit 6 of 'lpa', DA4S (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_LPA_DA4S 0x40
/** Vendor Specific Command Format; bit 0 of 'avscc', VSCF (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_AVSCC_VSCF 0x1
/** Autonomous Power Transition Support; bit 0 of 'apsta', APTS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_APSTA_APTS 0x1
/** Number of RPMB Units; bits 2:0 of 'rpmbs', NRPMBU (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RPMBS_NRPMBU 0x7
/** Authentication Method; bits 5:3 of 'rpmbs', AUTHM (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RPMBS_AUTHM 0x38
/** Total Size; bits 23:16 of 'rpmbs', TSZE (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RPMBS_TSZE 0xff0000
/** Access Size; bits 31:24 of 'rpmbs', ASZE (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_RPMBS_ASZE 0xff000000
/** Single Device Self-test Operation; bit 0 of 'dsto', SDSO (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_DSTO_SDSO 0x1
/** Host-Initiated Refresh Support; bit 1 of 'dsto', HIRS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_DSTO_HIRS 0x2
/** Host Controlled Thermal Management Support; bit 0 of 'hctma', HCTMS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_HCTMA_HCTMS 0x1
/** Crypto Erase Support; bit 0 of 'sanicap', CES (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_SANICAP_CES 0x1
/** Block Erase Support; bit 1 of 'sanicap', BES (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_SANICAP_BES 0x2
/** Overwrite Support; bit 2 of 'sanicap', OWS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_SANICAP_OWS 0x4
/** Verification Support; bit 3 of 'sanicap', VERS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_SANICAP_VERS 0x8
/** Namespace Verification Support; bit 4 of 'sanicap', NVERS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_SANICAP_NVERS 0x10
/** Sanitize Purge Request and Reporting Supported; bit 5 of 'sanicap', SPRRS (Base 2.4, Figure
 * 338) */
#define XNVME_SPEC_IDFY_CTRLR_SANICAP_SPRRS 0x20
/** No-Deallocate Inhibited; bit 29 of 'sanicap', NDI (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_SANICAP_NDI 0x20000000
/** No-Deallocate Modifies Media After Sanitize; bits 31:30 of 'sanicap', NODMMAS (Base 2.4, Figure
 * 338) */
#define XNVME_SPEC_IDFY_CTRLR_SANICAP_NODMMAS 0xc0000000
/** Report ANA Optimized State; bit 0 of 'anacap', RANAOS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ANACAP_RANAOS 0x1
/** Report ANA Non-Optimized State; bit 1 of 'anacap', RANANOS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ANACAP_RANANOS 0x2
/** Report ANA Inaccessible State; bit 2 of 'anacap', RANAIS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ANACAP_RANAIS 0x4
/** Report ANA Persistent Loss State; bit 3 of 'anacap', RANAPLS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ANACAP_RANAPLS 0x8
/** Report ANA Change State; bit 4 of 'anacap', RANACS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ANACAP_RANACS 0x10
/** ANA Group ID Locked When Attached Support; bit 6 of 'anacap', ANAGIDLWAS (Base 2.4, Figure 338)
 */
#define XNVME_SPEC_IDFY_CTRLR_ANACAP_ANAGIDLWAS 0x40
/** ANA Group ID Support; bit 7 of 'anacap', ANAGIDS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ANACAP_ANAGIDS 0x80
/** Minimum I/O Submission Queue Entry Size; bits 3:0 of 'sqes', MINSQES (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_SQES_MINSQES 0xf
/** Maximum I/O Submission Queue Entry Size; bits 7:4 of 'sqes', MAXSQES (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_SQES_MAXSQES 0xf0
/** Minimum I/O Completion Queue Entry Size; bits 3:0 of 'cqes', MINCQES (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CQES_MINCQES 0xf
/** Maximum I/O Completion Queue Entry Size; bits 7:4 of 'cqes', MAXCQES (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CQES_MAXCQES 0xf0
/** Compare Command Support; bit 0 of 'oncs', NVMCMPS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_NVMCMPS 0x1
/** Write Uncorrectable Support Variants; bit 1 of 'oncs', NVMWUSV (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_NVMWUSV 0x2
/** Dataset Management Support Variants; bit 2 of 'oncs', NVMDSMSV (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_NVMDSMSV 0x4
/** Write Zeroes Support Variants; bit 3 of 'oncs', NVMWZSV (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_NVMWZSV 0x8
/** Save and Select Feature Support; bit 4 of 'oncs', SSFS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_SSFS 0x10
/** Reservations Support; bit 5 of 'oncs', RESERVS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_RESERVS 0x20
/** Timestamp Support; bit 6 of 'oncs', TSS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_TSS 0x40
/** Verify Support; bit 7 of 'oncs', NVMVFYS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_NVMVFYS 0x80
/** Copy Support; bit 8 of 'oncs', NVMCPYS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_NVMCPYS 0x100
/** NVM Copy Single Atomicity; bit 9 of 'oncs', NVMCSA (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_NVMCSA 0x200
/** NVM All Fast Copy; bit 10 of 'oncs', NVMAFC (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_NVMAFC 0x400
/** Maximum Write Zeroes with Deallocate; bit 11 of 'oncs', MAXWZD (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_MAXWZD 0x800
/** Namespace Zeroes Support; bit 12 of 'oncs', NSZS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_NSZS 0x1000
/** Fused Compare and Write Supported; bit 0 of 'fuses', FCWS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_FUSES_FCWS 0x1
/** Format Namespace Scope; bit 0 of 'fna', FNS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_FNA_FNS 0x1
/** Secure Erase Namespace Scope; bit 1 of 'fna', SENS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_FNA_SENS 0x2
/** Cryptographic Erase Supported; bit 2 of 'fna', CRYES (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_FNA_CRYES 0x4
/** Format NVM Broadcast Support; bit 3 of 'fna', FNVMBS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_FNA_FNVMBS 0x8
/** Volatile Write Cache Present; bit 0 of 'vwc', VWCP (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_VWC_VWCP 0x1
/** Flush Behavior; bits 2:1 of 'vwc', FB (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_VWC_FB 0x6
/** Same NVM Vendor Specific Command Format; bit 0 of 'icsvscc', SNVSCF (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_ICSVSCC_SNVSCF 0x1
/** No Write Protect and Write Protect Support; bit 0 of 'nwpc', NWPWPS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_NWPC_NWPWPS 0x1
/** Write Protect Until Power Cycle Support; bit 1 of 'nwpc', WPUPCS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_NWPC_WPUPCS 0x2
/** Permanent Write Protect Support; bit 2 of 'nwpc', PWPS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_NWPC_PWPS 0x4
/** Copy Descriptor Format 0 Support; bit 0 of 'cdfs', CDF0S (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CDFS_CDF0S 0x1
/** Copy Descriptor Format 1 Support; bit 1 of 'cdfs', CDF1S (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CDFS_CDF1S 0x2
/** Copy Descriptor Format 2 Support; bit 2 of 'cdfs', CDF2S (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CDFS_CDF2S 0x4
/** Copy Descriptor Format 3 Support; bit 3 of 'cdfs', CDF3S (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CDFS_CDF3S 0x8
/** Copy Descriptor Format 4 Support; bit 4 of 'cdfs', CDF4S (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_CDFS_CDF4S 0x10
/** SGL Support; bits 1:0 of 'sgls', SGLS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_SGLS_SGLS 0x3
/** Keyed SGL Data Block Descriptor Support; bit 2 of 'sgls', KSDBDS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_SGLS_KSDBDS 0x4
/** SGL Descriptor Threshold; bits 15:8 of 'sgls', SDT (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_SGLS_SDT 0xff00
/** SGL Bit Bucket Descriptor Supported; bit 16 of 'sgls', SBBDS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_SGLS_SBBDS 0x10000
/** Metadata Buffer Alignment; bit 17 of 'sgls', MBA (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_SGLS_MBA 0x20000
/** Length Larger than Data Transfer Support; bit 18 of 'sgls', LLDTS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_SGLS_LLDTS 0x40000
/** MPTR SGL Descriptor Support; bit 19 of 'sgls', MSDS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_SGLS_MSDS 0x80000
/** SGL Address Offset Supported; bit 20 of 'sgls', SAOS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_SGLS_SAOS 0x100000
/** Transport SGL Data Block Descriptor Support; bit 21 of 'sgls', TSDBDS (Base 2.4, Figure 338) */
#define XNVME_SPEC_IDFY_CTRLR_SGLS_TSDBDS 0x200000

/**
 * Representation of I/O Command Set Vector
 *
 * @see Base 2.4, Figure 355
 *
 * @struct xnvme_spec_cs_vector
 */
struct xnvme_spec_cs_vector {
	uint64_t bits; ///< The command sets of the combination, see XNVME_SPEC_CS_VECTOR_BITS_*
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cs_vector) == 8, "Incorrect size")

/** NVM Command Set; bit 0 of 'bits', NVMCS (Base 2.4, Figure 355) */
#define XNVME_SPEC_CS_VECTOR_BITS_NVMCS 0x1
/** Key Value Command Set; bit 1 of 'bits', KVCS (Base 2.4, Figure 355) */
#define XNVME_SPEC_CS_VECTOR_BITS_KVCS 0x2
/** Zoned Namespace Command Set; bit 2 of 'bits', ZNSCS (Base 2.4, Figure 355) */
#define XNVME_SPEC_CS_VECTOR_BITS_ZNSCS 0x4
/** Subsystem Local Memory Command Set; bit 3 of 'bits', SLMCS (Base 2.4, Figure 355) */
#define XNVME_SPEC_CS_VECTOR_BITS_SLMCS 0x8
/** Computational Programs Namespace Command Set; bit 4 of 'bits', CPNCS (Base 2.4, Figure 355) */
#define XNVME_SPEC_CS_VECTOR_BITS_CPNCS 0x10

/** Number of I/O Command Set Combinations in the I/O Command Set data structure */
#define XNVME_SPEC_IDFY_CS_IOCSC_LEN 512

/**
 * Representation of I/O Command Set data structure
 *
 * @see Base 2.4, Figure 354
 *
 * @struct xnvme_spec_idfy_cs
 */
struct xnvme_spec_idfy_cs {
	struct xnvme_spec_cs_vector
		iocsc[XNVME_SPEC_IDFY_CS_IOCSC_LEN]; ///< I/O Command Set Combinations
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_idfy_cs) == 4096, "Incorrect size")

/**
 * The data returned by an Identify command, as bytes and as the structure of each CNS
 *
 * @union xnvme_spec_idfy
 */
union xnvme_spec_idfy {
	uint8_t raw[4096];                  ///< The data as returned
	struct xnvme_spec_idfy_ctrlr ctrlr; ///< Of XNVME_SPEC_IDFY_CTRLR
	struct xnvme_spec_idfy_ns ns;       ///< Of XNVME_SPEC_IDFY_NS
	struct xnvme_spec_idfy_cs cs;       ///< Of XNVME_SPEC_IDFY_IOCS
};
XNVME_STATIC_ASSERT(sizeof(union xnvme_spec_idfy) == 4096, "Incorrect size")

/**
 * NVMe Admin Command Opcodes
 *
 * @see specification Section xx, figure yy
 *
 * @enum xnvme_spec_adm_opc
 */
enum xnvme_spec_adm_opc {
	XNVME_SPEC_ADM_OPC_LOG  = 0x02, ///< XNVME_SPEC_ADM_OPC_LOG
	XNVME_SPEC_ADM_OPC_IDFY = 0x06, ///< XNVME_SPEC_ADM_OPC_IDFY

	XNVME_SPEC_ADM_OPC_SFEAT = 0x09, ///< XNVME_SPEC_ADM_OPC_SFEAT
	XNVME_SPEC_ADM_OPC_GFEAT = 0x0A, ///< XNVME_SPEC_ADM_OPC_GFEAT

	XNVME_SPEC_ADM_OPC_DSEND = 0x19, ///< XNVME_SPEC_ADM_OPC_DSEND
	XNVME_SPEC_ADM_OPC_DRECV = 0x1A, ///< XNVME_SPEC_ADM_OPC_DRECV
};

/**
 * NVMe Non-Volatile Memory Command Opcodes
 *
 * @see specification Section xx, figure yy
 *
 * @enum xnvme_spec_nvm_opc
 */
enum xnvme_spec_nvm_opc {
	XNVME_SPEC_NVM_OPC_FLUSH = 0x00, ///< XNVME_SPEC_NVM_OPC_FLUSH
	XNVME_SPEC_NVM_OPC_WRITE = 0x01, ///< XNVME_SPEC_NVM_OPC_WRITE
	XNVME_SPEC_NVM_OPC_READ  = 0x02, ///< XNVME_SPEC_NVM_OPC_READ

	XNVME_SPEC_NVM_OPC_WRITE_UNCORRECTABLE = 0x04, ///< XNVME_SPEC_NVM_OPC_WRITE_UNCORRECTABLE
	XNVME_SPEC_NVM_OPC_COMPARE             = 0x05, ///< XNVME_SPEC_NVM_OPC_COMPARE
	XNVME_SPEC_NVM_OPC_WRITE_ZEROES        = 0x08, ///< XNVME_SPEC_NVM_OPC_WRITE_ZEROES
	XNVME_SPEC_NVM_OPC_DATASET_MANAGEMENT  = 0x09, ///< XNVME_SPEC_NVM_OPC_DATASET_MANAGEMENT

	XNVME_SPEC_NVM_OPC_SCOPY        = 0x19, ///< XNVME_SPEC_NVM_OPC_SCOPY
	XNVME_SPEC_NVM_OPC_IO_MGMT_RECV = 0x12, ///< XNVME_SPEC_NVM_OPC_IO_MGMT
	XNVME_SPEC_NVM_OPC_IO_MGMT_SEND = 0x1D, ///< XNVME_SPEC_NVM_OPC_IO_MGMT_SEND

	XNVME_SPEC_NVM_OPC_FMT      = 0x80, ///< XNVME_SPEC_NVM_OPC_FMT
	XNVME_SPEC_NVM_OPC_SANITIZE = 0x84, ///< XNVME_SPEC_NVM_OPC_SANITIZE
};

/**
 * NVMe Feature Identifiers
 *
 * TODO: expand these
 *
 * @enum xnvme_spec_feat_id
 */
enum xnvme_spec_feat_id {
	XNVME_SPEC_FEAT_ARBITRATION    = 0x1,  ///< XNVME_SPEC_FEAT_ARBITRATION
	XNVME_SPEC_FEAT_PWR_MGMT       = 0x2,  ///< XNVME_SPEC_FEAT_PWR_MGMT
	XNVME_SPEC_FEAT_LBA_RANGETYPE  = 0x3,  ///< XNVME_SPEC_FEAT_LBA_RANGETYPE
	XNVME_SPEC_FEAT_TEMP_THRESHOLD = 0x4,  ///< XNVME_SPEC_FEAT_TEMP_THRESHOLD
	XNVME_SPEC_FEAT_ERROR_RECOVERY = 0x5,  ///< XNVME_SPEC_FEAT_ERROR_RECOVERY
	XNVME_SPEC_FEAT_VWCACHE        = 0x6,  ///< XNVME_SPEC_FEAT_VWCACHE
	XNVME_SPEC_FEAT_NQUEUES        = 0x7,  ///< XNVME_SPEC_FEAT_NQUEUES
	XNVME_SPEC_FEAT_FDP_MODE       = 0x1D, ///< XNVME_SPEC_FEAT_FDP_MODE
	XNVME_SPEC_FEAT_FDP_EVENTS     = 0x1E, ///< XNVME_SPEC_FEAT_FDP_EVENTS
};

/**
 * Select (SEL) of the Get Features command, which value of the feature to return
 *
 * @enum xnvme_spec_feat_sel
 */
enum xnvme_spec_feat_sel {
	XNVME_SPEC_FEAT_SEL_CURRENT   = 0x0, ///< Current
	XNVME_SPEC_FEAT_SEL_DEFAULT   = 0x1, ///< Default
	XNVME_SPEC_FEAT_SEL_SAVED     = 0x2, ///< Saved
	XNVME_SPEC_FEAT_SEL_SUPPORTED = 0x3  ///< Supported Capabilities
};

/**
 * Directive Type (DTYPE) of the Directive Send and Directive Receive commands
 *
 * @enum xnvme_spec_dir_types
 */
enum xnvme_spec_dir_types {
	XNVME_SPEC_DIR_IDENTIFY = 0x0, ///< Identify
	XNVME_SPEC_DIR_STREAMS  = 0x1, ///< Streams
};

/**
 * Directive Operation (DOPER) of the Directive Send command for the Identify directive
 *
 * @enum xnvme_spec_dsend_idfy_doper
 */
enum xnvme_spec_dsend_idfy_doper {
	XNVME_SPEC_DSEND_IDFY_ENDIR = 0x1, ///< Enable Directive
};

/**
 * Directive Operation (DOPER) of the Directive Send command for the Streams directive
 *
 * @enum xnvme_spec_dsend_streams_doper
 */
enum xnvme_spec_dsend_streams_doper {
	XNVME_SPEC_DSEND_STREAMS_RELID = 0x1, ///< Release Identifier
	XNVME_SPEC_DSEND_STREAMS_RELRS = 0x2, ///< Release Resources
};

/**
 * Directive Operation (DOPER) of the Directive Receive command for the Identify directive
 *
 * @enum xnvme_spec_drecv_idfy_doper
 */
enum xnvme_spec_drecv_idfy_doper {
	XNVME_SPEC_DRECV_IDFY_RETPR = 0x1, ///< Return Parameters
};

/**
 * Directive Operation (DOPER) of the Directive Receive command for the Streams directive
 *
 * @enum xnvme_spec_drecv_streams_doper
 */
enum xnvme_spec_drecv_streams_doper {
	XNVME_SPEC_DRECV_STREAMS_RETPR = 0x1, ///< Return Parameters
	XNVME_SPEC_DRECV_STREAMS_GETST = 0x2, ///< Get Status
	XNVME_SPEC_DRECV_STREAMS_ALLRS = 0x3, ///< Allocate Resources
};

/**
 * This structure holds the return parameters data structure for identify directive
 *
 * Each field is 256 bits; the specification gives none of them an abbreviation, and the masks
 * apply to their first 64 bits.
 *
 * @see Base 2.4, Figure 705
 * @struct xnvme_spec_idfy_dir_rp
 */
struct xnvme_spec_idfy_dir_rp {
	/** Directives Supported, bytes 31:00, see XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_SUPPORTED_* */
	uint64_t directives_supported[4];

	/** Directives Enabled, bytes 63:32, see XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_ENABLED_* */
	uint64_t directives_enabled[4];

	/**
	 * Directive Persistent Across Controller Level Resets, bytes 95:64, see
	 * XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_PERSISTENCE_*
	 */
	uint64_t directives_persistence[4];

	uint8_t rsvd96[4000];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_idfy_dir_rp) == 4096, "Incorrect size")

/** Identify Directive supported; bit 0 of 'directives_supported[0]', IDIRS (Base 2.4, Figure 705)
 */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_SUPPORTED_IDIRS 0x1
/** Streams Directive supported; bit 1 of 'directives_supported[0]', SDIRS (Base 2.4, Figure 705)
 */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_SUPPORTED_SDIRS 0x2
/** Data Placement Directive supported; bit 2 of 'directives_supported[0]', DPDIRS (Base 2.4,
 * Figure 705) */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_SUPPORTED_DPDIRS 0x4
/** Vendor Specific Directive supported; bit 15 of 'directives_supported[0]', VSDIRS (Base 2.4,
 * Figure 705) */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_SUPPORTED_VSDIRS 0x8000
/** Identify Directive enabled; bit 0 of 'directives_enabled[0]', IDIRE (Base 2.4, Figure 705) */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_ENABLED_IDIRE 0x1
/** Streams Directive enabled; bit 1 of 'directives_enabled[0]', SDIRE (Base 2.4, Figure 705) */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_ENABLED_SDIRE 0x2
/** Data Placement Directive enabled; bit 2 of 'directives_enabled[0]', DPDIRE (Base 2.4, Figure
 * 705) */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_ENABLED_DPDIRE 0x4
/** Vendor Specific Directive enabled; bit 15 of 'directives_enabled[0]', VSDIRE (Base 2.4, Figure
 * 705) */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_ENABLED_VSDIRE 0x8000
/** Identify Directive persistent across Controller Level Resets; bit 0 of
 * 'directives_persistence[0]', IDIRCLR (Base 2.4, Figure 705) */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_PERSISTENCE_IDIRCLR 0x1
/** Streams Directive persistent across Controller Level Resets; bit 1 of
 * 'directives_persistence[0]', SDIRCLR (Base 2.4, Figure 705) */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_PERSISTENCE_SDIRCLR 0x2
/** Data Placement Directive persistent across Controller Level Resets; bit 2 of
 * 'directives_persistence[0]', DPDIRCLR (Base 2.4, Figure 705) */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_PERSISTENCE_DPDIRCLR 0x4
/** Vendor Specific Directive persistent across Controller Level Resets; bit 15 of
 * 'directives_persistence[0]', VSDIRCLR (Base 2.4, Figure 705) */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_PERSISTENCE_VSDIRCLR 0x8000

/**
 * This structure holds the return parameters data structure for streams directive
 *
 * @see Base 2.4, Figure 712
 * @struct xnvme_spec_streams_dir_rp
 */
struct xnvme_spec_streams_dir_rp {
	uint16_t msl;  ///< MSL: Max Streams Limit
	uint16_t nssa; ///< NSSA: NVM Subsystem Streams Available
	uint16_t nsso; ///< NSSO: NVM Subsystem Streams Open

	uint8_t nssc; ///< NSSC: NVM Subsystem Stream Capability, see
		      ///< XNVME_SPEC_STREAMS_DIR_RP_NSSC_*

	uint8_t rsvd7[9];

	uint32_t sws; ///< SWS: Stream Write Size
	uint16_t sgs; ///< SGS: Stream Granularity Size

	uint16_t nsa; ///< NSA: Namespace Streams Allocated
	uint16_t nso; ///< NSO: Namespace Streams Open

	uint8_t rsvd26[6];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_streams_dir_rp) == 32, "Incorrect size")

/** Shared Stream Identifiers, by hosts of different non-zero Host Identifiers; bit 0 of 'nssc',
 * SSID (Base 2.4, Figure 712) */
#define XNVME_SPEC_STREAMS_DIR_RP_NSSC_SSID 0x1
/** Streams Require Non-Zero Host Identifier; bit 1 of 'nssc', SRNZID (Base 2.4, Figure 712) */
#define XNVME_SPEC_STREAMS_DIR_RP_NSSC_SRNZID 0x2

/**
 * This structure holds the get status data structure for streams directive
 *
 * @see Base 2.4, Figure 713
 * @struct xnvme_spec_streams_dir_gs
 */
struct xnvme_spec_streams_dir_gs {
	uint16_t osc;   ///< OSC: Open Stream Count
	uint16_t sid[]; ///< SID1 to SIDn: Stream Identifier
};

/**
 * Completion dword 0 of the Streams directive's Allocate Resources
 *
 * @struct xnvme_spec_alloc_resource
 */
struct xnvme_spec_alloc_resource {
	uint16_t nsa; ///< NSA: Namespace Streams Allocated
	uint16_t rsvd;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_alloc_resource) == 4, "Incorrect size")

/**
 * Number of Queues, as allocated, in completion dword 0 of Get or Set Features
 *
 * @struct xnvme_spec_feat_nqueues
 */
struct xnvme_spec_feat_nqueues {
	uint16_t nsqa; ///< NSQA: Number of I/O Submission Queues Allocated, zero-based
	uint16_t ncqa; ///< NCQA: Number of I/O Completion Queues Allocated, zero-based
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_feat_nqueues) == 4, "Incorrect size")

/**
 * The attributes of a feature, as dword 11 of Set Features and completion dword 0
 * of Get Features carry them, viewed by the feature they are of
 *
 * Flexible Data Placement is the exception: Set Features carries its attributes in dword 12,
 * and dword 11 is its ENDGID (Base 2.4, Figures 499 and 500).
 *
 * @union xnvme_spec_feat
 */
union xnvme_spec_feat {
	uint32_t temp_threshold; ///< Temperature Threshold, see XNVME_SPEC_FEAT_TEMP_THRESHOLD_*
	uint32_t error_recovery; ///< Error Recovery, see XNVME_SPEC_FEAT_ERROR_RECOVERY_*
	struct xnvme_spec_feat_nqueues nqueues; ///< Number of Queues
	uint32_t fdp_mode;       ///< Flexible Data Placement, Set Features dword 12, see
				 ///< XNVME_SPEC_FEAT_FDP_MODE_*
	uint32_t media_feedback; ///< Media Feedback, see XNVME_SPEC_FEAT_MEDIA_FEEDBACK_*
	uint32_t val;            ///< The dword whole, for a feature without a view
};
XNVME_STATIC_ASSERT(sizeof(union xnvme_spec_feat) == 4, "Incorrect size")

/** Temperature Threshold, in Kelvin; bits 15:0 of 'temp_threshold', TMPTH (Base 2.4, Figure 470)
 */
#define XNVME_SPEC_FEAT_TEMP_THRESHOLD_TMPTH 0xffff
/** Threshold Temperature Select; bits 19:16 of 'temp_threshold', TMPSEL (Base 2.4, Figure 470) */
#define XNVME_SPEC_FEAT_TEMP_THRESHOLD_TMPSEL 0xf0000
/** Threshold Type Select; bits 21:20 of 'temp_threshold', THSEL (Base 2.4, Figure 470) */
#define XNVME_SPEC_FEAT_TEMP_THRESHOLD_THSEL 0x300000
/** Temperature Threshold Hysteresis; bits 24:22 of 'temp_threshold', TMPTHH (Base 2.4,
 * Figure 470) */
#define XNVME_SPEC_FEAT_TEMP_THRESHOLD_TMPTHH 0x1c00000

/** Time Limited Error Recovery, in 100 ms units; bits 15:0 of 'error_recovery', TLER (NVM Command
 * Set 1.3, Figure 97) */
#define XNVME_SPEC_FEAT_ERROR_RECOVERY_TLER 0xffff
/** Deallocated or Unwritten Logical Block Error Enable; bit 16 of 'error_recovery', DULBE (NVM
 * Command Set 1.3, Figure 97) */
#define XNVME_SPEC_FEAT_ERROR_RECOVERY_DULBE 0x10000

/** Flexible Data Placement Enable; bit 0 of 'fdp_mode', FDPE (Base 2.4, Figure 500) */
#define XNVME_SPEC_FEAT_FDP_MODE_FDPE 0x1
/** Flexible Data Placement Configuration Index; bits 15:8 of 'fdp_mode', FDPCIDX (Base 2.4,
 * Figure 500) */
#define XNVME_SPEC_FEAT_FDP_MODE_FDPCIDX 0xff00

/** High ECC status; bit 0 of 'media_feedback' */
#define XNVME_SPEC_FEAT_MEDIA_FEEDBACK_HECC 0x1
/** Very High ECC status; bit 1 of 'media_feedback' */
#define XNVME_SPEC_FEAT_MEDIA_FEEDBACK_VHECC 0x2

/**
 * Representation of DSM ranges, one Range of the Dataset Management range definition
 *
 * @see NVM Command Set 1.3, Figures 47 and 48
 * @struct xnvme_spec_dsm_range
 */
struct xnvme_spec_dsm_range {
	uint32_t cattr; ///< CATTR: Context Attributes, see XNVME_SPEC_DSM_RANGE_CATTR_*
	uint32_t llb;   ///< LLB: Length in Logical Blocks, a 1-based value
	uint64_t slba;  ///< SLBA: Starting LBA
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_dsm_range) == 16, "Incorrect size")

/** Access Frequency; bits 3:0 of 'cattr', AF (NVM Command Set 1.3, Figure 48) */
#define XNVME_SPEC_DSM_RANGE_CATTR_AF 0xf
/** Access Latency; bits 5:4 of 'cattr', AL (NVM Command Set 1.3, Figure 48) */
#define XNVME_SPEC_DSM_RANGE_CATTR_AL 0x30
/** Sequential Read Range; bit 8 of 'cattr', SRR (NVM Command Set 1.3, Figure 48) */
#define XNVME_SPEC_DSM_RANGE_CATTR_SRR 0x100
/** Sequential Write Range; bit 9 of 'cattr', SWR (NVM Command Set 1.3, Figure 48) */
#define XNVME_SPEC_DSM_RANGE_CATTR_SWR 0x200
/** Write Prepare; bit 10 of 'cattr', WPREP (NVM Command Set 1.3, Figure 48) */
#define XNVME_SPEC_DSM_RANGE_CATTR_WPREP 0x400
/** Command Access Size, in logical blocks; bits 31:24 of 'cattr', CASZE (NVM Command Set 1.3,
 * Figure 48) */
#define XNVME_SPEC_DSM_RANGE_CATTR_CASZE 0xff000000

/**
 * Enumeration of NVMe flags
 *
 * The bits of Command Dword 12 of the Read and Write commands, bits 31:16, shifted down by 16;
 * PRINFO is bits 29:26 of the dword (NVM Command Set 1.3, Figures 11, 54 and 71).
 *
 * @enum xnvme_spec_flag
 */
enum xnvme_spec_flag {
	XNVME_SPEC_FLAG_LR  = 0x1 << 15, ///< Limited Retry (LR), bit 31 of dword 12
	XNVME_SPEC_FLAG_FUA = 0x1 << 14, ///< Force Unit Access (FUA), bit 30 of dword 12

	XNVME_SPEC_FLAG_PRINFO_RTCHK  = 0x1 << 10, ///< Reference Tag Check (RTCHK), PRINFO bit 0
	XNVME_SPEC_FLAG_PRINFO_ATCHK  = 0x1 << 11, ///< Application Tag Check (ATCHK), PRINFO bit 1
	XNVME_SPEC_FLAG_PRINFO_GRDCHK = 0x1 << 12, ///< Guard Check (GRDCHK), PRINFO bit 2
	XNVME_SPEC_FLAG_PRINFO_PRACT = 0x1 << 13, ///< Protection Information Action (PRACT), bit 3

	XNVME_SPEC_FLAG_STC = 0x1 << 8, ///< Storage Tag Check (STC), bit 24 of dword 12
};

/**
 * SGL Descriptor Type, bits 7:4 of the SGL Identifier
 *
 * @enum xnvme_nvme_sgl_descriptor_type
 */
enum xnvme_nvme_sgl_descriptor_type {
	XNVME_SPEC_SGL_DESCR_TYPE_DATA_BLOCK   = 0x0, ///< XNVME_SPEC_SGL_DESCR_TYPE_DATA_BLOCK
	XNVME_SPEC_SGL_DESCR_TYPE_BIT_BUCKET   = 0x1, ///< XNVME_SPEC_SGL_DESCR_TYPE_BIT_BUCKET
	XNVME_SPEC_SGL_DESCR_TYPE_SEGMENT      = 0x2, ///< XNVME_SPEC_SGL_DESCR_TYPE_SEGMENT
	XNVME_SPEC_SGL_DESCR_TYPE_LAST_SEGMENT = 0x3, ///< XNVME_SPEC_SGL_DESCR_TYPE_LAST_SEGMENT
	XNVME_SPEC_SGL_DESCR_TYPE_KEYED_DATA_BLOCK =
		0x4, ///< XNVME_SPEC_SGL_DESCR_TYPE_KEYED_DATA_BLOCK
	XNVME_SPEC_SGL_DESCR_TYPE_VENDOR_SPECIFIC =
		0xf, ///< XNVME_SPEC_SGL_DESCR_TYPE_VENDOR_SPECIFIC
};

/**
 * SGL Descriptor Sub Type, bits 3:0 of the SGL Identifier
 *
 * @enum xnvme_spec_sgl_descriptor_subtype
 */
enum xnvme_spec_sgl_descriptor_subtype {
	XNVME_SPEC_SGL_DESCR_SUBTYPE_ADDRESS = 0x0, ///< XNVME_SPEC_SGL_DESCR_SUBTYPE_ADDRESS
	XNVME_SPEC_SGL_DESCR_SUBTYPE_OFFSET  = 0x1, ///< XNVME_SPEC_SGL_DESCR_SUBTYPE_OFFSET
};

/**
 * SGL descriptor, as the SGL Data Block, Bit Bucket, Segment and Last Segment descriptors are
 *
 * The Keyed SGL Data Block descriptor (Base 2.4, Figure 123) has a 24-bit LEN in bytes 10:08 and
 * KEY in bytes 14:11, which this layout does not name.
 *
 * @see Base 2.4, Figures 116 and 119 to 122
 * @struct xnvme_spec_sgl_descriptor
 */
struct xnvme_spec_sgl_descriptor {
	uint64_t addr; ///< ADDR: Address
	uint32_t len;  ///< LEN: Length
	uint8_t rsvd12[3];
	uint8_t sglid; ///< SGLID: SGL Identifier, see XNVME_SPEC_SGL_DESCRIPTOR_SGLID_*
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_sgl_descriptor) == 16, "Incorrect size")

/** SGL Descriptor Sub Type, see enum xnvme_spec_sgl_descriptor_subtype; bits 3:0 of 'sglid',
 * SGLDST (Base 2.4, Figure 116) */
#define XNVME_SPEC_SGL_DESCRIPTOR_SGLID_SGLDST 0xf
/** SGL Descriptor Type, see enum xnvme_nvme_sgl_descriptor_type; bits 7:4 of 'sglid', SGLDT
 * (Base 2.4, Figure 116) */
#define XNVME_SPEC_SGL_DESCRIPTOR_SGLID_SGLDT 0xf0

/**
 * PRP or SGL for Data Transfer field
 *
 * @enum xnvme_spec_psdt
 */
enum xnvme_spec_psdt {
	XNVME_SPEC_PSDT_PRP                 = 0x0,
	XNVME_SPEC_PSDT_SGL_MPTR_CONTIGUOUS = 0x1,
	XNVME_SPEC_PSDT_SGL_MPTR_SGL        = 0x2,
};

/**
 * Data Pointer as PRP entries
 *
 * @struct xnvme_spec_dptr_prp
 */
struct xnvme_spec_dptr_prp {
	uint64_t prp1; ///< PRP entry 1
	uint64_t prp2; ///< PRP entry 2
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_dptr_prp) == 16, "Incorrect size")

/**
 * Data Pointer as the IOCTL of the Linux Kernel NVMe driver uses it
 *
 * @struct xnvme_spec_dptr_linux_ioctl
 */
struct xnvme_spec_dptr_linux_ioctl {
	uint64_t data;         ///< Address of the data
	uint32_t metadata_len; ///< Length of the metadata
	uint32_t data_len;     ///< Length of the data
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_dptr_linux_ioctl) == 16, "Incorrect size")

/**
 * Data Pointer, dwords 6 to 9 of a command
 *
 * @see Base 2.4, Figure 93
 * @union xnvme_spec_dptr
 */
union xnvme_spec_dptr {
	struct xnvme_spec_dptr_prp prp;               ///< As PRP entries
	struct xnvme_spec_sgl_descriptor sgl1;        ///< SGL1: SGL Entry 1
	struct xnvme_spec_dptr_linux_ioctl lnx_ioctl; ///< As the Linux Kernel NVMe driver uses it
};
XNVME_STATIC_ASSERT(sizeof(union xnvme_spec_dptr) == 16, "Incorrect size")

/**
 * NVMe Command Accessor for common use of the 64byte NVMe command
 *
 * Vendor specific commands name dwords 10 and 11 NDT and NDM (Base 2.4, Figure 94).
 *
 * @see Base 2.4, Figure 92 and Figure 93
 * @struct xnvme_spec_cmd_common
 */
struct xnvme_spec_cmd_common {
	/* cdw 00 */
	uint8_t opc;   ///< OPC: Opcode, see XNVME_SPEC_CMD_COMMON_OPC_*
	uint8_t flags; ///< Bits 15:08 of dword 0, FUSE and PSDT, see XNVME_SPEC_CMD_COMMON_FLAGS_*
	uint16_t cid;  ///< CID: Command Identifier

	uint32_t nsid; ///< NSID: Namespace Identifier

	uint32_t cdw2; ///< CDW2: Command Dword 2
	uint32_t cdw3; ///< CDW3: Command Dword 3

	uint64_t mptr; ///< MPTR: Metadata Pointer

	union xnvme_spec_dptr dptr; ///< DPTR: Data Pointer

	uint32_t cdw10; ///< CDW10: Command Dword 10
	uint32_t cdw11; ///< CDW11: Command Dword 11
	uint32_t cdw12; ///< CDW12: Command Dword 12
	uint32_t cdw13; ///< CDW13: Command Dword 13
	uint32_t cdw14; ///< CDW14: Command Dword 14
	uint32_t cdw15; ///< CDW15: Command Dword 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_common) == 64, "Incorrect size")

/** Data Transfer Direction; bits 1:0 of 'opc', DTD (Base 2.4, Figure 92) */
#define XNVME_SPEC_CMD_COMMON_OPC_DTD 0x3
/** Function; bits 7:2 of 'opc', FN (Base 2.4, Figure 92) */
#define XNVME_SPEC_CMD_COMMON_OPC_FN 0xfc
/** Fused Operation; bits 1:0 of 'flags', bits 09:08 of dword 0, FUSE (Base 2.4, Figure 92) */
#define XNVME_SPEC_CMD_COMMON_FLAGS_FUSE 0x3
/** PRP or SGL for Data Transfer; bits 7:6 of 'flags', bits 15:14 of dword 0, PSDT (Base 2.4,
 * Figure 92) */
#define XNVME_SPEC_CMD_COMMON_FLAGS_PSDT 0xc0

/**
 * NVMe Command Accessor for the Sanitize command
 *
 * @see Base 2.4, Figures 451 and 452
 * @struct xnvme_spec_cmd_sanitize
 */
struct xnvme_spec_cmd_sanitize {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint32_t cdw10; ///< Command dword 10, see XNVME_SPEC_CMD_SANITIZE_CDW10_*

	uint32_t ovrpat; ///< OVRPAT: Overwrite Pattern

	uint32_t cdw12_15[4]; ///< Command dword 12 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_sanitize) == 64, "Incorrect size")

/** Sanitize Action; bits 2:0 of 'cdw10', SANACT (Base 2.4, Figure 451) */
#define XNVME_SPEC_CMD_SANITIZE_CDW10_SANACT 0x7
/** Allow Unrestricted Sanitize Exit; bit 3 of 'cdw10', AUSE (Base 2.4, Figure 451) */
#define XNVME_SPEC_CMD_SANITIZE_CDW10_AUSE 0x8
/** Overwrite Pass Count; bits 7:4 of 'cdw10', OWPASS (Base 2.4, Figure 451) */
#define XNVME_SPEC_CMD_SANITIZE_CDW10_OWPASS 0xf0
/** Overwrite Invert Pattern Between Passes; bit 8 of 'cdw10', OIPBP (Base 2.4, Figure 451) */
#define XNVME_SPEC_CMD_SANITIZE_CDW10_OIPBP 0x100
/** No-Deallocate After Sanitize; bit 9 of 'cdw10', NDAS (Base 2.4, Figure 451) */
#define XNVME_SPEC_CMD_SANITIZE_CDW10_NDAS 0x200
/** Enter Media Verification State; bit 10 of 'cdw10', EMVS (Base 2.4, Figure 451) */
#define XNVME_SPEC_CMD_SANITIZE_CDW10_EMVS 0x400
/** Purge Required; bit 11 of 'cdw10', PREQ (Base 2.4, Figure 451) */
#define XNVME_SPEC_CMD_SANITIZE_CDW10_PREQ 0x800

/**
 * NVMe Command Accessor for the NVM-format command
 *
 * @struct xnvme_spec_cmd_format
 */
struct xnvme_spec_cmd_format {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint32_t cdw10; ///< Command dword 10, see XNVME_SPEC_CMD_FORMAT_CDW10_*

	uint32_t cdw11_15[5]; ///< Command dword 11 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_format) == 64, "Incorrect size")

/** LBA Format Lower, the 4 least significant bits of the index; bits 3:0 of 'cdw10' */
#define XNVME_SPEC_CMD_FORMAT_CDW10_LBAFL 0xf
/** Metadata Settings; bit 4 of 'cdw10' */
#define XNVME_SPEC_CMD_FORMAT_CDW10_MSET 0x10
/** Protection Information; bits 7:5 of 'cdw10' */
#define XNVME_SPEC_CMD_FORMAT_CDW10_PI 0xe0
/** Protection Information Location; bit 8 of 'cdw10' */
#define XNVME_SPEC_CMD_FORMAT_CDW10_PIL 0x100
/** Secure Erase Settings; bits 11:9 of 'cdw10' */
#define XNVME_SPEC_CMD_FORMAT_CDW10_SES 0xe00
/** LBA Format Upper, the 2 most significant bits of the index; bits 13:12 of 'cdw10' */
#define XNVME_SPEC_CMD_FORMAT_CDW10_LBAFU 0x3000

/**
 * NVMe Command Accessor for the get-features command
 *
 * @see Base 2.4, Figures 198 and 199
 * @struct xnvme_spec_cmd_gfeat
 */
struct xnvme_spec_cmd_gfeat {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint32_t cdw10;       ///< Command dword 10, see XNVME_SPEC_CMD_GFEAT_CDW10_*
	uint32_t cdw11;       ///< Command dword 11
	uint32_t cdw12_13[2]; ///< Command dword 12 to 13
	uint32_t cdw14;       ///< Command dword 14, see XNVME_SPEC_CMD_GFEAT_CDW14_*
	uint32_t cdw15;       ///< Command dword 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_gfeat) == 64, "Incorrect size")

/** Feature Identifier, see xnvme_spec_feat_id; bits 7:0 of 'cdw10', FID (Base 2.4, Figure 198) */
#define XNVME_SPEC_CMD_GFEAT_CDW10_FID 0xff
/** Select, see xnvme_spec_feat_sel; bits 10:8 of 'cdw10', SEL (Base 2.4, Figure 198) */
#define XNVME_SPEC_CMD_GFEAT_CDW10_SEL 0x700
/** UUID Index; bits 6:0 of 'cdw14', UIDX (Base 2.4, Figure 199) */
#define XNVME_SPEC_CMD_GFEAT_CDW14_UIDX 0x7f

/**
 * NVMe Command Accessor for the set-features command
 *
 * @see Base 2.4, Figures 464 and 465
 * @struct xnvme_spec_cmd_sfeat
 */
struct xnvme_spec_cmd_sfeat {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint32_t cdw10; ///< Command dword 10, see XNVME_SPEC_CMD_SFEAT_CDW10_*

	union xnvme_spec_feat feat; ///< Feature attributes, dword 11

	uint32_t cdw12; ///< Command dword 12
	uint32_t cdw13; ///< Command dword 13
	uint32_t cdw14; ///< Command dword 14, see XNVME_SPEC_CMD_SFEAT_CDW14_*
	uint32_t cdw15; ///< Command dword 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_sfeat) == 64, "Incorrect size")

/** Feature Identifier, see xnvme_spec_feat_id; bits 7:0 of 'cdw10', FID (Base 2.4, Figure 464) */
#define XNVME_SPEC_CMD_SFEAT_CDW10_FID 0xff
/** Save; bit 31 of 'cdw10', SV (Base 2.4, Figure 464) */
#define XNVME_SPEC_CMD_SFEAT_CDW10_SV 0x80000000
/** UUID Index; bits 6:0 of 'cdw14', UIDX (Base 2.4, Figure 465) */
#define XNVME_SPEC_CMD_SFEAT_CDW14_UIDX 0x7f

/**
 * NVMe Command Accessor for the directive send command
 *
 * @see Base 2.4, Figures 185, 186 and 706
 * @struct xnvme_spec_cmd_dsend
 */
struct xnvme_spec_cmd_dsend {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint32_t numd; ///< NUMD: Number of Dwords

	uint8_t doper;  ///< DOPER: Directive Operation
	uint8_t dtype;  ///< DTYPE: Directive Type, see xnvme_spec_dir_types
	uint16_t dspec; ///< DSPEC: Directive Specific

	uint32_t cdw12; ///< Command dword 12, see XNVME_SPEC_CMD_DSEND_CDW12_*

	uint32_t cdw13_15[3]; ///< Command dword 13 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_dsend) == 64, "Incorrect size")

/** Enable Directive, of the Identify directive's Enable Directive; bit 0 of 'cdw12', ENDIR (Base
 * 2.4, Figure 706) */
#define XNVME_SPEC_CMD_DSEND_CDW12_ENDIR 0x1
/** Directive Type to enable, of the Identify directive's Enable Directive; bits 15:8 of 'cdw12',
 * DTYPE (Base 2.4, Figure 706) */
#define XNVME_SPEC_CMD_DSEND_CDW12_DTYPE 0xff00

/**
 * NVMe Command Accessor for the directive receive command
 *
 * @see Base 2.4, Figures 182, 183 and 714
 * @struct xnvme_spec_cmd_drecv
 */
struct xnvme_spec_cmd_drecv {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint32_t numd; ///< NUMD: Number of Dwords

	uint8_t doper;  ///< DOPER: Directive Operation
	uint8_t dtype;  ///< DTYPE: Directive Type, see xnvme_spec_dir_types
	uint16_t dspec; ///< DSPEC: Directive Specific

	uint32_t cdw12; ///< Command dword 12, see XNVME_SPEC_CMD_DRECV_CDW12_*

	uint32_t cdw13_15[3]; ///< Command dword 13 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_drecv) == 64, "Incorrect size")

/** Namespace Streams Requested, of Allocate Resources; bits 15:0 of 'cdw12', NSR (Base 2.4,
 * Figure 714) */
#define XNVME_SPEC_CMD_DRECV_CDW12_NSR 0xffff

/**
 * NVMe Command Accessor for the identify command
 *
 * @see Base 2.4, Figures 333 to 335
 * @struct xnvme_spec_cmd_idfy
 */
struct xnvme_spec_cmd_idfy {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint8_t cns; ///< CNS: Controller or Namespace Structure
	uint8_t rsvd41;
	uint16_t cntid; ///< CNTID: Controller Identifier

	uint16_t cnssid; ///< CNSSID: CNS Specific Identifier
	uint8_t rsvd46;
	uint8_t csi; ///< CSI: Command Set Identifier

	uint32_t cdw12_13[2]; ///< Command dword 12 to 13

	uint32_t cdw14; ///< Command dword 14, see XNVME_SPEC_CMD_IDFY_CDW14_*

	uint32_t cdw15; ///< Command dword 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_idfy) == 64, "Incorrect size")

/** UUID Index; bits 6:0 of 'cdw14', UIDX (Base 2.4, Figure 335) */
#define XNVME_SPEC_CMD_IDFY_CDW14_UIDX 0x7f

/**
 * NVMe Command Accessor for the get-log-page command
 *
 * @see Base 2.4, Figures 204 to 208
 * @struct xnvme_spec_cmd_log
 */
struct xnvme_spec_cmd_log {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint32_t cdw10; ///< Command dword 10, see XNVME_SPEC_CMD_LOG_CDW10_*

	uint16_t numdu; ///< NUMDU: Number of Dwords Upper
	uint16_t lsi;   ///< LSI: Log Specific Identifier

	uint32_t lpol; ///< LPOL: Log Page Offset Lower
	uint32_t lpou; ///< LPOU: Log Page Offset Upper

	uint32_t cdw14; ///< Command dword 14, see XNVME_SPEC_CMD_LOG_CDW14_*

	uint32_t cdw15; ///< Command dword 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_log) == 64, "Incorrect size")

/** Log Page Identifier; bits 7:0 of 'cdw10', LID (Base 2.4, Figure 204) */
#define XNVME_SPEC_CMD_LOG_CDW10_LID 0xff
/** Log Specific Parameter; bits 14:8 of 'cdw10', LSP (Base 2.4, Figure 204) */
#define XNVME_SPEC_CMD_LOG_CDW10_LSP 0x7f00
/** Retain Asynchronous Event; bit 15 of 'cdw10', RAE (Base 2.4, Figure 204) */
#define XNVME_SPEC_CMD_LOG_CDW10_RAE 0x8000
/** Number of Dwords Lower; bits 31:16 of 'cdw10', NUMDL (Base 2.4, Figure 204) */
#define XNVME_SPEC_CMD_LOG_CDW10_NUMDL 0xffff0000

/** UUID Index; bits 6:0 of 'cdw14', UIDX (Base 2.4, Figure 208) */
#define XNVME_SPEC_CMD_LOG_CDW14_UIDX 0x7f
/** Offset Type; bit 23 of 'cdw14', OT (Base 2.4, Figure 208) */
#define XNVME_SPEC_CMD_LOG_CDW14_OT 0x800000
/** Command Set Identifier; bits 31:24 of 'cdw14', CSI (Base 2.4, Figure 208) */
#define XNVME_SPEC_CMD_LOG_CDW14_CSI 0xff000000

/**
 * NVMe Command Accessors for the NVM Command Set, as the Read and the Write command
 *
 * The members and masks are named as the Write command names them. The Read command names
 * 'cdw02_03' ELBTU, 'lbtl' ELBTL, 'lbat' ELBAT and 'lbatm' ELBATM, and reserves DTYPE and DSPEC.
 *
 * @see NVM Command Set 1.3, Figures 52 to 58 (Read) and Figures 69 to 75 (Write)
 * @struct xnvme_spec_cmd_nvm
 */
struct xnvme_spec_cmd_nvm {
	uint32_t cdw00_01[2]; ///< Command dword 0 and 1

	uint64_t cdw02_03; ///< Command dword 2 and 3, see XNVME_SPEC_CMD_NVM_CDW02_03_*

	uint32_t cdw04_09[6]; ///< Command dword 4 to 9

	uint64_t slba; ///< SLBA: Starting LBA

	uint32_t cdw12; ///< Command dword 12, see XNVME_SPEC_CMD_NVM_CDW12_*

	uint32_t cdw13; ///< Command dword 13, see XNVME_SPEC_CMD_NVM_CDW13_*

	uint32_t lbtl; ///< LBTL: Logical Block Tags Lower, with LBTU the LBST and the ILBRT

	uint16_t lbat;  ///< LBAT: Logical Block Application Tag
	uint16_t lbatm; ///< LBATM: Logical Block Application Tag Mask
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_nvm) == 64, "Incorrect size")

/** Logical Block Tags Upper; bits 47:00 of 'cdw02_03', LBTU (NVM Command Set 1.3, Figure 69) */
#define XNVME_SPEC_CMD_NVM_CDW02_03_LBTU 0xffffffffffff

/** Number of Logical Blocks, zero-based; bits 15:00 of 'cdw12', NLB (NVM Command Set 1.3,
 * Figure 71) */
#define XNVME_SPEC_CMD_NVM_CDW12_NLB 0xffff
/** Command Extension Type; bits 19:16 of 'cdw12', CETYPE (NVM Command Set 1.3, Figure 71) */
#define XNVME_SPEC_CMD_NVM_CDW12_CETYPE 0xf0000
/** Directive Type, Write only; bits 23:20 of 'cdw12', DTYPE (NVM Command Set 1.3, Figure 71) */
#define XNVME_SPEC_CMD_NVM_CDW12_DTYPE 0xf00000
/** Storage Tag Check; bit 24 of 'cdw12', STC (NVM Command Set 1.3, Figure 71) */
#define XNVME_SPEC_CMD_NVM_CDW12_STC 0x1000000
/** Protection Information; bits 29:26 of 'cdw12', PRINFO (NVM Command Set 1.3, Figure 71) */
#define XNVME_SPEC_CMD_NVM_CDW12_PRINFO 0x3c000000
/** Force Unit Access; bit 30 of 'cdw12', FUA (NVM Command Set 1.3, Figure 71) */
#define XNVME_SPEC_CMD_NVM_CDW12_FUA 0x40000000
/** Limited Retry; bit 31 of 'cdw12', LR (NVM Command Set 1.3, Figure 71) */
#define XNVME_SPEC_CMD_NVM_CDW12_LR 0x80000000

/** Dataset Management, if CETYPE is 0h; bits 07:00 of 'cdw13', DSM (NVM Command Set 1.3, Figure
 * 72)
 */
#define XNVME_SPEC_CMD_NVM_CDW13_DSM 0xff
/** Access Frequency, in DSM; bits 03:00 of 'cdw13', AF (NVM Command Set 1.3, Figure 72) */
#define XNVME_SPEC_CMD_NVM_CDW13_AF 0xf
/** Access Latency, in DSM; bits 05:04 of 'cdw13', AL (NVM Command Set 1.3, Figure 72) */
#define XNVME_SPEC_CMD_NVM_CDW13_AL 0x30
/** Sequential Request, in DSM; bit 06 of 'cdw13', SEQREQ (NVM Command Set 1.3, Figure 72) */
#define XNVME_SPEC_CMD_NVM_CDW13_SEQREQ 0x40
/** Incompressible, in DSM; bit 07 of 'cdw13', INCPRS (NVM Command Set 1.3, Figure 72) */
#define XNVME_SPEC_CMD_NVM_CDW13_INCPRS 0x80
/** Command Extension Value, if CETYPE is non-zero; bits 15:00 of 'cdw13', CEV (NVM Command Set
 * 1.3, Figure 73) */
#define XNVME_SPEC_CMD_NVM_CDW13_CEV 0xffff
/** Directive Specific, Write only; bits 31:16 of 'cdw13', DSPEC (NVM Command Set 1.3, Figure 72)
 */
#define XNVME_SPEC_CMD_NVM_CDW13_DSPEC 0xffff0000

/**
 * The Dataset Management command is used by the host to indicate attributes for ranges of logical
 * blocks
 *
 * @struct xnvme_spec_cmd_dsm
 */
struct xnvme_spec_cmd_dsm {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	// dword 10
	uint8_t nr; ///< NR: Number of Ranges, zero-based
	uint8_t rsvd1[3];

	uint32_t cdw11; ///< Command dword 11, see XNVME_SPEC_CMD_DSM_CDW11_*

	uint32_t cdw12_15[4]; ///< Command dword 12 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_dsm) == 64, "Incorrect size")

/** Integral Dataset for Read; bit 0 of 'cdw11' */
#define XNVME_SPEC_CMD_DSM_CDW11_IDR 0x1
/** Integral Dataset for Write; bit 1 of 'cdw11' */
#define XNVME_SPEC_CMD_DSM_CDW11_IDW 0x2
/** Deallocate; bit 2 of 'cdw11' */
#define XNVME_SPEC_CMD_DSM_CDW11_AD 0x4

/**
 * NVMe Command Accessor for the Write Zeroes command
 *
 * @see NVM Command Set 1.3, Figures 81 to 87
 * @struct xnvme_spec_nvm_write_zeroes
 */
struct xnvme_spec_nvm_write_zeroes {
	uint32_t cdw00_01[2]; ///< Command dword 0 and 1

	uint64_t cdw02_03; ///< Command dword 2 and 3, see XNVME_SPEC_NVM_WRITE_ZEROES_CDW02_03_*

	uint32_t cdw04_09[6]; ///< Command dword 4 to 9

	uint64_t slba; ///< SLBA: Starting LBA

	uint32_t cdw12; ///< Command dword 12, see XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_*

	uint32_t cdw13; ///< Command dword 13, see XNVME_SPEC_NVM_WRITE_ZEROES_CDW13_*

	uint32_t lbtl; ///< LBTL: Logical Block Tags Lower, with LBTU the LBST and the ILBRT

	uint16_t lbat;  ///< LBAT: Logical Block Application Tag
	uint16_t lbatm; ///< LBATM: Logical Block Application Tag Mask
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_write_zeroes) == 64, "Incorrect size")

/** Logical Block Tags Upper; bits 47:00 of 'cdw02_03', LBTU (NVM Command Set 1.3, Figure 81) */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW02_03_LBTU 0xffffffffffff

/** Number of Logical Blocks, zero-based; bits 15:00 of 'cdw12', NLB (NVM Command Set 1.3, Figure
 * 83) */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_NLB 0xffff
/** Command Extension Type; bits 19:16 of 'cdw12', CETYPE (NVM Command Set 1.3, Figure 83) */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_CETYPE 0xf0000
/** Directive Type; bits 22:20 of 'cdw12', DTYPE (NVM Command Set 1.3, Figure 83) */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_DTYPE 0x700000
/** Namespace Zeroes; bit 23 of 'cdw12', NSZ (NVM Command Set 1.3, Figure 83) */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_NSZ 0x800000
/** Storage Tag Check; bit 24 of 'cdw12', STC (NVM Command Set 1.3, Figure 83) */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_STC 0x1000000
/** Deallocate; bit 25 of 'cdw12', DEAC (NVM Command Set 1.3, Figure 83) */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_DEAC 0x2000000
/** Protection Information; bits 29:26 of 'cdw12', PRINFO (NVM Command Set 1.3, Figure 83) */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_PRINFO 0x3c000000
/** Force Unit Access; bit 30 of 'cdw12', FUA (NVM Command Set 1.3, Figure 83) */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_FUA 0x40000000
/** Limited Retry; bit 31 of 'cdw12', LR (NVM Command Set 1.3, Figure 83) */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_LR 0x80000000

/** Command Extension Value, if CETYPE is non-zero; bits 15:00 of 'cdw13', CEV (NVM Command Set
 * 1.3, Figure 85) */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW13_CEV 0xffff
/** Directive Specific; bits 31:16 of 'cdw13', DSPEC (NVM Command Set 1.3, Figure 84) */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW13_DSPEC 0xffff0000

/**
 * Command-set specific status codes related to Logical Block Namespaces
 *
 * @see Specification Section 6.TBD.1, figure 355
 */
enum xnvme_spec_nvm_cmd_cpl_sc {
	/// Copy Command Specific Status Values
	XNVME_SPEC_NVM_CMD_CPL_SC_WRITE_TO_RONLY =
		0x82, ///< XNVME_SPEC_NVM_CMD_CPL_SC_WRITE_TO_RONLY
};

/**
 * Source Range Entry of the Copy command, in Copy Descriptor Format 0h
 *
 * Format 2h has the same layout, with SNSID in 'spars' and FCO in 'sopt'.
 *
 * @see NVM Command Set 1.3, Figure 40
 * @struct xnvme_spec_nvm_scopy_fmt_zero
 */
struct xnvme_spec_nvm_scopy_fmt_zero {
	uint32_t spars; ///< SPARS: Source Parameters; reserved in Format 0h, SNSID in Format 2h

	uint8_t rsvd4[4];

	uint64_t slba; ///< SLBA: Starting LBA

	uint32_t rpars; ///< RPARS: Read Parameters, see XNVME_SPEC_NVM_SCOPY_FMT_ZERO_RPARS_*

	uint16_t cev;  ///< CEV: Command Extension Value
	uint16_t sopt; ///< SOPT: Source Options, see XNVME_SPEC_NVM_SCOPY_FMT_ZERO_SOPT_*

	uint32_t elbt;   ///< ELBT: Expected Logical Block Tags, the ELBST and the EILBRT
	uint16_t elbat;  ///< ELBAT: Expected Logical Block Application Tag
	uint16_t elbatm; ///< ELBATM: Expected Logical Block Application Tag Mask
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_scopy_fmt_zero) == 32, "Incorrect size")

/** Number of Logical Blocks, zero-based; bits 15:00 of 'rpars', NLB (NVM Command Set 1.3, Figure
 * 40) */
#define XNVME_SPEC_NVM_SCOPY_FMT_ZERO_RPARS_NLB 0xffff
/** Command Extension Type; bits 19:16 of 'rpars', CETYPE (NVM Command Set 1.3, Figure 40) */
#define XNVME_SPEC_NVM_SCOPY_FMT_ZERO_RPARS_CETYPE 0xf0000
/** Fast Copy Only, Format 2h only; bit 15 of 'sopt', FCO (NVM Command Set 1.3, Figure 40) */
#define XNVME_SPEC_NVM_SCOPY_FMT_ZERO_SOPT_FCO 0x8000

/**
 * Structure conversion for support Kernel format
 *
 * @enum xnvme_nvm_scopy_fmt
 */
enum xnvme_nvm_scopy_fmt {
	XNVME_NVM_SCOPY_FMT_ZERO =
		0x1, ///< user provides entries formatted as ::XNVME_NVM_SCOPY_FMT_ZERO
	XNVME_NVM_SCOPY_FMT_SRCLEN =
		0x1 << 8, ///< user provides entries formatted as ::XNVME_NVM_SCOPY_FMT_SRCLEN
};

/** Number of Source Range entries in ::xnvme_spec_nvm_scopy_source_range, a 4096-byte buffer */
#define XNVME_SPEC_NVM_SCOPY_NENTRY_MAX 128

/**
 * @see Specification Section 6.TBD.1
 */
struct xnvme_spec_nvm_scopy_source_range {
	struct xnvme_spec_nvm_scopy_fmt_zero entry[XNVME_SPEC_NVM_SCOPY_NENTRY_MAX];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_scopy_source_range) == 4096, "Incorrect size")

/**
 * NVMe Command Accessor for command with opcode XNVME_SPEC_NVM_CMD_OPC_SCOPY
 *
 * @see NVM Command Set 1.3, Figures 33 to 38
 * @struct xnvme_spec_nvm_cmd_scopy
 */
struct xnvme_spec_nvm_cmd_scopy {
	uint32_t cdw00_01[2]; ///< Command dword 0 and 1

	uint64_t cdw02_03; ///< Command dword 2 and 3, see XNVME_SPEC_NVM_CMD_SCOPY_CDW02_03_*

	uint32_t cdw04_09[6]; ///< Command dword 4 to 9

	uint64_t sdlba; ///< SDLBA: Starting Destination LBA

	uint32_t cdw12; ///< Command dword 12, see XNVME_SPEC_NVM_CMD_SCOPY_CDW12_*

	uint16_t cev;   ///< CEV: Command Extension Value
	uint16_t dspec; ///< DSPEC: Directive Specific

	uint32_t lbtl; ///< LBTL: Logical Block Tags Lower, with LBTU the LBST and the ILBRT

	uint16_t lbat;  ///< LBAT: Logical Block Application Tag
	uint16_t lbatm; ///< LBATM: Logical Block Application Tag Mask
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_cmd_scopy) == 64, "Incorrect size")

/** Logical Block Tags Upper; bits 47:00 of 'cdw02_03', LBTU (NVM Command Set 1.3, Figure 33) */
#define XNVME_SPEC_NVM_CMD_SCOPY_CDW02_03_LBTU 0xffffffffffff

/** Number of Ranges, zero-based; bits 07:00 of 'cdw12', NR (NVM Command Set 1.3, Figure 35) */
#define XNVME_SPEC_NVM_CMD_SCOPY_CDW12_NR 0xff
/** Descriptor Format; bits 11:08 of 'cdw12', DESFMT (NVM Command Set 1.3, Figure 35) */
#define XNVME_SPEC_NVM_CMD_SCOPY_CDW12_DESFMT 0xf00
/** Protection Information Read; bits 15:12 of 'cdw12', PRINFOR (NVM Command Set 1.3, Figure 35) */
#define XNVME_SPEC_NVM_CMD_SCOPY_CDW12_PRINFOR 0xf000
/** Command Extension Type; bits 19:16 of 'cdw12', CETYPE (NVM Command Set 1.3, Figure 35) */
#define XNVME_SPEC_NVM_CMD_SCOPY_CDW12_CETYPE 0xf0000
/** Directive Type; bits 23:20 of 'cdw12', DTYPE (NVM Command Set 1.3, Figure 35) */
#define XNVME_SPEC_NVM_CMD_SCOPY_CDW12_DTYPE 0xf00000
/** Storage Tag Check Write; bit 24 of 'cdw12', STCW (NVM Command Set 1.3, Figure 35) */
#define XNVME_SPEC_NVM_CMD_SCOPY_CDW12_STCW 0x1000000
/** Storage Tag Check Read; bit 25 of 'cdw12', STCR (NVM Command Set 1.3, Figure 35) */
#define XNVME_SPEC_NVM_CMD_SCOPY_CDW12_STCR 0x2000000
/** Protection Information Write; bits 29:26 of 'cdw12', PRINFOW (NVM Command Set 1.3, Figure 35)
 */
#define XNVME_SPEC_NVM_CMD_SCOPY_CDW12_PRINFOW 0x3c000000
/** Force Unit Access; bit 30 of 'cdw12', FUA (NVM Command Set 1.3, Figure 35) */
#define XNVME_SPEC_NVM_CMD_SCOPY_CDW12_FUA 0x40000000
/** Limited Retry; bit 31 of 'cdw12', LR (NVM Command Set 1.3, Figure 35) */
#define XNVME_SPEC_NVM_CMD_SCOPY_CDW12_LR 0x80000000

/**
 * Kernel format structure for scopy
 *
 * @note Intruder alert!T his is not defined in the NVMe specification. Yet, it is provided here as
 * though it was. It is intensionally done so to provide a format within the framework provided by
 * NVMe.
 *
 * @struct xnvme_spec_nvm_cmd_scopy_fmt_srclen
 */
struct xnvme_spec_nvm_cmd_scopy_fmt_srclen {
	uint64_t start;
	uint64_t len;
};

/**
 * NVMe Command Accessors for the NVM Command Set
 *
 * @union xnvme_spec_nvm_cmd
 */
union xnvme_spec_nvm_cmd {
	struct xnvme_spec_nvm_cmd_scopy scopy; ///< As the Copy command
};
XNVME_STATIC_ASSERT(sizeof(union xnvme_spec_nvm_cmd) == 64, "Incorrect size")

/**
 * Representation of NVMe completion result for NVM command set I/O command set
 * specific Identify Controller
 *
 * That is, for opcode XNVME_SPEC_OPC_IDFY(0x06) with XNVME_SPEC_IDFY_CTLR_IOCS(0x6)
 *
 * @see NVM Command Set 1.3, Figure 129
 * @struct xnvme_spec_nvm_idfy_ctrlr
 */
struct xnvme_spec_nvm_idfy_ctrlr {
	uint8_t vsl;    ///< VSL: Verify Size Limit
	uint8_t wzsl;   ///< WZSL: Write Zeroes Size Limit
	uint8_t wusl;   ///< WUSL: Write Uncorrectable Size Limit
	uint8_t dmrl;   ///< DMRL: Dataset Management Ranges Limit
	uint32_t dmrsl; ///< DMRSL: Dataset Management Range Size Limit
	uint64_t dmsl;  ///< DMSL: Dataset Management Size Limit

	uint8_t kpiocap; ///< KPIOCAP: Key Per I/O Capabilities, see
			 ///< XNVME_SPEC_NVM_IDFY_CTRLR_KPIOCAP_*
	uint8_t wzdsl;   ///< WZDSL: Write Zeroes With Deallocate Size Limit
	uint16_t aocs;   ///< AOCS: Admin Optional Command Support, see
			 ///< XNVME_SPEC_NVM_IDFY_CTRLR_AOCS_*
	uint32_t ver;    ///< VER: Version
	uint8_t lbamqf;  ///< LBAMQF: LBA Migration Queue Format
	uint8_t rla;     ///< RLA: Rate Limiting Attributes, see XNVME_SPEC_NVM_IDFY_CTRLR_RLA_*
	uint16_t slmc;   ///< SLMC: Soft Limit Maximum Controllers, zero-based

	uint8_t rsvd28[4068];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_idfy_ctrlr) == 4096, "Incorrect size")

/** Key Per I/O Supported; bit 0 of 'kpiocap', KPIOS (NVM Command Set 1.3, Figure 129) */
#define XNVME_SPEC_NVM_IDFY_CTRLR_KPIOCAP_KPIOS 0x1
/** Key Per I/O Scope; bit 1 of 'kpiocap', KPIOSC (NVM Command Set 1.3, Figure 129) */
#define XNVME_SPEC_NVM_IDFY_CTRLR_KPIOCAP_KPIOSC 0x2
/** Reporting Allocated LBA Supported; bit 0 of 'aocs', RALBAS (NVM Command Set 1.3, Figure 129) */
#define XNVME_SPEC_NVM_IDFY_CTRLR_AOCS_RALBAS 0x1
/** Hard Limit Support; bit 0 of 'rla', HLS (NVM Command Set 1.3, Figure 129) */
#define XNVME_SPEC_NVM_IDFY_CTRLR_RLA_HLS 0x1
/** Soft Limit Support; bit 1 of 'rla', SLS (NVM Command Set 1.3, Figure 129) */
#define XNVME_SPEC_NVM_IDFY_CTRLR_RLA_SLS 0x2

/**
 * Representation of NVMe completion result for NVM command set I/O command set
 * specific Identify Namespace
 *
 * That is, for opcode XNVME_SPEC_OPC_IDFY(0x06) with XNVME_SPEC_IDFY_NS_IOCS(0x05)
 *
 * @see NVM Command Set 1.3, Figure 127
 * @struct xnvme_spec_nvm_idfy_ns
 */
struct xnvme_spec_nvm_idfy_ns {
	uint64_t lbstm; ///< LBSTM: Logical Block Storage Tag Mask

	uint8_t pic;  ///< PIC: Protection Information Capabilities, see
		      ///< XNVME_SPEC_NVM_IDFY_NS_PIC_*
	uint8_t pifa; ///< PIFA: Protection Information Format Attribute, see ..._NS_PIFA_*

	uint8_t rsvd10[2];

	struct xnvme_spec_elbaf elbaf[64]; ///< ELBAF0 to ELBAF63: Extended LBA Formats

	uint32_t npdgl;  ///< NPDGL: Namespace Preferred Deallocate Granularity Large
	uint32_t nprg;   ///< NPRG: Namespace Preferred Read Granularity, zero-based
	uint32_t npra;   ///< NPRA: Namespace Preferred Read Alignment, zero-based
	uint32_t nors;   ///< NORS: Namespace Optimal Read Size, zero-based
	uint32_t npdal;  ///< NPDAL: Namespace Preferred Deallocate Alignment Large
	uint32_t lbapss; ///< LBAPSS: LBA Format Placement Shard Size
	uint32_t tlbaag; ///< TLBAAG: Tracked LBA Allocation Granularity

	uint8_t rsvd296[3800];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_idfy_ns) == 4096, "Incorrect size")

/** 16b Guard Protection Information Storage Tag Support; bit 0 of 'pic', PISTS16B (NVM Command
 * Set 1.3, Figure 127) */
#define XNVME_SPEC_NVM_IDFY_NS_PIC_PISTS16B 0x1
/** 16b Guard Protection Information Storage Tag Mask; bit 1 of 'pic', PISTM16B (NVM Command
 * Set 1.3, Figure 127) */
#define XNVME_SPEC_NVM_IDFY_NS_PIC_PISTM16B 0x2
/** Storage Tag Check Read Support; bit 2 of 'pic', STCRS (NVM Command Set 1.3, Figure 127) */
#define XNVME_SPEC_NVM_IDFY_NS_PIC_STCRS 0x4
/** Qualified Protection Information Format Support; bit 3 of 'pic', QPIFS (NVM Command Set 1.3,
 * Figure 127) */
#define XNVME_SPEC_NVM_IDFY_NS_PIC_QPIFS 0x8
/** Storage Tag Masking Level Attribute; bits 2:0 of 'pifa', STMLA (NVM Command Set 1.3, Figure
 * 127) */
#define XNVME_SPEC_NVM_IDFY_NS_PIFA_STMLA 0x7

/**
 * The data returned by an Identify command for the NVM command set, as the structure of each CNS
 *
 * @union xnvme_spec_nvm_idfy
 */
union xnvme_spec_nvm_idfy {
	union xnvme_spec_idfy base;             ///< As the common Identify data
	struct xnvme_spec_nvm_idfy_ctrlr ctrlr; ///< Of XNVME_SPEC_IDFY_CTRLR_IOCS
	struct xnvme_spec_nvm_idfy_ns ns;       ///< Of XNVME_SPEC_IDFY_NS_IOCS
};
XNVME_STATIC_ASSERT(sizeof(union xnvme_spec_nvm_idfy) == 4096, "Incorrect size")

/**
 * Log identifiers for Zoned Namespaces
 *
 * TODO: merge this with the set of log-identifiers
 *
 * @enum xnvme_spec_znd_log_lid
 */
enum xnvme_spec_znd_log_lid {
	XNVME_SPEC_LOG_ZND_CHANGES = 0xBF, ///< ZND_CMD_LOG_CHANGES
};

/**
 * Zoned Command Set opcodes
 *
 * @see Specification Section 6, figure ZONEDOPCODES
 *
 * @struct xnvme_spec_znd_opc
 */
enum xnvme_spec_znd_opc {
	XNVME_SPEC_ZND_OPC_MGMT_SEND = 0x79, ///< XNVME_SPEC_ZND_OPC_MGMT_SEND
	XNVME_SPEC_ZND_OPC_MGMT_RECV = 0x7A, ///< XNVME_SPEC_ZND_OPC_MGMT_RECV
	XNVME_SPEC_ZND_OPC_APPEND    = 0x7D, ///< XNVME_SPEC_ZND_OPC_APPEND
};

/**
 * NVMe Command Accessor for command with opcode ZND_CMD_OPC_MGMT_SEND
 *
 * @see Zoned Namespace Command Set 1.5, Figures 39 and 40
 * @struct xnvme_spec_znd_cmd_mgmt_send
 */
struct xnvme_spec_znd_cmd_mgmt_send {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint64_t slba; ///< SLBA: Starting LBA

	uint32_t cdw12; ///< Command dword 12, reserved

	uint32_t cdw13; ///< Command dword 13, see XNVME_SPEC_ZND_CMD_MGMT_SEND_CDW13_*

	uint32_t cdw14_15[2]; ///< Command dword 14 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_cmd_mgmt_send) == 64, "Incorrect size")

/** Zone Send Action; bits 7:0 of 'cdw13', ZSA (Zoned Namespace Command Set 1.5, Figure 40) */
#define XNVME_SPEC_ZND_CMD_MGMT_SEND_CDW13_ZSA 0xff
/** Select All, that is, ignore the slba, affects all LBAs; bit 8 of 'cdw13', SELALL (Zoned
 * Namespace Command Set 1.5, Figure 40) */
#define XNVME_SPEC_ZND_CMD_MGMT_SEND_CDW13_SELALL 0x100
/** Zone Send Action Specific Option, ZRWAA for Open Zone; bit 9 of 'cdw13', ZSASO (Zoned Namespace
 * Command Set 1.5, Figure 40) */
#define XNVME_SPEC_ZND_CMD_MGMT_SEND_CDW13_ZSASO 0x200
/** Zone Management, DL in bits 5:0 for Open Zone and Set Zone Descriptor Extension; bits 23:16
 * of 'cdw13', ZM (Zoned Namespace Command Set 1.5, Figure 40) */
#define XNVME_SPEC_ZND_CMD_MGMT_SEND_CDW13_ZM 0xff0000

/**
 * NVMe Command Accessor for a command with opcode XNVME_SPEC_ZND_OPC_MGMT_RECV
 *
 * @see enum xnvme_spec_znd_cmd_mgmt_recv
 * @see Zoned Namespace Command Set 1.5, Figures 33 to 35
 * @struct xnvme_spec_znd_cmd_mgmt_recv
 */
struct xnvme_spec_znd_cmd_mgmt_recv {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint64_t slba; ///< SLBA: Starting LBA

	uint32_t ndw; ///< NDW: Number of Dwords, zero-based

	uint32_t cdw13; ///< Command dword 13, see XNVME_SPEC_ZND_CMD_MGMT_RECV_CDW13_*

	uint32_t cdw14_15[2]; ///< Command dword 14 to 15, reserved
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_cmd_mgmt_recv) == 64, "Incorrect size")

/** Zone Receive Action; bits 7:0 of 'cdw13', ZRA (Zoned Namespace Command Set 1.5, Figure 35) */
#define XNVME_SPEC_ZND_CMD_MGMT_RECV_CDW13_ZRA 0xff
/** Zone Receive Action Specific; bits 15:8 of 'cdw13', ZRAS (Zoned Namespace Command Set 1.5,
 * Figure 35) */
#define XNVME_SPEC_ZND_CMD_MGMT_RECV_CDW13_ZRAS 0xff00
/** Zone Receive Action Specific Features, Partial Report for the Report Zones actions; bit 16 of
 * 'cdw13', ZRASPF (Zoned Namespace Command Set 1.5, Figure 35) */
#define XNVME_SPEC_ZND_CMD_MGMT_RECV_CDW13_ZRASPF 0x10000

/**
 * NVMe Command Accessor for a command with opcode ZND_CMD_OPC_APPEND
 *
 * @see Zoned Namespace Command Set 1.5, Figures 24 to 30
 * @struct xnvme_spec_znd_cmd_append
 */
struct xnvme_spec_znd_cmd_append {
	uint32_t cdw00_01[2]; ///< Command dword 0 and 1

	uint64_t cdw02_03; ///< Command dword 2 and 3, see XNVME_SPEC_ZND_CMD_APPEND_CDW02_03_*

	uint32_t cdw04_09[6]; ///< Command dword 4 to 9

	uint64_t zslba; ///< ZSLBA: Zone Start Logical Block Address

	uint32_t cdw12; ///< Command dword 12, see XNVME_SPEC_ZND_CMD_APPEND_CDW12_*

	uint32_t cdw13; ///< Command dword 13, see XNVME_SPEC_ZND_CMD_APPEND_CDW13_*

	uint32_t lbtl; ///< LBTL: Logical Block Tags Lower, with LBTU the LBST and the ILBRT

	uint16_t lbat;  ///< LBAT: Logical Block Application Tag
	uint16_t lbatm; ///< LBATM: Logical Block Application Tag Mask
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_cmd_append) == 64, "Incorrect size")

/** Logical Block Tags Upper; bits 47:00 of 'cdw02_03', LBTU (Zoned Namespace Command Set 1.5,
 * Figure 24) */
#define XNVME_SPEC_ZND_CMD_APPEND_CDW02_03_LBTU 0xffffffffffff

/** Number of Logical Blocks, zero-based; bits 15:0 of 'cdw12', NLB (Zoned Namespace Command
 * Set 1.5, Figure 26) */
#define XNVME_SPEC_ZND_CMD_APPEND_CDW12_NLB 0xffff
/** Command Extension Type; bits 19:16 of 'cdw12', CETYPE (Zoned Namespace Command Set 1.5, Figure
 * 26) */
#define XNVME_SPEC_ZND_CMD_APPEND_CDW12_CETYPE 0xf0000
/** Directive Type; bits 23:20 of 'cdw12', DTYPE (Zoned Namespace Command Set 1.5, Figure 26) */
#define XNVME_SPEC_ZND_CMD_APPEND_CDW12_DTYPE 0xf00000
/** Storage Tag Check; bit 24 of 'cdw12', STC (Zoned Namespace Command Set 1.5, Figure 26) */
#define XNVME_SPEC_ZND_CMD_APPEND_CDW12_STC 0x1000000
/** Protection Information Remap; bit 25 of 'cdw12', PIREMAP (Zoned Namespace Command Set 1.5,
 * Figure 26) */
#define XNVME_SPEC_ZND_CMD_APPEND_CDW12_PIREMAP 0x2000000
/** Protection Information; bits 29:26 of 'cdw12', PRINFO (Zoned Namespace Command Set 1.5, Figure
 * 26) */
#define XNVME_SPEC_ZND_CMD_APPEND_CDW12_PRINFO 0x3c000000
/** Force Unit Access; bit 30 of 'cdw12', FUA (Zoned Namespace Command Set 1.5, Figure 26) */
#define XNVME_SPEC_ZND_CMD_APPEND_CDW12_FUA 0x40000000
/** Limited Retry; bit 31 of 'cdw12', LR (Zoned Namespace Command Set 1.5, Figure 26) */
#define XNVME_SPEC_ZND_CMD_APPEND_CDW12_LR 0x80000000

/** Command Extension Value, if CETYPE is non-zero; bits 15:0 of 'cdw13', CEV (Zoned Namespace
 * Command Set 1.5, Figure 28) */
#define XNVME_SPEC_ZND_CMD_APPEND_CDW13_CEV 0xffff
/** Directive Specific; bits 31:16 of 'cdw13', DSPEC (Zoned Namespace Command Set 1.5, Figure 27)
 */
#define XNVME_SPEC_ZND_CMD_APPEND_CDW13_DSPEC 0xffff0000

/**
 * NVMe Command Accessors for the Zoned Command Set
 *
 * @union xnvme_spec_znd_cmd
 */
union xnvme_spec_znd_cmd {
	struct xnvme_spec_znd_cmd_mgmt_send mgmt_send; ///< As the Zone Management Send command
	struct xnvme_spec_znd_cmd_mgmt_recv mgmt_recv; ///< As the Zone Management Receive command
	struct xnvme_spec_znd_cmd_append append;       ///< As the Zone Append command
};
XNVME_STATIC_ASSERT(sizeof(union xnvme_spec_znd_cmd) == 64, "Incorrect size")

/**
 * NVMe Command Accessor for io management receive command
 *
 * @struct xnvme_spec_io_mgmt_recv_cmd
 */
struct xnvme_spec_io_mgmt_recv_cmd {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint8_t mo; ///< Management Operation, see enum xnvme_spec_io_mgmt_recv_mo
	uint8_t rsvd;
	uint16_t mos; ///< Management Operation Specific

	uint32_t numd; ///< cdw11

	uint32_t cdw12_15[4]; ///< Command dword 12 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_io_mgmt_recv_cmd) == 64, "Incorrect size")

/**
 * NVMe Command Accessor for I/O management send
 *
 * @struct xnvme_spec_io_mgmt_send_cmd
 */
struct xnvme_spec_io_mgmt_send_cmd {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint8_t mo; ///< Management Operation, see enum xnvme_spec_io_mgmt_send_mo
	uint8_t rsvd;
	uint16_t mos; ///< Management Operation Specific

	uint32_t cdw11_15[5]; ///< Command dword 11 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_io_mgmt_send_cmd) == 64, "Incorrect size")

/**
 * NVMe Command Accessors for I/O management
 *
 * @union xnvme_spec_io_mgmt_cmd
 */
union xnvme_spec_io_mgmt_cmd {
	struct xnvme_spec_io_mgmt_recv_cmd mgmt_recv; ///< As the I/O Management Receive command
	struct xnvme_spec_io_mgmt_send_cmd mgmt_send; ///< As the I/O Management Send command
};
XNVME_STATIC_ASSERT(sizeof(union xnvme_spec_io_mgmt_cmd) == 64, "Incorrect size")

/**
 * KV Command Set opcodes
 *
 * @see Key Value Command Set Specification Revision 1.0c, Figure 5
 *
 * @struct xnvme_spec_kv_opc
 */
enum xnvme_spec_kv_opc {
	XNVME_SPEC_KV_OPC_STORE    = 0x01, ///< XNVME_SPEC_KV_OPC_STORE
	XNVME_SPEC_KV_OPC_RETRIEVE = 0x02, ///< XNVME_SPEC_KV_OPC_RETRIEVE
	XNVME_SPEC_KV_OPC_DELETE   = 0x10, ///< XNVME_SPEC_KV_OPC_DELETE
	XNVME_SPEC_KV_OPC_EXIST    = 0x14, ///< XNVME_SPEC_KV_OPC_EXIST
	XNVME_SPEC_KV_OPC_LIST     = 0x06, ///< XNVME_SPEC_KV_OPC_LIST
};

/**
 * Command-set specific status codes related to key-value Namespaces
 *
 * @see Key Value Command Set Specification Revision 1.0c, Figure 4
 *
 * @enum xnvme_spec_kv_status_code
 */
enum xnvme_spec_kv_status_code {
	XNVME_SPEC_KV_SC_CAPACITY_EXCEEDED    = 0x81, ///<  XNVME_SPEC_KV_SC_CAPACITY_EXCEEDED
	XNVME_SPEC_KV_SC_NS_NOT_READY         = 0x82, ///< XNVME_SPEC_KV_SC_NS_NOT_READY
	XNVME_SPEC_KV_SC_RESERVATION_CONFLICT = 0x83, ///< XNVME_SPEC_KV_SC_RESERVATION_CONFLICT
	XNVME_SPEC_KV_SC_FORMAT_IN_PROGRESS   = 0x84, ///< XNVME_SPEC_KV_SC_FORMAT_IN_PROGRESS
	XNVME_SPEC_KV_SC_INVALID_VAL_SIZE     = 0x85, ///< XNVME_SPEC_KV_SC_INVALID_VAL_SIZE
	XNVME_SPEC_KV_SC_INVALID_KEY_SIZE     = 0x86, ///< XNVME_SPEC_KV_SC_INVALID_KEY_SIZE
	XNVME_SPEC_KV_SC_KEY_NOT_EXISTS       = 0x87, ///< XNVME_SPEC_KV_SC_KEY_NOT_EXISTS
	XNVME_SPEC_KV_SC_UNRECOVERED_ERR      = 0x88, ///< XNVME_SPEC_KV_SC_UNRECOVERED_ERR
	XNVME_SPEC_KV_SC_KEY_EXISTS           = 0x89, ///< XNVME_SPEC_KV_SC_KEY_EXISTS
};

/**
 * Command dword 11 of the commands of the KV Command Set
 *
 * @see Key Value Command Set 1.4, Figures 20 and 31
 *
 * @struct xnvme_spec_kvs_cmd_cdw11
 */
struct xnvme_spec_kvs_cmd_cdw11 {
	uint8_t kl; ///< KL: Key Length
	uint8_t ro; ///< RO: Retrieve Option, of Store the SO: Store Option, see XNVME_KVS_*_OPT_*
	uint16_t rsvd;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_kvs_cmd_cdw11) == 4, "Incorrect size")

/**
 * NVMe Command Accessors for the KV Command Set
 *
 * @see Key Value Command Set 1.4, Figures 8 to 34
 *
 * @struct xnvme_spec_kvs_cmd
 */
struct xnvme_spec_kvs_cmd {
	uint32_t cdw0;
	uint32_t nsid;  ///< NSID: Namespace Identifier
	uint64_t key;   ///< KV Key[63:00], command dwords 2 and 3
	uint64_t mptr;  ///< Reserved for MPTR
	uint32_t cdw06; ///< DPTR: Data Pointer, dwords 6 to 9
	uint32_t cdw07;
	uint32_t cdw08;
	uint32_t cdw09;
	uint32_t cdw10; ///< HBS: Host Buffer Size, of Store the VS: Value Size

	struct xnvme_spec_kvs_cmd_cdw11 cdw11; ///< Command dword 11
	uint32_t cdw12;
	uint32_t cdw13;  ///< Command dword 13, see XNVME_SPEC_KVS_CMD_CDW13_*
	uint64_t key_hi; ///< KV Key[127:64], command dwords 14 and 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_kvs_cmd) == 64, "Incorrect size")

/** Command Extension Value; bits 15:0 of 'cdw13', CEV (Key Value Command Set 1.4, Figure 21) */
#define XNVME_SPEC_KVS_CMD_CDW13_CEV 0xffff
/** Command Extension Type; bits 19:16 of 'cdw13', CETYPE (Key Value Command Set 1.4, Figure 21)
 */
#define XNVME_SPEC_KVS_CMD_CDW13_CETYPE 0xf0000

/**
 * NVMe Command Accessor for the Compare command
 *
 * @see NVM Command Set 1.3, Figures 25 to 30
 * @struct xnvme_spec_nvm_compare
 */
struct xnvme_spec_nvm_compare {
	uint32_t cdw00_01[2]; ///< Command dword 0 and 1

	uint64_t cdw02_03; ///< Command dword 2 and 3, see XNVME_SPEC_NVM_COMPARE_CDW02_03_*

	uint32_t cdw04_09[6]; ///< Command dword 4 to 9

	uint64_t slba; ///< SLBA: Starting LBA

	uint32_t cdw12; ///< Command dword 12, see XNVME_SPEC_NVM_COMPARE_CDW12_*

	uint32_t cdw13; ///< Command dword 13, see XNVME_SPEC_NVM_COMPARE_CDW13_*

	uint32_t elbtl; ///< ELBTL: Expected Logical Block Tags Lower, with ELBTU the LBST and the
			///< ILBRT

	uint16_t elbat;  ///< ELBAT: Expected Logical Block Application Tag
	uint16_t elbatm; ///< ELBATM: Expected Logical Block Application Tag Mask
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_compare) == 64, "Incorrect size")

/** Expected Logical Block Tags Upper; bits 47:00 of 'cdw02_03', ELBTU (NVM Command Set 1.3,
 * Figure 25) */
#define XNVME_SPEC_NVM_COMPARE_CDW02_03_ELBTU 0xffffffffffff

/** Number of Logical Blocks, zero-based; bits 15:00 of 'cdw12', NLB (NVM Command Set 1.3, Figure
 * 27) */
#define XNVME_SPEC_NVM_COMPARE_CDW12_NLB 0xffff
/** Command Extension Type; bits 19:16 of 'cdw12', CETYPE (NVM Command Set 1.3, Figure 27) */
#define XNVME_SPEC_NVM_COMPARE_CDW12_CETYPE 0xf0000
/** Storage Tag Check; bit 24 of 'cdw12', STC (NVM Command Set 1.3, Figure 27) */
#define XNVME_SPEC_NVM_COMPARE_CDW12_STC 0x1000000
/** Protection Information; bits 29:26 of 'cdw12', PRINFO (NVM Command Set 1.3, Figure 27) */
#define XNVME_SPEC_NVM_COMPARE_CDW12_PRINFO 0x3c000000
/** Force Unit Access; bit 30 of 'cdw12', FUA (NVM Command Set 1.3, Figure 27) */
#define XNVME_SPEC_NVM_COMPARE_CDW12_FUA 0x40000000
/** Limited Retry; bit 31 of 'cdw12', LR (NVM Command Set 1.3, Figure 27) */
#define XNVME_SPEC_NVM_COMPARE_CDW12_LR 0x80000000

/** Command Extension Value, if CETYPE is non-zero; bits 15:00 of 'cdw13', CEV (NVM Command Set
 * 1.3, Figure 28) */
#define XNVME_SPEC_NVM_COMPARE_CDW13_CEV 0xffff

/**
 * Submission Queue Entry
 *
 * The command as its dwords 0 to 15, and as the view of each command; they share the same 64
 * bytes.
 *
 * @union xnvme_spec_sqe
 */
union xnvme_spec_sqe {
	uint32_t cdw[16]; ///< The command as dwords 0 to 15, as the specification numbers them
	struct xnvme_spec_cmd_common common;
	struct xnvme_spec_cmd_sanitize sanitize;
	struct xnvme_spec_cmd_format format;
	struct xnvme_spec_cmd_log log;
	struct xnvme_spec_cmd_gfeat gfeat;
	struct xnvme_spec_cmd_sfeat sfeat;
	struct xnvme_spec_cmd_idfy idfy;
	struct xnvme_spec_cmd_dsend dsend;
	struct xnvme_spec_cmd_drecv drecv;
	struct xnvme_spec_cmd_nvm nvm;
	struct xnvme_spec_cmd_dsm dsm;
	struct xnvme_spec_nvm_cmd_scopy scopy;
	struct xnvme_spec_nvm_write_zeroes write_zeroes;
	union xnvme_spec_znd_cmd znd;
	union xnvme_spec_io_mgmt_cmd mgmt;
	struct xnvme_spec_kvs_cmd kvs;
	struct xnvme_spec_nvm_compare compare;
};
XNVME_STATIC_ASSERT(sizeof(union xnvme_spec_sqe) == 64, "Incorrect size")

/**
 * Command-set specific status codes related to Zoned Namespaces
 *
 * @see Specification Section 4.1.1.1.1, figures GENERICSTATUSCODES and
 * STATUSCODES
 *
 * @enum xnvme_spec_znd_status_code
 */
enum xnvme_spec_znd_status_code {
	// Namespace Management
	XNVME_SPEC_ZND_SC_INVALID_FORMAT = 0x7F, ///< ZND_SC_INVALID_FORMAT

	/// Zoned Command Set
	XNVME_SPEC_ZND_SC_INVALID_ZONE_OP = 0xB6, ///< ZND_SC_INVALID_ZONE_OP
	XNVME_SPEC_ZND_SC_NOZRWA          = 0xB7, ///< ZND_SC_NOZRWA
	XNVME_SPEC_ZND_SC_BOUNDARY_ERROR  = 0xB8, ///< ZND_SC_BOUNDARY_ERROR
	XNVME_SPEC_ZND_SC_IS_FULL         = 0xB9, ///< ZND_SC_IS_FULL
	XNVME_SPEC_ZND_SC_IS_READONLY     = 0xBA, ///< ZND_SC_IS_READONLY
	XNVME_SPEC_ZND_SC_IS_OFFLINE      = 0xBB, ///< ZND_SC_IS_OFFLINE
	XNVME_SPEC_ZND_SC_INVALID_WRITE   = 0xBC, ///< ZND_SC_INVALID_WRITE
	XNVME_SPEC_ZND_SC_TOO_MANY_ACTIVE = 0xBD, ///< ZND_SC_TOO_MANY_ACTIVE
	XNVME_SPEC_ZND_SC_TOO_MANY_OPEN   = 0xBE, ///< ZND_SC_TOO_MANY_OPEN
	XNVME_SPEC_ZND_SC_INVALID_TRANS   = 0xBF, ///< ZND_SC_INVALID_TRANS
};

/**
 * This is not defined in any spec... it would just seem sane if it was...
 *
 * @enum xnvme_spec_znd_mgmt_send_action_so
 */
enum xnvme_spec_znd_mgmt_send_action_so {
	XNVME_SPEC_ZND_MGMT_OPEN_WITH_ZRWA = 0x1, ///< XNVME_SPEC_ZND_MGMT_OPEN_WITH_ZRWA
};

/**
 * Actions for the Zone Management Send command
 *
 * @see Specification Section 6.1, figure TBDZMDW13
 *
 * @enum xnvme_spec_znd_cmd_mgmt_send_action
 */
enum xnvme_spec_znd_cmd_mgmt_send_action {
	XNVME_SPEC_ZND_CMD_MGMT_SEND_CLOSE   = 0x1, ///< XNVME_SPEC_ZND_CMD_MGMT_SEND_CLOSE
	XNVME_SPEC_ZND_CMD_MGMT_SEND_FINISH  = 0x2, ///< XNVME_SPEC_ZND_CMD_MGMT_SEND_FINISH
	XNVME_SPEC_ZND_CMD_MGMT_SEND_OPEN    = 0x3, ///< XNVME_SPEC_ZND_CMD_MGMT_SEND_OPEN
	XNVME_SPEC_ZND_CMD_MGMT_SEND_RESET   = 0x4, ///< XNVME_SPEC_ZND_CMD_MGMT_SEND_RESET
	XNVME_SPEC_ZND_CMD_MGMT_SEND_OFFLINE = 0x5, ///< XNVME_SPEC_ZND_CMD_MGMT_SEND_OFFLINE
	XNVME_SPEC_ZND_CMD_MGMT_SEND_DESCRIPTOR =
		0x10,                              ///< XNVME_SPEC_ZND_CMD_MGMT_SEND_DESCRIPTOR
	XNVME_SPEC_ZND_CMD_MGMT_SEND_FLUSH = 0x11, ///< XNVME_SPEC_ZND_CMD_MGMT_SEND_FLUSH
};

/**
 * Zone Receive Action (::xnvme_spec_znd_cmd_mgmt_recv_action) specific field values
 *
 * @see Specification Section 6.2, figure TBDZMRDW13
 *
 * @enum xnvme_spec_znd_cmd_mgmt_recv_action_sf
 */
enum xnvme_spec_znd_cmd_mgmt_recv_action_sf {
	XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_ALL     = 0x0, ///< XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_ALL
	XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_EMPTY   = 0x1, ///< XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_EMPTY
	XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_IOPEN   = 0x2, ///< XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_IOPEN
	XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_EOPEN   = 0x3, ///< XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_EOPEN
	XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_CLOSED  = 0x4, ///< XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_CLOSED
	XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_FULL    = 0x5, ///< XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_FULL
	XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_RONLY   = 0x6, ///< XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_RONLY
	XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_OFFLINE = 0x7, ///< XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_OFFLINE
};

/**
 * Zoned Receive Action
 *
 * @see Specification Section 6.2, figure TBDZMRDW13
 * @enum xnvme_spec_znd_cmd_mgmt_recv_action
 */
enum xnvme_spec_znd_cmd_mgmt_recv_action {
	///< XNvME_SPEC_ZND_CMD_MGMT_RECV_ACTION_REPORT
	XNVME_SPEC_ZND_CMD_MGMT_RECV_ACTION_REPORT = 0x0,
	///< XNvME_SPEC_ZND_CMD_MGMT_RECV_ACTION_REPORT_EXTENDED
	XNVME_SPEC_ZND_CMD_MGMT_RECV_ACTION_REPORT_EXTENDED = 0x1,
};

/**
 * Zone Type
 *
 * @see Specification Section 6.2.2.3, figure TBDZMRD
 * @enum xnvme_spec_znd_type
 */
enum xnvme_spec_znd_type {
	XNVME_SPEC_ZND_TYPE_SEQWR = 0x2, ///< XNVME_SPEC_ZND_TYPE_SEQWR
};

/**
 * Zone State as reported by Zone Management Receive
 *
 * @see Specification Section 6.2.2.3, figure TBDZMRZD
 * @enum xnvme_spec_znd_state
 */
enum xnvme_spec_znd_state {
	XNVME_SPEC_ZND_STATE_EMPTY   = 0x1, ///< XNVME_SPEC_ZND_STATE_EMPTY
	XNVME_SPEC_ZND_STATE_IOPEN   = 0x2, ///< XNVME_SPEC_ZND_STATE_IOPEN
	XNVME_SPEC_ZND_STATE_EOPEN   = 0x3, ///< XNVME_SPEC_ZND_STATE_EOPEN
	XNVME_SPEC_ZND_STATE_CLOSED  = 0x4, ///< XNVME_SPEC_ZND_STATE_CLOSED
	XNVME_SPEC_ZND_STATE_RONLY   = 0xD, ///< XNVME_SPEC_ZND_STATE_RONLY
	XNVME_SPEC_ZND_STATE_FULL    = 0xE, ///< XNVME_SPEC_ZND_STATE_FULL
	XNVME_SPEC_ZND_STATE_OFFLINE = 0xF, ///< XNVME_SPEC_ZND_STATE_OFFLINE
};

/**
 * Identify controller accessor only for Zoned specific fields
 *
 * @see Zoned Namespace Command Set 1.5, Figure 51
 * @struct xnvme_spec_znd_idfy_ctrlr
 */
struct xnvme_spec_znd_idfy_ctrlr {
	uint8_t zasl; ///< ZASL: Zone Append Size Limit

	uint8_t rsvd1[3];

	uint32_t zctratt; ///< ZCTRATT: Zoned Controller Attributes, see ..._ZCTRATT_*
	uint32_t tar;     ///< TAR: Total Active Resources
	uint32_t uar;     ///< UAR: Unallocated Active Resources
	uint32_t tor;     ///< TOR: Total Open Resources
	uint32_t uor;     ///< UOR: Unallocated Open Resources
	uint32_t tzrwar;  ///< TZRWAR: Total ZRWA Resources
	uint32_t uzrwar;  ///< UZRWAR: Unallocated ZRWA Resources
	uint32_t ver;     ///< VER: Version

	uint8_t rsvd36[4060];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_idfy_ctrlr) == 4096, "Incorrect size")

/** Zoned Namespace Resource Management; bit 0 of 'zctratt', ZNRM (Zoned Namespace Command Set 1.5,
 * Figure 51) */
#define XNVME_SPEC_ZND_IDFY_CTRLR_ZCTRATT_ZNRM 0x1

/**
 * LBA Format Extension
 *
 * @see Zoned Namespace Command Set 1.5, Figure 50
 *
 * @struct xnvme_spec_znd_idfy_lbafe
 */
struct xnvme_spec_znd_idfy_lbafe {
	uint64_t zsze; ///< ZSZE: Zone Size, in logical blocks
	uint8_t zdes;  ///< ZDES: Zone Descriptor Extension Size, in 64 byte units

	uint8_t rsvd[7];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_idfy_lbafe) == 16, "Incorrect size")

/**
 * Zoned Command Set specific identify namespace data structure
 *
 * @see Zoned Namespace Command Set 1.5, Figure 49
 * @struct xnvme_spec_znd_idfy_ns
 */
struct xnvme_spec_znd_idfy_ns {
	uint16_t zoc;  ///< ZOC: Zone Operation Characteristics, see XNVME_SPEC_ZND_IDFY_NS_ZOC_*
	uint16_t ozcs; ///< OZCS: Optional Zoned Command Support, see XNVME_SPEC_ZND_IDFY_NS_OZCS_*

	uint32_t mar; ///< MAR: Maximum Active Resources
	uint32_t mor; ///< MOR: Maximum Open Resources

	uint32_t rrl;  ///< RRL: Reset Recommended Limit
	uint32_t frl;  ///< FRL: Finish Recommended Limit
	uint32_t rrl1; ///< RRL1: Reset Recommended Limit 1
	uint32_t rrl2; ///< RRL2: Reset Recommended Limit 2
	uint32_t rrl3; ///< RRL3: Reset Recommended Limit 3
	uint32_t frl1; ///< FRL1: Finish Recommended Limit 1
	uint32_t frl2; ///< FRL2: Finish Recommended Limit 2
	uint32_t frl3; ///< FRL3: Finish Recommended Limit 3

	uint32_t numzrwa; ///< NUMZRWA: Number of ZRWA Resources
	uint16_t zrwafg;  ///< ZRWAFG: ZRWA Flush Granularity, in logical blocks
	uint16_t zrwasz;  ///< ZRWASZ: ZRWA Size, in logical blocks

	uint8_t zrwacap; ///< ZRWACAP: ZRWA Capability, see XNVME_SPEC_ZND_IDFY_NS_ZRWACAP_*

	uint8_t rsvd53[2763];

	struct xnvme_spec_znd_idfy_lbafe lbafe[64]; ///< LBAFE0 to LBAFE63: LBA Format Extensions

	uint8_t vs[256]; ///< VS: Vendor Specific
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_idfy_ns) == 4096, "Incorrect size")

/** Variable Zone Capacity; bit 0 of 'zoc', VZC (Zoned Namespace Command Set 1.5, Figure 49) */
#define XNVME_SPEC_ZND_IDFY_NS_ZOC_VZC 0x1
/** Zone Active Excursions; bit 1 of 'zoc', ZAE (Zoned Namespace Command Set 1.5, Figure 49) */
#define XNVME_SPEC_ZND_IDFY_NS_ZOC_ZAE 0x2
/** Read Across Zone Boundaries; bit 0 of 'ozcs', RAZB (Zoned Namespace Command Set 1.5, Figure 49)
 */
#define XNVME_SPEC_ZND_IDFY_NS_OZCS_RAZB 0x1
/** ZRWA Supported; bit 1 of 'ozcs', ZRWASUP (Zoned Namespace Command Set 1.5, Figure 49) */
#define XNVME_SPEC_ZND_IDFY_NS_OZCS_ZRWASUP 0x2
/** Explicit Flush Support; bit 0 of 'zrwacap', EXPFLUSHSUP (Zoned Namespace Command Set 1.5,
 * Figure 49) */
#define XNVME_SPEC_ZND_IDFY_NS_ZRWACAP_EXPFLUSHSUP 0x1

/**
 * The data returned by an Identify command for the Zoned command set, as the structure of each CNS
 *
 * @union xnvme_spec_znd_idfy
 */
union xnvme_spec_znd_idfy {
	union xnvme_spec_idfy base;              ///< As the common Identify data
	struct xnvme_spec_znd_idfy_ctrlr zctrlr; ///< Of XNVME_SPEC_IDFY_CTRLR_IOCS
	struct xnvme_spec_znd_idfy_ns zns;       ///< Of XNVME_SPEC_IDFY_NS_IOCS
};
XNVME_STATIC_ASSERT(sizeof(union xnvme_spec_znd_idfy) == 4096, "Incorrect size")

/** Number of Zone Identifiers the Changed Zone List log page holds */
#define ZND_CHANGES_LEN 511

/**
 * Zone Identifier List as returned by get-log-page
 *
 * @see Zoned Namespace Command Set 1.5, Figure 47
 *
 * @struct xnvme_spec_znd_log_changes
 */
struct xnvme_spec_znd_log_changes {
	uint16_t nzid; ///< NZID: Number of Zone Identifiers
	uint8_t rsvd2[6];
	uint64_t idents[ZND_CHANGES_LEN]; ///< Zone Identifier 0 to 510, each a ZSLBA
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_log_changes) == 4096, "Incorrect size")

/**
 * Zone Descriptor as reported by Zone Management Receive command
 *
 * @see Zoned Namespace Command Set 1.5, Figure 38
 *
 * @struct xnvme_spec_znd_descr
 */
struct xnvme_spec_znd_descr {
	uint8_t ztattr; ///< ZTATTR: Zone Type Attributes, see XNVME_SPEC_ZND_DESCR_ZTATTR_*
	uint8_t zsattr; ///< ZSATTR: Zone State Attributes, see XNVME_SPEC_ZND_DESCR_ZSATTR_*
	uint8_t za;     ///< ZA: Zone Attributes, see XNVME_SPEC_ZND_DESCR_ZA_*
	uint8_t zai;    ///< ZAI: Zone Attributes Information, see XNVME_SPEC_ZND_DESCR_ZAI_*

	uint8_t rsvd4[4];

	uint64_t zcap;  ///< ZCAP: Zone Capacity, in logical blocks
	uint64_t zslba; ///< ZSLBA: Zone Start Logical Block Address
	uint64_t wp;    ///< WP: Write Pointer
	uint8_t rsvd32[32];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_descr) == 64, "Incorrect size")

/** Zone Type, see xnvme_spec_znd_type; bits 3:0 of 'ztattr', ZT (Zoned Namespace Command Set 1.5,
 * Figure 38) */
#define XNVME_SPEC_ZND_DESCR_ZTATTR_ZT 0xf
/** Zone State, see xnvme_spec_znd_state; bits 7:4 of 'zsattr', ZS (Zoned Namespace Command Set
 * 1.5, Figure 38) */
#define XNVME_SPEC_ZND_DESCR_ZSATTR_ZS 0xf0
/** Zone Finished by Controller; bit 0 of 'za', ZFC (Zoned Namespace Command Set 1.5, Figure 38) */
#define XNVME_SPEC_ZND_DESCR_ZA_ZFC 0x1
/** Finish Zone Recommended; bit 1 of 'za', FZR (Zoned Namespace Command Set 1.5, Figure 38) */
#define XNVME_SPEC_ZND_DESCR_ZA_FZR 0x2
/** Reset Zone Recommended; bit 2 of 'za', RZR (Zoned Namespace Command Set 1.5, Figure 38) */
#define XNVME_SPEC_ZND_DESCR_ZA_RZR 0x4
/** Zone Random Write Area Valid; bit 3 of 'za', ZRWAV (Zoned Namespace Command Set 1.5, Figure 38)
 */
#define XNVME_SPEC_ZND_DESCR_ZA_ZRWAV 0x8
/** Zone Descriptor Extension Valid; bit 7 of 'za', ZDEV (Zoned Namespace Command Set 1.5,
 * Figure 38) */
#define XNVME_SPEC_ZND_DESCR_ZA_ZDEV 0x80
/** Finish Zone Recommended Time Limit; bits 1:0 of 'zai', FZRTL (Zoned Namespace Command Set 1.5,
 * Figure 38) */
#define XNVME_SPEC_ZND_DESCR_ZAI_FZRTL 0x3
/** Reset Zone Recommended Time Limit; bits 3:2 of 'zai', RZRTL (Zoned Namespace Command Set 1.5,
 * Figure 38) */
#define XNVME_SPEC_ZND_DESCR_ZAI_RZRTL 0xc

/**
 * Report Zones Data Structure header
 *
 * The first 64 bytes of the two Report Zones data structures
 *
 * @see Zoned Namespace Command Set 1.5, Figure 36 and Figure 37
 * @struct xnvme_spec_znd_report_hdr
 */
struct xnvme_spec_znd_report_hdr {
	/**
	 * NZ: Number of Zones; in the namespace when receiving without a partial report, in the
	 * data buffer when receiving with one
	 */
	uint64_t nz;
	uint8_t rsvd[56];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_report_hdr) == 64, "Incorrect size")

/**
 * Key Value Command Set KV Format data structure
 *
 * @see Key Value Command Set 1.4, Figure 43
 *
 * @struct xnvme_spec_kvs_idfy_ns_format
 */
struct xnvme_spec_kvs_idfy_ns_format {
	uint16_t kvkml; ///< KVKML: KV Key Max Length
	uint8_t rsvd2;
	uint8_t afo; ///< AFO: Additional Format Options, see XNVME_SPEC_KVS_IDFY_NS_FORMAT_AFO_*
	uint32_t kvvml; ///< KVVML: KV Value Max Length
	uint32_t mnks;  ///< MNKS: Max Num Keys
	uint8_t rsvd12[4];
};

/** Relative Performance, 00b best to 11b degraded; bits 1:0 of 'afo', RP (Key Value Command Set
 * 1.4, Figure 43) */
#define XNVME_SPEC_KVS_IDFY_NS_FORMAT_AFO_RP 0x3

/**
 * Key Value Command Set identify namespace data structure
 *
 * @see Key Value Command Set 1.4, Figure 42
 *
 * @struct xnvme_spec_kvs_idfy_ns
 */
struct xnvme_spec_kvs_idfy_ns {
	uint64_t nsze; ///< NSZE: Namespace Size, in bytes
	uint8_t rsvd8[8];
	uint64_t nuse;  ///< NUSE: Namespace Utilization, in bytes
	uint8_t nsfeat; ///< NSFEAT: Namespace Features, see XNVME_SPEC_KVS_IDFY_NS_NSFEAT_*
	uint8_t nkvf;   ///< NKVF: Number of KV Formats
	uint8_t nmic;   ///< NMIC: Namespace Multi-path I/O and Namespace Sharing Capabilities
	uint8_t rescap; ///< RESCAP: Reservation Capabilities
	uint8_t fpi;    ///< FPI: Format Progress Indicator
	uint8_t kvfc;   ///< KVFC: KV Format Capabilities, see XNVME_SPEC_KVS_IDFY_NS_KVFC_*
	uint8_t rsvd30[2];
	uint32_t novg;     ///< NOVG: Namespace Optimal Value Granularity
	uint32_t anagrpid; ///< ANAGRPID: ANA Group Identifier
	uint8_t rsvd40[3];
	uint8_t nsattr;    ///< NSATTR: Namespace Attributes
	uint16_t nvmsetid; ///< NVMSETID: NVM Set Identifier
	uint16_t endgid;   ///< ENDGID: Endurance Group Identifier
	uint64_t nguid[2]; ///< NGUID: Namespace Globally Unique Identifier
	uint64_t eui64;    ///< EUI64: IEEE Extended Unique Identifier
	struct xnvme_spec_kvs_idfy_ns_format kvf[16]; ///< KVF0 to KVF15: KV Format 0 to 15 Support
	uint8_t rsvd328[3512];                        // 3839:328
	uint8_t vs[256];                              ///< VS: Vendor Specific
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_kvs_idfy_ns) == 4096, "Incorrect size")

/** Unique ID Reuse; bit 3 of 'nsfeat', UIDR (Key Value Command Set 1.4, Figure 42) */
#define XNVME_SPEC_KVS_IDFY_NS_NSFEAT_UIDR 0x8
/** KV Format Index; bits 3:0 of 'kvfc', KVFI (Key Value Command Set 1.4, Figure 42) */
#define XNVME_SPEC_KVS_IDFY_NS_KVFC_KVFI 0xf

/**
 * The data returned by an Identify command for the Key Value command set, as the structure of each
 * CNS
 *
 * @union xnvme_spec_kvs_idfy
 */
union xnvme_spec_kvs_idfy {
	union xnvme_spec_idfy base;       ///< As the common Identify data
	struct xnvme_spec_kvs_idfy_ns ns; ///< Of XNVME_SPEC_IDFY_NS_IOCS
};
XNVME_STATIC_ASSERT(sizeof(union xnvme_spec_kvs_idfy) == 4096, "Incorrect size")
