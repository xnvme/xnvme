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
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cmd_drecv, cdw12, XNVME_SPEC_CMD_DRECV_CDW12_NSR),
		    "cdw12 of struct xnvme_spec_cmd_drecv")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_drecv, doper) == 44,
		    "doper of struct xnvme_spec_cmd_drecv")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_drecv, dtype) == 45,
		    "dtype of struct xnvme_spec_cmd_drecv")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_drecv, dspec) == 46,
		    "dspec of struct xnvme_spec_cmd_drecv")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_drecv, cdw12) == 48,
		    "cdw12 of struct xnvme_spec_cmd_drecv")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_alloc_resource, nsa) == 0,
		    "nsa of struct xnvme_spec_alloc_resource")
XNVME_STATIC_ASSERT(FITS(union xnvme_spec_feat, temp_threshold,
			 XNVME_SPEC_FEAT_TEMP_THRESHOLD_TMPTH |
				 XNVME_SPEC_FEAT_TEMP_THRESHOLD_TMPSEL |
				 XNVME_SPEC_FEAT_TEMP_THRESHOLD_THSEL),
		    "temp_threshold of union xnvme_spec_feat")
XNVME_STATIC_ASSERT(FITS(union xnvme_spec_feat, error_recovery,
			 XNVME_SPEC_FEAT_ERROR_RECOVERY_TLER |
				 XNVME_SPEC_FEAT_ERROR_RECOVERY_DULBE),
		    "error_recovery of union xnvme_spec_feat")
XNVME_STATIC_ASSERT(FITS(union xnvme_spec_feat, fdp_mode,
			 XNVME_SPEC_FEAT_FDP_MODE_FDPE | XNVME_SPEC_FEAT_FDP_MODE_FDPCI),
		    "fdp_mode of union xnvme_spec_feat")
XNVME_STATIC_ASSERT(FITS(union xnvme_spec_feat, media_feedback,
			 XNVME_SPEC_FEAT_MEDIA_FEEDBACK_HECC |
				 XNVME_SPEC_FEAT_MEDIA_FEEDBACK_VHECC),
		    "media_feedback of union xnvme_spec_feat")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_feat_nqueues, ncqa) == 2,
		    "ncqa of struct xnvme_spec_feat_nqueues")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cmd_nvm, cdw12,
			 XNVME_SPEC_CMD_NVM_CDW12_NLB | XNVME_SPEC_CMD_NVM_CDW12_DTYPE |
				 XNVME_SPEC_CMD_NVM_CDW12_PRINFO | XNVME_SPEC_CMD_NVM_CDW12_FUA |
				 XNVME_SPEC_CMD_NVM_CDW12_LR),
		    "cdw12 of struct xnvme_spec_cmd_nvm")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_nvm, cdw12) == 48,
		    "cdw12 of struct xnvme_spec_cmd_nvm")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cmd_nvm, cdw13,
			 XNVME_SPEC_CMD_NVM_CDW13_AF | XNVME_SPEC_CMD_NVM_CDW13_AL |
				 XNVME_SPEC_CMD_NVM_CDW13_SR | XNVME_SPEC_CMD_NVM_CDW13_INCOM |
				 XNVME_SPEC_CMD_NVM_CDW13_DSPEC),
		    "cdw13 of struct xnvme_spec_cmd_nvm")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_nvm, cdw13) == 52,
		    "cdw13 of struct xnvme_spec_cmd_nvm")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_nvm, ilbrt) == 56,
		    "ilbrt of struct xnvme_spec_cmd_nvm")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_nvm, lbatm) == 62,
		    "lbatm of struct xnvme_spec_cmd_nvm")
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
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_fdp_conf_desc, fdpa,
			 XNVME_SPEC_FDP_CONF_DESC_FDPA_RGIF |
				 XNVME_SPEC_FDP_CONF_DESC_FDPA_FDPVWC |
				 XNVME_SPEC_FDP_CONF_DESC_FDPA_FDPCV),
		    "fdpa of struct xnvme_spec_fdp_conf_desc")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_fdp_conf_desc, fdpa) == 2,
		    "fdpa of struct xnvme_spec_fdp_conf_desc")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_fdp_event_media_reallocated, sef,
			 XNVME_SPEC_FDP_EVENT_MEDIA_REALLOCATED_SEF_LBAV),
		    "sef of struct xnvme_spec_fdp_event_media_reallocated")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_fdp_event_media_reallocated, sef) == 0,
		    "sef of struct xnvme_spec_fdp_event_media_reallocated")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_fdp_event, fdpef,
			 XNVME_SPEC_FDP_EVENT_FDPEF_PIV | XNVME_SPEC_FDP_EVENT_FDPEF_NSIDV |
				 XNVME_SPEC_FDP_EVENT_FDPEF_LV),
		    "fdpef of struct xnvme_spec_fdp_event")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_fdp_event, fdpef) == 1,
		    "fdpef of struct xnvme_spec_fdp_event")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_fdp_event_desc, fdpeta,
			 XNVME_SPEC_FDP_EVENT_DESC_FDPETA_EE),
		    "fdpeta of struct xnvme_spec_fdp_event_desc")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_fdp_event_desc, fdpeta) == 1,
		    "fdpeta of struct xnvme_spec_fdp_event_desc")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_log_sanitize_entry, sstat,
			 XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_SOS |
				 XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_OPC |
				 XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_GDE |
				 XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_MVCNCLD),
		    "sstat of struct xnvme_spec_log_sanitize_entry")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_log_sanitize_entry, sstat) == 2,
		    "sstat of struct xnvme_spec_log_sanitize_entry")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_dir_rp, directives_supported,
			 XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_SUPPORTED_IDENTIFY |
				 XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_SUPPORTED_STREAMS |
				 XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_SUPPORTED_DATA_PLACEMENT),
		    "directives_supported of struct xnvme_spec_idfy_dir_rp")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_dir_rp, directives_enabled,
			 XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_ENABLED_IDENTIFY |
				 XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_ENABLED_STREAMS |
				 XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_ENABLED_DATA_PLACEMENT),
		    "directives_enabled of struct xnvme_spec_idfy_dir_rp")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_dir_rp, directives_persistence,
			 XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_PERSISTENCE_IDENTIFY |
				 XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_PERSISTENCE_STREAMS |
				 XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_PERSISTENCE_DATA_PLACEMENT),
		    "directives_persistence of struct xnvme_spec_idfy_dir_rp")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_idfy_dir_rp, directives_enabled) == 32,
		    "directives_enabled of struct xnvme_spec_idfy_dir_rp")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_idfy_dir_rp, directives_persistence) == 64,
		    "directives_persistence of struct xnvme_spec_idfy_dir_rp")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_streams_dir_rp, nssc,
			 XNVME_SPEC_STREAMS_DIR_RP_NSSC_MULTI_HOST),
		    "nssc of struct xnvme_spec_streams_dir_rp")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_streams_dir_rp, nssc) == 6,
		    "nssc of struct xnvme_spec_streams_dir_rp")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_nvm_idfy_ns, pic,
			 XNVME_SPEC_NVM_IDFY_NS_PIC_GPISTS | XNVME_SPEC_NVM_IDFY_NS_PIC_GPISTM |
				 XNVME_SPEC_NVM_IDFY_NS_PIC_STCRS),
		    "pic of struct xnvme_spec_nvm_idfy_ns")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_nvm_idfy_ns, pic) == 8,
		    "pic of struct xnvme_spec_nvm_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_znd_idfy_ns, zoc,
			 XNVME_SPEC_ZND_IDFY_NS_ZOC_VZCAP | XNVME_SPEC_ZND_IDFY_NS_ZOC_ZAE),
		    "zoc of struct xnvme_spec_znd_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_znd_idfy_ns, ozcs,
			 XNVME_SPEC_ZND_IDFY_NS_OZCS_RAZB | XNVME_SPEC_ZND_IDFY_NS_OZCS_ZRWASUP),
		    "ozcs of struct xnvme_spec_znd_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_znd_idfy_ns, zrwacap,
			 XNVME_SPEC_ZND_IDFY_NS_ZRWACAP_EXPFLUSHSUP),
		    "zrwacap of struct xnvme_spec_znd_idfy_ns")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_znd_idfy_ns, ozcs) == 2,
		    "ozcs of struct xnvme_spec_znd_idfy_ns")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_znd_idfy_ns, zrwacap) == 52,
		    "zrwacap of struct xnvme_spec_znd_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_znd_descr, zt, XNVME_SPEC_ZND_DESCR_ZT_ZT),
		    "zt of struct xnvme_spec_znd_descr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_znd_descr, zs, XNVME_SPEC_ZND_DESCR_ZS_ZS),
		    "zs of struct xnvme_spec_znd_descr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_znd_descr, za,
			 XNVME_SPEC_ZND_DESCR_ZA_ZFC | XNVME_SPEC_ZND_DESCR_ZA_ZFR |
				 XNVME_SPEC_ZND_DESCR_ZA_RZR | XNVME_SPEC_ZND_DESCR_ZA_ZRWAV |
				 XNVME_SPEC_ZND_DESCR_ZA_ZDEV),
		    "za of struct xnvme_spec_znd_descr")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_znd_descr, zs) == 1,
		    "zs of struct xnvme_spec_znd_descr")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_znd_descr, za) == 2,
		    "za of struct xnvme_spec_znd_descr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_fs_idfy_ctrlr, caps,
			 XNVME_SPEC_FS_IDFY_CTRLR_CAPS_DIRECT),
		    "caps of struct xnvme_spec_fs_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_fs_idfy_ctrlr, properties,
			 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_PERMISSIONS_POSIX |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_PERMISSIONS_ACL |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_STAMP_CREATION |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_STAMP_ACCESS |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_STAMP_CHANGE |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_STAMP_ARCHIVE |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_HARDLINKS |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_SYMLINKS |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_CASE_SENSITIVE |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_CASE_PRESERVING |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_JOURNALING_BLOCK |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_JOURNALING_META |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_SNAPSHOTTING |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_COMPRESSED |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_ENCRYPTED),
		    "properties of struct xnvme_spec_fs_idfy_ctrlr")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_fs_idfy_ctrlr, caps) == 520,
		    "caps of struct xnvme_spec_fs_idfy_ctrlr")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_fs_idfy_ctrlr, properties) == 560,
		    "properties of struct xnvme_spec_fs_idfy_ctrlr")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_fs_idfy_ctrlr, limits) == 528,
		    "limits of struct xnvme_spec_fs_idfy_ctrlr")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_fs_idfy_ctrlr, iosizes) == 568,
		    "iosizes of struct xnvme_spec_fs_idfy_ctrlr")

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

	memset(&sqe, 0, sizeof(sqe));
	sqe.cdw[11] = 0xcafe0103; // Directive Receive dword 11: DOPER 3, DTYPE 1, DSPEC 0xcafe
	sqe.cdw[12] = 0xffff0042; // Directive Receive dword 12, bits 15:0: NSR 0x42
	if (sqe.drecv.doper != 0x03 || sqe.drecv.dtype != 0x01 || sqe.drecv.dspec != 0xcafe ||
	    xnvme_bits_get(sqe.drecv.cdw12, XNVME_SPEC_CMD_DRECV_CDW12_NSR) != 0x42) {
		xnvme_cli_pinf("FAILED: drecv: doper: %#x, dtype: %#x, dspec: %#x, cdw12: %#x",
			       sqe.drecv.doper, sqe.drecv.dtype, sqe.drecv.dspec, sqe.drecv.cdw12);
		err = -EINVAL;
	}

	return err;
}

static int
sub_cdw0_views(struct xnvme_cli *XNVME_UNUSED(cli))
{
	uint32_t cdw0;
	int err = 0;

	{
		struct xnvme_spec_alloc_resource ar;

		cdw0 = 0xffff0042; // Allocate Resources completion dword 0: NSA 0x42
		memcpy(&ar, &cdw0, sizeof(ar));
		if (ar.nsa != 0x42) {
			xnvme_cli_pinf("FAILED: alloc_resource: nsa: %#x", ar.nsa);
			err = -EINVAL;
		}
	}

	{
		union xnvme_spec_feat feat;

		feat.val = 0x0075012c; // TMPTH 300, TMPSEL 0x5, THSEL 0x3, reserved bit 22 set
		if (xnvme_bits_get(feat.temp_threshold, XNVME_SPEC_FEAT_TEMP_THRESHOLD_TMPTH) !=
			    300 ||
		    xnvme_bits_get(feat.temp_threshold, XNVME_SPEC_FEAT_TEMP_THRESHOLD_TMPSEL) !=
			    0x5 ||
		    xnvme_bits_get(feat.temp_threshold, XNVME_SPEC_FEAT_TEMP_THRESHOLD_THSEL) !=
			    0x3) {
			xnvme_cli_pinf("FAILED: feat: temp_threshold: %#x", feat.temp_threshold);
			err = -EINVAL;
		}

		feat.val = 0x00010064; // Error Recovery: TLER 100, DULBE
		if (xnvme_bits_get(feat.error_recovery, XNVME_SPEC_FEAT_ERROR_RECOVERY_TLER) !=
			    100 ||
		    !xnvme_bits_get(feat.error_recovery, XNVME_SPEC_FEAT_ERROR_RECOVERY_DULBE)) {
			xnvme_cli_pinf("FAILED: feat: error_recovery: %#x", feat.error_recovery);
			err = -EINVAL;
		}

		feat.val = 0x0003003f; // Number of Queues: NSQA 63, NCQA 3
		if (feat.nqueues.nsqa != 63 || feat.nqueues.ncqa != 3) {
			xnvme_cli_pinf("FAILED: feat: nqueues: nsqa: %u, ncqa: %u",
				       feat.nqueues.nsqa, feat.nqueues.ncqa);
			err = -EINVAL;
		}

		feat.val = 0x00000201; // Flexible Data Placement: FDPE, FDPCI 0x2
		if (!xnvme_bits_get(feat.fdp_mode, XNVME_SPEC_FEAT_FDP_MODE_FDPE) ||
		    xnvme_bits_get(feat.fdp_mode, XNVME_SPEC_FEAT_FDP_MODE_FDPCI) != 0x2) {
			xnvme_cli_pinf("FAILED: feat: fdp_mode: %#x", feat.fdp_mode);
			err = -EINVAL;
		}

		feat.val = 0x00000002; // Media Feedback: VHECC
		if (xnvme_bits_get(feat.media_feedback, XNVME_SPEC_FEAT_MEDIA_FEEDBACK_HECC) ||
		    !xnvme_bits_get(feat.media_feedback, XNVME_SPEC_FEAT_MEDIA_FEEDBACK_VHECC)) {
			xnvme_cli_pinf("FAILED: feat: media_feedback: %#x", feat.media_feedback);
			err = -EINVAL;
		}
	}

	return err;
}

static int
sub_data_masks(struct xnvme_cli *XNVME_UNUSED(cli))
{
	uint8_t raw[4096] = {0};
	int err = 0;

	{
		struct xnvme_spec_fdp_conf_desc desc;

		memset(raw, 0, sizeof(raw));
		raw[2] = 0x95; // FDP configuration descriptor FDPA: RGIF 0x5, FDPVWC, FDPCV
		memcpy(&desc, raw, sizeof(desc));
		if (xnvme_bits_get(desc.fdpa, XNVME_SPEC_FDP_CONF_DESC_FDPA_RGIF) != 0x5 ||
		    !xnvme_bits_get(desc.fdpa, XNVME_SPEC_FDP_CONF_DESC_FDPA_FDPVWC) ||
		    !xnvme_bits_get(desc.fdpa, XNVME_SPEC_FDP_CONF_DESC_FDPA_FDPCV)) {
			xnvme_cli_pinf("FAILED: fdpa: %#" PRIx8, desc.fdpa);
			err = -EINVAL;
		}
	}

	{
		struct xnvme_spec_fdp_event_media_reallocated mr;

		memset(raw, 0, sizeof(raw));
		raw[0] = 0x01; // Media reallocated SEF: LBAV
		memcpy(&mr, raw, sizeof(mr));
		if (!xnvme_bits_get(mr.sef, XNVME_SPEC_FDP_EVENT_MEDIA_REALLOCATED_SEF_LBAV)) {
			xnvme_cli_pinf("FAILED: sef: %#" PRIx8, mr.sef);
			err = -EINVAL;
		}
	}

	{
		struct xnvme_spec_fdp_event event;

		memset(raw, 0, sizeof(raw));
		raw[1] = 0x05; // FDP event FDPEF: PIV, LV
		memcpy(&event, raw, sizeof(event));
		if (!xnvme_bits_get(event.fdpef, XNVME_SPEC_FDP_EVENT_FDPEF_PIV) ||
		    xnvme_bits_get(event.fdpef, XNVME_SPEC_FDP_EVENT_FDPEF_NSIDV) ||
		    !xnvme_bits_get(event.fdpef, XNVME_SPEC_FDP_EVENT_FDPEF_LV)) {
			xnvme_cli_pinf("FAILED: fdpef: %#" PRIx8, event.fdpef);
			err = -EINVAL;
		}
	}

	{
		struct xnvme_spec_fdp_event_desc desc;

		memset(raw, 0, sizeof(raw));
		raw[1] = 0x01; // FDP event descriptor FDPETA: event enabled
		memcpy(&desc, raw, sizeof(desc));
		if (!xnvme_bits_get(desc.fdpeta, XNVME_SPEC_FDP_EVENT_DESC_FDPETA_EE)) {
			xnvme_cli_pinf("FAILED: fdpeta: %#" PRIx8, desc.fdpeta);
			err = -EINVAL;
		}
	}

	{
		struct xnvme_spec_log_sanitize_entry log;

		memset(raw, 0, sizeof(raw));
		raw[2] = 0x1a; // Sanitize status SSTAT: SOS 010b, OPC 0x3
		raw[3] = 0x02; // MVCNCLD
		memcpy(&log, raw, sizeof(log));
		if (xnvme_bits_get(log.sstat, XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_SOS) != 0x2 ||
		    xnvme_bits_get(log.sstat, XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_OPC) != 0x3 ||
		    xnvme_bits_get(log.sstat, XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_GDE) ||
		    !xnvme_bits_get(log.sstat, XNVME_SPEC_LOG_SANITIZE_ENTRY_SSTAT_MVCNCLD)) {
			xnvme_cli_pinf("FAILED: sstat: %#" PRIx16, log.sstat);
			err = -EINVAL;
		}
	}

	{
		struct xnvme_spec_idfy_dir_rp rp;

		memset(raw, 0, sizeof(raw));
		raw[0] = 0x07;  // Directives supported: identify, streams, data placement
		raw[32] = 0x02; // Directives enabled: streams
		raw[64] = 0x04; // Directives persistence: data placement
		memcpy(&rp, raw, sizeof(rp));
		if (!xnvme_bits_get(rp.directives_supported,
				    XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_SUPPORTED_IDENTIFY) ||
		    !xnvme_bits_get(rp.directives_supported,
				    XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_SUPPORTED_DATA_PLACEMENT) ||
		    xnvme_bits_get(rp.directives_enabled,
				   XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_ENABLED_IDENTIFY) ||
		    !xnvme_bits_get(rp.directives_enabled,
				    XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_ENABLED_STREAMS) ||
		    xnvme_bits_get(rp.directives_persistence,
				   XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_PERSISTENCE_STREAMS) ||
		    !xnvme_bits_get(
			    rp.directives_persistence,
			    XNVME_SPEC_IDFY_DIR_RP_DIRECTIVES_PERSISTENCE_DATA_PLACEMENT)) {
			xnvme_cli_pinf("FAILED: directives: %#" PRIx8 ", %#" PRIx8 ", %#" PRIx8,
				       rp.directives_supported, rp.directives_enabled,
				       rp.directives_persistence);
			err = -EINVAL;
		}
	}

	{
		struct xnvme_spec_streams_dir_rp rp;

		memset(raw, 0, sizeof(raw));
		raw[6] = 0x01; // Streams return parameters NSSC: multi-host
		memcpy(&rp, raw, sizeof(rp));
		if (!xnvme_bits_get(rp.nssc, XNVME_SPEC_STREAMS_DIR_RP_NSSC_MULTI_HOST)) {
			xnvme_cli_pinf("FAILED: nssc: %#" PRIx8, rp.nssc);
			err = -EINVAL;
		}
	}

	{
		struct xnvme_spec_nvm_idfy_ns ns;

		memset(raw, 0, sizeof(raw));
		raw[8] = 0x05; // NVM Identify Namespace PIC: 16BPISTS, STCRS
		memcpy(&ns, raw, sizeof(ns));
		if (!xnvme_bits_get(ns.pic, XNVME_SPEC_NVM_IDFY_NS_PIC_GPISTS) ||
		    xnvme_bits_get(ns.pic, XNVME_SPEC_NVM_IDFY_NS_PIC_GPISTM) ||
		    !xnvme_bits_get(ns.pic, XNVME_SPEC_NVM_IDFY_NS_PIC_STCRS)) {
			xnvme_cli_pinf("FAILED: pic: %#" PRIx8, ns.pic);
			err = -EINVAL;
		}
	}

	{
		union xnvme_spec_nvm_idfy idfy;

		memset(&idfy, 0, sizeof(idfy));
		idfy.base.raw[8] = 0x04; // NVM Identify Namespace PIC: STCRS
		idfy.base.raw[0] = 0x07; // NVM Identify Controller VSL
		if (!xnvme_bits_get(idfy.ns.pic, XNVME_SPEC_NVM_IDFY_NS_PIC_STCRS) ||
		    idfy.ctrlr.vsl != 0x07) {
			xnvme_cli_pinf("FAILED: nvm_idfy: pic: %#" PRIx8 ", vsl: %#" PRIx8,
				       idfy.ns.pic, idfy.ctrlr.vsl);
			err = -EINVAL;
		}
	}

	{
		struct xnvme_spec_znd_idfy_ns ns;

		memset(raw, 0, sizeof(raw));
		raw[0] = 0x02;  // Zoned Identify Namespace ZOC: ZAE
		raw[2] = 0x02;  // OZCS: ZRWASUP
		raw[52] = 0x01; // ZRWACAP: EXPFLUSHSUP
		memcpy(&ns, raw, sizeof(ns));
		if (xnvme_bits_get(ns.zoc, XNVME_SPEC_ZND_IDFY_NS_ZOC_VZCAP) ||
		    !xnvme_bits_get(ns.zoc, XNVME_SPEC_ZND_IDFY_NS_ZOC_ZAE) ||
		    xnvme_bits_get(ns.ozcs, XNVME_SPEC_ZND_IDFY_NS_OZCS_RAZB) ||
		    !xnvme_bits_get(ns.ozcs, XNVME_SPEC_ZND_IDFY_NS_OZCS_ZRWASUP) ||
		    !xnvme_bits_get(ns.zrwacap, XNVME_SPEC_ZND_IDFY_NS_ZRWACAP_EXPFLUSHSUP)) {
			xnvme_cli_pinf("FAILED: zoc: %#" PRIx16 ", ozcs: %#" PRIx16
				       ", zrwacap: %#" PRIx8,
				       ns.zoc, ns.ozcs, ns.zrwacap);
			err = -EINVAL;
		}
	}

	{
		union xnvme_spec_znd_idfy idfy;

		memset(&idfy, 0, sizeof(idfy));
		idfy.base.raw[0] = 0x07; // Zoned Identify Controller ZASL, Identify Namespace ZOC
		if (idfy.zctrlr.zasl != 0x07 ||
		    !xnvme_bits_get(idfy.zns.zoc, XNVME_SPEC_ZND_IDFY_NS_ZOC_ZAE)) {
			xnvme_cli_pinf("FAILED: znd_idfy: zasl: %#" PRIx8 ", zoc: %#" PRIx16,
				       idfy.zctrlr.zasl, idfy.zns.zoc);
			err = -EINVAL;
		}
	}

	{
		struct xnvme_spec_znd_descr descr;

		memset(raw, 0, sizeof(raw));
		raw[0] = 0x02; // Zone descriptor ZT: sequential write required
		raw[1] = 0xe0; // ZS: full
		raw[2] = 0x88; // ZA: ZRWAV, ZDEV
		memcpy(&descr, raw, sizeof(descr));
		if (xnvme_bits_get(descr.zt, XNVME_SPEC_ZND_DESCR_ZT_ZT) !=
			    XNVME_SPEC_ZND_TYPE_SEQWR ||
		    xnvme_bits_get(descr.zs, XNVME_SPEC_ZND_DESCR_ZS_ZS) !=
			    XNVME_SPEC_ZND_STATE_FULL ||
		    xnvme_bits_get(descr.za, XNVME_SPEC_ZND_DESCR_ZA_ZFC) ||
		    !xnvme_bits_get(descr.za, XNVME_SPEC_ZND_DESCR_ZA_ZRWAV) ||
		    !xnvme_bits_get(descr.za, XNVME_SPEC_ZND_DESCR_ZA_ZDEV)) {
			xnvme_cli_pinf("FAILED: zt: %#" PRIx8 ", zs: %#" PRIx8 ", za: %#" PRIx8,
				       descr.zt, descr.zs, descr.za);
			err = -EINVAL;
		}
	}

	{
		union xnvme_spec_kvs_idfy idfy;

		memset(&idfy, 0, sizeof(idfy));
		idfy.base.raw[25] = 0x03; // Key Value Identify Namespace NKVF
		if (idfy.ns.nkvf != 0x03) {
			xnvme_cli_pinf("FAILED: kvs_idfy: nkvf: %#" PRIx8, idfy.ns.nkvf);
			err = -EINVAL;
		}
	}

	{
		struct xnvme_spec_fs_idfy_ctrlr ctrlr;

		memset(raw, 0, sizeof(raw));
		raw[520] = 0x01; // File-system Identify Controller caps: direct
		raw[561] = 0x40; // properties bit 14: encrypted
		memcpy(&ctrlr, raw, sizeof(ctrlr));
		if (!xnvme_bits_get(ctrlr.caps, XNVME_SPEC_FS_IDFY_CTRLR_CAPS_DIRECT) ||
		    xnvme_bits_get(ctrlr.properties,
				   XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_PERMISSIONS_POSIX) ||
		    !xnvme_bits_get(ctrlr.properties,
				    XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_ENCRYPTED)) {
			xnvme_cli_pinf("FAILED: caps: %#" PRIx64 ", properties: %#" PRIx64,
				       ctrlr.caps, ctrlr.properties);
			err = -EINVAL;
		}
	}

	return err;
}

static int
sub_io_dwords(struct xnvme_cli *XNVME_UNUSED(cli))
{
	union xnvme_spec_sqe sqe = {0};
	int err = 0;

	sqe.cdw[12] = 0xd4a0abcd; // NVM dword 12: NLB 0xabcd, DTYPE 0xa, PRINFO 0x5, LR, FUA
	if (xnvme_bits_get(sqe.nvm.cdw12, XNVME_SPEC_CMD_NVM_CDW12_NLB) != 0xabcd ||
	    xnvme_bits_get(sqe.nvm.cdw12, XNVME_SPEC_CMD_NVM_CDW12_DTYPE) != 0xa ||
	    xnvme_bits_get(sqe.nvm.cdw12, XNVME_SPEC_CMD_NVM_CDW12_PRINFO) != 0x5 ||
	    !xnvme_bits_get(sqe.nvm.cdw12, XNVME_SPEC_CMD_NVM_CDW12_FUA) ||
	    !xnvme_bits_get(sqe.nvm.cdw12, XNVME_SPEC_CMD_NVM_CDW12_LR)) {
		xnvme_cli_pinf("FAILED: nvm: cdw12: %#x", sqe.nvm.cdw12);
		err = -EINVAL;
	}

	sqe.cdw[13] = 0x1234005b; // NVM dword 13: AF 0xb, AL 0x1, SR, DSPEC 0x1234
	if (xnvme_bits_get(sqe.nvm.cdw13, XNVME_SPEC_CMD_NVM_CDW13_AF) != 0xb ||
	    xnvme_bits_get(sqe.nvm.cdw13, XNVME_SPEC_CMD_NVM_CDW13_AL) != 0x1 ||
	    !xnvme_bits_get(sqe.nvm.cdw13, XNVME_SPEC_CMD_NVM_CDW13_SR) ||
	    xnvme_bits_get(sqe.nvm.cdw13, XNVME_SPEC_CMD_NVM_CDW13_INCOM) ||
	    xnvme_bits_get(sqe.nvm.cdw13, XNVME_SPEC_CMD_NVM_CDW13_DSPEC) != 0x1234) {
		xnvme_cli_pinf("FAILED: nvm: cdw13: %#x", sqe.nvm.cdw13);
		err = -EINVAL;
	}

	sqe.cdw[14] = 0x89abcdef; // NVM dword 14: ILBRT
	sqe.cdw[15] = 0xff001234; // NVM dword 15: LBAT 0x1234, LBATM 0xff00
	if (sqe.nvm.ilbrt != 0x89abcdef || sqe.nvm.lbat != 0x1234 || sqe.nvm.lbatm != 0xff00) {
		xnvme_cli_pinf("FAILED: nvm: ilbrt: %#x, lbat: %#x, lbatm: %#x", sqe.nvm.ilbrt,
			       sqe.nvm.lbat, sqe.nvm.lbatm);
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
		"io-dwords",
		"Read the I/O commands through their dwords and their masks",
		"Read the I/O commands through their dwords and their masks",
		sub_io_dwords,
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
	{
		"cdw0-views",
		"Read completion dword 0 through the views of features and resources",
		"Read completion dword 0 through the views of features and resources",
		sub_cdw0_views,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
	{
		"data-masks",
		"Read the fields of bits of data structures through their masks",
		"Read the fields of bits of data structures through their masks",
		sub_data_masks,
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
