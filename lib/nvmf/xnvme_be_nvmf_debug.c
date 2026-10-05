#include <errno.h>
#include <stddef.h>

#include <xnvme_be_nvmf_debug.h>

#ifdef XNVME_DEBUG_ENABLED

int nvmf_debug_log_level;
int nvmf_debug_categories[NVMF_DEBUG_CATEGORY_MAX];

const char *
nvmf_debug_log_level_str(void)
{
	switch (nvmf_debug_log_level) {
	case NVMF_DEBUG_LOG_LEVEL_NONE:
		return "NONE";
	case NVMF_DEBUG_LOG_LEVEL_ERROR:
		return "ERROR";
	case NVMF_DEBUG_LOG_LEVEL_WARN:
		return "WARN";
	case NVMF_DEBUG_LOG_LEVEL_INFO:
		return "INFO";
	case NVMF_DEBUG_LOG_LEVEL_DEBUG:
		return "DEBUG";
	default:
		return "UNKNOWN";
	}
}

static inline const char *
_xnvme_resolve_generic_sc(struct xnvme_spec_cpl *cpl)
{
	static const char *generic_codes[] = {
		[0x00] = "Successful Completion",
		[0x01] = "Invalid Command Opcode",
		[0x02] = "Invalid Field in Command",
		[0x03] = "Command ID Conflict",
		[0x04] = "Data Transfer Error",
		[0x05] = "Commands Aborted due to Power Loss Notification",
		[0x06] = "Internal Device Error",
		[0x07] = "Command Abort Requested",
		[0x08] = "Command Aborted due to SQ Deletion",
		[0x09] = "Command Aborted due to Failed Fused Command",
		[0x0A] = "Command Aborted due to Missing Fused Command",
		[0x0B] = "Invalid Namespace or Format",
		[0x0C] = "Command Sequence Error",
		[0x0D] = "Invalid SGL Segment Descriptor",
		[0x0E] = "Invalid Number of SGL Descriptors",
		[0x0F] = "Data SGL Length Invalid",
		[0x10] = "Metadata SGL Length Invalid",
		[0x11] = "SGL Descriptor Type Invalid",
		[0x12] = "Invalid Use of Controller Memory Buffer",
		[0x13] = "PRP Offset Invalid",
		[0x14] = "Atomic Write Unit Exceeded",
		[0x15] = "Operation Denied",
		[0x16] = "SGL Offset Invalid",
		[0x17] = "Reserved",
		[0x18] = "Host Identifier Inconsistent Format",
		[0x19] = "Keep Alive Timeout Expired",
		[0x1A] = "Keep Alive Timeout Invalid",
		[0x1B] = "Command Aborted due to Preempt and Abort",
		[0x1C] = "Sanitize Failed",
		[0x1D] = "Sanitize In Progress",
		[0x1E] = "SGL Data Block Granularity Invalid",
		[0x1F] = "Command Not Supported for Queue in CMB",
		[0x20] = "Namespace is write protected",
		[0x21] = "Command Interrupted",
		[0x22] = "Transient Transport Error",
		[0x23] = "Command Prohibited by Command and Feature Lockdown",
		[0x24] = "Admin Command Media Not Ready",
		[0x25] = "Invalid Key Tag",
		[0x26] = "Host Dispersed Namespace Support Not Enabled",
		[0x27] = "Host Identifier Not Initialized",
		[0x28] = "Incorrect Key",
		[0x29] = "FDP Disabled",
		[0x2A] = "Invalid Placement Handle List",
		[0x2B] = "Sanitize Namespace Failed",
		[0x2C] = "Sanitize Namespace In Progress",
	};
	static const char *nvm_codes[] = {
		[0x00] = "LBA Out of Range",    [0x01] = "Capacity Exceeded",
		[0x02] = "Namespace Not Ready", [0x03] = "Reservation Conflict",
		[0x04] = "Format In Progress",  [0x05] = "Invalid Value Size",
		[0x06] = "Invalid Key Size",    [0x07] = "KV Key Does Not Exist",
		[0x08] = "Unrecovered Error",   [0x09] = "Key Exists",
	};

	uint8_t sc = cpl->status.sc;

	if (sc <= 0x2C) {
		return generic_codes[sc] ? generic_codes[sc] : "Unknown";
	}
	if (sc >= 0x80 && sc <= 0x89) {
		return nvm_codes[sc - 0x80];
	}
	return "Unknown";
}

static inline const char *
_xnvme_resolve_io_sc(struct xnvme_spec_cpl *cpl)
{
	static const char *io_codes[] = {
		[0x00] = "Conflicting Attributes",
		[0x01] = "Invalid Protection Information",
		[0x02] = "Attempted Write to Read Only Range",
		[0x03] = "Command Size Limit Exceeded",
		[0x04] = "Invalid Command ID",
		[0x05] = "Incompatible Namespace or Format",
		[0x06] = "Fast Copy Not Possible",
		[0x07] = "Overlapping I/O Range",
		[0x08] = "Namespace Not Reachable",
		[0x09] = "Insufficient Resources",
		[0x0A] = "Insufficient Program Resources",
		[0x0B] = "Invalid Memory Namespace",
		[0x0C] = "Invalid Memory Range Set",
		[0x0D] = "Invalid Memory Range Set Identifier",
		[0x0E] = "Invalid Program Data",
		[0x0F] = "Invalid Program Index",
		[0x10] = "Invalid Program Type",
		[0x11] = "Maximum Memory Ranges Exceeded",
		[0x12] = "Maximum Memory Range Sets Exceeded",
		[0x13] = "Maximum Programs Activated",
		[0x14] = "Maximum Program Bytes Exceeded",
		[0x15] = "Memory Range Set In Use",
		[0x16] = "No Program",
		[0x17] = "Overlapping Memory Ranges",
		[0x18] = "Program Not Activated",
		[0x19] = "Program In Use",
		[0x1A] = "Program Index Not Downloadable",
		[0x1B] = "Program Too Big",
		[0x1C] = "Successful Media Verification Read",
	};
	static const char *zns_codes[] = {
		[0x00] = "Zoned Boundary Error", [0x01] = "Zone Is Full",
		[0x02] = "Zone Is Read Only",    [0x03] = "Zone Is Offline",
		[0x04] = "Zone Invalid Write",   [0x05] = "Too Many Active Zones",
		[0x06] = "Too Many Open Zones",  [0x07] = "Invalid Zone State Transition",
	};

	uint8_t sc = cpl->status.sc;

	if (sc >= 0x80 && sc <= 0x9C) {
		return io_codes[sc - 0x80];
	}
	if (sc >= 0xB8 && sc <= 0xBF) {
		return zns_codes[sc - 0xB8];
	}
	return "Unknown";
}

static inline const char *
_xnvme_resolve_fabrics_sc(struct xnvme_spec_cpl *cpl)
{
	/* indexed by sc - 0x80; gaps at 0x06-0x0F are NULL */
	static const char *fabrics_codes[] = {
		[0x00] = "Incompatible Format",        [0x01] = "Controller Busy",
		[0x02] = "Connect Invalid Parameters", [0x03] = "Connect Restart Discovery",
		[0x04] = "Connect Invalid Host",       [0x05] = "Invalid Queue Type",
		[0x10] = "Discover Restart",           [0x11] = "Authentication Required",
	};

	uint8_t sc = cpl->status.sc;

	if (sc >= 0xB0 && sc <= 0xBF) {
		return "Transport Specific";
	}
	if (sc >= 0x80 && sc <= 0x91) {
		return fabrics_codes[sc - 0x80] ? fabrics_codes[sc - 0x80] : "Unknown";
	}
	return "Unknown";
}

static inline const char *
_xnvme_resolve_command_specific_sc(struct xnvme_spec_cpl *cpl)
{
	/* 0x04 and 0x17 are unassigned (NULL) */
	static const char *cmd_codes[] = {
		[0x00] = "Completion Queue Invalid",
		[0x01] = "Invalid Queue Identifier",
		[0x02] = "Invalid Queue Size",
		[0x03] = "Abort Command Limit Exceeded",
		[0x05] = "Asynchronous Event Request Limit Exceeded",
		[0x06] = "Invalid Firmware Slot",
		[0x07] = "Invalid Firmware Image",
		[0x08] = "Invalid Interrupt Vector",
		[0x09] = "Invalid Log Page",
		[0x0A] = "Invalid Format",
		[0x0B] = "Firmware Activation Requires Conventional Reset",
		[0x0C] = "Invalid Queue Deletion",
		[0x0D] = "Feature Identifier Not Saveable",
		[0x0E] = "Feature Not Changeable",
		[0x0F] = "Feature Not Namespace Specific",
		[0x10] = "Firmware Activation Requires NVM Subsystem Reset",
		[0x11] = "Firmware Activation Requires Controller Level Reset",
		[0x12] = "Firmware Activation Requires Maximum Time Violation",
		[0x13] = "Firmware Activation Prohibited",
		[0x14] = "Overlapping Range",
		[0x15] = "Namespace Insufficient Capacity",
		[0x16] = "Namespace Identifier Unavailable",
		[0x18] = "Namespace Already Attached",
		[0x19] = "Namespace Is Private",
		[0x1A] = "Namespace Not Attached",
		[0x1B] = "Thin Provisioning Not Supported",
		[0x1C] = "Controller List Invalid",
		[0x1D] = "Device Self-test In Progress",
		[0x1E] = "Boot Partition Write Prohibited",
		[0x1F] = "Invalid Controller Identifier",
		[0x20] = "Invalid Secondary Controller State",
		[0x21] = "Invalid Number of Controller Resources",
		[0x22] = "Invalid Resource Identifier",
		[0x23] = "Sanitize Prohibited While Persistent Memory Region is Enabled",
		[0x24] = "ANA Group Identifier Invalid",
		[0x25] = "ANA Attach Failed",
		[0x26] = "Insufficient Capacity",
		[0x27] = "Namespace Attachment Limit Exceeded",
		[0x28] = "Prohibition of Command Execution Not Supported",
		[0x29] = "I/O Command Set Not Supported",
		[0x2A] = "I/O Command Set Not Enabled",
		[0x2B] = "I/O Command Set Combination Rejected",
		[0x2C] = "Invalid I/O Command Set",
		[0x2D] = "Identifier Unavailable",
		[0x2E] = "Namespace Is Dispersed",
		[0x2F] = "Invalid Discovery Information",
		[0x30] = "Zoning Data Structure Locked",
		[0x31] = "Zoning Data Structure Not Found",
		[0x32] = "Insufficient Discovery Resources",
		[0x33] = "Requested Function Disabled",
		[0x34] = "ZoneGroup Originator Invalid",
		[0x35] = "Invalid Host",
		[0x36] = "Invalid NVM Subsystem",
		[0x37] = "Invalid Controller Data Queue",
		[0x38] = "Not Enough Resources",
		[0x39] = "Controller Suspended",
		[0x3A] = "Controller Not Suspended",
		[0x3B] = "Controller Data Queue Full",
		[0x3C] = "Request Exceeds Maximum Namespace Sanitize Operations In Progress",
		[0x3D] = "Manufacturing Default Personality Required",
		[0x3E] = "Invalid Power Limit",
		[0x3F] = "Cross-Controller Reset in Progress",
		[0x40] = "Cross-Controller Reset Log Page Full",
		[0x41] = "Cross-Controller Reset Limit Exceeded",
	};

	uint8_t sc = cpl->status.sc;

	if (sc <= 0x41) {
		return cmd_codes[sc] ? cmd_codes[sc] : "Unknown";
	}
	if (sc >= 0x70 && sc <= 0x7F) {
		return "Directive Specific";
	}
	if (sc >= 0x80 && sc <= 0xBF) {
		if (cpl->cid == 0xFFFF) {
			return _xnvme_resolve_io_sc(cpl);
		}
		return _xnvme_resolve_fabrics_sc(cpl);
	}
	return "Unknown";
}

void
_xnvme_print_error_code(struct xnvme_spec_cpl *cpl)
{
	static const char *sct_names[] = {
		[0x0] = "Generic Command Status",
		[0x1] = "Command Specific Status",
		[0x2] = "Media and Data Integrity Errors",
		[0x3] = "Path Related Errors",
	};

	uint8_t sct = cpl->status.sct;
	const char *sct_name = (sct <= 0x3) ? sct_names[sct] : "Unknown";
	const char *sc_name;

	switch (sct) {
	case 0x0:
		sc_name = _xnvme_resolve_generic_sc(cpl);
		break;
	case 0x1:
		sc_name = _xnvme_resolve_command_specific_sc(cpl);
		break;
	default:
		XNVME_DEBUG("INFO: NVMe CPL val=0x%04x sct=0x%02x(%s) sc=0x%02x m=%d dnr=%d",
			    cpl->status.val, sct, sct_name, cpl->status.sc, cpl->status.m,
			    cpl->status.dnr);
		return;
	}

	XNVME_DEBUG("INFO: NVMe CPL val=0x%04x sct=0x%02x(%s) sc=0x%02x(%s) m=%d dnr=%d",
		    cpl->status.val, sct, sct_name, cpl->status.sc, sc_name, cpl->status.m,
		    cpl->status.dnr);

	return;
}

static inline void
parse_categories(char *val)
{
	char *string = strdup(val);
	char *token = strtok(string, ",");
	while (token) {
		if (strcasecmp(token, "all") == 0) {
			for (int i = 0; i < NVMF_DEBUG_CATEGORY_MAX; ++i) {
				nvmf_debug_categories[i] = 1;
			}
			break;
		}
		if (strcasecmp(token, "core") == 0) {
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CORE] = 1;
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CORE_CTRLR] = 1;
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CORE_QPAIR] = 1;
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CORE_DEV] = 1;
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CORE_NAMESPACE] = 1;
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CORE_SUBSYS] = 1;
		} else if (strcasecmp(token, "core_ctrlr") == 0) {
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CORE_CTRLR] = 1;
		} else if (strcasecmp(token, "core_qpair") == 0) {
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CORE_QPAIR] = 1;
		} else if (strcasecmp(token, "core_dev") == 0) {
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CORE_DEV] = 1;
		} else if (strcasecmp(token, "core_namespace") == 0) {
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CORE_NAMESPACE] = 1;
		} else if (strcasecmp(token, "core_subsys") == 0) {
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CORE_SUBSYS] = 1;
		} else if (strcasecmp(token, "discovery") == 0) {
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_DISCOVERY] = 1;
		} else if (strcasecmp(token, "fabrics") == 0) {
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_FABRICS] = 1;
		} else if (strcasecmp(token, "nvme") == 0) {
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_NVME] = 1;
		} else if (strcasecmp(token, "rdmacm") == 0) {
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_RDMACM] = 1;
		} else if (strcasecmp(token, "verbs") == 0) {
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_VERBS] = 1;
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_VERBS_CTRL] = 1;
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_VERBS_DATA] = 1;
		} else if (strcasecmp(token, "verbs_ctrl") == 0) {
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_VERBS_CTRL] = 1;
		} else if (strcasecmp(token, "verbs_data") == 0) {
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_VERBS_DATA] = 1;
		} else if (strcasecmp(token, "cmd") == 0) {
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CMD] = 1;
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CMD_ADMIN] = 1;
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CMD_IO] = 1;
		} else if (strcasecmp(token, "cmd_admin") == 0) {
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CMD_ADMIN] = 1;
		} else if (strcasecmp(token, "cmd_io") == 0) {
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CMD_IO] = 1;
		} else if (strcasecmp(token, "dump") == 0) {
			nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CMD_DUMP] = 1;
		} else {
			XNVME_DEBUG("WARN: Unknown debug category: %s", token);
		}
		token = strtok(NULL, ",");
	}
	free(string);
}

static inline void
parse_log_level(char *val)
{
	char *string = strdup(val);
	char *token = strtok(string, ",");

	while (token) {
		if (strcasecmp(token, "none") == 0) {
			nvmf_debug_log_level = NVMF_DEBUG_LOG_LEVEL_NONE;
		} else if (strcasecmp(token, "error") == 0) {
			nvmf_debug_log_level = NVMF_DEBUG_LOG_LEVEL_ERROR;
		} else if (strcasecmp(token, "warn") == 0) {
			nvmf_debug_log_level = NVMF_DEBUG_LOG_LEVEL_WARN;
		} else if (strcasecmp(token, "info") == 0) {
			nvmf_debug_log_level = NVMF_DEBUG_LOG_LEVEL_INFO;
		} else if (strcasecmp(token, "debug") == 0) {
			nvmf_debug_log_level = NVMF_DEBUG_LOG_LEVEL_DEBUG;
		} else if (strcasecmp(token, "trace") == 0) {
			nvmf_debug_log_level = NVMF_DEBUG_LOG_LEVEL_TRACE;
		} else {
			XNVME_DEBUG("WARN: Unknown debug log level: %s", token);
		}
		token = strtok(NULL, ",");
	}
	free(string);
}

__attribute__((constructor)) static int
xnvme_be_nvmf_debug_init(void)
{
	char *val;
	val = getenv("NVMF_DEBUG_LOG_LEVEL");
	if (val) {
		parse_log_level(val);
	}

	val = getenv("NVMF_DEBUG_CATEGORIES");
	if (val) {
		parse_categories(val);
	}

	XNVME_DEBUG("INFO: NVMF_DEBUG_LOG_LEVEL=%d", nvmf_debug_log_level);
	XNVME_DEBUG("INFO: NVMF_DEBUG_CATEGORY[%s]=%d", "CORE",
		    nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CORE]);
	XNVME_DEBUG("INFO: NVMF_DEBUG_CATEGORY[%s]=%d", "CORE_CTRLR",
		    nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CORE_CTRLR]);
	XNVME_DEBUG("INFO: NVMF_DEBUG_CATEGORY[%s]=%d", "CORE_QPAIR",
		    nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CORE_QPAIR]);
	XNVME_DEBUG("INFO: NVMF_DEBUG_CATEGORY[%s]=%d", "CORE_DEV",
		    nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CORE_DEV]);

	XNVME_DEBUG("INFO: NVMF_DEBUG_CATEGORY[%s]=%d", "RDMACM",
		    nvmf_debug_categories[NVMF_DEBUG_CATEGORY_RDMACM]);

	XNVME_DEBUG("INFO: NVMF_DEBUG_CATEGORY[%s]=%d", "VERBS",
		    nvmf_debug_categories[NVMF_DEBUG_CATEGORY_VERBS]);
	XNVME_DEBUG("INFO: NVMF_DEBUG_CATEGORY[%s]=%d", "VERBS_CTRL",
		    nvmf_debug_categories[NVMF_DEBUG_CATEGORY_VERBS_CTRL]);
	XNVME_DEBUG("INFO: NVMF_DEBUG_CATEGORY[%s]=%d", "VERBS_DATA",
		    nvmf_debug_categories[NVMF_DEBUG_CATEGORY_VERBS_DATA]);

	XNVME_DEBUG("INFO: NVMF_DEBUG_CATEGORY[%s]=%d", "NVME",
		    nvmf_debug_categories[NVMF_DEBUG_CATEGORY_NVME]);
	XNVME_DEBUG("INFO: NVMF_DEBUG_CATEGORY[%s]=%d", "FABRICS",
		    nvmf_debug_categories[NVMF_DEBUG_CATEGORY_FABRICS]);

	XNVME_DEBUG("INFO: NVMF_DEBUG_CATEGORY[%s]=%d", "CMD",
		    nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CMD]);
	XNVME_DEBUG("INFO: NVMF_DEBUG_CATEGORY[%s]=%d", "CMD_ADMIN",
		    nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CMD_ADMIN]);
	XNVME_DEBUG("INFO: NVMF_DEBUG_CATEGORY[%s]=%d", "CMD_IO",
		    nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CMD_IO]);
	XNVME_DEBUG("INFO: NVMF debug initialization complete");

	return 0;
}
#endif
