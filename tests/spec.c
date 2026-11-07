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
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cmd_nvm, cdw02_03, XNVME_SPEC_CMD_NVM_CDW02_03_LBTU),
		    "cdw02_03 of struct xnvme_spec_cmd_nvm")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cmd_nvm, cdw12,
			 XNVME_SPEC_CMD_NVM_CDW12_NLB | XNVME_SPEC_CMD_NVM_CDW12_CETYPE |
				 XNVME_SPEC_CMD_NVM_CDW12_STC | XNVME_SPEC_CMD_NVM_CDW12_DTYPE |
				 XNVME_SPEC_CMD_NVM_CDW12_PRINFO | XNVME_SPEC_CMD_NVM_CDW12_FUA |
				 XNVME_SPEC_CMD_NVM_CDW12_LR),
		    "cdw12 of struct xnvme_spec_cmd_nvm")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_nvm, cdw12) == 48,
		    "cdw12 of struct xnvme_spec_cmd_nvm")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cmd_nvm, cdw13,
			 XNVME_SPEC_CMD_NVM_CDW13_DSM | XNVME_SPEC_CMD_NVM_CDW13_AF |
				 XNVME_SPEC_CMD_NVM_CDW13_AL | XNVME_SPEC_CMD_NVM_CDW13_SEQREQ |
				 XNVME_SPEC_CMD_NVM_CDW13_INCPRS | XNVME_SPEC_CMD_NVM_CDW13_CEV |
				 XNVME_SPEC_CMD_NVM_CDW13_DSPEC),
		    "cdw13 of struct xnvme_spec_cmd_nvm")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_nvm, cdw13) == 52,
		    "cdw13 of struct xnvme_spec_cmd_nvm")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_nvm, lbtl) == 56,
		    "lbtl of struct xnvme_spec_cmd_nvm")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_nvm, lbatm) == 62,
		    "lbatm of struct xnvme_spec_cmd_nvm")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_nvm_write_zeroes, cdw12,
			 XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_NLB |
				 XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_CETYPE |
				 XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_DTYPE |
				 XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_NSZ |
				 XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_STC |
				 XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_DEAC |
				 XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_PRINFO |
				 XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_FUA |
				 XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_LR),
		    "cdw12 of struct xnvme_spec_nvm_write_zeroes")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_nvm_write_zeroes, cdw12) == 48,
		    "cdw12 of struct xnvme_spec_nvm_write_zeroes")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_nvm_write_zeroes, lbtl) == 56,
		    "lbtl of struct xnvme_spec_nvm_write_zeroes")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_nvm_write_zeroes, cdw13,
			 XNVME_SPEC_NVM_WRITE_ZEROES_CDW13_CEV |
				 XNVME_SPEC_NVM_WRITE_ZEROES_CDW13_DSPEC),
		    "cdw13 of struct xnvme_spec_nvm_write_zeroes")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_nvm_write_zeroes, lbatm) == 62,
		    "lbatm of struct xnvme_spec_nvm_write_zeroes")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_nvm_compare, cdw12,
			 XNVME_SPEC_NVM_COMPARE_CDW12_NLB | XNVME_SPEC_NVM_COMPARE_CDW12_CETYPE |
				 XNVME_SPEC_NVM_COMPARE_CDW12_STC |
				 XNVME_SPEC_NVM_COMPARE_CDW12_PRINFO |
				 XNVME_SPEC_NVM_COMPARE_CDW12_FUA |
				 XNVME_SPEC_NVM_COMPARE_CDW12_LR),
		    "cdw12 of struct xnvme_spec_nvm_compare")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_nvm_compare, cdw12) == 48,
		    "cdw12 of struct xnvme_spec_nvm_compare")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_nvm_compare, cdw13, XNVME_SPEC_NVM_COMPARE_CDW13_CEV),
		    "cdw13 of struct xnvme_spec_nvm_compare")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cmd_dsm, cdw11,
			 XNVME_SPEC_CMD_DSM_CDW11_IDR | XNVME_SPEC_CMD_DSM_CDW11_IDW |
				 XNVME_SPEC_CMD_DSM_CDW11_AD),
		    "cdw11 of struct xnvme_spec_cmd_dsm")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_dsm, nr) == 40,
		    "nr of struct xnvme_spec_cmd_dsm")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cmd_dsm, cdw11) == 44,
		    "cdw11 of struct xnvme_spec_cmd_dsm")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_nvm_scopy_fmt_zero, rpars,
			 XNVME_SPEC_NVM_SCOPY_FMT_ZERO_RPARS_NLB |
				 XNVME_SPEC_NVM_SCOPY_FMT_ZERO_RPARS_CETYPE),
		    "rpars of struct xnvme_spec_nvm_scopy_fmt_zero")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_nvm_scopy_fmt_zero, sopt,
			 XNVME_SPEC_NVM_SCOPY_FMT_ZERO_SOPT_FCO),
		    "sopt of struct xnvme_spec_nvm_scopy_fmt_zero")
XNVME_STATIC_ASSERT(
	FITS(struct xnvme_spec_nvm_cmd_scopy, cdw12,
	     XNVME_SPEC_NVM_CMD_SCOPY_CDW12_NR | XNVME_SPEC_NVM_CMD_SCOPY_CDW12_DESFMT |
		     XNVME_SPEC_NVM_CMD_SCOPY_CDW12_CETYPE | XNVME_SPEC_NVM_CMD_SCOPY_CDW12_STCW |
		     XNVME_SPEC_NVM_CMD_SCOPY_CDW12_STCR | XNVME_SPEC_NVM_CMD_SCOPY_CDW12_PRINFOR |
		     XNVME_SPEC_NVM_CMD_SCOPY_CDW12_DTYPE |
		     XNVME_SPEC_NVM_CMD_SCOPY_CDW12_PRINFOW | XNVME_SPEC_NVM_CMD_SCOPY_CDW12_FUA |
		     XNVME_SPEC_NVM_CMD_SCOPY_CDW12_LR),
	"cdw12 of struct xnvme_spec_nvm_cmd_scopy")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_nvm_cmd_scopy, cdw12) == 48,
		    "cdw12 of struct xnvme_spec_nvm_cmd_scopy")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_nvm_cmd_scopy, dspec) == 54,
		    "dspec of struct xnvme_spec_nvm_cmd_scopy")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_nvm_cmd_scopy, lbatm) == 62,
		    "lbatm of struct xnvme_spec_nvm_cmd_scopy")
XNVME_STATIC_ASSERT(offsetof(union xnvme_spec_nvm_cmd, scopy.cdw12) == 48,
		    "scopy of union xnvme_spec_nvm_cmd")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_znd_cmd_append, cdw12,
			 XNVME_SPEC_ZND_CMD_APPEND_CDW12_NLB |
				 XNVME_SPEC_ZND_CMD_APPEND_CDW12_DTYPE |
				 XNVME_SPEC_ZND_CMD_APPEND_CDW12_PRINFO |
				 XNVME_SPEC_ZND_CMD_APPEND_CDW12_FUA |
				 XNVME_SPEC_ZND_CMD_APPEND_CDW12_LR),
		    "cdw12 of struct xnvme_spec_znd_cmd_append")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_znd_cmd_append, cdw12) == 48,
		    "cdw12 of struct xnvme_spec_znd_cmd_append")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_znd_cmd_mgmt_send, cdw13,
			 XNVME_SPEC_ZND_CMD_MGMT_SEND_CDW13_ZSA |
				 XNVME_SPEC_ZND_CMD_MGMT_SEND_CDW13_SELECT_ALL |
				 XNVME_SPEC_ZND_CMD_MGMT_SEND_CDW13_ZSASO),
		    "cdw13 of struct xnvme_spec_znd_cmd_mgmt_send")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_znd_cmd_mgmt_send, cdw13) == 52,
		    "cdw13 of struct xnvme_spec_znd_cmd_mgmt_send")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_znd_cmd_mgmt_recv, cdw13,
			 XNVME_SPEC_ZND_CMD_MGMT_RECV_CDW13_ZRA |
				 XNVME_SPEC_ZND_CMD_MGMT_RECV_CDW13_ZRASF |
				 XNVME_SPEC_ZND_CMD_MGMT_RECV_CDW13_PARTIAL),
		    "cdw13 of struct xnvme_spec_znd_cmd_mgmt_recv")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_znd_cmd_mgmt_recv, cdw13) == 52,
		    "cdw13 of struct xnvme_spec_znd_cmd_mgmt_recv")
XNVME_STATIC_ASSERT(offsetof(union xnvme_spec_znd_cmd, append.cdw12) == 48,
		    "append of union xnvme_spec_znd_cmd")
XNVME_STATIC_ASSERT(offsetof(union xnvme_spec_znd_cmd, mgmt_recv.cdw13) == 52,
		    "mgmt_recv of union xnvme_spec_znd_cmd")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_io_mgmt_recv_cmd, mo) == 40,
		    "mo of struct xnvme_spec_io_mgmt_recv_cmd")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_io_mgmt_recv_cmd, mos) == 42,
		    "mos of struct xnvme_spec_io_mgmt_recv_cmd")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_io_mgmt_send_cmd, mo) == 40,
		    "mo of struct xnvme_spec_io_mgmt_send_cmd")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_io_mgmt_send_cmd, mos) == 42,
		    "mos of struct xnvme_spec_io_mgmt_send_cmd")
XNVME_STATIC_ASSERT(offsetof(union xnvme_spec_io_mgmt_cmd, mgmt_recv.numd) == 44,
		    "mgmt_recv of union xnvme_spec_io_mgmt_cmd")
XNVME_STATIC_ASSERT(offsetof(union xnvme_spec_io_mgmt_cmd, mgmt_send.mos) == 42,
		    "mgmt_send of union xnvme_spec_io_mgmt_cmd")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_kvs_cmd, cdw11) == 44,
		    "cdw11 of struct xnvme_spec_kvs_cmd")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_kvs_cmd, cdw11.ro) == 45,
		    "cdw11.ro of struct xnvme_spec_kvs_cmd")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_sgl_descriptor, id,
			 XNVME_SPEC_SGL_DESCRIPTOR_ID_SUBTYPE | XNVME_SPEC_SGL_DESCRIPTOR_ID_TYPE),
		    "id of struct xnvme_spec_sgl_descriptor")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_sgl_descriptor, len) == 8,
		    "len of struct xnvme_spec_sgl_descriptor")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_sgl_descriptor, id) == 15,
		    "id of struct xnvme_spec_sgl_descriptor")
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
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cqe, result,
			 XNVME_SPEC_CQE_RESULT_CDW0 | XNVME_SPEC_CQE_RESULT_CDW1),
		    "result of struct xnvme_spec_cqe")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_cqe, sqhd) == 8, "sqhd of struct xnvme_spec_cqe")
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
			 XNVME_SPEC_IDFY_NS_NSFEAT_THINP | XNVME_SPEC_IDFY_NS_NSFEAT_NSABP |
				 XNVME_SPEC_IDFY_NS_NSFEAT_DAE |
				 XNVME_SPEC_IDFY_NS_NSFEAT_UIDREUSE |
				 XNVME_SPEC_IDFY_NS_NSFEAT_OPTPERF |
				 XNVME_SPEC_IDFY_NS_NSFEAT_MAM |
				 XNVME_SPEC_IDFY_NS_NSFEAT_OPTRPERF),
		    "nsfeat of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, flbas,
			 XNVME_SPEC_IDFY_NS_FLBAS_FIDXL | XNVME_SPEC_IDFY_NS_FLBAS_MTELBA |
				 XNVME_SPEC_IDFY_NS_FLBAS_FIDXU),
		    "flbas of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, mc,
			 XNVME_SPEC_IDFY_NS_MC_MTELBAS | XNVME_SPEC_IDFY_NS_MC_MTSBS),
		    "mc of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, dpc,
			 XNVME_SPEC_IDFY_NS_DPC_PIT1S | XNVME_SPEC_IDFY_NS_DPC_PIT2S |
				 XNVME_SPEC_IDFY_NS_DPC_PIT3S | XNVME_SPEC_IDFY_NS_DPC_PIIFB |
				 XNVME_SPEC_IDFY_NS_DPC_PIILB),
		    "dpc of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, dps,
			 XNVME_SPEC_IDFY_NS_DPS_PIT | XNVME_SPEC_IDFY_NS_DPS_PIP),
		    "dps of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, nmic,
			 XNVME_SPEC_IDFY_NS_NMIC_SHRNS | XNVME_SPEC_IDFY_NS_NMIC_DISNS),
		    "nmic of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, rescap,
			 XNVME_SPEC_IDFY_NS_RESCAP_PTPLS | XNVME_SPEC_IDFY_NS_RESCAP_WES |
				 XNVME_SPEC_IDFY_NS_RESCAP_EAS | XNVME_SPEC_IDFY_NS_RESCAP_WEROS |
				 XNVME_SPEC_IDFY_NS_RESCAP_EAROS |
				 XNVME_SPEC_IDFY_NS_RESCAP_WEARS |
				 XNVME_SPEC_IDFY_NS_RESCAP_EAARS | XNVME_SPEC_IDFY_NS_RESCAP_IEKS),
		    "rescap of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, fpi,
			 XNVME_SPEC_IDFY_NS_FPI_RFNVM | XNVME_SPEC_IDFY_NS_FPI_FPIS),
		    "fpi of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, dlfeat,
			 XNVME_SPEC_IDFY_NS_DLFEAT_DRB | XNVME_SPEC_IDFY_NS_DLFEAT_WZDS |
				 XNVME_SPEC_IDFY_NS_DLFEAT_GDS),
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
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr_nvmf, fcatt,
			 XNVME_SPEC_IDFY_CTRLR_NVMF_FCATT_DCMS |
				 XNVME_SPEC_IDFY_CTRLR_NVMF_FCATT_NZNSETIDS),
		    "fcatt of struct xnvme_spec_idfy_ctrlr_nvmf")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr_nvmf, ofcs,
			 XNVME_SPEC_IDFY_CTRLR_NVMF_OFCS_DCS),
		    "ofcs of struct xnvme_spec_idfy_ctrlr_nvmf")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, cmic,
			 XNVME_SPEC_IDFY_CTRLR_CMIC_MPORTS | XNVME_SPEC_IDFY_CTRLR_CMIC_MCTRS |
				 XNVME_SPEC_IDFY_CTRLR_CMIC_FT | XNVME_SPEC_IDFY_CTRLR_CMIC_ANARS),
		    "cmic of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, ver,
			 XNVME_SPEC_IDFY_CTRLR_VER_TER | XNVME_SPEC_IDFY_CTRLR_VER_MNR |
				 XNVME_SPEC_IDFY_CTRLR_VER_MJR),
		    "ver of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(
	FITS(struct xnvme_spec_idfy_ctrlr, oaes,
	     XNVME_SPEC_IDFY_CTRLR_OAES_NSAN | XNVME_SPEC_IDFY_CTRLR_OAES_FAN |
		     XNVME_SPEC_IDFY_CTRLR_OAES_ANACN | XNVME_SPEC_IDFY_CTRLR_OAES_PLEAN |
		     XNVME_SPEC_IDFY_CTRLR_OAES_LSIAN | XNVME_SPEC_IDFY_CTRLR_OAES_EGEAN |
		     XNVME_SPEC_IDFY_CTRLR_OAES_NNSS | XNVME_SPEC_IDFY_CTRLR_OAES_ZDCN |
		     XNVME_SPEC_IDFY_CTRLR_OAES_DLPCN),
	"oaes of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(
	FITS(struct xnvme_spec_idfy_ctrlr, ctratt,
	     XNVME_SPEC_IDFY_CTRLR_CTRATT_HIDS | XNVME_SPEC_IDFY_CTRLR_CTRATT_NOPSPM |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_NSETS | XNVME_SPEC_IDFY_CTRLR_CTRATT_RRLVLS |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_EGS | XNVME_SPEC_IDFY_CTRLR_CTRATT_PLM |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_TBKAS | XNVME_SPEC_IDFY_CTRLR_CTRATT_NG |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_SQA | XNVME_SPEC_IDFY_CTRLR_CTRATT_ULIST |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_MDS | XNVME_SPEC_IDFY_CTRLR_CTRATT_FCM |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_VCM | XNVME_SPEC_IDFY_CTRLR_CTRATT_DEG |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_DNVMS | XNVME_SPEC_IDFY_CTRLR_CTRATT_ELBAS |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_FDPS),
	"ctratt of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, nvmsr,
			 XNVME_SPEC_IDFY_CTRLR_NVMSR_NVMESD | XNVME_SPEC_IDFY_CTRLR_NVMSR_NVMEE),
		    "nvmsr of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, vwci,
			 XNVME_SPEC_IDFY_CTRLR_VWCI_VWCR | XNVME_SPEC_IDFY_CTRLR_VWCI_VWCRV),
		    "vwci of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, mec,
			 XNVME_SPEC_IDFY_CTRLR_MEC_TWPME | XNVME_SPEC_IDFY_CTRLR_MEC_PCIEME),
		    "mec of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, oacs,
			 XNVME_SPEC_IDFY_CTRLR_OACS_SSRS | XNVME_SPEC_IDFY_CTRLR_OACS_FNVMS |
				 XNVME_SPEC_IDFY_CTRLR_OACS_FWDS | XNVME_SPEC_IDFY_CTRLR_OACS_NMS |
				 XNVME_SPEC_IDFY_CTRLR_OACS_DSTS |
				 XNVME_SPEC_IDFY_CTRLR_OACS_DIRS |
				 XNVME_SPEC_IDFY_CTRLR_OACS_NSRS | XNVME_SPEC_IDFY_CTRLR_OACS_VMS |
				 XNVME_SPEC_IDFY_CTRLR_OACS_DBCS |
				 XNVME_SPEC_IDFY_CTRLR_OACS_GLSS |
				 XNVME_SPEC_IDFY_CTRLR_OACS_CFLS),
		    "oacs of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, frmw,
			 XNVME_SPEC_IDFY_CTRLR_FRMW_FFSRO | XNVME_SPEC_IDFY_CTRLR_FRMW_NOFS |
				 XNVME_SPEC_IDFY_CTRLR_FRMW_FAWR |
				 XNVME_SPEC_IDFY_CTRLR_FRMW_SMUD),
		    "frmw of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, lpa,
			 XNVME_SPEC_IDFY_CTRLR_LPA_SMARTS | XNVME_SPEC_IDFY_CTRLR_LPA_CSES |
				 XNVME_SPEC_IDFY_CTRLR_LPA_LPEDS | XNVME_SPEC_IDFY_CTRLR_LPA_TS |
				 XNVME_SPEC_IDFY_CTRLR_LPA_PES | XNVME_SPEC_IDFY_CTRLR_LPA_MLPS |
				 XNVME_SPEC_IDFY_CTRLR_LPA_DA4S),
		    "lpa of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, avscc, XNVME_SPEC_IDFY_CTRLR_AVSCC_VSCF),
		    "avscc of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, apsta, XNVME_SPEC_IDFY_CTRLR_APSTA_APTS),
		    "apsta of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, rpmbs,
			 XNVME_SPEC_IDFY_CTRLR_RPMBS_NRPMBU | XNVME_SPEC_IDFY_CTRLR_RPMBS_AUTHM |
				 XNVME_SPEC_IDFY_CTRLR_RPMBS_TSZE |
				 XNVME_SPEC_IDFY_CTRLR_RPMBS_ASZE),
		    "rpmbs of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, dsto, XNVME_SPEC_IDFY_CTRLR_DSTO_SDSO),
		    "dsto of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, hctma, XNVME_SPEC_IDFY_CTRLR_HCTMA_HCTMS),
		    "hctma of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, sanicap,
			 XNVME_SPEC_IDFY_CTRLR_SANICAP_CES | XNVME_SPEC_IDFY_CTRLR_SANICAP_BES |
				 XNVME_SPEC_IDFY_CTRLR_SANICAP_OWS |
				 XNVME_SPEC_IDFY_CTRLR_SANICAP_NDI |
				 XNVME_SPEC_IDFY_CTRLR_SANICAP_NODMMAS),
		    "sanicap of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, anacap,
			 XNVME_SPEC_IDFY_CTRLR_ANACAP_RANAOS |
				 XNVME_SPEC_IDFY_CTRLR_ANACAP_RANANOS |
				 XNVME_SPEC_IDFY_CTRLR_ANACAP_RANAIS |
				 XNVME_SPEC_IDFY_CTRLR_ANACAP_RANAPLS |
				 XNVME_SPEC_IDFY_CTRLR_ANACAP_RANACS |
				 XNVME_SPEC_IDFY_CTRLR_ANACAP_ANAGIDLWAS |
				 XNVME_SPEC_IDFY_CTRLR_ANACAP_ANAGIDS),
		    "anacap of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, sqes,
			 XNVME_SPEC_IDFY_CTRLR_SQES_MINSQES | XNVME_SPEC_IDFY_CTRLR_SQES_MAXSQES),
		    "sqes of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, cqes,
			 XNVME_SPEC_IDFY_CTRLR_CQES_MINCQES | XNVME_SPEC_IDFY_CTRLR_CQES_MAXCQES),
		    "cqes of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(
	FITS(struct xnvme_spec_idfy_ctrlr, oncs,
	     XNVME_SPEC_IDFY_CTRLR_ONCS_NVMCMPS | XNVME_SPEC_IDFY_CTRLR_ONCS_NVMWUSV |
		     XNVME_SPEC_IDFY_CTRLR_ONCS_NVMDSMSV | XNVME_SPEC_IDFY_CTRLR_ONCS_NVMWZSV |
		     XNVME_SPEC_IDFY_CTRLR_ONCS_SSFS | XNVME_SPEC_IDFY_CTRLR_ONCS_RESERVS |
		     XNVME_SPEC_IDFY_CTRLR_ONCS_TSS | XNVME_SPEC_IDFY_CTRLR_ONCS_NVMVFYS |
		     XNVME_SPEC_IDFY_CTRLR_ONCS_NVMCPYS),
	"oncs of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, fna,
			 XNVME_SPEC_IDFY_CTRLR_FNA_FNS | XNVME_SPEC_IDFY_CTRLR_FNA_SENS |
				 XNVME_SPEC_IDFY_CTRLR_FNA_CRYES |
				 XNVME_SPEC_IDFY_CTRLR_FNA_FNVMBS),
		    "fna of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, vwc,
			 XNVME_SPEC_IDFY_CTRLR_VWC_VWCP | XNVME_SPEC_IDFY_CTRLR_VWC_FB),
		    "vwc of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, cdfs,
			 XNVME_SPEC_IDFY_CTRLR_CDFS_CDF0S | XNVME_SPEC_IDFY_CTRLR_CDFS_CDF1S),
		    "cdfs of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(
	FITS(struct xnvme_spec_idfy_ctrlr, sgls,
	     XNVME_SPEC_IDFY_CTRLR_SGLS_SGLS | XNVME_SPEC_IDFY_CTRLR_SGLS_KSDBDS |
		     XNVME_SPEC_IDFY_CTRLR_SGLS_SDT | XNVME_SPEC_IDFY_CTRLR_SGLS_SBBDS |
		     XNVME_SPEC_IDFY_CTRLR_SGLS_MBA | XNVME_SPEC_IDFY_CTRLR_SGLS_LLDTS |
		     XNVME_SPEC_IDFY_CTRLR_SGLS_MSDS | XNVME_SPEC_IDFY_CTRLR_SGLS_SAOS |
		     XNVME_SPEC_IDFY_CTRLR_SGLS_TSDBDS),
	"sgls of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_cs_vector, bits,
			 XNVME_SPEC_CS_VECTOR_BITS_NVMCS | XNVME_SPEC_CS_VECTOR_BITS_KVCS |
				 XNVME_SPEC_CS_VECTOR_BITS_ZNSCS |
				 XNVME_SPEC_CS_VECTOR_BITS_SLMCS |
				 XNVME_SPEC_CS_VECTOR_BITS_CPNCS),
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
			 XNVME_SPEC_NVM_IDFY_NS_PIC_PISTS16B |
				 XNVME_SPEC_NVM_IDFY_NS_PIC_PISTM16B |
				 XNVME_SPEC_NVM_IDFY_NS_PIC_STCRS |
				 XNVME_SPEC_NVM_IDFY_NS_PIC_QPIFS),
		    "pic of struct xnvme_spec_nvm_idfy_ns")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_nvm_idfy_ns, pic) == 8,
		    "pic of struct xnvme_spec_nvm_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_znd_idfy_ns, zoc,
			 XNVME_SPEC_ZND_IDFY_NS_ZOC_VZC | XNVME_SPEC_ZND_IDFY_NS_ZOC_ZAE),
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
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_znd_descr, ztattr, XNVME_SPEC_ZND_DESCR_ZTATTR_ZT),
		    "ztattr of struct xnvme_spec_znd_descr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_znd_descr, zsattr, XNVME_SPEC_ZND_DESCR_ZSATTR_ZS),
		    "zsattr of struct xnvme_spec_znd_descr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_znd_descr, za,
			 XNVME_SPEC_ZND_DESCR_ZA_ZFC | XNVME_SPEC_ZND_DESCR_ZA_FZR |
				 XNVME_SPEC_ZND_DESCR_ZA_RZR | XNVME_SPEC_ZND_DESCR_ZA_ZRWAV |
				 XNVME_SPEC_ZND_DESCR_ZA_ZDEV),
		    "za of struct xnvme_spec_znd_descr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_znd_descr, zai,
			 XNVME_SPEC_ZND_DESCR_ZAI_FZRTL | XNVME_SPEC_ZND_DESCR_ZAI_RZRTL),
		    "zai of struct xnvme_spec_znd_descr")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_znd_descr, zsattr) == 1,
		    "zsattr of struct xnvme_spec_znd_descr")
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

/**
 * A member is where the specification places it and as wide: 'offset' and 'size' are the bytes of
 * its figure
 */
#define SPEC_AT(type, field, offset, size)                                      \
	XNVME_STATIC_ASSERT(offsetof(type, field) == (offset) &&                \
				    XNVME_SIZEOF_MEMBER(type, field) == (size), \
			    #field " of " #type)

/* Zoned Namespace Command Set 1.5, Figure 51 */
SPEC_AT(struct xnvme_spec_znd_idfy_ctrlr, zasl, 0, 1)
SPEC_AT(struct xnvme_spec_znd_idfy_ctrlr, zctratt, 4, 4)
SPEC_AT(struct xnvme_spec_znd_idfy_ctrlr, tar, 8, 4)
SPEC_AT(struct xnvme_spec_znd_idfy_ctrlr, uar, 12, 4)
SPEC_AT(struct xnvme_spec_znd_idfy_ctrlr, tor, 16, 4)
SPEC_AT(struct xnvme_spec_znd_idfy_ctrlr, uor, 20, 4)
SPEC_AT(struct xnvme_spec_znd_idfy_ctrlr, tzrwar, 24, 4)
SPEC_AT(struct xnvme_spec_znd_idfy_ctrlr, uzrwar, 28, 4)
SPEC_AT(struct xnvme_spec_znd_idfy_ctrlr, ver, 32, 4)
XNVME_STATIC_ASSERT(XNVME_SPEC_ZND_IDFY_CTRLR_ZCTRATT_ZNRM == 0x1, "ZNRM is bit 0 of ZCTRATT")

/* Zoned Namespace Command Set 1.5, Figure 49 */
SPEC_AT(struct xnvme_spec_znd_idfy_ns, rrl, 12, 4)
SPEC_AT(struct xnvme_spec_znd_idfy_ns, frl, 16, 4)
SPEC_AT(struct xnvme_spec_znd_idfy_ns, rrl1, 20, 4)
SPEC_AT(struct xnvme_spec_znd_idfy_ns, rrl2, 24, 4)
SPEC_AT(struct xnvme_spec_znd_idfy_ns, rrl3, 28, 4)
SPEC_AT(struct xnvme_spec_znd_idfy_ns, frl1, 32, 4)
SPEC_AT(struct xnvme_spec_znd_idfy_ns, frl2, 36, 4)
SPEC_AT(struct xnvme_spec_znd_idfy_ns, frl3, 40, 4)
SPEC_AT(struct xnvme_spec_znd_idfy_ns, numzrwa, 44, 4)
SPEC_AT(struct xnvme_spec_znd_idfy_ns, zrwafg, 48, 2)
SPEC_AT(struct xnvme_spec_znd_idfy_ns, zrwasz, 50, 2)
SPEC_AT(struct xnvme_spec_znd_idfy_ns, zrwacap, 52, 1)
SPEC_AT(struct xnvme_spec_znd_idfy_ns, lbafe, 2816, 64 * 16)
SPEC_AT(struct xnvme_spec_znd_idfy_ns, vs, 3840, 256)
/* Zoned Namespace Command Set 1.5, Figure 50 */
SPEC_AT(struct xnvme_spec_znd_idfy_lbafe, zsze, 0, 8)
SPEC_AT(struct xnvme_spec_znd_idfy_lbafe, zdes, 8, 1)

/* Zoned Namespace Command Set 1.5, Figure 47 */
SPEC_AT(struct xnvme_spec_znd_log_changes, nzid, 0, 2)
SPEC_AT(struct xnvme_spec_znd_log_changes, idents, 8, 511 * 8)

/* Zoned Namespace Command Set 1.5, Figure 38 */
SPEC_AT(struct xnvme_spec_znd_descr, ztattr, 0, 1)
SPEC_AT(struct xnvme_spec_znd_descr, zsattr, 1, 1)
SPEC_AT(struct xnvme_spec_znd_descr, za, 2, 1)
SPEC_AT(struct xnvme_spec_znd_descr, zai, 3, 1)
SPEC_AT(struct xnvme_spec_znd_descr, zcap, 8, 8)
SPEC_AT(struct xnvme_spec_znd_descr, zslba, 16, 8)
SPEC_AT(struct xnvme_spec_znd_descr, wp, 24, 8)
XNVME_STATIC_ASSERT(XNVME_SPEC_ZND_DESCR_ZAI_FZRTL == 0x3 && XNVME_SPEC_ZND_DESCR_ZAI_RZRTL == 0xc,
		    "FZRTL is bits 1:0 and RZRTL bits 3:2 of ZAI")

/* Zoned Namespace Command Set 1.5, Figure 36 */
SPEC_AT(struct xnvme_spec_znd_report_hdr, nz, 0, 8)

/* NVM Command Set 1.3, Figures 69 to 75 */
SPEC_AT(struct xnvme_spec_cmd_nvm, cdw02_03, 8, 8)
SPEC_AT(struct xnvme_spec_cmd_nvm, slba, 40, 8)
SPEC_AT(struct xnvme_spec_cmd_nvm, cdw12, 48, 4)
SPEC_AT(struct xnvme_spec_cmd_nvm, cdw13, 52, 4)
SPEC_AT(struct xnvme_spec_cmd_nvm, lbtl, 56, 4)
SPEC_AT(struct xnvme_spec_cmd_nvm, lbat, 60, 2)
SPEC_AT(struct xnvme_spec_cmd_nvm, lbatm, 62, 2)

/* NVM Command Set 1.3, Figures 81 to 87 */
SPEC_AT(struct xnvme_spec_nvm_write_zeroes, cdw02_03, 8, 8)
SPEC_AT(struct xnvme_spec_nvm_write_zeroes, slba, 40, 8)
SPEC_AT(struct xnvme_spec_nvm_write_zeroes, cdw13, 52, 4)
SPEC_AT(struct xnvme_spec_nvm_write_zeroes, lbtl, 56, 4)
SPEC_AT(struct xnvme_spec_nvm_write_zeroes, lbat, 60, 2)
SPEC_AT(struct xnvme_spec_nvm_write_zeroes, lbatm, 62, 2)
XNVME_STATIC_ASSERT(XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_DTYPE == 0x700000 &&
			    XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_NSZ == 0x800000,
		    "DTYPE is bits 22:20 and NSZ bit 23 of Write Zeroes dword 12")

/* NVM Command Set 1.3, Figures 25 to 30 */
SPEC_AT(struct xnvme_spec_nvm_compare, cdw02_03, 8, 8)
SPEC_AT(struct xnvme_spec_nvm_compare, slba, 40, 8)
SPEC_AT(struct xnvme_spec_nvm_compare, cdw13, 52, 4)
SPEC_AT(struct xnvme_spec_nvm_compare, elbtl, 56, 4)
SPEC_AT(struct xnvme_spec_nvm_compare, elbat, 60, 2)
SPEC_AT(struct xnvme_spec_nvm_compare, elbatm, 62, 2)
XNVME_STATIC_ASSERT(XNVME_SPEC_NVM_COMPARE_CDW12_CETYPE == 0xf0000,
		    "CETYPE is bits 19:16 of Compare dword 12")

/* NVM Command Set 1.3, Figures 33 to 38 */
SPEC_AT(struct xnvme_spec_nvm_cmd_scopy, cdw02_03, 8, 8)
SPEC_AT(struct xnvme_spec_nvm_cmd_scopy, sdlba, 40, 8)
SPEC_AT(struct xnvme_spec_nvm_cmd_scopy, cev, 52, 2)
SPEC_AT(struct xnvme_spec_nvm_cmd_scopy, dspec, 54, 2)
SPEC_AT(struct xnvme_spec_nvm_cmd_scopy, lbtl, 56, 4)
SPEC_AT(struct xnvme_spec_nvm_cmd_scopy, lbat, 60, 2)
SPEC_AT(struct xnvme_spec_nvm_cmd_scopy, lbatm, 62, 2)
XNVME_STATIC_ASSERT(XNVME_SPEC_NVM_CMD_SCOPY_CDW12_CETYPE == 0xf0000 &&
			    XNVME_SPEC_NVM_CMD_SCOPY_CDW12_STCW == 0x1000000 &&
			    XNVME_SPEC_NVM_CMD_SCOPY_CDW12_STCR == 0x2000000,
		    "CETYPE is bits 19:16, STCW bit 24 and STCR bit 25 of Copy dword 12")

/* NVM Command Set 1.3, Figure 40 */
SPEC_AT(struct xnvme_spec_nvm_scopy_fmt_zero, spars, 0, 4)
SPEC_AT(struct xnvme_spec_nvm_scopy_fmt_zero, slba, 8, 8)
SPEC_AT(struct xnvme_spec_nvm_scopy_fmt_zero, rpars, 16, 4)
SPEC_AT(struct xnvme_spec_nvm_scopy_fmt_zero, cev, 20, 2)
SPEC_AT(struct xnvme_spec_nvm_scopy_fmt_zero, sopt, 22, 2)
SPEC_AT(struct xnvme_spec_nvm_scopy_fmt_zero, elbt, 24, 4)
SPEC_AT(struct xnvme_spec_nvm_scopy_fmt_zero, elbat, 28, 2)
SPEC_AT(struct xnvme_spec_nvm_scopy_fmt_zero, elbatm, 30, 2)
XNVME_STATIC_ASSERT(XNVME_SPEC_NVM_SCOPY_FMT_ZERO_RPARS_NLB == 0xffff &&
			    XNVME_SPEC_NVM_SCOPY_FMT_ZERO_RPARS_CETYPE == 0xf0000,
		    "NLB is bits 15:00 and CETYPE bits 19:16 of RPARS")

/* NVM Command Set 1.3, Figure 129 */
SPEC_AT(struct xnvme_spec_nvm_idfy_ctrlr, dmrsl, 4, 4)
SPEC_AT(struct xnvme_spec_nvm_idfy_ctrlr, dmsl, 8, 8)
SPEC_AT(struct xnvme_spec_nvm_idfy_ctrlr, kpiocap, 16, 1)
SPEC_AT(struct xnvme_spec_nvm_idfy_ctrlr, wzdsl, 17, 1)
SPEC_AT(struct xnvme_spec_nvm_idfy_ctrlr, aocs, 18, 2)
SPEC_AT(struct xnvme_spec_nvm_idfy_ctrlr, ver, 20, 4)
SPEC_AT(struct xnvme_spec_nvm_idfy_ctrlr, lbamqf, 24, 1)
SPEC_AT(struct xnvme_spec_nvm_idfy_ctrlr, rla, 25, 1)
SPEC_AT(struct xnvme_spec_nvm_idfy_ctrlr, slmc, 26, 2)
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_nvm_idfy_ctrlr, kpiocap,
			 XNVME_SPEC_NVM_IDFY_CTRLR_KPIOCAP_KPIOS |
				 XNVME_SPEC_NVM_IDFY_CTRLR_KPIOCAP_KPIOSC),
		    "kpiocap of struct xnvme_spec_nvm_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_nvm_idfy_ctrlr, rla,
			 XNVME_SPEC_NVM_IDFY_CTRLR_RLA_HLS | XNVME_SPEC_NVM_IDFY_CTRLR_RLA_SLS),
		    "rla of struct xnvme_spec_nvm_idfy_ctrlr")
XNVME_STATIC_ASSERT(XNVME_SPEC_NVM_IDFY_CTRLR_KPIOCAP_KPIOSC == 0x2 &&
			    XNVME_SPEC_NVM_IDFY_CTRLR_RLA_SLS == 0x2,
		    "KPIOSC and SLS are bit 1 of their fields")

/* NVM Command Set 1.3, Figure 127 */
SPEC_AT(struct xnvme_spec_nvm_idfy_ns, pic, 8, 1)
SPEC_AT(struct xnvme_spec_nvm_idfy_ns, pifa, 9, 1)
SPEC_AT(struct xnvme_spec_nvm_idfy_ns, elbaf, 12, 64 * 4)
SPEC_AT(struct xnvme_spec_nvm_idfy_ns, npdgl, 268, 4)
SPEC_AT(struct xnvme_spec_nvm_idfy_ns, nprg, 272, 4)
SPEC_AT(struct xnvme_spec_nvm_idfy_ns, npra, 276, 4)
SPEC_AT(struct xnvme_spec_nvm_idfy_ns, nors, 280, 4)
SPEC_AT(struct xnvme_spec_nvm_idfy_ns, npdal, 284, 4)
SPEC_AT(struct xnvme_spec_nvm_idfy_ns, lbapss, 288, 4)
SPEC_AT(struct xnvme_spec_nvm_idfy_ns, tlbaag, 292, 4)
XNVME_STATIC_ASSERT(XNVME_SPEC_NVM_IDFY_NS_PIC_QPIFS == 0x8 &&
			    XNVME_SPEC_NVM_IDFY_NS_PIFA_STMLA == 0x7,
		    "QPIFS is bit 3 of PIC and STMLA bits 2:0 of PIFA")

/* NVM Command Set 1.3, Figure 128 */
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_elbaf, bits,
			 XNVME_SPEC_ELBAF_BITS_STS | XNVME_SPEC_ELBAF_BITS_PIF |
				 XNVME_SPEC_ELBAF_BITS_QPIF),
		    "bits of struct xnvme_spec_elbaf")
XNVME_STATIC_ASSERT(XNVME_SPEC_ELBAF_BITS_STS == 0x7f && XNVME_SPEC_ELBAF_BITS_PIF == 0x180 &&
			    XNVME_SPEC_ELBAF_BITS_QPIF == 0x1e00,
		    "STS is bits 6:0, PIF bits 8:7 and QPIF bits 12:9 of ELBAF")

/* NVM Command Set 1.3, Figures 123 and 125 */
SPEC_AT(struct xnvme_spec_idfy_ns, rescap, 31, 1)
SPEC_AT(struct xnvme_spec_idfy_ns, msrc, 80, 1)
SPEC_AT(struct xnvme_spec_idfy_ns, kpios, 81, 1)
SPEC_AT(struct xnvme_spec_idfy_ns, nulbaf, 82, 1)
SPEC_AT(struct xnvme_spec_idfy_ns, kpiodaag, 84, 4)
SPEC_AT(struct xnvme_spec_idfy_ns, anagrpid, 92, 4)
SPEC_AT(struct xnvme_spec_idfy_ns, nsattr, 99, 1)
SPEC_AT(struct xnvme_spec_idfy_ns, nvmsetid, 100, 2)
SPEC_AT(struct xnvme_spec_idfy_ns, endgid, 102, 2)
SPEC_AT(struct xnvme_spec_idfy_ns, nguid, 104, 16)
SPEC_AT(struct xnvme_spec_idfy_ns, eui64, 120, 8)
SPEC_AT(struct xnvme_spec_idfy_ns, vs, 384, 3712)
SPEC_AT(struct xnvme_spec_lbaf, ms, 0, 2)
SPEC_AT(struct xnvme_spec_lbaf, lbads, 2, 1)
SPEC_AT(struct xnvme_spec_lbaf, rp, 3, 1)
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, kpios,
			 XNVME_SPEC_IDFY_NS_KPIOS_KPIOENS | XNVME_SPEC_IDFY_NS_KPIOS_KPIOSNS),
		    "kpios of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ns, nsattr, XNVME_SPEC_IDFY_NS_NSATTR_CWP),
		    "nsattr of struct xnvme_spec_idfy_ns")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_NS_NSFEAT_OPTPERF == 0x30 &&
			    XNVME_SPEC_IDFY_NS_NSFEAT_MAM == 0x40 &&
			    XNVME_SPEC_IDFY_NS_NSFEAT_OPTRPERF == 0x80,
		    "OPTPERF is bits 5:4, MAM bit 6 and OPTRPERF bit 7 of NSFEAT")

/* NVM Command Set 1.3, Figures 155, 157 and 159 */
SPEC_AT(struct xnvme_pif_g16, guard, 0, 2)
SPEC_AT(struct xnvme_pif_g16, app_tag, 2, 2)
SPEC_AT(struct xnvme_pif_g16, stor_ref_space, 4, 4)
SPEC_AT(struct xnvme_pif_g32, guard, 0, 4)
SPEC_AT(struct xnvme_pif_g32, app_tag, 4, 2)
SPEC_AT(struct xnvme_pif_g32, stor_ref_space, 6, 10)
SPEC_AT(struct xnvme_pif_g64, guard, 0, 8)
SPEC_AT(struct xnvme_pif_g64, app_tag, 8, 2)
SPEC_AT(struct xnvme_pif_g64, stor_ref_space, 10, 6)

/* Base 2.4, Figure 340 */
SPEC_AT(struct xnvme_spec_power_state, actp, 20, 2)
SPEC_AT(struct xnvme_spec_power_state, apw_aps, 22, 1)
SPEC_AT(struct xnvme_spec_power_state, epfrt, 23, 1)
SPEC_AT(struct xnvme_spec_power_state, fqvt, 24, 1)
SPEC_AT(struct xnvme_spec_power_state, epfvt, 25, 1)
SPEC_AT(struct xnvme_spec_power_state, epfrts_fqvts, 26, 1)
SPEC_AT(struct xnvme_spec_power_state, epfvts, 27, 1)
SPEC_AT(struct xnvme_spec_power_state, mbw, 28, 1)
SPEC_AT(struct xnvme_spec_power_state, mbws, 29, 1)
SPEC_AT(struct xnvme_spec_power_state, miiell, 30, 2)
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_power_state, epfrts_fqvts,
			 XNVME_SPEC_POWER_STATE_EPFRTS_FQVTS_EPFRTS |
				 XNVME_SPEC_POWER_STATE_EPFRTS_FQVTS_FQVTS),
		    "epfrts_fqvts of struct xnvme_spec_power_state")
XNVME_STATIC_ASSERT(XNVME_SPEC_POWER_STATE_EPFRTS_FQVTS_FQVTS == 0xf0 &&
			    XNVME_SPEC_POWER_STATE_EPFVTS_EPFVTS == 0xf &&
			    XNVME_SPEC_POWER_STATE_MBWS_MBWS == 0x7,
		    "FQVTS is bits 215:212, EPFVTS bits 219:216 and MBWS bits 234:232")

/* Base 2.4, Figure 338, bytes 2047:1792 */
SPEC_AT(struct xnvme_spec_idfy_ctrlr_nvmf, fcatt, 1802 - 1792, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr_nvmf, ofcs, 1804 - 1792, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr_nvmf, dctype, 1806 - 1792, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr_nvmf, ccrl, 1807 - 1792, 1)

/* Base 2.4, Figure 355 */
XNVME_STATIC_ASSERT(XNVME_SPEC_CS_VECTOR_BITS_SLMCS == 0x8 &&
			    XNVME_SPEC_CS_VECTOR_BITS_CPNCS == 0x10,
		    "SLMCS is bit 3 and CPNCS bit 4 of the I/O Command Set Vector")

/* Base 2.4, Figure 338 */
SPEC_AT(struct xnvme_spec_idfy_ctrlr, rtd3e, 88, 4)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, rrls, 100, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, bpcap, 102, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, chsi, 103, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, nssl, 104, 4)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, plsi, 110, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, cntrltype, 111, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, crcap, 134, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, ciu, 135, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, cirn, 136, 8)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, nvmsr, 253, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, did, 356, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, kpioc, 358, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, mptfawr, 360, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, rmdca, 362, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, megcap, 368, 16)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, tmpthha, 384, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, mupa, 385, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, cqt, 386, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, cdpa, 388, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, mup, 390, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, ipmsr, 392, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, msmt, 394, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, mnens, 396, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, mnecpens, 398, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, mensnn, 400, 4)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, ensa, 404, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, endsfs, 405, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, vsen1, 406, 4)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, vsen2, 410, 4)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, vsen3, 414, 4)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, vsen4, 418, 4)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, msvmt, 422, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, sqes, 512, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, fuses, 522, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, icsvscc, 530, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, nwpc, 531, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, maxcna, 560, 4)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, oaqd, 564, 4)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, rhiri, 568, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, hirt, 569, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, cmmrtd, 570, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, nmmrtd, 572, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, minmrtg, 574, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, maxmrtg, 575, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, trattr, 576, 1)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, mcudmq, 578, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, mnsudmq, 580, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, mcmr, 582, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, nmcmr, 584, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, mcdqpc, 586, 2)
SPEC_AT(struct xnvme_spec_idfy_ctrlr, subnqn, 768, 256)
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, cmic,
			 XNVME_SPEC_IDFY_CTRLR_CMIC_MPORTS | XNVME_SPEC_IDFY_CTRLR_CMIC_MCTRS |
				 XNVME_SPEC_IDFY_CTRLR_CMIC_FT | XNVME_SPEC_IDFY_CTRLR_CMIC_ANARS),
		    "cmic of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, ver,
			 XNVME_SPEC_IDFY_CTRLR_VER_TER | XNVME_SPEC_IDFY_CTRLR_VER_MNR |
				 XNVME_SPEC_IDFY_CTRLR_VER_MJR),
		    "ver of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(
	FITS(struct xnvme_spec_idfy_ctrlr, oaes,
	     XNVME_SPEC_IDFY_CTRLR_OAES_NSAN | XNVME_SPEC_IDFY_CTRLR_OAES_FAN |
		     XNVME_SPEC_IDFY_CTRLR_OAES_ANACN | XNVME_SPEC_IDFY_CTRLR_OAES_PLEAN |
		     XNVME_SPEC_IDFY_CTRLR_OAES_LSIAN | XNVME_SPEC_IDFY_CTRLR_OAES_EGEAN |
		     XNVME_SPEC_IDFY_CTRLR_OAES_NNSS | XNVME_SPEC_IDFY_CTRLR_OAES_TTHR |
		     XNVME_SPEC_IDFY_CTRLR_OAES_RGCNS | XNVME_SPEC_IDFY_CTRLR_OAES_ANSAN |
		     XNVME_SPEC_IDFY_CTRLR_OAES_CCRCN | XNVME_SPEC_IDFY_CTRLR_OAES_LHCN |
		     XNVME_SPEC_IDFY_CTRLR_OAES_RLCC | XNVME_SPEC_IDFY_CTRLR_OAES_ZDCN |
		     XNVME_SPEC_IDFY_CTRLR_OAES_DLPCN),
	"oaes of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_OAES_TTHR == 0x10000, "TTHR is bit 16 of OAES")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_OAES_RGCNS == 0x20000, "RGCNS is bit 17 of OAES")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_OAES_ANSAN == 0x80000, "ANSAN is bit 19 of OAES")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_OAES_CCRCN == 0x100000, "CCRCN is bit 20 of OAES")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_OAES_LHCN == 0x200000, "LHCN is bit 21 of OAES")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_OAES_RLCC == 0x400000, "RLCC is bit 22 of OAES")
XNVME_STATIC_ASSERT(
	FITS(struct xnvme_spec_idfy_ctrlr, ctratt,
	     XNVME_SPEC_IDFY_CTRLR_CTRATT_HIDS | XNVME_SPEC_IDFY_CTRLR_CTRATT_NOPSPM |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_NSETS | XNVME_SPEC_IDFY_CTRLR_CTRATT_RRLVLS |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_EGS | XNVME_SPEC_IDFY_CTRLR_CTRATT_PLM |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_TBKAS | XNVME_SPEC_IDFY_CTRLR_CTRATT_NG |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_SQA | XNVME_SPEC_IDFY_CTRLR_CTRATT_ULIST |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_MDS | XNVME_SPEC_IDFY_CTRLR_CTRATT_FCM |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_VCM | XNVME_SPEC_IDFY_CTRLR_CTRATT_DEG |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_DNVMS | XNVME_SPEC_IDFY_CTRLR_CTRATT_ELBAS |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_MEM | XNVME_SPEC_IDFY_CTRLR_CTRATT_HMBR |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_RHII | XNVME_SPEC_IDFY_CTRLR_CTRATT_FDPS |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_PLS | XNVME_SPEC_IDFY_CTRLR_CTRATT_PMS |
		     XNVME_SPEC_IDFY_CTRLR_CTRATT_VMS | XNVME_SPEC_IDFY_CTRLR_CTRATT_IIELLSS),
	"ctratt of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_CTRATT_MEM == 0x10000, "MEM is bit 16 of CTRATT")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_CTRATT_HMBR == 0x20000, "HMBR is bit 17 of CTRATT")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_CTRATT_RHII == 0x40000, "RHII is bit 18 of CTRATT")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_CTRATT_PLS == 0x100000, "PLS is bit 20 of CTRATT")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_CTRATT_PMS == 0x200000, "PMS is bit 21 of CTRATT")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_CTRATT_VMS == 0x400000, "VMS is bit 22 of CTRATT")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_CTRATT_IIELLSS == 0x1800000,
		    "IIELLSS is bits 24:23 of CTRATT")
XNVME_STATIC_ASSERT(
	FITS(struct xnvme_spec_idfy_ctrlr, rrls,
	     XNVME_SPEC_IDFY_CTRLR_RRLS_RRL0 | XNVME_SPEC_IDFY_CTRLR_RRLS_RRL1 |
		     XNVME_SPEC_IDFY_CTRLR_RRLS_RRL2 | XNVME_SPEC_IDFY_CTRLR_RRLS_RRL3 |
		     XNVME_SPEC_IDFY_CTRLR_RRLS_RRL4 | XNVME_SPEC_IDFY_CTRLR_RRLS_RRL5 |
		     XNVME_SPEC_IDFY_CTRLR_RRLS_RRL6 | XNVME_SPEC_IDFY_CTRLR_RRLS_RRL7 |
		     XNVME_SPEC_IDFY_CTRLR_RRLS_RRL8 | XNVME_SPEC_IDFY_CTRLR_RRLS_RRL9 |
		     XNVME_SPEC_IDFY_CTRLR_RRLS_RRL10 | XNVME_SPEC_IDFY_CTRLR_RRLS_RRL11 |
		     XNVME_SPEC_IDFY_CTRLR_RRLS_RRL12 | XNVME_SPEC_IDFY_CTRLR_RRLS_RRL13 |
		     XNVME_SPEC_IDFY_CTRLR_RRLS_RRL14 | XNVME_SPEC_IDFY_CTRLR_RRLS_RRL15),
	"rrls of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_RRLS_RRL0 == 0x1, "RRL0 is bit 0 of RRLS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_RRLS_RRL1 == 0x2, "RRL1 is bit 1 of RRLS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_RRLS_RRL2 == 0x4, "RRL2 is bit 2 of RRLS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_RRLS_RRL3 == 0x8, "RRL3 is bit 3 of RRLS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_RRLS_RRL4 == 0x10, "RRL4 is bit 4 of RRLS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_RRLS_RRL5 == 0x20, "RRL5 is bit 5 of RRLS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_RRLS_RRL6 == 0x40, "RRL6 is bit 6 of RRLS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_RRLS_RRL7 == 0x80, "RRL7 is bit 7 of RRLS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_RRLS_RRL8 == 0x100, "RRL8 is bit 8 of RRLS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_RRLS_RRL9 == 0x200, "RRL9 is bit 9 of RRLS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_RRLS_RRL10 == 0x400, "RRL10 is bit 10 of RRLS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_RRLS_RRL11 == 0x800, "RRL11 is bit 11 of RRLS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_RRLS_RRL12 == 0x1000, "RRL12 is bit 12 of RRLS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_RRLS_RRL13 == 0x2000, "RRL13 is bit 13 of RRLS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_RRLS_RRL14 == 0x4000, "RRL14 is bit 14 of RRLS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_RRLS_RRL15 == 0x8000, "RRL15 is bit 15 of RRLS")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, nvmsr,
			 XNVME_SPEC_IDFY_CTRLR_NVMSR_NVMESD | XNVME_SPEC_IDFY_CTRLR_NVMSR_NVMEE),
		    "nvmsr of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, vwci,
			 XNVME_SPEC_IDFY_CTRLR_VWCI_VWCR | XNVME_SPEC_IDFY_CTRLR_VWCI_VWCRV),
		    "vwci of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, mec,
			 XNVME_SPEC_IDFY_CTRLR_MEC_TWPME | XNVME_SPEC_IDFY_CTRLR_MEC_PCIEME),
		    "mec of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(
	FITS(struct xnvme_spec_idfy_ctrlr, oacs,
	     XNVME_SPEC_IDFY_CTRLR_OACS_SSRS | XNVME_SPEC_IDFY_CTRLR_OACS_FNVMS |
		     XNVME_SPEC_IDFY_CTRLR_OACS_FWDS | XNVME_SPEC_IDFY_CTRLR_OACS_NMS |
		     XNVME_SPEC_IDFY_CTRLR_OACS_DSTS | XNVME_SPEC_IDFY_CTRLR_OACS_DIRS |
		     XNVME_SPEC_IDFY_CTRLR_OACS_NSRS | XNVME_SPEC_IDFY_CTRLR_OACS_VMS |
		     XNVME_SPEC_IDFY_CTRLR_OACS_DBCS | XNVME_SPEC_IDFY_CTRLR_OACS_GLSS |
		     XNVME_SPEC_IDFY_CTRLR_OACS_CFLS | XNVME_SPEC_IDFY_CTRLR_OACS_HMLMS |
		     XNVME_SPEC_IDFY_CTRLR_OACS_ENSS | XNVME_SPEC_IDFY_CTRLR_OACS_CCFLS),
	"oacs of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_OACS_HMLMS == 0x800, "HMLMS is bit 11 of OACS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_OACS_ENSS == 0x1000, "ENSS is bit 12 of OACS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_OACS_CCFLS == 0x2000, "CCFLS is bit 13 of OACS")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, frmw,
			 XNVME_SPEC_IDFY_CTRLR_FRMW_FFSRO | XNVME_SPEC_IDFY_CTRLR_FRMW_NOFS |
				 XNVME_SPEC_IDFY_CTRLR_FRMW_FAWR |
				 XNVME_SPEC_IDFY_CTRLR_FRMW_SMUD),
		    "frmw of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, lpa,
			 XNVME_SPEC_IDFY_CTRLR_LPA_SMARTS | XNVME_SPEC_IDFY_CTRLR_LPA_CSES |
				 XNVME_SPEC_IDFY_CTRLR_LPA_LPEDS | XNVME_SPEC_IDFY_CTRLR_LPA_TS |
				 XNVME_SPEC_IDFY_CTRLR_LPA_PES | XNVME_SPEC_IDFY_CTRLR_LPA_MLPS |
				 XNVME_SPEC_IDFY_CTRLR_LPA_DA4S),
		    "lpa of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, avscc, XNVME_SPEC_IDFY_CTRLR_AVSCC_VSCF),
		    "avscc of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, apsta, XNVME_SPEC_IDFY_CTRLR_APSTA_APTS),
		    "apsta of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, rpmbs,
			 XNVME_SPEC_IDFY_CTRLR_RPMBS_NRPMBU | XNVME_SPEC_IDFY_CTRLR_RPMBS_AUTHM |
				 XNVME_SPEC_IDFY_CTRLR_RPMBS_TSZE |
				 XNVME_SPEC_IDFY_CTRLR_RPMBS_ASZE),
		    "rpmbs of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, dsto,
			 XNVME_SPEC_IDFY_CTRLR_DSTO_SDSO | XNVME_SPEC_IDFY_CTRLR_DSTO_HIRS),
		    "dsto of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_DSTO_HIRS == 0x2, "HIRS is bit 1 of DSTO")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, hctma, XNVME_SPEC_IDFY_CTRLR_HCTMA_HCTMS),
		    "hctma of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(
	FITS(struct xnvme_spec_idfy_ctrlr, sanicap,
	     XNVME_SPEC_IDFY_CTRLR_SANICAP_CES | XNVME_SPEC_IDFY_CTRLR_SANICAP_BES |
		     XNVME_SPEC_IDFY_CTRLR_SANICAP_OWS | XNVME_SPEC_IDFY_CTRLR_SANICAP_VERS |
		     XNVME_SPEC_IDFY_CTRLR_SANICAP_NVERS | XNVME_SPEC_IDFY_CTRLR_SANICAP_SPRRS |
		     XNVME_SPEC_IDFY_CTRLR_SANICAP_NDI | XNVME_SPEC_IDFY_CTRLR_SANICAP_NODMMAS),
	"sanicap of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_SANICAP_VERS == 0x8, "VERS is bit 3 of SANICAP")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_SANICAP_NVERS == 0x10, "NVERS is bit 4 of SANICAP")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_SANICAP_SPRRS == 0x20, "SPRRS is bit 5 of SANICAP")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, anacap,
			 XNVME_SPEC_IDFY_CTRLR_ANACAP_RANAOS |
				 XNVME_SPEC_IDFY_CTRLR_ANACAP_RANANOS |
				 XNVME_SPEC_IDFY_CTRLR_ANACAP_RANAIS |
				 XNVME_SPEC_IDFY_CTRLR_ANACAP_RANAPLS |
				 XNVME_SPEC_IDFY_CTRLR_ANACAP_RANACS |
				 XNVME_SPEC_IDFY_CTRLR_ANACAP_ANAGIDLWAS |
				 XNVME_SPEC_IDFY_CTRLR_ANACAP_ANAGIDS),
		    "anacap of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, sqes,
			 XNVME_SPEC_IDFY_CTRLR_SQES_MINSQES | XNVME_SPEC_IDFY_CTRLR_SQES_MAXSQES),
		    "sqes of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, cqes,
			 XNVME_SPEC_IDFY_CTRLR_CQES_MINCQES | XNVME_SPEC_IDFY_CTRLR_CQES_MAXCQES),
		    "cqes of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(
	FITS(struct xnvme_spec_idfy_ctrlr, oncs,
	     XNVME_SPEC_IDFY_CTRLR_ONCS_NVMCMPS | XNVME_SPEC_IDFY_CTRLR_ONCS_NVMWUSV |
		     XNVME_SPEC_IDFY_CTRLR_ONCS_NVMDSMSV | XNVME_SPEC_IDFY_CTRLR_ONCS_NVMWZSV |
		     XNVME_SPEC_IDFY_CTRLR_ONCS_SSFS | XNVME_SPEC_IDFY_CTRLR_ONCS_RESERVS |
		     XNVME_SPEC_IDFY_CTRLR_ONCS_TSS | XNVME_SPEC_IDFY_CTRLR_ONCS_NVMVFYS |
		     XNVME_SPEC_IDFY_CTRLR_ONCS_NVMCPYS | XNVME_SPEC_IDFY_CTRLR_ONCS_NVMCSA |
		     XNVME_SPEC_IDFY_CTRLR_ONCS_NVMAFC | XNVME_SPEC_IDFY_CTRLR_ONCS_MAXWZD |
		     XNVME_SPEC_IDFY_CTRLR_ONCS_NSZS),
	"oncs of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_ONCS_NVMCSA == 0x200, "NVMCSA is bit 9 of ONCS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_ONCS_NVMAFC == 0x400, "NVMAFC is bit 10 of ONCS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_ONCS_MAXWZD == 0x800, "MAXWZD is bit 11 of ONCS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_ONCS_NSZS == 0x1000, "NSZS is bit 12 of ONCS")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, fuses, XNVME_SPEC_IDFY_CTRLR_FUSES_FCWS),
		    "fuses of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_FUSES_FCWS == 0x1, "FCWS is bit 0 of FUSES")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, fna,
			 XNVME_SPEC_IDFY_CTRLR_FNA_FNS | XNVME_SPEC_IDFY_CTRLR_FNA_SENS |
				 XNVME_SPEC_IDFY_CTRLR_FNA_CRYES |
				 XNVME_SPEC_IDFY_CTRLR_FNA_FNVMBS),
		    "fna of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, vwc,
			 XNVME_SPEC_IDFY_CTRLR_VWC_VWCP | XNVME_SPEC_IDFY_CTRLR_VWC_FB),
		    "vwc of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, icsvscc,
			 XNVME_SPEC_IDFY_CTRLR_ICSVSCC_SNVSCF),
		    "icsvscc of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_ICSVSCC_SNVSCF == 0x1, "SNVSCF is bit 0 of ICSVSCC")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, nwpc,
			 XNVME_SPEC_IDFY_CTRLR_NWPC_NWPWPS | XNVME_SPEC_IDFY_CTRLR_NWPC_WPUPCS |
				 XNVME_SPEC_IDFY_CTRLR_NWPC_PWPS),
		    "nwpc of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_NWPC_NWPWPS == 0x1, "NWPWPS is bit 0 of NWPC")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_NWPC_WPUPCS == 0x2, "WPUPCS is bit 1 of NWPC")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_NWPC_PWPS == 0x4, "PWPS is bit 2 of NWPC")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_idfy_ctrlr, cdfs,
			 XNVME_SPEC_IDFY_CTRLR_CDFS_CDF0S | XNVME_SPEC_IDFY_CTRLR_CDFS_CDF1S |
				 XNVME_SPEC_IDFY_CTRLR_CDFS_CDF2S |
				 XNVME_SPEC_IDFY_CTRLR_CDFS_CDF3S |
				 XNVME_SPEC_IDFY_CTRLR_CDFS_CDF4S),
		    "cdfs of struct xnvme_spec_idfy_ctrlr")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_CDFS_CDF2S == 0x4, "CDF2S is bit 2 of CDFS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_CDFS_CDF3S == 0x8, "CDF3S is bit 3 of CDFS")
XNVME_STATIC_ASSERT(XNVME_SPEC_IDFY_CTRLR_CDFS_CDF4S == 0x10, "CDF4S is bit 4 of CDFS")
XNVME_STATIC_ASSERT(
	FITS(struct xnvme_spec_idfy_ctrlr, sgls,
	     XNVME_SPEC_IDFY_CTRLR_SGLS_SGLS | XNVME_SPEC_IDFY_CTRLR_SGLS_KSDBDS |
		     XNVME_SPEC_IDFY_CTRLR_SGLS_SDT | XNVME_SPEC_IDFY_CTRLR_SGLS_SBBDS |
		     XNVME_SPEC_IDFY_CTRLR_SGLS_MBA | XNVME_SPEC_IDFY_CTRLR_SGLS_LLDTS |
		     XNVME_SPEC_IDFY_CTRLR_SGLS_MSDS | XNVME_SPEC_IDFY_CTRLR_SGLS_SAOS |
		     XNVME_SPEC_IDFY_CTRLR_SGLS_TSDBDS),
	"sgls of struct xnvme_spec_idfy_ctrlr")

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
	if (xnvme_bits_get(idfy.ns.flbas, XNVME_SPEC_IDFY_NS_FLBAS_FIDXL) != 0xa ||
	    !xnvme_bits_get(idfy.ns.flbas, XNVME_SPEC_IDFY_NS_FLBAS_MTELBA) ||
	    xnvme_bits_get(idfy.ns.flbas, XNVME_SPEC_IDFY_NS_FLBAS_FIDXU) != 0x2) {
		xnvme_cli_pinf("FAILED: flbas: %#" PRIx8, idfy.ns.flbas);
		err = -EINVAL;
	}

	memset(&idfy, 0, sizeof(idfy));
	idfy.raw[521] = 0x01; // Identify Controller ONCS bit 8: copy
	if (!xnvme_bits_get(idfy.ctrlr.oncs, XNVME_SPEC_IDFY_CTRLR_ONCS_NVMCPYS) ||
	    xnvme_bits_get(idfy.ctrlr.oncs, XNVME_SPEC_IDFY_CTRLR_ONCS_NVMCMPS)) {
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
		if (!xnvme_bits_get(ns.pic, XNVME_SPEC_NVM_IDFY_NS_PIC_PISTS16B) ||
		    xnvme_bits_get(ns.pic, XNVME_SPEC_NVM_IDFY_NS_PIC_PISTM16B) ||
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
		if (xnvme_bits_get(ns.zoc, XNVME_SPEC_ZND_IDFY_NS_ZOC_VZC) ||
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
		if (xnvme_bits_get(descr.ztattr, XNVME_SPEC_ZND_DESCR_ZTATTR_ZT) !=
			    XNVME_SPEC_ZND_TYPE_SEQWR ||
		    xnvme_bits_get(descr.zsattr, XNVME_SPEC_ZND_DESCR_ZSATTR_ZS) !=
			    XNVME_SPEC_ZND_STATE_FULL ||
		    xnvme_bits_get(descr.za, XNVME_SPEC_ZND_DESCR_ZA_ZFC) ||
		    !xnvme_bits_get(descr.za, XNVME_SPEC_ZND_DESCR_ZA_ZRWAV) ||
		    !xnvme_bits_get(descr.za, XNVME_SPEC_ZND_DESCR_ZA_ZDEV)) {
			xnvme_cli_pinf("FAILED: zt: %#" PRIx8 ", zs: %#" PRIx8 ", za: %#" PRIx8,
				       descr.ztattr, descr.zsattr, descr.za);
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

	sqe.cdw[13] = 0x1234005b; // NVM dword 13: AF 0xb, AL 0x1, SEQREQ, DSPEC 0x1234
	if (xnvme_bits_get(sqe.nvm.cdw13, XNVME_SPEC_CMD_NVM_CDW13_AF) != 0xb ||
	    xnvme_bits_get(sqe.nvm.cdw13, XNVME_SPEC_CMD_NVM_CDW13_AL) != 0x1 ||
	    !xnvme_bits_get(sqe.nvm.cdw13, XNVME_SPEC_CMD_NVM_CDW13_SEQREQ) ||
	    xnvme_bits_get(sqe.nvm.cdw13, XNVME_SPEC_CMD_NVM_CDW13_INCPRS) ||
	    xnvme_bits_get(sqe.nvm.cdw13, XNVME_SPEC_CMD_NVM_CDW13_DSPEC) != 0x1234) {
		xnvme_cli_pinf("FAILED: nvm: cdw13: %#x", sqe.nvm.cdw13);
		err = -EINVAL;
	}

	sqe.cdw[14] = 0x89abcdef; // NVM dword 14: LBTL
	sqe.cdw[15] = 0xff001234; // NVM dword 15: LBAT 0x1234, LBATM 0xff00
	if (sqe.nvm.lbtl != 0x89abcdef || sqe.nvm.lbat != 0x1234 || sqe.nvm.lbatm != 0xff00) {
		xnvme_cli_pinf("FAILED: nvm: lbtl: %#x, lbat: %#x, lbatm: %#x", sqe.nvm.lbtl,
			       sqe.nvm.lbat, sqe.nvm.lbatm);
		err = -EINVAL;
	}

	memset(&sqe, 0, sizeof(sqe));
	sqe.cdw[2] = 0x89abcdef; // NVM dword 2 and 3: LBTU 0x456789abcdef, reserved bits set
	sqe.cdw[3] = 0xffff4567;
	sqe.cdw[12] = 0x010a0000; // NVM dword 12: CETYPE 0xa, STC
	sqe.cdw[13] = 0x0000beef; // NVM dword 13: CEV 0xbeef
	if (xnvme_bits_get(sqe.nvm.cdw02_03, XNVME_SPEC_CMD_NVM_CDW02_03_LBTU) != 0x456789abcdef ||
	    xnvme_bits_get(sqe.nvm.cdw12, XNVME_SPEC_CMD_NVM_CDW12_CETYPE) != 0xa ||
	    !xnvme_bits_get(sqe.nvm.cdw12, XNVME_SPEC_CMD_NVM_CDW12_STC) ||
	    xnvme_bits_get(sqe.nvm.cdw12, XNVME_SPEC_CMD_NVM_CDW12_DTYPE) ||
	    xnvme_bits_get(sqe.nvm.cdw13, XNVME_SPEC_CMD_NVM_CDW13_CEV) != 0xbeef ||
	    xnvme_bits_get(sqe.nvm.cdw13, XNVME_SPEC_CMD_NVM_CDW13_DSM) != 0xef) {
		xnvme_cli_pinf("FAILED: nvm: cdw02_03: %#" PRIx64 ", cdw12: %#x, cdw13: %#x",
			       sqe.nvm.cdw02_03, sqe.nvm.cdw12, sqe.nvm.cdw13);
		err = -EINVAL;
	}

	memset(&sqe, 0, sizeof(sqe));
	sqe.cdw[12] = 0xf6000007; // Write Zeroes dword 12: NLB 0x7, DEAC, PRINFO 0xd, FUA, LR
	if (xnvme_bits_get(sqe.write_zeroes.cdw12, XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_NLB) != 0x7 ||
	    xnvme_bits_get(sqe.write_zeroes.cdw12, XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_STC) ||
	    !xnvme_bits_get(sqe.write_zeroes.cdw12, XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_DEAC) ||
	    xnvme_bits_get(sqe.write_zeroes.cdw12, XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_PRINFO) !=
		    0xd ||
	    !xnvme_bits_get(sqe.write_zeroes.cdw12, XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_FUA) ||
	    !xnvme_bits_get(sqe.write_zeroes.cdw12, XNVME_SPEC_NVM_WRITE_ZEROES_CDW12_LR)) {
		xnvme_cli_pinf("FAILED: write_zeroes: cdw12: %#x", sqe.write_zeroes.cdw12);
		err = -EINVAL;
	}

	memset(&sqe, 0, sizeof(sqe));
	sqe.cdw[12] = 0x49000010; // Compare dword 12: NLB 0x10, STC, PRINFO 0x2, FUA
	if (xnvme_bits_get(sqe.compare.cdw12, XNVME_SPEC_NVM_COMPARE_CDW12_NLB) != 0x10 ||
	    !xnvme_bits_get(sqe.compare.cdw12, XNVME_SPEC_NVM_COMPARE_CDW12_STC) ||
	    xnvme_bits_get(sqe.compare.cdw12, XNVME_SPEC_NVM_COMPARE_CDW12_PRINFO) != 0x2 ||
	    !xnvme_bits_get(sqe.compare.cdw12, XNVME_SPEC_NVM_COMPARE_CDW12_FUA) ||
	    xnvme_bits_get(sqe.compare.cdw12, XNVME_SPEC_NVM_COMPARE_CDW12_LR)) {
		xnvme_cli_pinf("FAILED: compare: cdw12: %#x", sqe.compare.cdw12);
		err = -EINVAL;
	}

	memset(&sqe, 0, sizeof(sqe));
	sqe.cdw[10] = 0xff; // Dataset Management dword 10: NR 0xff
	sqe.cdw[11] = 0x5;  // Dataset Management dword 11: IDR, AD
	if (sqe.dsm.nr != 0xff || !xnvme_bits_get(sqe.dsm.cdw11, XNVME_SPEC_CMD_DSM_CDW11_IDR) ||
	    xnvme_bits_get(sqe.dsm.cdw11, XNVME_SPEC_CMD_DSM_CDW11_IDW) ||
	    !xnvme_bits_get(sqe.dsm.cdw11, XNVME_SPEC_CMD_DSM_CDW11_AD)) {
		xnvme_cli_pinf("FAILED: dsm: nr: %#x, cdw11: %#x", sqe.dsm.nr, sqe.dsm.cdw11);
		err = -EINVAL;
	}

	memset(&sqe, 0, sizeof(sqe));
	// Copy dword 12: NR 0x3, DESFMT 0x1, PRINFOR 0xc, DTYPE 0xb, PRINFOW 0x2, LR
	sqe.cdw[12] = 0x88b0c103;
	sqe.cdw[13] = 0xbeef0000; // Copy dword 13: DSPEC 0xbeef
	if (xnvme_bits_get(sqe.scopy.cdw12, XNVME_SPEC_NVM_CMD_SCOPY_CDW12_NR) != 0x3 ||
	    xnvme_bits_get(sqe.scopy.cdw12, XNVME_SPEC_NVM_CMD_SCOPY_CDW12_DESFMT) != 0x1 ||
	    xnvme_bits_get(sqe.scopy.cdw12, XNVME_SPEC_NVM_CMD_SCOPY_CDW12_PRINFOR) != 0xc ||
	    xnvme_bits_get(sqe.scopy.cdw12, XNVME_SPEC_NVM_CMD_SCOPY_CDW12_DTYPE) != 0xb ||
	    xnvme_bits_get(sqe.scopy.cdw12, XNVME_SPEC_NVM_CMD_SCOPY_CDW12_PRINFOW) != 0x2 ||
	    xnvme_bits_get(sqe.scopy.cdw12, XNVME_SPEC_NVM_CMD_SCOPY_CDW12_FUA) ||
	    !xnvme_bits_get(sqe.scopy.cdw12, XNVME_SPEC_NVM_CMD_SCOPY_CDW12_LR) ||
	    sqe.scopy.dspec != 0xbeef) {
		xnvme_cli_pinf("FAILED: scopy: cdw12: %#x, dspec: %#x", sqe.scopy.cdw12,
			       sqe.scopy.dspec);
		err = -EINVAL;
	}

	memset(&sqe, 0, sizeof(sqe));
	sqe.cdw[12] = 0x5c30001f; // Zone Append dword 12: NLB 0x1f, DTYPE 0x3, PRINFO 0x7, FUA
	if (xnvme_bits_get(sqe.znd.append.cdw12, XNVME_SPEC_ZND_CMD_APPEND_CDW12_NLB) != 0x1f ||
	    xnvme_bits_get(sqe.znd.append.cdw12, XNVME_SPEC_ZND_CMD_APPEND_CDW12_DTYPE) != 0x3 ||
	    xnvme_bits_get(sqe.znd.append.cdw12, XNVME_SPEC_ZND_CMD_APPEND_CDW12_PRINFO) != 0x7 ||
	    !xnvme_bits_get(sqe.znd.append.cdw12, XNVME_SPEC_ZND_CMD_APPEND_CDW12_FUA) ||
	    xnvme_bits_get(sqe.znd.append.cdw12, XNVME_SPEC_ZND_CMD_APPEND_CDW12_LR)) {
		xnvme_cli_pinf("FAILED: znd.append: cdw12: %#x", sqe.znd.append.cdw12);
		err = -EINVAL;
	}

	memset(&sqe, 0, sizeof(sqe));
	sqe.cdw[13] = 0x104; // Zone Management Send dword 13: ZSA 0x4, Select All
	if (xnvme_bits_get(sqe.znd.mgmt_send.cdw13, XNVME_SPEC_ZND_CMD_MGMT_SEND_CDW13_ZSA) !=
		    0x4 ||
	    !xnvme_bits_get(sqe.znd.mgmt_send.cdw13,
			    XNVME_SPEC_ZND_CMD_MGMT_SEND_CDW13_SELECT_ALL) ||
	    xnvme_bits_get(sqe.znd.mgmt_send.cdw13, XNVME_SPEC_ZND_CMD_MGMT_SEND_CDW13_ZSASO)) {
		xnvme_cli_pinf("FAILED: znd.mgmt_send: cdw13: %#x", sqe.znd.mgmt_send.cdw13);
		err = -EINVAL;
	}

	memset(&sqe, 0, sizeof(sqe));
	sqe.cdw[13] = 0x10301; // Zone Management Receive dword 13: ZRA 0x1, ZRASF 0x3, Partial
	if (xnvme_bits_get(sqe.znd.mgmt_recv.cdw13, XNVME_SPEC_ZND_CMD_MGMT_RECV_CDW13_ZRA) !=
		    0x1 ||
	    xnvme_bits_get(sqe.znd.mgmt_recv.cdw13, XNVME_SPEC_ZND_CMD_MGMT_RECV_CDW13_ZRASF) !=
		    0x3 ||
	    !xnvme_bits_get(sqe.znd.mgmt_recv.cdw13, XNVME_SPEC_ZND_CMD_MGMT_RECV_CDW13_PARTIAL)) {
		xnvme_cli_pinf("FAILED: znd.mgmt_recv: cdw13: %#x", sqe.znd.mgmt_recv.cdw13);
		err = -EINVAL;
	}

	memset(&sqe, 0, sizeof(sqe));
	sqe.cdw[8] = 0x1000;     // SGL descriptor bytes 11:8: length
	sqe.cdw[9] = 0x31000000; // SGL descriptor byte 15: Last Segment, Offset
	if (sqe.common.dptr.sgl.len != 0x1000 ||
	    xnvme_bits_get(sqe.common.dptr.sgl.id, XNVME_SPEC_SGL_DESCRIPTOR_ID_TYPE) !=
		    XNVME_SPEC_SGL_DESCR_TYPE_LAST_SEGMENT ||
	    xnvme_bits_get(sqe.common.dptr.sgl.id, XNVME_SPEC_SGL_DESCRIPTOR_ID_SUBTYPE) !=
		    XNVME_SPEC_SGL_DESCR_SUBTYPE_OFFSET) {
		xnvme_cli_pinf("FAILED: sgl: len: %#x, id: %#x", sqe.common.dptr.sgl.len,
			       sqe.common.dptr.sgl.id);
		err = -EINVAL;
	}

	return err;
}

static int
sub_cqe_result(struct xnvme_cli *XNVME_UNUSED(cli))
{
	uint32_t dwords[4] = {0x11223344, 0x55667788, 0x0,
			      0x0}; // Dwords 0 and 1: command specific
	struct xnvme_spec_cqe cqe;

	memcpy(&cqe, dwords, sizeof(cqe));
	if (xnvme_bits_get(cqe.result, XNVME_SPEC_CQE_RESULT_CDW0) != 0x11223344 ||
	    xnvme_bits_get(cqe.result, XNVME_SPEC_CQE_RESULT_CDW1) != 0x55667788) {
		xnvme_cli_pinf("FAILED: cqe: result: %#" PRIx64, cqe.result);
		return -EINVAL;
	}

	return 0;
}

static int
sub_pif_reftag(struct xnvme_cli *XNVME_UNUSED(cli))
{
	static uint8_t data[4096];
	uint8_t md[16];
	const uint8_t g64[6] = {0x00, 0x00, 0x89, 0xab, 0xcd, 0xef};
	const uint8_t g16[4] = {0x89, 0xab, 0xcd, 0xef};
	struct xnvme_pi_ctx ctx = {0};
	int err = 0;

	// 64b Guard: the reference tag is the low bits of the 48 bits of bytes 15:10
	memset(md, 0, sizeof(md));
	if (xnvme_pi_ctx_init(&ctx, sizeof(data), sizeof(md), false, false, XNVME_PI_TYPE1,
			      XNVME_PI_FLAGS_REFTAG_CHECK, 0x89abcdef, 0, 0,
			      XNVME_SPEC_NVM_NS_64B_GUARD)) {
		xnvme_cli_pinf("FAILED: xnvme_pi_ctx_init(64b)");
		return -EINVAL;
	}
	xnvme_pi_generate(&ctx, data, md, 1);
	if (memcmp(&md[10], g64, sizeof(g64)) || xnvme_pi_verify(&ctx, data, md, 1)) {
		xnvme_cli_pinf("FAILED: 64b guard: reference tag in bytes 15:10");
		err = -EINVAL;
	}

	// 16b Guard: the reference tag is bytes 7:4
	memset(md, 0, sizeof(md));
	if (xnvme_pi_ctx_init(&ctx, sizeof(data), 8, false, false, XNVME_PI_TYPE1,
			      XNVME_PI_FLAGS_REFTAG_CHECK, 0x89abcdef, 0, 0,
			      XNVME_SPEC_NVM_NS_16B_GUARD)) {
		xnvme_cli_pinf("FAILED: xnvme_pi_ctx_init(16b)");
		return -EINVAL;
	}
	xnvme_pi_generate(&ctx, data, md, 1);
	if (memcmp(&md[4], g16, sizeof(g16)) || xnvme_pi_verify(&ctx, data, md, 1)) {
		xnvme_cli_pinf("FAILED: 16b guard: reference tag in bytes 7:4");
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
		"cqe-result",
		"Read the command specific dwords of the completion through their masks",
		"Read the command specific dwords of the completion through their masks",
		sub_cqe_result,
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
		"pif-reftag",
		"Write and read the reference tag of the protection information formats",
		"Write and read the reference tag of the protection information formats",
		sub_pif_reftag,
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
