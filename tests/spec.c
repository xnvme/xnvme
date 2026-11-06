// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <string.h>
#include <libxnvme.h>

/**
 * The masks of each field of bits fit the field, which the structure must still
 * have: a field renamed or narrowed without its masks fails the build
 */
#define FITS(type, field, mask)                                                       \
	((uint64_t)(mask) / ((uint64_t)1 << (4 * XNVME_SIZEOF_MEMBER(type, field))) / \
		 ((uint64_t)1 << (4 * XNVME_SIZEOF_MEMBER(type, field))) ==           \
	 0)

XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cmd_sanitize, cdw10,
			 XNVME_SPEC_CMD_SANITIZE_CDW10_SANACT |
				 XNVME_SPEC_CMD_SANITIZE_CDW10_AUSE |
				 XNVME_SPEC_CMD_SANITIZE_CDW10_OWPASS |
				 XNVME_SPEC_CMD_SANITIZE_CDW10_OIPBP |
				 XNVME_SPEC_CMD_SANITIZE_CDW10_NDAS),
		    "cdw10 of struct xnvme_spec_cmd_sanitize")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_sanitize, cdw10) == 40,
		    "cdw10 of struct xnvme_spec_cmd_sanitize")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_sanitize, ovrpat) == 44,
		    "ovrpat of struct xnvme_spec_cmd_sanitize")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cmd_format, cdw10,
			 XNVME_SPEC_CMD_FORMAT_CDW10_LBAFL | XNVME_SPEC_CMD_FORMAT_CDW10_MSET |
				 XNVME_SPEC_CMD_FORMAT_CDW10_PI | XNVME_SPEC_CMD_FORMAT_CDW10_PIL |
				 XNVME_SPEC_CMD_FORMAT_CDW10_SES |
				 XNVME_SPEC_CMD_FORMAT_CDW10_LBAFU),
		    "cdw10 of struct xnvme_spec_cmd_format")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_format, cdw10) == 40,
		    "cdw10 of struct xnvme_spec_cmd_format")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cmd_gfeat, cdw10,
			 XNVME_SPEC_CMD_GFEAT_CDW10_FID | XNVME_SPEC_CMD_GFEAT_CDW10_SEL),
		    "cdw10 of struct xnvme_spec_cmd_gfeat")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_gfeat, cdw10) == 40,
		    "cdw10 of struct xnvme_spec_cmd_gfeat")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cmd_sfeat, cdw10,
			 XNVME_SPEC_CMD_SFEAT_CDW10_FID | XNVME_SPEC_CMD_SFEAT_CDW10_SV),
		    "cdw10 of struct xnvme_spec_cmd_sfeat")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_sfeat, cdw10) == 40,
		    "cdw10 of struct xnvme_spec_cmd_sfeat")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_sfeat, feat) == 44,
		    "feat of struct xnvme_spec_cmd_sfeat")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cmd_dsend, cdw12,
			 XNVME_SPEC_CMD_DSEND_CDW12_ENDIR | XNVME_SPEC_CMD_DSEND_CDW12_TDTYPE),
		    "cdw12 of struct xnvme_spec_cmd_dsend")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_dsend, doper) == 44,
		    "doper of struct xnvme_spec_cmd_dsend")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_dsend, dtype) == 45,
		    "dtype of struct xnvme_spec_cmd_dsend")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_dsend, dspec) == 46,
		    "dspec of struct xnvme_spec_cmd_dsend")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_dsend, cdw12) == 48,
		    "cdw12 of struct xnvme_spec_cmd_dsend")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cmd_log, cdw10,
			 XNVME_SPEC_CMD_LOG_CDW10_LID | XNVME_SPEC_CMD_LOG_CDW10_LSP |
				 XNVME_SPEC_CMD_LOG_CDW10_RAE | XNVME_SPEC_CMD_LOG_CDW10_NUMDL),
		    "cdw10 of struct xnvme_spec_cmd_log")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cmd_log, cdw14,
			 XNVME_SPEC_CMD_LOG_CDW14_UUIDX | XNVME_SPEC_CMD_LOG_CDW14_OT |
				 XNVME_SPEC_CMD_LOG_CDW14_CSI),
		    "cdw14 of struct xnvme_spec_cmd_log")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_log, cdw10) == 40,
		    "cdw10 of struct xnvme_spec_cmd_log")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_log, lsi) == 46,
		    "lsi of struct xnvme_spec_cmd_log")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_log, cdw14) == 56,
		    "cdw14 of struct xnvme_spec_cmd_log")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cmd_idfy, cdw14, XNVME_SPEC_CMD_IDFY_CDW14_UUID),
		    "cdw14 of struct xnvme_spec_cmd_idfy")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_idfy, cns) == 40,
		    "cns of struct xnvme_spec_cmd_idfy")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_idfy, cntid) == 42,
		    "cntid of struct xnvme_spec_cmd_idfy")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_idfy, nvmsetid) == 44,
		    "nvmsetid of struct xnvme_spec_cmd_idfy")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_idfy, csi) == 47,
		    "csi of struct xnvme_spec_cmd_idfy")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_idfy, cdw14) == 56,
		    "cdw14 of struct xnvme_spec_cmd_idfy")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cmd_common, flags,
			 XNVME_SPEC_CMD_COMMON_FLAGS_FUSE | XNVME_SPEC_CMD_COMMON_FLAGS_PSDT),
		    "flags of struct xnvme_spec_cmd_common")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_common, flags) == 1,
		    "flags of struct xnvme_spec_cmd_common")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_common, dptr) == 24,
		    "dptr of struct xnvme_spec_cmd_common")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cqe, status,
			 XNVME_SPEC_CQE_STATUS_P | XNVME_SPEC_CQE_STATUS_SC |
				 XNVME_SPEC_CQE_STATUS_SCT | XNVME_SPEC_CQE_STATUS_CRD |
				 XNVME_SPEC_CQE_STATUS_M | XNVME_SPEC_CQE_STATUS_DNR),
		    "status of struct xnvme_spec_cqe")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cqe, status) == 14,
		    "status of struct xnvme_spec_cqe")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_log_erri_entry, status,
			 XNVME_SPEC_LOG_ERRI_ENTRY_STATUS_P | XNVME_SPEC_LOG_ERRI_ENTRY_STATUS_SC |
				 XNVME_SPEC_LOG_ERRI_ENTRY_STATUS_SCT |
				 XNVME_SPEC_LOG_ERRI_ENTRY_STATUS_CRD |
				 XNVME_SPEC_LOG_ERRI_ENTRY_STATUS_M |
				 XNVME_SPEC_LOG_ERRI_ENTRY_STATUS_DNR),
		    "status of struct xnvme_spec_log_erri_entry")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_log_erri_entry, status) == 12,
		    "status of struct xnvme_spec_log_erri_entry")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_lbaf, rp, XNVME_SPEC_LBAF_RP_RP),
		    "rp of struct xnvme_spec_lbaf")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, nsfeat,
			 XNVME_SPEC_IDFY_NS_NSFEAT_THIN_PROV |
				 XNVME_SPEC_IDFY_NS_NSFEAT_NS_ATOMIC_WRITE_UNIT |
				 XNVME_SPEC_IDFY_NS_NSFEAT_DEALLOC_OR_UNWRITTEN_ERROR |
				 XNVME_SPEC_IDFY_NS_NSFEAT_GUID_NEVER_REUSED |
				 XNVME_SPEC_IDFY_NS_NSFEAT_OPTIMAL_PERFORMANCE),
		    "nsfeat of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, flbas,
			 XNVME_SPEC_IDFY_NS_FLBAS_FORMAT | XNVME_SPEC_IDFY_NS_FLBAS_EXTENDED |
				 XNVME_SPEC_IDFY_NS_FLBAS_FORMAT_MSB),
		    "flbas of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, mc,
			 XNVME_SPEC_IDFY_NS_MC_EXTENDED | XNVME_SPEC_IDFY_NS_MC_POINTER),
		    "mc of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, dpc,
			 XNVME_SPEC_IDFY_NS_DPC_PIT1 | XNVME_SPEC_IDFY_NS_DPC_PIT2 |
				 XNVME_SPEC_IDFY_NS_DPC_PIT3 | XNVME_SPEC_IDFY_NS_DPC_MD_START |
				 XNVME_SPEC_IDFY_NS_DPC_MD_END),
		    "dpc of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, dps,
			 XNVME_SPEC_IDFY_NS_DPS_PIT | XNVME_SPEC_IDFY_NS_DPS_MD_START),
		    "dps of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, nmic, XNVME_SPEC_IDFY_NS_NMIC_CAN_SHARE),
		    "nmic of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, nsrescap,
			 XNVME_SPEC_IDFY_NS_NSRESCAP_PERSIST |
				 XNVME_SPEC_IDFY_NS_NSRESCAP_WRITE_EXCLUSIVE |
				 XNVME_SPEC_IDFY_NS_NSRESCAP_EXCLUSIVE_ACCESS |
				 XNVME_SPEC_IDFY_NS_NSRESCAP_WRITE_EXCLUSIVE_REG_ONLY |
				 XNVME_SPEC_IDFY_NS_NSRESCAP_EXCLUSIVE_ACCESS_REG_ONLY |
				 XNVME_SPEC_IDFY_NS_NSRESCAP_WRITE_EXCLUSIVE_ALL_REG |
				 XNVME_SPEC_IDFY_NS_NSRESCAP_EXCLUSIVE_ACCESS_ALL_REG |
				 XNVME_SPEC_IDFY_NS_NSRESCAP_IGNORE_EXISTING_KEY),
		    "nsrescap of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, fpi,
			 XNVME_SPEC_IDFY_NS_FPI_PERCENTAGE_REMAINING |
				 XNVME_SPEC_IDFY_NS_FPI_FPI_SUPPORTED),
		    "fpi of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, dlfeat,
			 XNVME_SPEC_IDFY_NS_DLFEAT_READ_VALUE |
				 XNVME_SPEC_IDFY_NS_DLFEAT_WRITE_ZERO_DEALLOCATE |
				 XNVME_SPEC_IDFY_NS_DLFEAT_GUARD_VALUE),
		    "dlfeat of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_elbaf, bits,
			 XNVME_SPEC_ELBAF_BITS_STS | XNVME_SPEC_ELBAF_BITS_PIF),
		    "bits of struct xnvme_spec_elbaf")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_power_state, mxps_nops,
			 XNVME_SPEC_POWER_STATE_MXPS_NOPS_MXPS |
				 XNVME_SPEC_POWER_STATE_MXPS_NOPS_NOPS),
		    "mxps_nops of struct xnvme_spec_power_state")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_power_state, rrt, XNVME_SPEC_POWER_STATE_RRT_RRT),
		    "rrt of struct xnvme_spec_power_state")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_power_state, rrl, XNVME_SPEC_POWER_STATE_RRL_RRL),
		    "rrl of struct xnvme_spec_power_state")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_power_state, rwt, XNVME_SPEC_POWER_STATE_RWT_RWT),
		    "rwt of struct xnvme_spec_power_state")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_power_state, rwl, XNVME_SPEC_POWER_STATE_RWL_RWL),
		    "rwl of struct xnvme_spec_power_state")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_power_state, ips, XNVME_SPEC_POWER_STATE_IPS_IPS),
		    "ips of struct xnvme_spec_power_state")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_power_state, apw_aps,
			 XNVME_SPEC_POWER_STATE_APW_APS_APW | XNVME_SPEC_POWER_STATE_APW_APS_APS),
		    "apw_aps of struct xnvme_spec_power_state")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr_nvmf, ctrattr,
			 XNVME_SPEC_IDFY_CTRLR_NVMF_CTRATTR_CTRLR_MODEL),
		    "ctrattr of struct xnvme_spec_idfy_ctrlr_nvmf")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr_nvmf, ofcs,
			 XNVME_SPEC_IDFY_CTRLR_NVMF_OFCS_DISC_DEL),
		    "ofcs of struct xnvme_spec_idfy_ctrlr_nvmf")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, cmic,
			 XNVME_SPEC_IDFY_CTRLR_CMIC_MULTI_PORT |
				 XNVME_SPEC_IDFY_CTRLR_CMIC_MULTI_HOST |
				 XNVME_SPEC_IDFY_CTRLR_CMIC_SR_IOV |
				 XNVME_SPEC_IDFY_CTRLR_CMIC_ANA_RPRT),
		    "cmic of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, ver,
			 XNVME_SPEC_IDFY_CTRLR_VER_TER | XNVME_SPEC_IDFY_CTRLR_VER_MNR |
				 XNVME_SPEC_IDFY_CTRLR_VER_MJR),
		    "ver of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, oaes,
			 XNVME_SPEC_IDFY_CTRLR_OAES_NS_ATTRIBUTE_NOTICES |
				 XNVME_SPEC_IDFY_CTRLR_OAES_FW_ACTIVATION_NOTICES |
				 XNVME_SPEC_IDFY_CTRLR_OAES_ANA_NOTICES |
				 XNVME_SPEC_IDFY_CTRLR_OAES_PLEAL_NOTICES |
				 XNVME_SPEC_IDFY_CTRLR_OAES_LBA_SIA_NOTICES |
				 XNVME_SPEC_IDFY_CTRLR_OAES_EGEA_NOTICES |
				 XNVME_SPEC_IDFY_CTRLR_OAES_NORMAL_NVM_SS |
				 XNVME_SPEC_IDFY_CTRLR_OAES_ZONE_CHANGES |
				 XNVME_SPEC_IDFY_CTRLR_OAES_DISCOVERY_LOG_NOTICES),
		    "oaes of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(
	FITS(struct xnvme_spec_idfy_ctrlr, ctratt,
	     XNVME_SPEC_IDFY_CTRLR_CTRATT_HOST_ID_EXHID_SUPPORTED |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_NON_OPERATIONAL_POWER_STATE_PERMISSIVE_MODE |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_NVM_SETS |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_READ_RECOVERY_LEVELS |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_ENDURANCE_GROUPS |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_PREDICTABLE_LATENCY_MODE |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_TBKAS |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_NAMESPACE_GRANULARITY |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_SQ_ASSOCIATIONS |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_UUID_LIST |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_MULTI_DOMAIN_SUBSYSTEM |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_FIXED_CAPACITY_MANAGEMENT |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_VARIABLE_CAPACITY_MANAGEMENT |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_DELETE_ENDURANCE_GROUP |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_DELETE_NVM_SET |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_EXTENDED_LBA_FORMATS |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_FLEXIBLE_DATA_PLACEMENT),
	"ctratt of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, nvmsr,
			 XNVME_SPEC_IDFY_CTRLR_NVMSR_NVMESD | XNVME_SPEC_IDFY_CTRLR_NVMSR_NVMEE),
		    "nvmsr of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, vwci,
			 XNVME_SPEC_IDFY_CTRLR_VWCI_VWCR | XNVME_SPEC_IDFY_CTRLR_VWCI_VWCRV),
		    "vwci of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, mec,
			 XNVME_SPEC_IDFY_CTRLR_MEC_SMBUSME | XNVME_SPEC_IDFY_CTRLR_MEC_PCIEME),
		    "mec of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, oacs,
			 XNVME_SPEC_IDFY_CTRLR_OACS_SECURITY | XNVME_SPEC_IDFY_CTRLR_OACS_FORMAT |
				 XNVME_SPEC_IDFY_CTRLR_OACS_FIRMWARE |
				 XNVME_SPEC_IDFY_CTRLR_OACS_NS_MANAGE |
				 XNVME_SPEC_IDFY_CTRLR_OACS_DEVICE_SELF_TEST |
				 XNVME_SPEC_IDFY_CTRLR_OACS_DIRECTIVES |
				 XNVME_SPEC_IDFY_CTRLR_OACS_NVME_MI |
				 XNVME_SPEC_IDFY_CTRLR_OACS_VIRTUALIZATION_MANAGEMENT |
				 XNVME_SPEC_IDFY_CTRLR_OACS_DOORBELL_BUFFER_CONFIG |
				 XNVME_SPEC_IDFY_CTRLR_OACS_LBA_STATUS |
				 XNVME_SPEC_IDFY_CTRLR_OACS_CMD_FEATURE_LOCKDOWN),
		    "oacs of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, frmw,
			 XNVME_SPEC_IDFY_CTRLR_FRMW_SLOT1_RO |
				 XNVME_SPEC_IDFY_CTRLR_FRMW_NUM_SLOTS |
				 XNVME_SPEC_IDFY_CTRLR_FRMW_ACTIVATION_WITHOUT_RESET |
				 XNVME_SPEC_IDFY_CTRLR_FRMW_MUL_UPDATE_DETECTION),
		    "frmw of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, lpa,
			 XNVME_SPEC_IDFY_CTRLR_LPA_NS_SMART | XNVME_SPEC_IDFY_CTRLR_LPA_CELP |
				 XNVME_SPEC_IDFY_CTRLR_LPA_EDLP |
				 XNVME_SPEC_IDFY_CTRLR_LPA_TELEMETRY |
				 XNVME_SPEC_IDFY_CTRLR_LPA_PEL | XNVME_SPEC_IDFY_CTRLR_LPA_MEL |
				 XNVME_SPEC_IDFY_CTRLR_LPA_TEL_DA4),
		    "lpa of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, avscc,
			 XNVME_SPEC_IDFY_CTRLR_AVSCC_SPEC_FORMAT),
		    "avscc of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, apsta,
			 XNVME_SPEC_IDFY_CTRLR_APSTA_SUPPORTED),
		    "apsta of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, rpmbs,
			 XNVME_SPEC_IDFY_CTRLR_RPMBS_NUM_RPMB_UNITS |
				 XNVME_SPEC_IDFY_CTRLR_RPMBS_AUTH_METHOD |
				 XNVME_SPEC_IDFY_CTRLR_RPMBS_TOTAL_SIZE |
				 XNVME_SPEC_IDFY_CTRLR_RPMBS_ACCESS_SIZE),
		    "rpmbs of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, dsto, XNVME_SPEC_IDFY_CTRLR_DSTO_ONE_ONLY),
		    "dsto of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, hctma,
			 XNVME_SPEC_IDFY_CTRLR_HCTMA_SUPPORTED),
		    "hctma of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, sanicap,
			 XNVME_SPEC_IDFY_CTRLR_SANICAP_CRYPTO_ERASE |
				 XNVME_SPEC_IDFY_CTRLR_SANICAP_BLOCK_ERASE |
				 XNVME_SPEC_IDFY_CTRLR_SANICAP_OVERWRITE |
				 XNVME_SPEC_IDFY_CTRLR_SANICAP_NDI |
				 XNVME_SPEC_IDFY_CTRLR_SANICAP_NODMMAS),
		    "sanicap of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, anacap,
			 XNVME_SPEC_IDFY_CTRLR_ANACAP_OPTIMIZE |
				 XNVME_SPEC_IDFY_CTRLR_ANACAP_NON_OPTIMIZE |
				 XNVME_SPEC_IDFY_CTRLR_ANACAP_INACCESSIBLE |
				 XNVME_SPEC_IDFY_CTRLR_ANACAP_PERSIST_LOSS |
				 XNVME_SPEC_IDFY_CTRLR_ANACAP_CHANGE |
				 XNVME_SPEC_IDFY_CTRLR_ANACAP_NS_ANAGRPID |
				 XNVME_SPEC_IDFY_CTRLR_ANACAP_MGT_ANAGRPID),
		    "anacap of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, sqes,
			 XNVME_SPEC_IDFY_CTRLR_SQES_MIN | XNVME_SPEC_IDFY_CTRLR_SQES_MAX),
		    "sqes of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, cqes,
			 XNVME_SPEC_IDFY_CTRLR_CQES_MIN | XNVME_SPEC_IDFY_CTRLR_CQES_MAX),
		    "cqes of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(
	FITS(struct xnvme_spec_idfy_ctrlr, oncs,
	     XNVME_SPEC_IDFY_CTRLR_ONCS_COMPARE | XNVME_SPEC_IDFY_CTRLR_ONCS_WRITE_UNC |
		     XNVME_SPEC_IDFY_CTRLR_ONCS_DSM | XNVME_SPEC_IDFY_CTRLR_ONCS_WRITE_ZEROES |
		     XNVME_SPEC_IDFY_CTRLR_ONCS_SET_FEATURES_SAVE |
		     XNVME_SPEC_IDFY_CTRLR_ONCS_RESERVATIONS |
		     XNVME_SPEC_IDFY_CTRLR_ONCS_TIMESTAMP | XNVME_SPEC_IDFY_CTRLR_ONCS_VERIFY |
		     XNVME_SPEC_IDFY_CTRLR_ONCS_COPY),
	"oncs of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, fna,
			 XNVME_SPEC_IDFY_CTRLR_FNA_FORMAT_ALL_NS |
				 XNVME_SPEC_IDFY_CTRLR_FNA_ERASE_ALL_NS |
				 XNVME_SPEC_IDFY_CTRLR_FNA_CRYPTO_ERASE_SUPPORTED |
				 XNVME_SPEC_IDFY_CTRLR_FNA_NSID_FFFFFFFF),
		    "fna of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, vwc,
			 XNVME_SPEC_IDFY_CTRLR_VWC_PRESENT |
				 XNVME_SPEC_IDFY_CTRLR_VWC_FLUSH_BROADCAST),
		    "vwc of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, cdfs,
			 XNVME_SPEC_IDFY_CTRLR_CDFS_FORMAT0 | XNVME_SPEC_IDFY_CTRLR_CDFS_FORMAT1),
		    "cdfs of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, sgls,
			 XNVME_SPEC_IDFY_CTRLR_SGLS_SUPPORTED |
				 XNVME_SPEC_IDFY_CTRLR_SGLS_KEYED_SGL |
				 XNVME_SPEC_IDFY_CTRLR_SGLS_SGL_DESC_THRESHOLD |
				 XNVME_SPEC_IDFY_CTRLR_SGLS_BIT_BUCKET_DESCRIPTOR |
				 XNVME_SPEC_IDFY_CTRLR_SGLS_METADATA_POINTER |
				 XNVME_SPEC_IDFY_CTRLR_SGLS_OVERSIZED_SGL |
				 XNVME_SPEC_IDFY_CTRLR_SGLS_METADATA_ADDRESS |
				 XNVME_SPEC_IDFY_CTRLR_SGLS_SGL_OFFSET |
				 XNVME_SPEC_IDFY_CTRLR_SGLS_TRANSPORT_SGL),
		    "sgls of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cs_vector, bits,
			 XNVME_SPEC_CS_VECTOR_BITS_NVM | XNVME_SPEC_CS_VECTOR_BITS_KV |
				 XNVME_SPEC_CS_VECTOR_BITS_ZNS),
		    "bits of struct xnvme_spec_cs_vector")

struct offset_case {
	const char *name;
	size_t offset; ///< As the structure has it
	size_t spec;   ///< As the specification has it
};

#define OFFSET(type, field, spec) {#type "." #field, offsetof(type, field), spec}

static const struct offset_case g_offsets[] = {
	OFFSET(struct xnvme_spec_idfy_ns, nsfeat, 24),
	OFFSET(struct xnvme_spec_idfy_ns, flbas, 26),
	OFFSET(struct xnvme_spec_idfy_ns, dps, 29),
	OFFSET(struct xnvme_spec_idfy_ns, dlfeat, 33),
	OFFSET(struct xnvme_spec_idfy_ns, lbaf, 128),
	OFFSET(struct xnvme_spec_idfy_ctrlr, cmic, 76),
	OFFSET(struct xnvme_spec_idfy_ctrlr, ver, 80),
	OFFSET(struct xnvme_spec_idfy_ctrlr, oaes, 92),
	OFFSET(struct xnvme_spec_idfy_ctrlr, ctratt, 96),
	OFFSET(struct xnvme_spec_idfy_ctrlr, oacs, 256),
	OFFSET(struct xnvme_spec_idfy_ctrlr, lpa, 261),
	OFFSET(struct xnvme_spec_idfy_ctrlr, sanicap, 328),
	OFFSET(struct xnvme_spec_idfy_ctrlr, sqes, 512),
	OFFSET(struct xnvme_spec_idfy_ctrlr, oncs, 520),
	OFFSET(struct xnvme_spec_idfy_ctrlr, vwc, 525),
	OFFSET(struct xnvme_spec_idfy_ctrlr, sgls, 536),
	OFFSET(struct xnvme_spec_idfy_ctrlr, nvmf_specific, 1792),
	OFFSET(struct xnvme_spec_idfy_ctrlr, psd, 2048),
	OFFSET(struct xnvme_spec_power_state, mxps_nops, 3),
	OFFSET(struct xnvme_spec_power_state, apw_aps, 22),
};

static int
sub_idfy_offsets(struct xnvme_cli *XNVME_UNUSED(cli))
{
	int err = 0;

	for (size_t i = 0; i < sizeof(g_offsets) / sizeof(*g_offsets); ++i) {
		if (g_offsets[i].offset != g_offsets[i].spec) {
			xnvme_cli_pinf("FAILED: %s: offset %zu != %zu", g_offsets[i].name,
				       g_offsets[i].offset, g_offsets[i].spec);
			err = -EINVAL;
		}
	}

	return err;
}

static int
sub_idfy_masks(struct xnvme_cli *XNVME_UNUSED(cli))
{
	union xnvme_spec_idfy idfy;
	int err = 0;

	memset(&idfy, 0, sizeof(idfy));
	idfy.raw[26] = 0x5a; // Identify Namespace FLBAS: format 0xa, extended, format MSB 0x2
	if (xnvme_bits_get(idfy.ns.flbas, XNVME_SPEC_IDFY_NS_FLBAS_FORMAT) != 0xa ||
	    !xnvme_bits_get(idfy.ns.flbas, XNVME_SPEC_IDFY_NS_FLBAS_EXTENDED) ||
	    xnvme_bits_get(idfy.ns.flbas, XNVME_SPEC_IDFY_NS_FLBAS_FORMAT_MSB) != 0x2) {
		xnvme_cli_pinf("FAILED: flbas: %#" PRIx8, idfy.ns.flbas);
		err = -EINVAL;
	}

	memset(&idfy, 0, sizeof(idfy));
	idfy.raw[521] = 0x01; // Identify Controller ONCS bit 8: copy
	if (!xnvme_bits_get(idfy.ctrlr.oncs, XNVME_SPEC_IDFY_CTRLR_ONCS_COPY) ||
	    xnvme_bits_get(idfy.ctrlr.oncs, XNVME_SPEC_IDFY_CTRLR_ONCS_COMPARE)) {
		xnvme_cli_pinf("FAILED: oncs: %#" PRIx16, idfy.ctrlr.oncs);
		err = -EINVAL;
	}

	memset(&idfy, 0, sizeof(idfy));
	idfy.raw[331] = 0xc0; // Identify Controller SANICAP bits 31:30: NODMMAS
	if (xnvme_bits_get(idfy.ctrlr.sanicap, XNVME_SPEC_IDFY_CTRLR_SANICAP_NODMMAS) != 0x3) {
		xnvme_cli_pinf("FAILED: sanicap: %#" PRIx32, idfy.ctrlr.sanicap);
		err = -EINVAL;
	}

	return err;
}

static int
sub_status_masks(struct xnvme_cli *XNVME_UNUSED(cli))
{
	uint8_t raw[sizeof(struct xnvme_spec_cqe)] = {0};
	struct xnvme_spec_cqe cpl;
	int err = 0;

	raw[14] = 0x05; // Completion Queue Entry DW3 bits 31:16: phase tag, status code 0x2,
	raw[15] = 0x82; // status code type 0x1, do not retry
	memcpy(&cpl, raw, sizeof(cpl));

	if (!xnvme_bits_get(cpl.status, XNVME_SPEC_CQE_STATUS_P) ||
	    xnvme_bits_get(cpl.status, XNVME_SPEC_CQE_STATUS_SC) != 0x2 ||
	    xnvme_bits_get(cpl.status, XNVME_SPEC_CQE_STATUS_SCT) != 0x1 ||
	    xnvme_bits_get(cpl.status, XNVME_SPEC_CQE_STATUS_CRD) ||
	    xnvme_bits_get(cpl.status, XNVME_SPEC_CQE_STATUS_M) ||
	    !xnvme_bits_get(cpl.status, XNVME_SPEC_CQE_STATUS_DNR)) {
		xnvme_cli_pinf("FAILED: status: %#" PRIx16, cpl.status);
		err = -EINVAL;
	}

	return err;
}

static int
sub_sqe_dwords(struct xnvme_cli *XNVME_UNUSED(cli))
{
	union xnvme_spec_sqe sqe = {0};
	int err = 0;

	sqe.cdw[0] = 0x4102;     // Dword 0: opcode 0x02, FUSE 0x1, PSDT 0x1
	sqe.cdw[1] = 0x12345678; // Dword 1: namespace identifier
	sqe.cdw[10] = 0x1;       // Dwords 10 and 11: starting LBA
	sqe.cdw[11] = 0x2;

	if (sqe.common.opcode != 0x02 ||
	    xnvme_bits_get(sqe.common.flags, XNVME_SPEC_CMD_COMMON_FLAGS_FUSE) != 0x1 ||
	    xnvme_bits_get(sqe.common.flags, XNVME_SPEC_CMD_COMMON_FLAGS_PSDT) != 0x1 ||
	    sqe.common.nsid != 0x12345678 || sqe.nvm.slba != 0x200000001ULL) {
		xnvme_cli_pinf("FAILED: opcode: %#x, nsid: %#x, slba: %#" PRIx64,
			       sqe.common.opcode, sqe.common.nsid, (uint64_t)sqe.nvm.slba);
		err = -EINVAL;
	}

	memset(&sqe, 0, sizeof(sqe));
	sqe.cdw[10] = 0x00020106; // Identify dword 10: CNS 0x06, CNTID 0x2
	sqe.cdw[11] = 0x05000003; // Identify dword 11: NVM set 0x3, CSI 0x5
	sqe.cdw[14] = 0xffffff7f; // Identify dword 14, bits 6:0: UUID index 0x7f
	if (sqe.idfy.cns != 0x06 || sqe.idfy.cntid != 0x2 || sqe.idfy.nvmsetid != 0x3 ||
	    sqe.idfy.csi != 0x5 ||
	    xnvme_bits_get(sqe.idfy.cdw14, XNVME_SPEC_CMD_IDFY_CDW14_UUID) != 0x7f) {
		xnvme_cli_pinf("FAILED: idfy: cns: %#x, cntid: %#x, nvmsetid: %#x, csi: %#x",
			       sqe.idfy.cns, sqe.idfy.cntid, sqe.idfy.nvmsetid, sqe.idfy.csi);
		err = -EINVAL;
	}

	memset(&sqe, 0, sizeof(sqe));
	sqe.cdw[10] = 0x00ffc502; // Get Log Page dword 10: LID 0x02, LSP 0x45, RAE, NUMDL 0xff
	sqe.cdw[14] = 0x02800003; // Get Log Page dword 14: UUID index 0x3, OT, CSI 0x02
	if (xnvme_bits_get(sqe.log.cdw10, XNVME_SPEC_CMD_LOG_CDW10_LID) != 0x02 ||
	    xnvme_bits_get(sqe.log.cdw10, XNVME_SPEC_CMD_LOG_CDW10_LSP) != 0x45 ||
	    !xnvme_bits_get(sqe.log.cdw10, XNVME_SPEC_CMD_LOG_CDW10_RAE) ||
	    xnvme_bits_get(sqe.log.cdw10, XNVME_SPEC_CMD_LOG_CDW10_NUMDL) != 0xff ||
	    xnvme_bits_get(sqe.log.cdw14, XNVME_SPEC_CMD_LOG_CDW14_UUIDX) != 0x3 ||
	    !xnvme_bits_get(sqe.log.cdw14, XNVME_SPEC_CMD_LOG_CDW14_OT) ||
	    xnvme_bits_get(sqe.log.cdw14, XNVME_SPEC_CMD_LOG_CDW14_CSI) != 0x02) {
		xnvme_cli_pinf("FAILED: log: cdw10: %#x, cdw14: %#x", sqe.log.cdw10,
			       sqe.log.cdw14);
		err = -EINVAL;
	}

	memset(&sqe, 0, sizeof(sqe));
	sqe.cdw[10] = 0x00002b35; // Format dword 10: LBAFL 5, MSET, PI 1, PIL, SES 5, LBAFU 2
	if (xnvme_bits_get(sqe.format.cdw10, XNVME_SPEC_CMD_FORMAT_CDW10_LBAFL) != 0x5 ||
	    !xnvme_bits_get(sqe.format.cdw10, XNVME_SPEC_CMD_FORMAT_CDW10_MSET) ||
	    xnvme_bits_get(sqe.format.cdw10, XNVME_SPEC_CMD_FORMAT_CDW10_PI) != 0x1 ||
	    !xnvme_bits_get(sqe.format.cdw10, XNVME_SPEC_CMD_FORMAT_CDW10_PIL) ||
	    xnvme_bits_get(sqe.format.cdw10, XNVME_SPEC_CMD_FORMAT_CDW10_SES) != 0x5 ||
	    xnvme_bits_get(sqe.format.cdw10, XNVME_SPEC_CMD_FORMAT_CDW10_LBAFU) != 0x2) {
		xnvme_cli_pinf("FAILED: format: cdw10: %#x", sqe.format.cdw10);
		err = -EINVAL;
	}

	memset(&sqe, 0, sizeof(sqe));
	sqe.cdw[10] = 0x0000035a; // Sanitize dword 10: SANACT 2, AUSE, OWPASS 5, OIPBP, NDAS
	if (xnvme_bits_get(sqe.sanitize.cdw10, XNVME_SPEC_CMD_SANITIZE_CDW10_SANACT) != 0x2 ||
	    !xnvme_bits_get(sqe.sanitize.cdw10, XNVME_SPEC_CMD_SANITIZE_CDW10_AUSE) ||
	    xnvme_bits_get(sqe.sanitize.cdw10, XNVME_SPEC_CMD_SANITIZE_CDW10_OWPASS) != 0x5 ||
	    !xnvme_bits_get(sqe.sanitize.cdw10, XNVME_SPEC_CMD_SANITIZE_CDW10_OIPBP) ||
	    !xnvme_bits_get(sqe.sanitize.cdw10, XNVME_SPEC_CMD_SANITIZE_CDW10_NDAS)) {
		xnvme_cli_pinf("FAILED: sanitize: cdw10: %#x", sqe.sanitize.cdw10);
		err = -EINVAL;
	}

	memset(&sqe, 0, sizeof(sqe));
	sqe.cdw[10] = 0x00000307; // Get Features dword 10: FID 0x07, SEL 0x3
	if (xnvme_bits_get(sqe.gfeat.cdw10, XNVME_SPEC_CMD_GFEAT_CDW10_FID) != 0x07 ||
	    xnvme_bits_get(sqe.gfeat.cdw10, XNVME_SPEC_CMD_GFEAT_CDW10_SEL) != 0x3) {
		xnvme_cli_pinf("FAILED: gfeat: cdw10: %#x", sqe.gfeat.cdw10);
		err = -EINVAL;
	}

	memset(&sqe, 0, sizeof(sqe));
	sqe.cdw[10] = 0x8000001d; // Set Features dword 10: FID 0x1d, SV
	if (xnvme_bits_get(sqe.sfeat.cdw10, XNVME_SPEC_CMD_SFEAT_CDW10_FID) != 0x1d ||
	    !xnvme_bits_get(sqe.sfeat.cdw10, XNVME_SPEC_CMD_SFEAT_CDW10_SV)) {
		xnvme_cli_pinf("FAILED: sfeat: cdw10: %#x", sqe.sfeat.cdw10);
		err = -EINVAL;
	}

	memset(&sqe, 0, sizeof(sqe));
	sqe.cdw[11] = 0xbeef0001; // Directive Send dword 11: DOPER 0x01, DTYPE 0x00, DSPEC 0xbeef
	sqe.cdw[12] = 0x00000101; // Directive Send dword 12: ENDIR, TDTYPE 0x01
	if (sqe.dsend.doper != 0x01 || sqe.dsend.dtype != 0x00 || sqe.dsend.dspec != 0xbeef ||
	    !xnvme_bits_get(sqe.dsend.cdw12, XNVME_SPEC_CMD_DSEND_CDW12_ENDIR) ||
	    xnvme_bits_get(sqe.dsend.cdw12, XNVME_SPEC_CMD_DSEND_CDW12_TDTYPE) != 0x01) {
		xnvme_cli_pinf("FAILED: dsend: doper: %#x, dtype: %#x, dspec: %#x, cdw12: %#x",
			       sqe.dsend.doper, sqe.dsend.dtype, sqe.dsend.dspec, sqe.dsend.cdw12);
		err = -EINVAL;
	}

	return err;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"idfy-offsets",
		"Check the offsets of the identify fields of bits",
		"Check the offsets of the identify fields of bits",
		sub_idfy_offsets,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
	{
		"idfy-masks",
		"Read identify fields of bits through their masks",
		"Read identify fields of bits through their masks",
		sub_idfy_masks,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
	{
		"sqe-dwords",
		"Read the submission queue entry through its dwords and its views",
		"Read the submission queue entry through its dwords and its views",
		sub_sqe_dwords,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
	{
		"status-masks",
		"Read the completion status through its masks",
		"Read the completion status through its masks",
		sub_status_masks,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Exercise the layout of the specification structures",
	.descr_short = "Exercise the layout of the specification structures",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
