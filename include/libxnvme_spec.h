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
	union {
		struct {
			/* dword 0 */
			uint32_t cdw0; ///< command-specific

			/* dword 1 */
			uint32_t rsvd1;
		};

		uint64_t result; /* Combined result of cdw 0 and cdw 1 */
	};

	/* dword 2 */
	uint16_t sqhd; ///< submission queue head pointer
	uint16_t sqid; ///< submission queue identifier

	/* dword 3 */
	uint16_t cid;    ///< command identifier
	uint16_t status; ///< Status Field and Phase Tag, see XNVME_SPEC_CQE_STATUS_*
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cqe) == 16, "Incorrect size")

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
 * NVMe get-log-page entry for error information
 *
 * NVMe 1.4: Figure 194
 *
 * NOTE: using __attribute__((packed))__ as GCC does not like "uint16_t ct"
 * for some reason. Without the packing it becomes 2 bytes larger
 *
 * @struct xnvme_spec_log_health_entry
 */
struct __attribute__((packed)) xnvme_spec_log_health_entry {
	uint8_t crit_warn;          ///< Critical Warning
	uint16_t comp_temp;         ///< Composite Temperature (Temp.)
	uint8_t avail_spare;        ///< Available Spare (pct)
	uint8_t avail_spare_thresh; ///< Available Spare Threshold (pct)
	uint8_t pct_used;           ///< Percentage used (pct), can exceed 100
	uint8_t eg_crit_warn_sum;   ///< Endurance Group Critical Warning Summary
	uint8_t rsvd8[25];

	uint8_t data_units_read[16];    ///< Data Units Read
	uint8_t data_units_written[16]; ///< Data Units Written

	uint8_t host_read_cmds[16];  ///< Host Read Commands
	uint8_t host_write_cmds[16]; ///< Host Write Commands

	uint8_t ctrlr_busy_time[16];  ///< Controller Busy Time
	uint8_t pwr_cycles[16];       ///< Power Cycles
	uint8_t pwr_on_hours[16];     ///< Power On Hours
	uint8_t unsafe_shutdowns[16]; ///< Unsafe Shutdowns
	uint8_t mdi_errs[16];         ///< Media and Data Integrity Errors
	uint8_t nr_err_logs[16];      ///< Nr. of Error Information Log Entries (life)

	uint32_t warn_comp_temp_time; ///< Warning Composite Temp. Time
	uint32_t crit_comp_temp_time; ///< Critical Composite Temp. Time
	uint16_t temp_sens[8];        ///< Temp. Sensor Temp. 1-8
	uint32_t tmt1tc;              ///< Thermal Management Temp. 1 Trans. Count
	uint32_t tmt2tc;              ///< Thermal Management Temp. 2 Trans. Count
	uint32_t tttmt1;              ///< Total Time for Thermal Mgmt. Temp. 1
	uint32_t tttmt2;              ///< Total Time for Thermal Mgmt. Temp. 2
	uint8_t rsvd[280];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_log_health_entry) == 512, "Incorrect size")

/**
 * NVMe get-log-page entry for sanitize status information
 *
 * @struct xnvme_spec_log_sanitize_entry
 */

struct __attribute__((packed)) xnvme_spec_log_sanitize_entry {
	uint16_t sprog;   ///< Sanitize Progress
	uint16_t sstat;   ///< Sanitize Status, see XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_*
	uint32_t scdw10;  ///< Sanitize Command Dword 10 Information
	uint32_t eto;     ///< Est. Time for Overwrite
	uint32_t etbe;    ///< Est. Time for Block Erase
	uint32_t etce;    ///< Est. Time for Crypto Erase
	uint32_t etodmm;  ///< Est. Time for Overwrite with No-Deallocate Media Modification
	uint32_t etbenmm; ///< Est. Time for Block Erase with No-Deallocate Media Modification
	uint32_t etcenmm; ///< Est. Time for Crypto Erase With No-Deallocate Media Modification
	uint32_t etpvds;  ///< Est. Time For Post-Verification Deallocation State
	uint8_t ssi;      ///< Sanitize State Information
	uint8_t rsvd[475];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_log_sanitize_entry) == 512, "Incorrect size")

/**
 * Sanitize Operation Status, 000b never started, 001b sanitized, 010b sanitizing, 011b failed,
 * 100b sanitized with unexpected deallocate; bits 2:0 of 'sstat'
 */
#define XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_SOS 0x7
/** Overwrite Passes Completed; bits 7:3 of 'sstat' */
#define XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_OPC 0xf8
/** Global Data Erased; bit 8 of 'sstat' */
#define XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_GDE 0x100
/** Media Verification Canceled; bit 9 of 'sstat' */
#define XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_MVCNCLD 0x200

/**
 * NVMe get-log-page entry for error information
 *
 * NVMe 1.4 - Figure ?
 *
 * @struct xnvme_spec_log_erri_entry
 */
struct xnvme_spec_log_erri_entry {
	uint64_t ecnt;
	uint16_t sqid;
	uint16_t cid;
	uint16_t status; ///< Status Field and Phase Tag, see XNVME_SPEC_LOG_ERRI_ENTRY_STATUS_*
	uint16_t eloc;
	uint64_t lba;
	uint32_t nsid;
	uint8_t ven_si;
	uint8_t trtype;
	uint8_t reserved30[2];
	uint64_t cmd_si;
	uint16_t trtype_si;
	uint8_t reserved42[22];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_log_erri_entry) == 64, "Incorrect size")

/** Phase Tag; bit 0 of 'status' */
#define XNVME_SPEC_LOG_ERRI_ENTRY_STATUS_P 0x1
/** Status Code; bits 8:1 of 'status' */
#define XNVME_SPEC_LOG_ERRI_ENTRY_STATUS_SC 0x1fe
/** Status Code Type; bits 11:9 of 'status' */
#define XNVME_SPEC_LOG_ERRI_ENTRY_STATUS_SCT 0xe00
/** Command Retry Delay; bits 13:12 of 'status' */
#define XNVME_SPEC_LOG_ERRI_ENTRY_STATUS_CRD 0x3000
/** More; bit 14 of 'status' */
#define XNVME_SPEC_LOG_ERRI_ENTRY_STATUS_M 0x4000
/** Do Not Retry; bit 15 of 'status' */
#define XNVME_SPEC_LOG_ERRI_ENTRY_STATUS_DNR 0x8000

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
 * @struct xnvme_spec_lbaf
 */
struct xnvme_spec_lbaf {
	uint16_t ms; ///< metadata size
	uint8_t ds;  ///< lba data size
	uint8_t rp;  ///< relative performance, see XNVME_SPEC_LBAF_RP_*
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_lbaf) == 4, "Incorrect size")

/** Relative performance, 00b best to 11b degraded; bits 1:0 of 'rp' */
#define XNVME_SPEC_LBAF_RP_RP 0x3

/**
 * Command Set Identifiers
 *
 * @see Specification Section 5.15.2.1, figure X1
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
 * @struct xnvme_spec_idfy_ns
 */
struct xnvme_spec_idfy_ns {
	uint64_t nsze; ///< namespace size
	uint64_t ncap; ///< namespace capacity
	uint64_t nuse; ///< namespace utilization

	uint8_t nsfeat; ///< Namespace features, see XNVME_SPEC_IDFY_NS_NSFEAT_*

	uint8_t nlbaf; ///< number of lba formats

	uint8_t flbas; ///< Formatted LBA size, see XNVME_SPEC_IDFY_NS_FLBAS_*

	uint8_t mc; ///< Metadata capabilities, see XNVME_SPEC_IDFY_NS_MC_*

	uint8_t dpc; ///< End-to-end data protection capabilities, see XNVME_SPEC_IDFY_NS_DPC_*

	uint8_t dps; ///< End-to-end data protection type settings, see XNVME_SPEC_IDFY_NS_DPS_*

	uint8_t nmic; ///< Namespace multi-path I/O and namespace sharing capabilities, see
		      ///< XNVME_SPEC_IDFY_NS_NMIC_*

	uint8_t nsrescap; ///< Reservation capabilities, see XNVME_SPEC_IDFY_NS_NSRESCAP_*

	uint8_t fpi; ///< Format progress indicator, see XNVME_SPEC_IDFY_NS_FPI_*

	uint8_t dlfeat; ///< Deallocate logical block features, see XNVME_SPEC_IDFY_NS_DLFEAT_*

	uint16_t nawun; ///< namespace atomic write unit normal

	uint16_t nawupf; ///< namespace atomic write unit power fail

	uint16_t nacwu; ///< namespace atomic compare & write unit

	uint16_t nabsn; ///< namespace atomic boundary size normal

	uint16_t nabo; ///< namespace atomic boundary offset

	uint16_t nabspf; ///< namespace atomic boundary size power fail

	uint16_t noiob; ///< namespace optimal I/O boundary in logical blocks

	uint64_t nvmcap[2]; ///< NVM Capacity

	uint16_t npwg; ///< namespace preferred write granularity

	uint16_t npwa; ///< namespace preferred write alignment

	uint16_t npdg; ///< namespace preferred deallocate granularity

	uint16_t npda; ///< namespace preferred deallocate alignment

	uint16_t nows; ///< namespace optimal write size

	uint16_t mssrl; ///< maximum single source range length

	uint32_t mcl; ///< max copy length

	uint8_t msrc; ///< maximum source range count

	uint8_t reserved81[11];

	uint32_t anagrpid; ///< ANA group identifier

	uint8_t reserved96[3];

	uint8_t nsattr; ///< namespace attributes

	uint16_t nvmsetid; ///< NVM set identifier

	uint16_t endgid; ///< Endurance group identifier

	uint8_t nguid[16]; ///< namespace globally unique identifier

	uint64_t eui64; ///< IEEE extended unique identifier

	struct xnvme_spec_lbaf lbaf[64]; ///< LBA format support

	uint8_t vendor_specific[3712];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_idfy_ns) == 4096, "Incorrect size")

/** Thin provisioning is supported; bit 0 of 'nsfeat' */
#define XNVME_SPEC_IDFY_NS_NSFEAT_THIN_PROV 0x1
/** NAWUN, NAWUPF and NACWU are defined; bit 1 of 'nsfeat' */
#define XNVME_SPEC_IDFY_NS_NSFEAT_NS_ATOMIC_WRITE_UNIT 0x2
/** Deallocated or unwritten logical block error; bit 2 of 'nsfeat' */
#define XNVME_SPEC_IDFY_NS_NSFEAT_DEALLOC_OR_UNWRITTEN_ERROR 0x4
/** Non-zero NGUID and EUI64 are never reused; bit 3 of 'nsfeat' */
#define XNVME_SPEC_IDFY_NS_NSFEAT_GUID_NEVER_REUSED 0x8
/** NPWG, NPWA, NPDG, NPDA and NOWS are defined; bit 4 of 'nsfeat' */
#define XNVME_SPEC_IDFY_NS_NSFEAT_OPTIMAL_PERFORMANCE 0x10
/** The least significant bits of the index into 'lbaf'; bits 3:0 of 'flbas' */
#define XNVME_SPEC_IDFY_NS_FLBAS_FORMAT 0xf
/** The metadata is transferred at the end of the data; bit 4 of 'flbas' */
#define XNVME_SPEC_IDFY_NS_FLBAS_EXTENDED 0x10
/** The most significant bits of the index into 'lbaf'; bits 6:5 of 'flbas' */
#define XNVME_SPEC_IDFY_NS_FLBAS_FORMAT_MSB 0x60
/** Metadata can be transferred as part of the data; bit 0 of 'mc' */
#define XNVME_SPEC_IDFY_NS_MC_EXTENDED 0x1
/** Metadata can be transferred with a separate metadata pointer; bit 1 of 'mc' */
#define XNVME_SPEC_IDFY_NS_MC_POINTER 0x2
/** Protection information type 1 is supported; bit 0 of 'dpc' */
#define XNVME_SPEC_IDFY_NS_DPC_PIT1 0x1
/** Protection information type 2 is supported; bit 1 of 'dpc' */
#define XNVME_SPEC_IDFY_NS_DPC_PIT2 0x2
/** Protection information type 3 is supported; bit 2 of 'dpc' */
#define XNVME_SPEC_IDFY_NS_DPC_PIT3 0x4
/** Protection information can be the first eight bytes of the metadata; bit 3 of 'dpc' */
#define XNVME_SPEC_IDFY_NS_DPC_MD_START 0x8
/** Protection information can be the last eight bytes of the metadata; bit 4 of 'dpc' */
#define XNVME_SPEC_IDFY_NS_DPC_MD_END 0x10
/** The protection information type, 0 when disabled; bits 2:0 of 'dps' */
#define XNVME_SPEC_IDFY_NS_DPS_PIT 0x7
/** The protection information is at the start of the metadata, else its end; bit 3 of 'dps' */
#define XNVME_SPEC_IDFY_NS_DPS_MD_START 0x8
/** The namespace may be attached to two or more controllers; bit 0 of 'nmic' */
#define XNVME_SPEC_IDFY_NS_NMIC_CAN_SHARE 0x1
/** Persist through power loss; bit 0 of 'nsrescap' */
#define XNVME_SPEC_IDFY_NS_NSRESCAP_PERSIST 0x1
/** Write exclusive; bit 1 of 'nsrescap' */
#define XNVME_SPEC_IDFY_NS_NSRESCAP_WRITE_EXCLUSIVE 0x2
/** Exclusive access; bit 2 of 'nsrescap' */
#define XNVME_SPEC_IDFY_NS_NSRESCAP_EXCLUSIVE_ACCESS 0x4
/** Write exclusive, registrants only; bit 3 of 'nsrescap' */
#define XNVME_SPEC_IDFY_NS_NSRESCAP_WRITE_EXCLUSIVE_REG_ONLY 0x8
/** Exclusive access, registrants only; bit 4 of 'nsrescap' */
#define XNVME_SPEC_IDFY_NS_NSRESCAP_EXCLUSIVE_ACCESS_REG_ONLY 0x10
/** Write exclusive, all registrants; bit 5 of 'nsrescap' */
#define XNVME_SPEC_IDFY_NS_NSRESCAP_WRITE_EXCLUSIVE_ALL_REG 0x20
/** Exclusive access, all registrants; bit 6 of 'nsrescap' */
#define XNVME_SPEC_IDFY_NS_NSRESCAP_EXCLUSIVE_ACCESS_ALL_REG 0x40
/** Ignore existing key; bit 7 of 'nsrescap' */
#define XNVME_SPEC_IDFY_NS_NSRESCAP_IGNORE_EXISTING_KEY 0x80
/** The percentage of the format remaining; bits 6:0 of 'fpi' */
#define XNVME_SPEC_IDFY_NS_FPI_PERCENTAGE_REMAINING 0x7f
/** The format progress indicator is supported; bit 7 of 'fpi' */
#define XNVME_SPEC_IDFY_NS_FPI_FPI_SUPPORTED 0x80
/** Read from a deallocated block: 000b not reported, 001b all bytes 0x00, 010b all bytes 0xFF;
 * bits 2:0 of 'dlfeat' */
#define XNVME_SPEC_IDFY_NS_DLFEAT_READ_VALUE 0x7
/** The Deallocate bit of Write Zeroes is supported; bit 3 of 'dlfeat' */
#define XNVME_SPEC_IDFY_NS_DLFEAT_WRITE_ZERO_DEALLOCATE 0x8
/** The guard of a deallocated block is the CRC of the value read, else 0xFFFF; bit 4 of 'dlfeat'
 */
#define XNVME_SPEC_IDFY_NS_DLFEAT_GUARD_VALUE 0x10

/**
 * Protection information of a logical block, in the layout of the 16b Guard format, 'g16', or
 * of the 64b Guard format, 'g64'
 *
 * @struct xnvme_pif
 */
struct xnvme_pif {
	union {
		struct {
			uint16_t guard;
			uint16_t app_tag;
			uint32_t stor_ref_space;
		} g16;
		struct {
			uint64_t guard;
			uint16_t app_tag;
			uint16_t stor_ref_space_p1;
			uint32_t stor_ref_space_p2;
		} g64;
	};
};
XNVME_STATIC_ASSERT(XNVME_SIZEOF_MEMBER(struct xnvme_pif, g16) == 8, "Incorrect size");
XNVME_STATIC_ASSERT(XNVME_SIZEOF_MEMBER(struct xnvme_pif, g64) == 16, "Incorrect size");

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
};

/**
 * Extended LBA Format
 *
 * @struct xnvme_spec_elbaf
 */
struct xnvme_spec_elbaf {
	uint32_t bits; ///< Extended LBA format, see XNVME_SPEC_ELBAF_BITS_*
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_elbaf) == 4, "Incorrect size")

/** Storage tag size; bits 6:0 of 'bits' */
#define XNVME_SPEC_ELBAF_BITS_STS 0x7f
/** Protection information format, see xnvme_spec_nvm_ns_pif; bits 8:7 of 'bits' */
#define XNVME_SPEC_ELBAF_BITS_PIF 0x180

/** Size, in bytes, of the Serial Number (SN) of the Identify Controller data structure */
#define XNVME_SPEC_CTRLR_SN_LEN 20
/** Size, in bytes, of the Model Number (MN) of the Identify Controller data structure */
#define XNVME_SPEC_CTRLR_MN_LEN 40
/** Size, in bytes, of the Firmware Revision (FR) of the Identify Controller data structure */
#define XNVME_SPEC_CTRLR_FR_LEN 8

/**
 * Power State Descriptor
 *
 * @struct xnvme_spec_power_state
 */
struct xnvme_spec_power_state {
	uint16_t mp; ///< maximum power, bits 15:00

	uint8_t reserved1;

	uint8_t mxps_nops; ///< bits 31:24, see XNVME_SPEC_POWER_STATE_MXPS_NOPS_*

	uint32_t enlat; ///< entry latency in microseconds, bits 63:32
	uint32_t exlat; ///< exit latency in microseconds, bits 95:64

	uint8_t rrt; ///< relative read throughput, see XNVME_SPEC_POWER_STATE_RRT_*
	uint8_t rrl; ///< relative read latency, see XNVME_SPEC_POWER_STATE_RRL_*
	uint8_t rwt; ///< relative write throughput, see XNVME_SPEC_POWER_STATE_RWT_*
	uint8_t rwl; ///< relative write latency, see XNVME_SPEC_POWER_STATE_RWL_*

	uint16_t idlp; ///< idle power, bits 143:128

	uint8_t ips; ///< idle power scale, see XNVME_SPEC_POWER_STATE_IPS_*

	uint8_t reserved8;

	uint16_t actp; ///< active power, bits 175:160

	uint8_t apw_aps; ///< bits 183:176, see XNVME_SPEC_POWER_STATE_APW_APS_*

	uint8_t reserved10[9];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_power_state) == 32, "Incorrect size")

/** Maximum power scale; bit 0 of 'mxps_nops' */
#define XNVME_SPEC_POWER_STATE_MXPS_NOPS_MXPS 0x1
/** Non-operational state; bit 1 of 'mxps_nops' */
#define XNVME_SPEC_POWER_STATE_MXPS_NOPS_NOPS 0x2
/** Relative read throughput; bits 4:0 of 'rrt' */
#define XNVME_SPEC_POWER_STATE_RRT_RRT 0x1f
/** Relative read latency; bits 4:0 of 'rrl' */
#define XNVME_SPEC_POWER_STATE_RRL_RRL 0x1f
/** Relative write throughput; bits 4:0 of 'rwt' */
#define XNVME_SPEC_POWER_STATE_RWT_RWT 0x1f
/** Relative write latency; bits 4:0 of 'rwl' */
#define XNVME_SPEC_POWER_STATE_RWL_RWL 0x1f
/** Idle power scale; bits 7:6 of 'ips' */
#define XNVME_SPEC_POWER_STATE_IPS_IPS 0xc0
/** Active power workload; bits 2:0 of 'apw_aps' */
#define XNVME_SPEC_POWER_STATE_APW_APS_APW 0x7
/** Active power scale; bits 7:6 of 'apw_aps' */
#define XNVME_SPEC_POWER_STATE_APW_APS_APS 0xc0

/**
 * NVMe over Fabrics attributes of the Identify Controller data, bytes 1792-2047
 *
 * @struct xnvme_spec_idfy_ctrlr_nvmf
 */
struct xnvme_spec_idfy_ctrlr_nvmf {
	uint32_t ioccsz; ///< I/O queue command capsule supported size (16-byte units)

	uint32_t iorcsz; ///< I/O queue response capsule supported size (16-byte units)

	uint16_t icdoff; ///< In-capsule data offset (16-byte units)

	uint8_t ctrattr; ///< Controller attributes, see XNVME_SPEC_IDFY_CTRLR_NVMF_CTRATTR_*

	uint8_t msdbd; ///< Maximum SGL block descriptors (0 = no limit)

	uint16_t ofcs; ///< Optional Fabric Commands supported, see
		       ///< XNVME_SPEC_IDFY_CTRLR_NVMF_OFCS_*

	uint8_t reserved[242];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_idfy_ctrlr_nvmf) == 256, "Incorrect size")

/** The controller model, 1 static, 0 dynamic; bit 0 of 'ctrattr' */
#define XNVME_SPEC_IDFY_CTRLR_NVMF_CTRATTR_CTRLR_MODEL 0x1
/** The Disconnect command is supported; bit 0 of 'ofcs' */
#define XNVME_SPEC_IDFY_CTRLR_NVMF_OFCS_DISC_DEL 0x1

/**
 * Representation of NVMe completion result Identify Controller
 *
 * A field of bits is an integer of the width the specification gives it; its documentation
 * names the bits.
 *
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

	uint32_t rtd3r; ///< RTD3 Resume Latency
	uint32_t rtd3e; ///< RTD3 Resume Latency

	uint32_t
		oaes; ///< Optional asynchronous events supported, see XNVME_SPEC_IDFY_CTRLR_OAES_*

	uint32_t ctratt; ///< Controller attributes, see XNVME_SPEC_IDFY_CTRLR_CTRATT_*

	uint16_t rrls; ///< Read Recovery Levels Supported

	uint8_t reserved_102[9];

	uint8_t cntrltype; ///< Controller Type

	uint8_t fguid[16]; ///< FRU Globally Unique Ident.

	uint16_t crdt1; ///< Command Retry Delay Time1

	uint16_t crdt2; ///< Command Retry Delay Time2

	uint16_t crdt3; ///< Command Retry Delay Time3

	uint8_t reserved_134[119];

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

	uint16_t domain_identifier; ///< Domain Identifier

	uint8_t reserved_358[10];

	uint64_t megcap[2]; ///< Max Endurance Group Capacity

	uint8_t reserved_384[128];

	/* bytes 512-703: nvm command set attributes */

	uint8_t sqes; ///< Submission queue entry size, see XNVME_SPEC_IDFY_CTRLR_SQES_*

	uint8_t cqes; ///< Completion queue entry size, see XNVME_SPEC_IDFY_CTRLR_CQES_*

	uint16_t maxcmd;

	uint32_t nn; ///< Number of Namespaces

	uint16_t oncs; ///< Optional NVM command support, see XNVME_SPEC_IDFY_CTRLR_ONCS_*

	uint16_t fuses; ///< Fused Operation Support

	uint8_t fna; ///< Format NVM attributes, see XNVME_SPEC_IDFY_CTRLR_FNA_*

	uint8_t vwc; ///< Volatile write cache, see XNVME_SPEC_IDFY_CTRLR_VWC_*

	uint16_t awun;  ///< Atomic Write Unit Normal
	uint16_t awupf; ///< Atomic Write Unit Power Fail
	uint8_t nvscc;  ///< NVM vendor specific command configuration

	uint8_t nwpc; ///< Namespace write protection capabilities

	uint16_t acwu; ///< atomic compare & write unit

	uint16_t cdfs; ///< Copy descriptor formats supported, see XNVME_SPEC_IDFY_CTRLR_CDFS_*

	uint32_t sgls; ///< SGL support, see XNVME_SPEC_IDFY_CTRLR_SGLS_*

	uint32_t mnan; ///< Maximum Number of Allowed Namespaces

	uint64_t maxdna[2]; ///< Maximum Domain Namespace Attachments
	uint32_t maxcna;    ///< Maximum I/O Controllers Namespace Attachments

	uint8_t reserved_564[204];

	uint8_t subnqn[256]; ///< NVM Subsystems NVMe Qualified Name

	uint8_t reserved_1024[768];

	struct xnvme_spec_idfy_ctrlr_nvmf nvmf_specific; ///< NVMe over Fabrics-specific fields

	/* bytes 2048-3071: power state descriptors */
	struct xnvme_spec_power_state psd[32];

	/* bytes 3072-4095: vendor specific */
	uint8_t vs[1024];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_idfy_ctrlr) == 4096, "Incorrect size")

/** Multiple ports; bit 0 of 'cmic' */
#define XNVME_SPEC_IDFY_CTRLR_CMIC_MULTI_PORT 0x1
/** Multiple hosts; bit 1 of 'cmic' */
#define XNVME_SPEC_IDFY_CTRLR_CMIC_MULTI_HOST 0x2
/** SR-IOV; bit 2 of 'cmic' */
#define XNVME_SPEC_IDFY_CTRLR_CMIC_SR_IOV 0x4
/** Asymmetric namespace access reporting; bit 3 of 'cmic' */
#define XNVME_SPEC_IDFY_CTRLR_CMIC_ANA_RPRT 0x8
/** Tertiary version; bits 7:0 of 'ver' */
#define XNVME_SPEC_IDFY_CTRLR_VER_TER 0xff
/** Minor version; bits 15:8 of 'ver' */
#define XNVME_SPEC_IDFY_CTRLR_VER_MNR 0xff00
/** Major version; bits 31:16 of 'ver' */
#define XNVME_SPEC_IDFY_CTRLR_VER_MJR 0xffff0000
/** Namespace Attribute Notices; bit 8 of 'oaes' */
#define XNVME_SPEC_IDFY_CTRLR_OAES_NS_ATTRIBUTE_NOTICES 0x100
/** Firmware Activation Notices; bit 9 of 'oaes' */
#define XNVME_SPEC_IDFY_CTRLR_OAES_FW_ACTIVATION_NOTICES 0x200
/** Asymmetric Namespace Access Change Notices; bit 11 of 'oaes' */
#define XNVME_SPEC_IDFY_CTRLR_OAES_ANA_NOTICES 0x800
/** Predictable Latency Event Aggregate Log Change Notices; bit 12 of 'oaes' */
#define XNVME_SPEC_IDFY_CTRLR_OAES_PLEAL_NOTICES 0x1000
/** LBA Status Information Alert Notices; bit 13 of 'oaes' */
#define XNVME_SPEC_IDFY_CTRLR_OAES_LBA_SIA_NOTICES 0x2000
/** Endurance Group Event Aggregate Log Change Notices; bit 14 of 'oaes' */
#define XNVME_SPEC_IDFY_CTRLR_OAES_EGEA_NOTICES 0x4000
/** Normal NVM Subsystem Shutdown event; bit 15 of 'oaes' */
#define XNVME_SPEC_IDFY_CTRLR_OAES_NORMAL_NVM_SS 0x8000
/** Zone Descriptor Changed Notices, and the Changed Zone List log page; bit 27 of 'oaes' */
#define XNVME_SPEC_IDFY_CTRLR_OAES_ZONE_CHANGES 0x8000000
/** Discovery Log Page Change Notices; bit 31 of 'oaes' */
#define XNVME_SPEC_IDFY_CTRLR_OAES_DISCOVERY_LOG_NOTICES 0x80000000
/** 128-bit host identifier; bit 0 of 'ctratt' */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_HOST_ID_EXHID_SUPPORTED 0x1
/** Non-operational power state permissive mode; bit 1 of 'ctratt' */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_NON_OPERATIONAL_POWER_STATE_PERMISSIVE_MODE 0x2
/** NVM sets; bit 2 of 'ctratt' */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_NVM_SETS 0x4
/** Read recovery levels; bit 3 of 'ctratt' */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_READ_RECOVERY_LEVELS 0x8
/** Endurance groups; bit 4 of 'ctratt' */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_ENDURANCE_GROUPS 0x10
/** Predictable latency mode; bit 5 of 'ctratt' */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_PREDICTABLE_LATENCY_MODE 0x20
/** Traffic based keep alive; bit 6 of 'ctratt' */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_TBKAS 0x40
/** Namespace granularity; bit 7 of 'ctratt' */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_NAMESPACE_GRANULARITY 0x80
/** SQ associations; bit 8 of 'ctratt' */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_SQ_ASSOCIATIONS 0x100
/** UUID list; bit 9 of 'ctratt' */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_UUID_LIST 0x200
/** Multi-domain subsystem; bit 10 of 'ctratt' */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_MULTI_DOMAIN_SUBSYSTEM 0x400
/** Fixed capacity management; bit 11 of 'ctratt' */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_FIXED_CAPACITY_MANAGEMENT 0x800
/** Variable capacity management; bit 12 of 'ctratt' */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_VARIABLE_CAPACITY_MANAGEMENT 0x1000
/** Delete endurance group; bit 13 of 'ctratt' */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_DELETE_ENDURANCE_GROUP 0x2000
/** Delete NVM set; bit 14 of 'ctratt' */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_DELETE_NVM_SET 0x4000
/** Extended LBA formats; bit 15 of 'ctratt' */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_EXTENDED_LBA_FORMATS 0x8000
/** Flexible data placement; bit 19 of 'ctratt' */
#define XNVME_SPEC_IDFY_CTRLR_CTRATT_FLEXIBLE_DATA_PLACEMENT 0x80000
/** NVMe storage device; bit 0 of 'nvmsr' */
#define XNVME_SPEC_IDFY_CTRLR_NVMSR_NVMESD 0x1
/** NVMe enclosure; bit 1 of 'nvmsr' */
#define XNVME_SPEC_IDFY_CTRLR_NVMSR_NVMEE 0x2
/** VPD write cycles remaining; bits 6:0 of 'vwci' */
#define XNVME_SPEC_IDFY_CTRLR_VWCI_VWCR 0x7f
/** The VPD write cycles remaining are valid; bit 7 of 'vwci' */
#define XNVME_SPEC_IDFY_CTRLR_VWCI_VWCRV 0x80
/** SMBus/I2C port management endpoint; bit 0 of 'mec' */
#define XNVME_SPEC_IDFY_CTRLR_MEC_SMBUSME 0x1
/** PCIe port management endpoint; bit 1 of 'mec' */
#define XNVME_SPEC_IDFY_CTRLR_MEC_PCIEME 0x2
/** Security send and receive; bit 0 of 'oacs' */
#define XNVME_SPEC_IDFY_CTRLR_OACS_SECURITY 0x1
/** Format NVM; bit 1 of 'oacs' */
#define XNVME_SPEC_IDFY_CTRLR_OACS_FORMAT 0x2
/** Firmware commit and image download; bit 2 of 'oacs' */
#define XNVME_SPEC_IDFY_CTRLR_OACS_FIRMWARE 0x4
/** Namespace management and attachment; bit 3 of 'oacs' */
#define XNVME_SPEC_IDFY_CTRLR_OACS_NS_MANAGE 0x8
/** Device self-test; bit 4 of 'oacs' */
#define XNVME_SPEC_IDFY_CTRLR_OACS_DEVICE_SELF_TEST 0x10
/** Directives; bit 5 of 'oacs' */
#define XNVME_SPEC_IDFY_CTRLR_OACS_DIRECTIVES 0x20
/** NVMe-MI send and receive; bit 6 of 'oacs' */
#define XNVME_SPEC_IDFY_CTRLR_OACS_NVME_MI 0x40
/** Virtualization management; bit 7 of 'oacs' */
#define XNVME_SPEC_IDFY_CTRLR_OACS_VIRTUALIZATION_MANAGEMENT 0x80
/** Doorbell buffer config; bit 8 of 'oacs' */
#define XNVME_SPEC_IDFY_CTRLR_OACS_DOORBELL_BUFFER_CONFIG 0x100
/** Get LBA status; bit 9 of 'oacs' */
#define XNVME_SPEC_IDFY_CTRLR_OACS_LBA_STATUS 0x200
/** Command and feature lockdown; bit 10 of 'oacs' */
#define XNVME_SPEC_IDFY_CTRLR_OACS_CMD_FEATURE_LOCKDOWN 0x400
/** The first firmware slot is read-only; bit 0 of 'frmw' */
#define XNVME_SPEC_IDFY_CTRLR_FRMW_SLOT1_RO 0x1
/** The number of firmware slots; bits 3:1 of 'frmw' */
#define XNVME_SPEC_IDFY_CTRLR_FRMW_NUM_SLOTS 0xe
/** Activation without reset; bit 4 of 'frmw' */
#define XNVME_SPEC_IDFY_CTRLR_FRMW_ACTIVATION_WITHOUT_RESET 0x10
/** Multiple update detection; bit 5 of 'frmw' */
#define XNVME_SPEC_IDFY_CTRLR_FRMW_MUL_UPDATE_DETECTION 0x20
/** Per namespace SMART / health log page; bit 0 of 'lpa' */
#define XNVME_SPEC_IDFY_CTRLR_LPA_NS_SMART 0x1
/** Commands supported and effects log page; bit 1 of 'lpa' */
#define XNVME_SPEC_IDFY_CTRLR_LPA_CELP 0x2
/** Extended data for get log page; bit 2 of 'lpa' */
#define XNVME_SPEC_IDFY_CTRLR_LPA_EDLP 0x4
/** Telemetry log pages and notices; bit 3 of 'lpa' */
#define XNVME_SPEC_IDFY_CTRLR_LPA_TELEMETRY 0x8
/** Persistent event log; bit 4 of 'lpa' */
#define XNVME_SPEC_IDFY_CTRLR_LPA_PEL 0x10
/** Miscellaneous log; bit 5 of 'lpa' */
#define XNVME_SPEC_IDFY_CTRLR_LPA_MEL 0x20
/** Telemetry data area 4; bit 6 of 'lpa' */
#define XNVME_SPEC_IDFY_CTRLR_LPA_TEL_DA4 0x40
/** Admin vendor specific commands use the format of the specification; bit 0 of 'avscc' */
#define XNVME_SPEC_IDFY_CTRLR_AVSCC_SPEC_FORMAT 0x1
/** Autonomous power state transitions are supported; bit 0 of 'apsta' */
#define XNVME_SPEC_IDFY_CTRLR_APSTA_SUPPORTED 0x1
/** Number of RPMB units; bits 2:0 of 'rpmbs' */
#define XNVME_SPEC_IDFY_CTRLR_RPMBS_NUM_RPMB_UNITS 0x7
/** Authentication method; bits 5:3 of 'rpmbs' */
#define XNVME_SPEC_IDFY_CTRLR_RPMBS_AUTH_METHOD 0x38
/** Total size; bits 23:16 of 'rpmbs' */
#define XNVME_SPEC_IDFY_CTRLR_RPMBS_TOTAL_SIZE 0xff0000
/** Access size; bits 31:24 of 'rpmbs' */
#define XNVME_SPEC_IDFY_CTRLR_RPMBS_ACCESS_SIZE 0xff000000
/** Only one device self-test operation at a time; bit 0 of 'dsto' */
#define XNVME_SPEC_IDFY_CTRLR_DSTO_ONE_ONLY 0x1
/** Host controlled thermal management is supported; bit 0 of 'hctma' */
#define XNVME_SPEC_IDFY_CTRLR_HCTMA_SUPPORTED 0x1
/** Crypto erase; bit 0 of 'sanicap' */
#define XNVME_SPEC_IDFY_CTRLR_SANICAP_CRYPTO_ERASE 0x1
/** Block erase; bit 1 of 'sanicap' */
#define XNVME_SPEC_IDFY_CTRLR_SANICAP_BLOCK_ERASE 0x2
/** Overwrite; bit 2 of 'sanicap' */
#define XNVME_SPEC_IDFY_CTRLR_SANICAP_OVERWRITE 0x4
/** No-deallocate inhibited; bit 29 of 'sanicap' */
#define XNVME_SPEC_IDFY_CTRLR_SANICAP_NDI 0x20000000
/** No-deallocate modifies media after sanitize; bits 31:30 of 'sanicap' */
#define XNVME_SPEC_IDFY_CTRLR_SANICAP_NODMMAS 0xc0000000
/** ANA optimized state; bit 0 of 'anacap' */
#define XNVME_SPEC_IDFY_CTRLR_ANACAP_OPTIMIZE 0x1
/** ANA non-optimized state; bit 1 of 'anacap' */
#define XNVME_SPEC_IDFY_CTRLR_ANACAP_NON_OPTIMIZE 0x2
/** ANA inaccessible state; bit 2 of 'anacap' */
#define XNVME_SPEC_IDFY_CTRLR_ANACAP_INACCESSIBLE 0x4
/** ANA persistent loss state; bit 3 of 'anacap' */
#define XNVME_SPEC_IDFY_CTRLR_ANACAP_PERSIST_LOSS 0x8
/** ANA change state; bit 4 of 'anacap' */
#define XNVME_SPEC_IDFY_CTRLR_ANACAP_CHANGE 0x10
/** The ANAGRPID does not change while the namespace is attached; bit 6 of 'anacap' */
#define XNVME_SPEC_IDFY_CTRLR_ANACAP_NS_ANAGRPID 0x40
/** A non-zero ANAGRPID is supported by namespace management; bit 7 of 'anacap' */
#define XNVME_SPEC_IDFY_CTRLR_ANACAP_MGT_ANAGRPID 0x80
/** The minimum submission queue entry size, as a power of two; bits 3:0 of 'sqes' */
#define XNVME_SPEC_IDFY_CTRLR_SQES_MIN 0xf
/** The maximum submission queue entry size, as a power of two; bits 7:4 of 'sqes' */
#define XNVME_SPEC_IDFY_CTRLR_SQES_MAX 0xf0
/** The minimum completion queue entry size, as a power of two; bits 3:0 of 'cqes' */
#define XNVME_SPEC_IDFY_CTRLR_CQES_MIN 0xf
/** The maximum completion queue entry size, as a power of two; bits 7:4 of 'cqes' */
#define XNVME_SPEC_IDFY_CTRLR_CQES_MAX 0xf0
/** Compare; bit 0 of 'oncs' */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_COMPARE 0x1
/** Write uncorrectable; bit 1 of 'oncs' */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_WRITE_UNC 0x2
/** Dataset management; bit 2 of 'oncs' */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_DSM 0x4
/** Write zeroes; bit 3 of 'oncs' */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_WRITE_ZEROES 0x8
/** The save field of set features; bit 4 of 'oncs' */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_SET_FEATURES_SAVE 0x10
/** Reservations; bit 5 of 'oncs' */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_RESERVATIONS 0x20
/** Timestamp; bit 6 of 'oncs' */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_TIMESTAMP 0x40
/** Verify; bit 7 of 'oncs' */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_VERIFY 0x80
/** Copy; bit 8 of 'oncs' */
#define XNVME_SPEC_IDFY_CTRLR_ONCS_COPY 0x100
/** A format applies to all namespaces; bit 0 of 'fna' */
#define XNVME_SPEC_IDFY_CTRLR_FNA_FORMAT_ALL_NS 0x1
/** A secure erase applies to all namespaces; bit 1 of 'fna' */
#define XNVME_SPEC_IDFY_CTRLR_FNA_ERASE_ALL_NS 0x2
/** Cryptographic erase is supported; bit 2 of 'fna' */
#define XNVME_SPEC_IDFY_CTRLR_FNA_CRYPTO_ERASE_SUPPORTED 0x4
/** A format of NSID FFFFFFFFh is supported; bit 3 of 'fna' */
#define XNVME_SPEC_IDFY_CTRLR_FNA_NSID_FFFFFFFF 0x8
/** A volatile write cache is present; bit 0 of 'vwc' */
#define XNVME_SPEC_IDFY_CTRLR_VWC_PRESENT 0x1
/** Flush with NSID FFFFFFFFh; bits 2:1 of 'vwc' */
#define XNVME_SPEC_IDFY_CTRLR_VWC_FLUSH_BROADCAST 0x6
/** Copy descriptor format 0; bit 0 of 'cdfs' */
#define XNVME_SPEC_IDFY_CTRLR_CDFS_FORMAT0 0x1
/** Copy descriptor format 1; bit 1 of 'cdfs' */
#define XNVME_SPEC_IDFY_CTRLR_CDFS_FORMAT1 0x2
/** SGLs are supported, and their alignment; bits 1:0 of 'sgls' */
#define XNVME_SPEC_IDFY_CTRLR_SGLS_SUPPORTED 0x3
/** Keyed SGL data block descriptor; bit 2 of 'sgls' */
#define XNVME_SPEC_IDFY_CTRLR_SGLS_KEYED_SGL 0x4
/** SGL descriptor threshold; bits 15:8 of 'sgls' */
#define XNVME_SPEC_IDFY_CTRLR_SGLS_SGL_DESC_THRESHOLD 0xff00
/** SGL bit bucket descriptor; bit 16 of 'sgls' */
#define XNVME_SPEC_IDFY_CTRLR_SGLS_BIT_BUCKET_DESCRIPTOR 0x10000
/** Byte aligned contiguous metadata pointer; bit 17 of 'sgls' */
#define XNVME_SPEC_IDFY_CTRLR_SGLS_METADATA_POINTER 0x20000
/** SGL data longer than the data; bit 18 of 'sgls' */
#define XNVME_SPEC_IDFY_CTRLR_SGLS_OVERSIZED_SGL 0x40000
/** MPTR as an address of an SGL segment; bit 19 of 'sgls' */
#define XNVME_SPEC_IDFY_CTRLR_SGLS_METADATA_ADDRESS 0x80000
/** Address as an offset; bit 20 of 'sgls' */
#define XNVME_SPEC_IDFY_CTRLR_SGLS_SGL_OFFSET 0x100000
/** Transport SGL data block descriptor; bit 21 of 'sgls' */
#define XNVME_SPEC_IDFY_CTRLR_SGLS_TRANSPORT_SGL 0x200000

/**
 * Representation of I/O Command Set Vector
 *
 * See NVMe spec tbd, section xyz, for details
 *
 * @struct xnvme_spec_cs_vector
 */
struct xnvme_spec_cs_vector {
	uint64_t bits; ///< The command sets of the combination, see XNVME_SPEC_CS_VECTOR_BITS_*
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cs_vector) == 8, "Incorrect size")

/** The NVM command set; bit 0 of 'bits' */
#define XNVME_SPEC_CS_VECTOR_BITS_NVM 0x1
/** The Key Value command set; bit 1 of 'bits' */
#define XNVME_SPEC_CS_VECTOR_BITS_KV 0x2
/** The Zoned Namespace command set; bit 2 of 'bits' */
#define XNVME_SPEC_CS_VECTOR_BITS_ZNS 0x4

/** Number of I/O Command Set Combinations in the I/O Command Set data structure */
#define XNVME_SPEC_IDFY_CS_IOCSC_LEN 512

/**
 * Representation of I/O Command Set data structure
 *
 * See NVMe spec tbd, section xyz, for details
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
 * @struct xnvme_spec_idfy_dir_rp
 */
struct xnvme_spec_idfy_dir_rp {
	/**
	 * Directives Supported, byte 0 of bytes 31:0, see
	 * XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_SUPPORTED_*
	 */
	uint8_t directives_supported;
	uint8_t rsvd1[31];

	/**
	 * Directives Enabled, byte 32 of bytes 63:32, see
	 * XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_ENABLED_*
	 */
	uint8_t directives_enabled;
	uint8_t rsvd2[31];

	/**
	 * Directives Persistent Across Controller Level Resets, byte 64 of bytes 95:64, see
	 * XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_PERSISTENCE_*
	 */
	uint8_t directives_persistence;
	uint8_t rsvd3[31];

	uint8_t rsvd4[4000];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_idfy_dir_rp) == 4096, "Incorrect size")

/** Identify Directive supported; bit 0 of 'directives_supported' */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_SUPPORTED_IDENTIFY 0x1
/** Streams Directive supported; bit 1 of 'directives_supported' */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_SUPPORTED_STREAMS 0x2
/** Data Placement Directive supported; bit 2 of 'directives_supported' */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_SUPPORTED_DATA_PLACEMENT 0x4
/** Identify Directive enabled; bit 0 of 'directives_enabled' */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_ENABLED_IDENTIFY 0x1
/** Streams Directive enabled; bit 1 of 'directives_enabled' */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_ENABLED_STREAMS 0x2
/** Data Placement Directive enabled; bit 2 of 'directives_enabled' */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_ENABLED_DATA_PLACEMENT 0x4
/** Identify Directive persistent; bit 0 of 'directives_persistence' */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_PERSISTENCE_IDENTIFY 0x1
/** Streams Directive persistent; bit 1 of 'directives_persistence' */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_PERSISTENCE_STREAMS 0x2
/** Data Placement Directive persistent; bit 2 of 'directives_persistence' */
#define XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_PERSISTENCE_DATA_PLACEMENT 0x4

/**
 * This structure holds the return parameters data structure for streams directive
 *
 * @struct xnvme_spec_streams_dir_rp
 */
struct xnvme_spec_streams_dir_rp {
	uint16_t msl;  ///< Max streams limit
	uint16_t nssa; ///< NVM subsystem streams available
	uint16_t nsso; ///< NVM subsystems streams open

	uint8_t nssc; ///< NVM Subsystem Streams Capability, see XNVME_SPEC_STREAMS_DIR_RP_NSSC_*

	uint8_t reserved1[9];

	uint32_t sws; ///< Stream write size
	uint16_t sgs; ///< Stream granularity size

	uint16_t nsa; ///< Namespace streams allocated
	uint16_t nso; ///< Namespace streams open

	uint8_t reserved2[6];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_streams_dir_rp) == 32, "Incorrect size")

/** Stream identifiers shared by hosts of different non-zero Host Identifiers; bit 0 of 'nssc' */
#define XNVME_SPEC_STREAMS_DIR_RP_NSSC_MULTI_HOST 0x1

/**
 * This structure holds the get status data structure for streams directive
 *
 * @struct xnvme_spec_streams_dir_gs
 */
struct xnvme_spec_streams_dir_gs {
	uint16_t open_sc; ///< Open stream count
	uint16_t sid[];   ///< Stream identifier
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
 * @union xnvme_spec_feat
 */
union xnvme_spec_feat {
	uint32_t temp_threshold; ///< Temperature Threshold, see XNVME_SPEC_FEAT_TEMP_THRESHOLD_*
	uint32_t error_recovery; ///< Error Recovery, see XNVME_SPEC_FEAT_ERROR_RECOVERY_*
	struct xnvme_spec_feat_nqueues nqueues; ///< Number of Queues
	uint32_t fdp_mode;       ///< Flexible Data Placement, see XNVME_SPEC_FEAT_FDP_MODE_*
	uint32_t media_feedback; ///< Media Feedback, see XNVME_SPEC_FEAT_MEDIA_FEEDBACK_*
	uint32_t val;            ///< The dword whole, for a feature without a view
};
XNVME_STATIC_ASSERT(sizeof(union xnvme_spec_feat) == 4, "Incorrect size")

/** Temperature Threshold, in Kelvin; bits 15:0 of 'temp_threshold' */
#define XNVME_SPEC_FEAT_TEMP_THRESHOLD_TMPTH 0xffff
/** Threshold Temperature Select; bits 19:16 of 'temp_threshold' */
#define XNVME_SPEC_FEAT_TEMP_THRESHOLD_TMPSEL 0xf0000
/** Threshold Type Select; bits 21:20 of 'temp_threshold' */
#define XNVME_SPEC_FEAT_TEMP_THRESHOLD_THSEL 0x300000

/** Time Limited Error Recovery, in 100 ms units; bits 15:0 of 'error_recovery' */
#define XNVME_SPEC_FEAT_ERROR_RECOVERY_TLER 0xffff
/** Deallocated or Unwritten Logical Block Error Enable; bit 16 of 'error_recovery' */
#define XNVME_SPEC_FEAT_ERROR_RECOVERY_DULBE 0x10000

/** Flexible Data Placement Enable; bit 0 of 'fdp_mode' */
#define XNVME_SPEC_FEAT_FDP_MODE_FDPE 0x1
/** Flexible Data Placement Configuration Index; bits 15:8 of 'fdp_mode' */
#define XNVME_SPEC_FEAT_FDP_MODE_FDPCI 0xff00

/** High ECC status; bit 0 of 'media_feedback' */
#define XNVME_SPEC_FEAT_MEDIA_FEEDBACK_HECC 0x1
/** Very High ECC status; bit 1 of 'media_feedback' */
#define XNVME_SPEC_FEAT_MEDIA_FEEDBACK_VHECC 0x2

/**
 * Representation of DSM ranges as defined in NVM Express 1.3 Figure 207 Dataset
 * Management Range Definition Figure 207
 *
 * @struct xnvme_spec_dsm_range
 */
struct xnvme_spec_dsm_range {
	uint32_t cattr; ///< Context attributes
	uint32_t llb;   ///< Length in logical blocks (this is a 1-based value)
	uint64_t slba;  ///< Starting LBA
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_dsm_range) == 16, "Incorrect size")

/**
 * Enumeration of NVMe flags
 *
 * TODO: document
 * @enum xnvme_spec_flag
 */
enum xnvme_spec_flag {
	// Limited Retry (LR)
	XNVME_SPEC_FLAG_LIMITED_RETRY = 0x1 << 15,

	// Force Unit Access (FUA)
	XNVME_SPEC_FLAG_FORCE_UNIT_ACCESS = 0x1 << 14,

	// Protection Information Check (PRCK)
	XNVME_SPEC_FLAG_PRINFO_PRCHK_REF   = 0x1 << 10,
	XNVME_SPEC_FLAG_PRINFO_PRCHK_APP   = 0x1 << 11,
	XNVME_SPEC_FLAG_PRINFO_PRCHK_GUARD = 0x1 << 12,

	// Protection Information Action (PRACT)
	XNVME_SPEC_FLAG_PRINFO_PRACT = 0x1 << 13,
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
 * SGL descriptor
 *
 * @struct xnvme_spec_sgl_descriptor
 */
struct xnvme_spec_sgl_descriptor {
	uint64_t addr; ///< common field

	union {
		struct {
			uint64_t rsvd    : 56;
			uint64_t subtype : 4; ///< SGL subtype
			uint64_t type    : 4; ///< SGL type
		} generic;

		struct {
			uint64_t len     : 32; ///< Length of entry
			uint64_t rsvd    : 24;
			uint64_t subtype : 4; ///< SGL subtype
			uint64_t type    : 4; ///< SGL type
		} unkeyed;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_sgl_descriptor) == 16, "Incorrect size")

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
 * @union xnvme_spec_dptr
 */
union xnvme_spec_dptr {
	struct xnvme_spec_dptr_prp prp;               ///< As PRP entries
	struct xnvme_spec_sgl_descriptor sgl;         ///< As an SGL descriptor
	struct xnvme_spec_dptr_linux_ioctl lnx_ioctl; ///< As the Linux Kernel NVMe driver uses it
};
XNVME_STATIC_ASSERT(sizeof(union xnvme_spec_dptr) == 16, "Incorrect size")

/**
 * NVMe Command Accessor for common use of the 64byte NVMe command
 *
 * @see Specification section 4.3, figure 2
 * @see Specification section 4.3, figure 3
 *
 * @struct xnvme_spec_cmd_common
 */
struct xnvme_spec_cmd_common {
	/* cdw 00 */
	uint8_t opcode; ///< OPC: Command Opcode
	uint8_t flags;  ///< FUSE and PSDT, see XNVME_SPEC_CMD_COMMON_FLAGS_*
	uint16_t cid;   ///< CID: Command Identifier

	/* cdw 01 */
	uint32_t nsid; ///< NSID: Namespace identifier

	uint32_t cdw02;
	uint32_t cdw03;

	/* cdw 04-05 */
	uint64_t mptr; ///< MPTR -- metadata pointer

	/* cdw 06-09: */
	union xnvme_spec_dptr dptr; ///< DPTR: Data Pointer

	/* cdw 10 */
	uint32_t ndt; ///< NDT: Number of dwords in Data Transfer

	/* cdw 11 */
	uint32_t ndm; ///< NDM: Number of dwords in Meta Transfer

	uint32_t cdw12;
	uint32_t cdw13;
	uint32_t cdw14;
	uint32_t cdw15;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_common) == 64, "Incorrect size")

/** Fused Operation; bits 1:0 of 'flags', bits 9:8 of dword 0 */
#define XNVME_SPEC_CMD_COMMON_FLAGS_FUSE 0x3
/** PRP or SGL for Data Transfer; bits 7:6 of 'flags', bits 15:14 of dword 0 */
#define XNVME_SPEC_CMD_COMMON_FLAGS_PSDT 0xc0

/**
 * NVMe Command Accessor for the Sanitize command
 *
 * @struct xnvme_spec_cmd_sanitize
 */
struct xnvme_spec_cmd_sanitize {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint32_t cdw10; ///< Command dword 10, see XNVME_SPEC_CMD_SANITIZE_CDW10_*

	uint32_t ovrpat; ///< Overwrite Pattern

	uint32_t cdw12_15[4]; ///< Command dword 12 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_sanitize) == 64, "Incorrect size")

/** Sanitize Action; bits 2:0 of 'cdw10' */
#define XNVME_SPEC_CMD_SANITIZE_CDW10_SANACT 0x7
/** Allow Unrestricted Sanitize Exit; bit 3 of 'cdw10' */
#define XNVME_SPEC_CMD_SANITIZE_CDW10_AUSE 0x8
/** Overwrite Pass Count; bits 7:4 of 'cdw10' */
#define XNVME_SPEC_CMD_SANITIZE_CDW10_OWPASS 0xf0
/** Overwrite Invert Pattern Between Passes; bit 8 of 'cdw10' */
#define XNVME_SPEC_CMD_SANITIZE_CDW10_OIPBP 0x100
/** No-Deallocate After Sanitize; bit 9 of 'cdw10' */
#define XNVME_SPEC_CMD_SANITIZE_CDW10_NDAS 0x200

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
 * @struct xnvme_spec_cmd_gfeat
 */
struct xnvme_spec_cmd_gfeat {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint32_t cdw10;       ///< Command dword 10, see XNVME_SPEC_CMD_GFEAT_CDW10_*
	uint32_t cdw11;       ///< Command dword 11
	uint32_t cdw12_15[4]; ///< Command dword 12 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_gfeat) == 64, "Incorrect size")

/** Feature Identifier, see xnvme_spec_feat_id; bits 7:0 of 'cdw10' */
#define XNVME_SPEC_CMD_GFEAT_CDW10_FID 0xff
/** Select, see xnvme_spec_feat_sel; bits 10:8 of 'cdw10' */
#define XNVME_SPEC_CMD_GFEAT_CDW10_SEL 0x700

/**
 * NVMe Command Accessor for the set-features command
 *
 * @struct xnvme_spec_cmd_sfeat
 */
struct xnvme_spec_cmd_sfeat {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint32_t cdw10; ///< Command dword 10, see XNVME_SPEC_CMD_SFEAT_CDW10_*

	union xnvme_spec_feat feat; ///< Feature attributes, dword 11

	uint32_t cdw12;       ///< Command dword 12
	uint32_t cdw13_15[3]; ///< Command dword 13 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_sfeat) == 64, "Incorrect size")

/** Feature Identifier, see xnvme_spec_feat_id; bits 7:0 of 'cdw10' */
#define XNVME_SPEC_CMD_SFEAT_CDW10_FID 0xff
/** Save; bit 31 of 'cdw10' */
#define XNVME_SPEC_CMD_SFEAT_CDW10_SV 0x80000000

/**
 * NVMe Command Accessor for the directive send command
 *
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

/** Enable Directive, of the Identify directive's Enable Directive; bit 0 of 'cdw12' */
#define XNVME_SPEC_CMD_DSEND_CDW12_ENDIR 0x1
/** Target Directive Type, of the Identify directive's Enable Directive; bits 15:8 of 'cdw12' */
#define XNVME_SPEC_CMD_DSEND_CDW12_TDTYPE 0xff00

/**
 * NVMe Command Accessor for the directive receive command
 *
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

/** Namespace Streams Requested, of Allocate Resources; bits 15:0 of 'cdw12' */
#define XNVME_SPEC_CMD_DRECV_CDW12_NSR 0xffff

/**
 * NVMe Command Accessor for the identify command
 *
 * @struct xnvme_spec_cmd_idfy
 */
struct xnvme_spec_cmd_idfy {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint8_t cns; ///< CNS: Controller or Namespace Structure
	uint8_t rsvd1;
	uint16_t cntid; ///< CNTID: Controller Identifier

	uint16_t nvmsetid; ///< NVM Set Identifier
	uint8_t rsvd2;
	uint8_t csi; ///< CSI: Command Set Identifier

	uint32_t cdw12_13[2]; ///< Command dword 12 to 13

	uint32_t cdw14; ///< Command dword 14, see XNVME_SPEC_CMD_IDFY_CDW14_*

	uint32_t cdw15; ///< Command dword 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_idfy) == 64, "Incorrect size")

/** UUID Index; bits 6:0 of 'cdw14' */
#define XNVME_SPEC_CMD_IDFY_CDW14_UUID 0x7f

/**
 * NVMe Command Accessor for the get-log-page command
 *
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

/** Log Page Identifier; bits 7:0 of 'cdw10' */
#define XNVME_SPEC_CMD_LOG_CDW10_LID 0xff
/** Log Specific Parameter; bits 14:8 of 'cdw10' */
#define XNVME_SPEC_CMD_LOG_CDW10_LSP 0x7f00
/** Retain Asynchronous Event; bit 15 of 'cdw10' */
#define XNVME_SPEC_CMD_LOG_CDW10_RAE 0x8000
/** Number of Dwords Lower; bits 31:16 of 'cdw10' */
#define XNVME_SPEC_CMD_LOG_CDW10_NUMDL 0xffff0000

/** UUID Index; bits 6:0 of 'cdw14' */
#define XNVME_SPEC_CMD_LOG_CDW14_UUIDX 0x7f
/** Offset Type; bit 23 of 'cdw14' */
#define XNVME_SPEC_CMD_LOG_CDW14_OT 0x800000
/** Command Set Identifier; bits 31:24 of 'cdw14' */
#define XNVME_SPEC_CMD_LOG_CDW14_CSI 0xff000000

/**
 * NVMe Command Accessors for the NVM Command Set
 *
 * @struct xnvme_spec_cmd_nvm
 */
struct xnvme_spec_cmd_nvm {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint64_t slba; ///< SLBA: Start Logical Block Address

	uint32_t cdw12; ///< Command dword 12, see XNVME_SPEC_CMD_NVM_CDW12_*

	uint32_t cdw13; ///< Command dword 13, see XNVME_SPEC_CMD_NVM_CDW13_*

	/* cdw 14 */
	uint32_t ilbrt; ///< Initial Logical Block Reference Tag

	/* cdw 15 */
	uint16_t lbat;  ///< Logical Block Application Tag
	uint16_t lbatm; ///< logical Block Application Tag Mask
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_nvm) == 64, "Incorrect size")

/** Number of Logical Blocks, zero-based; bits 15:0 of 'cdw12' */
#define XNVME_SPEC_CMD_NVM_CDW12_NLB 0xffff
/** Directive Type; bits 23:20 of 'cdw12' */
#define XNVME_SPEC_CMD_NVM_CDW12_DTYPE 0xf00000
/** Protection Information Field; bits 29:26 of 'cdw12' */
#define XNVME_SPEC_CMD_NVM_CDW12_PRINFO 0x3c000000
/** Force Unit Access; bit 30 of 'cdw12' */
#define XNVME_SPEC_CMD_NVM_CDW12_FUA 0x40000000
/** Limited Retry; bit 31 of 'cdw12' */
#define XNVME_SPEC_CMD_NVM_CDW12_LR 0x80000000

/** Access Frequency; bits 3:0 of 'cdw13' */
#define XNVME_SPEC_CMD_NVM_CDW13_AF 0xf
/** Access Latency; bits 5:4 of 'cdw13' */
#define XNVME_SPEC_CMD_NVM_CDW13_AL 0x30
/** Sequential Request; bit 6 of 'cdw13' */
#define XNVME_SPEC_CMD_NVM_CDW13_SR 0x40
/** Incompressible; bit 7 of 'cdw13' */
#define XNVME_SPEC_CMD_NVM_CDW13_INCOM 0x80
/** Directive Specific; bits 31:16 of 'cdw13' */
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
	uint32_t nr    : 8; ///< NR: Number of Ranges
	uint32_t rsvd1 : 24;

	// dword 11
	uint32_t idr   : 1; ///< IDR: Integral Dataset for Read
	uint32_t idw   : 1; ///< IDW: Integral Dataset for Write
	uint32_t ad    : 1; ///< AD: Deallocate
	uint32_t rsvd2 : 29;

	uint32_t cdw12_15[4]; ///< Command dword 12 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_dsm) == 64, "Incorrect size")

/**
 * NVMe Command Accessor for the Write Zeroes command
 *
 * @struct xnvme_spec_nvm_write_zeroes
 */
struct xnvme_spec_nvm_write_zeroes {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	/* cdw 10-11 */
	uint64_t slba; ///< Start LBA

	uint32_t cdw12; ///< Command dword 12, see XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_*

	/* cdw 13 */
	uint32_t cdw_13; ///< Command dword 13

	/* cdw 14 */
	uint32_t ilbrt; ///< Initial Logical Block Reference Tag

	/* cdw 15 */
	uint16_t lbat;  ///< Logical Block Application Tag
	uint16_t lbatm; ///< logical Block Application Tag Mask
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_write_zeroes) == 64, "Incorrect size")

/** Number of Logical Blocks, zero-based; bits 15:0 of 'cdw12' */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_NLB 0xffff
/** Storage Tag Check; bit 24 of 'cdw12' */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_STC 0x1000000
/** Deallocate; bit 25 of 'cdw12' */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_DEAC 0x2000000
/** Protection Information Field; bits 29:26 of 'cdw12' */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_PRINFO 0x3c000000
/** Force Unit Access; bit 30 of 'cdw12' */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_FUA 0x40000000
/** Limited Retry; bit 31 of 'cdw12' */
#define XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_LR 0x80000000

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
 * @struct xnvme_spec_nvm_scopy_fmt_zero
 */
struct xnvme_spec_nvm_scopy_fmt_zero {
	uint8_t rsvd0[8];

	uint64_t slba; ///< Start LBA

	uint32_t nlb    : 16; ///< Number of logical blocks, zero-based

	uint32_t rsvd20 : 16;

	uint32_t eilbrt; ///< Expected Initial Logical Block Ref. Tag
	uint32_t elbatm; ///< Expected Logical Block App. Tag Mask
	uint32_t elbat;  ///< Expected Logical Block App. Tag
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_scopy_fmt_zero) == 32, "Incorrect size")

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
 * @struct xnvme_spec_nvm_cmd_scopy
 */
struct xnvme_spec_nvm_cmd_scopy {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	/* cdw 10-11 */
	uint64_t sdlba; ///< Start Destination LBA

	/* cdw 12 */
	uint32_t nr      : 8; ///< Number of Ranges
	uint32_t df      : 4; ///< Descriptor Format
	uint32_t prinfor : 4; ///< Protection Info. Field Read
	uint32_t rsvd1   : 4;
	uint32_t dtype   : 4; ///< Directive Type
	uint32_t rsvd2   : 2;
	uint32_t prinfow : 4; ///< Protection Info. Field Write
	uint32_t fua     : 1; ///< Force Unit Access
	uint32_t lr      : 1; ///< Limited Retry

	/* cdw 13 */
	uint32_t rsvd3   : 16;
	uint32_t dspec   : 16; ///< Directive Specific

	/* cdw 14 */
	uint32_t ilbrt; ///< Initial Logical Block Ref. Tag

	/* cdw 15 */
	uint32_t lbat  : 16; ///< Logical Block App. Tag
	uint32_t lbatm : 16; ///< Logical Block App. Tag Mask
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_cmd_scopy) == 64, "Incorrect size")

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
 * @struct xnvme_spec_nvm_cmd
 */
struct xnvme_spec_nvm_cmd {
	union {
		struct xnvme_spec_nvm_cmd_scopy scopy;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_cmd) == 64, "Incorrect size")

/**
 * Representation of NVMe completion result for NVM command set I/O command set
 * specific Identify Controller
 *
 * That is, for opcode XNVME_SPEC_OPC_IDFY(0x06) with XNVME_SPEC_IDFY_CTLR_IOCS(0x6)
 *
 * @struct xnvme_spec_nvm_idfy_ctrlr
 */
struct xnvme_spec_nvm_idfy_ctrlr {
	uint8_t vsl;    ///< Verify Size Limit
	uint8_t wzsl;   ///< Write Zeroes Size Limit
	uint8_t wusl;   ///< Write Uncorrectable Size Limit
	uint8_t dmrl;   ///< Dataset Management Ranges Limit
	uint32_t dmrsl; ///< Dataset Management Range Size Limit
	uint64_t dmsl;  ///< Dataset Management Size Limit

	uint8_t reserved16[4080];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_idfy_ctrlr) == 4096, "Incorrect size")

/**
 * Representation of NVMe completion result for NVM command set I/O command set
 * specific Identify Namespace
 *
 * That is, for opcode XNVME_SPEC_OPC_IDFY(0x06) with XNVME_SPEC_IDFY_NS_IOCS(0x05)
 *
 * @struct xnvme_spec_nvm_idfy_ns
 */
struct xnvme_spec_nvm_idfy_ns {
	uint64_t lbstm; ///< Logical block storage tag mask

	uint8_t pic; ///< Protection Information Capabilities, see XNVME_SPEC_NVM_IDFY_NS_PIC_*

	uint8_t reserved9[3];

	struct xnvme_spec_elbaf elbaf[64];

	uint8_t reserved268[3828];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_idfy_ns) == 4096, "Incorrect size")

/** 16b Guard Protection Information Storage Tag Support; bit 0 of 'pic' */
#define XNVME_SPEC_NVM_IDFY_NS_PIC_GPISTS 0x1
/** 16b Guard Protection Information Storage Tag Mask; bit 1 of 'pic' */
#define XNVME_SPEC_NVM_IDFY_NS_PIC_GPISTM 0x2
/** Storage Tag Check Read Support; bit 2 of 'pic' */
#define XNVME_SPEC_NVM_IDFY_NS_PIC_STCRS 0x4

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
 * @struct xnvme_spec_znd_cmd_mgmt_send
 */
struct xnvme_spec_znd_cmd_mgmt_send {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	/* cdw 10-11 */
	uint64_t slba; ///< Start LBA

	/* cdw 12 */
	uint32_t nrange; ///< Number of ranges <= 32

	/* cdw 13 */
	uint32_t zsa        : 8; ///< Zone Send Action
	uint32_t select_all : 1; ///< Select All, that is, ignore the slba, affects all LBAs
	uint32_t zsaso      : 1; ///< Zone Send Action Specific Option
	uint32_t rsvd       : 22;

	uint32_t cdw14_15[2]; ///< Command dword 14 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_cmd_mgmt_send) == 64, "Incorrect size")

/**
 * NVMe Command Accessor for a command with opcode XNVME_SPEC_ZND_OPC_MGMT_RECV
 *
 * @see enum xnvme_spec_znd_cmd_mgmt_recv
 * @struct xnvme_spec_znd_cmd_mgmt_recv
 */
struct xnvme_spec_znd_cmd_mgmt_recv {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	/* cdw 10-11 */
	uint64_t slba; ///< Start LBA

	/* cdw 12 */
	uint32_t ndwords; ///< Number of dwords in data-payload

	/* cdw 13 */
	uint32_t zra     : 8; ///< Zone Receive Action
	uint32_t zrasf   : 8; ///< Zone Receive Action Specific Field
	uint32_t partial : 1; ///< Partial

	uint32_t rsvd    : 15;

	/* cdw 14-15 */
	uint64_t addrs_dst; ///< destination addresses
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_cmd_mgmt_recv) == 64, "Incorrect size")

/**
 * NVMe Command Accessor for a command with opcode ZND_CMD_OPC_APPEND
 *
 * @struct xnvme_spec_znd_cmd_append
 */
struct xnvme_spec_znd_cmd_append {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint64_t zslba; ///< SLBA: Start Logical Block Address

	uint32_t nlb    : 16; ///< NLB: Number of logical blocks
	uint32_t rsvd   : 4;
	uint32_t dtype  : 4; ///< DT: Directive Type
	uint32_t prinfo : 4; ///< PI: Protection Information Field
	uint32_t rsvd2  : 2;
	uint32_t fua    : 1; ///< FUA: Force unit access
	uint32_t lr     : 1; ///< LR: Limited retry

	uint32_t cdw13_15[3]; ///< Command dword 13 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_cmd_append) == 64, "Incorrect size")

/**
 * NVMe Command Accessors for the Zoned Command Set
 *
 * @struct xnvme_spec_znd_cmd
 */
struct xnvme_spec_znd_cmd {
	union {
		struct xnvme_spec_znd_cmd_mgmt_send mgmt_send;
		struct xnvme_spec_znd_cmd_mgmt_recv mgmt_recv;
		struct xnvme_spec_znd_cmd_append append;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_cmd) == 64, "Incorrect size")

/**
 * NVMe Command Accessor for io management receive command
 *
 * @struct xnvme_spec_io_mgmt_recv_cmd
 */
struct xnvme_spec_io_mgmt_recv_cmd {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	uint32_t mo   : 8; ///< Management Operation
	uint32_t rsvd : 8;
	uint32_t mos  : 16; ///< Management Operation Specific

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

	uint32_t mo   : 8; ///< Management Operation
	uint32_t rsvd : 8;
	uint32_t mos  : 16; ///< Management Operation Specific

	uint32_t cdw11_15[5]; ///< Command dword 11 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_io_mgmt_send_cmd) == 64, "Incorrect size")

/**
 * NVMe Command Accessors for I/O management
 *
 * @struct xnvme_spec_io_mgmt_cmd
 */
struct xnvme_spec_io_mgmt_cmd {
	union {
		struct xnvme_spec_io_mgmt_recv_cmd mgmt_recv;
		struct xnvme_spec_io_mgmt_send_cmd mgmt_send;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_io_mgmt_cmd) == 64, "Incorrect size")

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
 * NVMe Command Accessors for the KV Command Set
 *
 * @see Key Value Command Set Specification Revision 1.0c, Section 3
 *
 * @struct xnvme_spec_kvs_cmd
 */
struct xnvme_spec_kvs_cmd {
	uint32_t cdw0;
	uint32_t nsid;
	/* cdw 02-03 */
	uint64_t key;    ///< KV key bits 0:63
	uint64_t mptr;   ///< Reserverd for MPTR
	/* cdw 06-09: */ ///< DPTR -- data pointer
	uint32_t cdw06;
	uint32_t cdw07;
	uint32_t cdw08;
	uint32_t cdw09;
	uint32_t cdw10; ///< Host Buffer Size or Value Size

	/* cdw 11 */
	struct {
		uint8_t kl;
		uint8_t ro;
		uint16_t rsvd;
	} cdw11;
	uint32_t cdw12;
	uint32_t cdw13;
	/* cdw 14-15 */
	uint64_t key_hi; ///< KV key bits 64:127
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_kvs_cmd) == 64, "Incorrect size")

/**
 * NVMe Command Accessor for the Compare command
 *
 * @struct xnvme_spec_nvm_compare
 */
struct xnvme_spec_nvm_compare {
	uint32_t cdw00_09[10]; ///< Command dword 0 to 9

	/* cdw 10-11 */
	uint64_t slba; ///< Start LBA

	uint32_t cdw12; ///< Command dword 12, see XNVME_SPEC_NVM_COMPARE_CDW12_*

	/* cdw 13 */
	uint32_t cdw_13; ///< Command dword 13

	/* cdw 14 */
	uint32_t ilbrt; ///< Initial Logical Block Reference Tag

	/* cdw 15 */
	uint16_t lbat;  ///< Logical Block Application Tag
	uint16_t lbatm; ///< logical Block Application Tag Mask
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_compare) == 64, "Incorrect size")

/** Number of Logical Blocks, zero-based; bits 15:0 of 'cdw12' */
#define XNVME_SPEC_NVM_COMPARE_CDW12_NLB 0xffff
/** Storage Tag Check; bit 24 of 'cdw12' */
#define XNVME_SPEC_NVM_COMPARE_CDW12_STC 0x1000000
/** Protection Information Field; bits 29:26 of 'cdw12' */
#define XNVME_SPEC_NVM_COMPARE_CDW12_PRINFO 0x3c000000
/** Force Unit Access; bit 30 of 'cdw12' */
#define XNVME_SPEC_NVM_COMPARE_CDW12_FUA 0x40000000
/** Limited Retry; bit 31 of 'cdw12' */
#define XNVME_SPEC_NVM_COMPARE_CDW12_LR 0x80000000

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
	struct xnvme_spec_znd_cmd znd;
	struct xnvme_spec_io_mgmt_cmd mgmt;
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
 * @see Specification, section 3.1.2
 * @struct xnvme_spec_znd_idfy_ctrlr
 */
struct xnvme_spec_znd_idfy_ctrlr {
	uint8_t zasl; ///< Zone Append Size Limit

	uint8_t rsvd8[4095];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_idfy_ctrlr) == 4096, "Incorrect size")

/**
 * LBA Format Extension
 *
 * @see Specification Section 5.1.2, figure LBAFE
 *
 * @struct xnvme_spec_znd_idfy_lbafe
 */
struct xnvme_spec_znd_idfy_lbafe {
	uint64_t zsze; ///< Zone Size in number of logical blocks
	uint8_t zdes;  ///< Zone Descriptor Extensions Size

	uint8_t rsvd[7];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_idfy_lbafe) == 16, "Incorrect size")

/**
 * Zoned Command Set specific identify namespace data structure
 *
 * @see TP4053, section 3
 * @struct xnvme_spec_znd_idfy_ns
 */
struct xnvme_spec_znd_idfy_ns {
	uint16_t zoc;  ///< Zone Operation Characteristics, see XNVME_SPEC_ZND_IDFY_NS_ZOC_*
	uint16_t ozcs; ///< Optional Zoned Command Support, see XNVME_SPEC_ZND_IDFY_NS_OZCS_*

	uint32_t mar; ///< Maximum Active Resources
	uint32_t mor; ///< Maximum Open Resources

	uint32_t rrl; ///< Reset Recommended Limit
	uint32_t frl; ///< Finish Recommended Limit

	uint8_t rsvd12[24];

	uint32_t numzrwa; ///< Number of ZRWA resources available to the namespace
	uint16_t zrwafg;  ///< ZRWA Flush Granularity; in unit of LBAs
	uint16_t zrwas;   ///< ZRWA Size; in unit of LBAs

	uint8_t zrwacap; ///< ZRWA Capability, see XNVME_SPEC_ZND_IDFY_NS_ZRWACAP_*

	uint8_t rsvd53[2763];

	struct xnvme_spec_znd_idfy_lbafe lbafe[16];

	uint8_t rsvd3072[768];

	uint8_t vs[256]; ///< Vendor Specific
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_idfy_ns) == 4096, "Incorrect size")

/** Variable Zone Capacity; bit 0 of 'zoc' */
#define XNVME_SPEC_ZND_IDFY_NS_ZOC_VZCAP 0x1
/** Zone Active Excursions; bit 1 of 'zoc' */
#define XNVME_SPEC_ZND_IDFY_NS_ZOC_ZAE 0x2
/** Read Across Zone Boundaries; bit 0 of 'ozcs' */
#define XNVME_SPEC_ZND_IDFY_NS_OZCS_RAZB 0x1
/** Zone Random Write Area Supported; bit 1 of 'ozcs' */
#define XNVME_SPEC_ZND_IDFY_NS_OZCS_ZRWASUP 0x2
/** Explicit ZRWA Flush Supported; bit 0 of 'zrwacap' */
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
 * @see Specification Section 5.3.1.1, figure TBDZONELIST
 *
 * @struct xnvme_spec_znd_log_changes
 */
struct xnvme_spec_znd_log_changes {
	uint16_t nidents; ///< Number of Zone Identifiers
	uint8_t rsvd2[6];
	uint64_t idents[ZND_CHANGES_LEN]; ///< Zone Identifiers
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_log_changes) == 4096, "Incorrect size")

/**
 * Zone Descriptor as reported by Zone Management Receive command
 *
 * @see Specification Section 6.2.2.3, figure TBDZMRZD
 *
 * @struct xnvme_spec_znd_descr
 */
struct xnvme_spec_znd_descr {
	uint8_t zt; ///< Zone Type, see XNVME_SPEC_ZND_DESCR_ZT_*
	uint8_t zs; ///< Zone State, see XNVME_SPEC_ZND_DESCR_ZS_*
	uint8_t za; ///< Zone Attributes, see XNVME_SPEC_ZND_DESCR_ZA_*

	uint8_t rsvd7[5];

	uint64_t zcap;  ///< Zone Capacity (in number of LBAs)
	uint64_t zslba; ///< Zone Start LBA
	uint64_t wp;    ///< Write Pointer
	uint8_t rsvd63[32];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_descr) == 64, "Incorrect size")

/** Zone Type, see xnvme_spec_znd_type; bits 3:0 of 'zt' */
#define XNVME_SPEC_ZND_DESCR_ZT_ZT 0xf
/** Zone State, see xnvme_spec_znd_state; bits 7:4 of 'zs' */
#define XNVME_SPEC_ZND_DESCR_ZS_ZS 0xf0
/** Zone Finished by Controller; bit 0 of 'za' */
#define XNVME_SPEC_ZND_DESCR_ZA_ZFC 0x1
/** Finish Zone Recommended; bit 1 of 'za' */
#define XNVME_SPEC_ZND_DESCR_ZA_ZFR 0x2
/** Reset Zone Recommended; bit 2 of 'za' */
#define XNVME_SPEC_ZND_DESCR_ZA_RZR 0x4
/** Zone Random Write Area Valid; bit 3 of 'za' */
#define XNVME_SPEC_ZND_DESCR_ZA_ZRWAV 0x8
/** Zone Descriptor Extension Valid; bit 7 of 'za' */
#define XNVME_SPEC_ZND_DESCR_ZA_ZDEV 0x80

/**
 * Report Zones Data Structure header
 *
 * The first 16 bytes of the two Report Zoned data structures
 *
 * @see Specification Section 6.2.2.1, figure TBDZMRRZ
 * @see Specification Section 6.2.2.2, figure TBDZMRERZ
 * @struct xnvme_spec_znd_report_hdr
 */
struct xnvme_spec_znd_report_hdr {
	/**
	 * Number of zones in namespace when receiving with partial=0
	 * Number of zones in the data-buffer, when receiving with partial=1
	 */
	uint64_t nzones;
	uint8_t rsvd[56];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_report_hdr) == 64, "Incorrect size")

/**
 * Key Value Command Set KV Format data structure
 *
 * @see Key Value Command Set Specification Revision 1.0c, Figure 40
 *
 * @struct xnvme_spec_kvs_idfy_ns_format
 */
struct xnvme_spec_kvs_idfy_ns_format {
	uint16_t kml; ///< KV Key Max Length
	uint8_t rsvd2;
	uint8_t fopt; ///< Additional format options
	uint32_t vml; ///< KV Value Max Length
	uint32_t mnk; ///< Max Num Keys
	uint8_t rsvd12[4];
};

/**
 * Key Value Command Set identify namespace data structure
 *
 * @see Key Value Command Set Specification Revision 1.0c, Figure 39
 *
 * @struct xnvme_spec_kvs_idfy_ns
 */
struct xnvme_spec_kvs_idfy_ns {
	uint64_t nsze; ///< Namespace Size
	uint8_t rsvd8[8];
	uint64_t nuse;  ///< Namespace Utilization
	uint8_t nsfeat; ///< Namespace Features
	uint8_t nkvf;   ///< Number of KV Formats
	uint8_t nmic;   ///< Namespace Multi-path I/O and Namespace Sharing Capabilities
	uint8_t rescap; ///< Reservation Capabilities
	uint8_t fpi;    ///< Format Progress Indicator
	uint8_t rsvd29[3];
	uint32_t novg;     ///< Namespace Optimal Value Granularity
	uint32_t anagrpid; ///< ANA Group Identifier
	uint8_t rsvd40[3];
	uint8_t nsattr;                               ///< Namespace Attributes
	uint16_t nvmsetid;                            ///< NVM Set Identifier
	uint16_t endgid;                              ///< Endurance Group Identifier
	uint64_t nguid[2];                            ///< Namespace Globally Unique Identifier
	uint64_t eui64;                               ///< IEEE Extended Unique Identifier
	struct xnvme_spec_kvs_idfy_ns_format kvf[16]; ///< KV Format 0 to 15
	uint8_t rsvd328[3512];                        // 3839:328
	uint8_t vs[256];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_kvs_idfy_ns) == 4096, "Incorrect size")

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
