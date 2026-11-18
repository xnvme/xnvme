// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>

const char *
xnvme_spec_adm_opc_str(enum xnvme_spec_admin_opcode eval)
{
	switch (eval) {
	case XNVME_SPEC_ADMIN_GET_FEATURES_OPCODE:
		return "ADM_OPC_GFEAT";
	case XNVME_SPEC_ADMIN_IDENTIFY_OPCODE:
		return "ADM_OPC_IDFY";
	case XNVME_SPEC_ADMIN_GET_LOG_PAGE_OPCODE:
		return "ADM_OPC_LOG";
	case XNVME_SPEC_ADMIN_SET_FEATURES_OPCODE:
		return "ADM_OPC_SFEAT";
	case XNVME_SPEC_ADMIN_DIRECTIVE_SEND_OPCODE:
		return "ADM_OPC_DSEND";
	case XNVME_SPEC_ADMIN_DIRECTIVE_RECEIVE_OPCODE:
		return "ADM_OPC_DRECV";
	default:
		break;
	}

	return "ENOSYS";
}

const char *
xnvme_spec_csi_str(enum xnvme_spec_admin_identify_cdw11_csi eval)
{
	switch (eval) {
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CSI_NVM_COMMAND_SET:
		return "CSI_NVM";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CSI_ZONED_NAMESPACE_COMMAND_SET:
		return "CSI_ZONED";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CSI_KEY_VALUE_COMMAND_SET:
		return "CSI_KV";
	default:
		break;
	}

	return "ENOSYS";
}

const char *
xnvme_spec_feat_id_str(enum xnvme_spec_feat_id eval)
{
	switch (eval) {
	case XNVME_SPEC_FEAT_ARBITRATION:
		return "FEAT_ARBITRATION";
	case XNVME_SPEC_FEAT_ERROR_RECOVERY:
		return "FEAT_ERROR_RECOVERY";
	case XNVME_SPEC_FEAT_LBA_RANGETYPE:
		return "FEAT_LBA_RANGETYPE";
	case XNVME_SPEC_FEAT_NQUEUES:
		return "FEAT_NQUEUES";
	case XNVME_SPEC_FEAT_PWR_MGMT:
		return "FEAT_PWR_MGMT";
	case XNVME_SPEC_FEAT_TEMP_THRESHOLD:
		return "FEAT_TEMP_THRESHOLD";
	case XNVME_SPEC_FEAT_VWCACHE:
		return "FEAT_VWCACHE";
	case XNVME_SPEC_FEAT_FDP_MODE:
		return "FEAT_FDP_MODE";
	case XNVME_SPEC_FEAT_FDP_EVENTS:
		return "FEAT_FDP_EVENTS";
	}

	return "ENOSYS";
}

const char *
xnvme_spec_feat_sel_str(enum xnvme_spec_feat_sel eval)
{
	switch (eval) {
	case XNVME_SPEC_FEAT_SEL_CURRENT:
		return "FEAT_SEL_CURRENT";
	case XNVME_SPEC_FEAT_SEL_DEFAULT:
		return "FEAT_SEL_DEFAULT";
	case XNVME_SPEC_FEAT_SEL_SAVED:
		return "FEAT_SEL_SAVED";
	case XNVME_SPEC_FEAT_SEL_SUPPORTED:
		return "FEAT_SEL_SUPPORTED";
	}

	return "ENOSYS";
}

const char *
xnvme_spec_flag_str(enum xnvme_spec_flag eval)
{
	switch (eval) {
	case XNVME_SPEC_FLAG_FORCE_UNIT_ACCESS:
		return "FLAG_FORCE_UNIT_ACCESS";
	case XNVME_SPEC_FLAG_LIMITED_RETRY:
		return "FLAG_LIMITED_RETRY";
	case XNVME_SPEC_FLAG_PRINFO_PRACT:
		return "FLAG_PRINFO_PRACT";
	case XNVME_SPEC_FLAG_PRINFO_PRCHK_APP:
		return "FLAG_PRINFO_PRCHK_APP";
	case XNVME_SPEC_FLAG_PRINFO_PRCHK_GUARD:
		return "FLAG_PRINFO_PRCHK_GUARD";
	case XNVME_SPEC_FLAG_PRINFO_PRCHK_REF:
		return "FLAG_PRINFO_PRCHK_REF";
	}

	return "ENOSYS";
}

const char *
xnvme_spec_idfy_cns_str(int eval)
{
	switch (eval) {
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_IDENTIFY_CONTROLLER_DATA_STRUCTURE:
		return "IDFY_CTRLR";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_CONTROLLER_DATA_STRUCTURE:
		return "IDFY_CTRLR_IOCS";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_NAMESPACE_ATTACHED_CONTROLLER_LIST:
		return "IDFY_CTRLR_NS";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_PRIMARY_CONTROLLER_CAPABILITIES_DATA_STRUCTURE:
		return "IDFY_CTRLR_PRI";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_SECONDARY_CONTROLLER_LIST:
		return "IDFY_CTRLR_SEC";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_CONTROLLER_LIST:
		return "IDFY_CTRLR_SUB";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_IDENTIFY_IO_COMMAND_SET_DATA_STRUCTURE:
		return "IDFY_IOCS";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_NVM_COMMAND_SET_IDENTIFY_NAMESPACE_DATA_STRUCTURE:
		return "IDFY_NS";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_NAMESPACE_IDENTIFICATION_DESCRIPTOR_LIST:
		return "IDFY_NS_DESC";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_NAMESPACE_GRANULARITY_LIST:
		return "IDFY_NSGRAN";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_ACTIVE_NAMESPACE_ID_LIST:
		return "IDFY_NSLIST";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_ALLOCATED_NAMESPACE_ID_LIST:
		return "IDFY_NSLIST_ALLOC";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_IO_COMMAND_SET_SPECIFIC_ALLOCATED_NAMESPACE_ID_LIST:
		return "IDFY_NSLIST_ALLOC_IOCS";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_IO_COMMAND_SET_SPECIFIC_ACTIVE_NAMESPACE_ID_LIST:
		return "IDFY_NSLIST_IOCS";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_IDENTIFY_NAMESPACE_DATA_STRUCTURE_FOR_AN_ALLOCATED_NAMESPACE_ID:
		return "IDFY_NS_ALLOC";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_DATA_STRUCTURE_FOR_AN_ALLOCATED_NAMESPACE_ID:
		return "IDFY_NS_ALLOC_IOCS";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_DATA_STRUCTURE:
		return "IDFY_NS_IOCS";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_NVM_SET_LIST:
		return "IDFY_SETL";
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_UUID_LIST:
		return "IDFY_UUIDL";
	}

	return "ENOSYS";
}

const char *
xnvme_spec_log_lpi_str(int eval)
{
	switch (eval) {
	case XNVME_SPEC_BASE_LOG_PAGE_CHANGED_ATTACHED_NAMESPACE_LIST:
		return "LOG_CHNS";
	case XNVME_SPEC_BASE_LOG_PAGE_COMMANDS_SUPPORTED_AND_EFFECTS:
		return "LOG_CSAE";
	case XNVME_SPEC_BASE_LOG_PAGE_ERROR_INFORMATION:
		return "LOG_ERRI";
	case XNVME_SPEC_BASE_LOG_PAGE_FIRMWARE_SLOT_INFORMATION:
		return "LOG_FW";
	case XNVME_SPEC_BASE_LOG_PAGE_SMART_HEALTH_INFORMATION:
		return "LOG_HEALTH";
	case XNVME_SPEC_BASE_LOG_PAGE_SUPPORTED_LOG_PAGES:
		return "LOG_RSVD";
	case XNVME_SPEC_BASE_LOG_PAGE_DEVICE_SELF_TEST:
		return "LOG_SELFTEST";
	case XNVME_SPEC_BASE_LOG_PAGE_TELEMETRY_CONTROLLER_INITIATED:
		return "LOG_TELECTRLR";
	case XNVME_SPEC_BASE_LOG_PAGE_TELEMETRY_HOST_INITIATED:
		return "LOG_TELEHOST";
	case XNVME_SPEC_BASE_LOG_PAGE_FDP_CONFIGURATIONS:
		return "LOG_FDPCONF";
	case XNVME_SPEC_BASE_LOG_PAGE_RECLAIM_UNIT_HANDLE_USAGE:
		return "LOG_FDPRUHU";
	case XNVME_SPEC_BASE_LOG_PAGE_FDP_STATISTICS:
		return "LOG_FDPSTATS";
	case XNVME_SPEC_BASE_LOG_PAGE_FDP_EVENTS:
		return "LOG_FDPEVENTS";
	case XNVME_SPEC_BASE_LOG_PAGE_SANITIZE_STATUS:
		return "LOG_SANITIZE";
	}

	return "ENOSYS";
}

const char *
xnvme_spec_znd_log_lid_str(int eval)
{
	switch (eval) {
	case XNVME_SPEC_ZNS_LOG_PAGE_CHANGED_ZONE_LIST:
		return "LOG_ZND_CHANGES";
	}

	return "ENOSYS";
}

const char *
xnvme_spec_nvm_cmd_cpl_sc_str(enum xnvme_spec_nvm_cmd_cpl_sc eval)
{
	switch (eval) {
	case XNVME_SPEC_NVM_CMD_CPL_SC_WRITE_TO_RONLY:
		return "NVM_CMD_CPL_SC_WRITE_TO_RONLY";
	}

	return "ENOSYS";
}

const char *
xnvme_spec_nvm_opc_str(int eval)
{
	switch (eval) {
	case XNVME_SPEC_ADMIN_FORMAT_NVM_OPCODE:
		return "NVM_OPC_FMT";
	case XNVME_SPEC_NVM_READ_OPCODE:
		return "NVM_OPC_READ";
	case XNVME_SPEC_ADMIN_SANITIZE_OPCODE:
		return "NVM_OPC_SANITIZE";
	case XNVME_SPEC_NVM_COPY_OPCODE:
		return "NVM_OPC_SCOPY";
	case XNVME_SPEC_NVM_WRITE_OPCODE:
		return "NVM_OPC_WRITE";
	case XNVME_SPEC_NVM_WRITE_UNCORRECTABLE_OPCODE:
		return "NVM_OPC_WRITE_UNCORRECTABLE";
	case XNVME_SPEC_NVM_COMPARE_OPCODE:
		return "NVM_OPC_COMPARE";
	case XNVME_SPEC_NVM_WRITE_ZEROES_OPCODE:
		return "NVM_OPC_WRITE_ZEROES";
	case XNVME_SPEC_NVM_DATASET_MANAGEMENT_OPCODE:
		return "XNVME_SPEC_NVM_OPC_DATASET_MANAGEMENT";
	case XNVME_SPEC_IO_FLUSH_OPCODE:
		return "NVM_OPC_FLUSH";
	case XNVME_SPEC_IO_IO_MANAGEMENT_RECEIVE_OPCODE:
		return "XNVME_SPEC_NVM_OPC_IO_MGMT_RECV";
	case XNVME_SPEC_IO_IO_MANAGEMENT_SEND_OPCODE:
		return "XNVME_SPEC_NVM_OPC_IO_MGMT_SEND";
	}

	return "ENOSYS";
}

const char *
xnvme_spec_psdt_str(enum xnvme_spec_psdt eval)
{
	switch (eval) {
	case XNVME_SPEC_PSDT_PRP:
		return "PSDT_PRP";
	case XNVME_SPEC_PSDT_SGL_MPTR_CONTIGUOUS:
		return "PSDT_SGL_MPTR_CONTIGUOUS";
	case XNVME_SPEC_PSDT_SGL_MPTR_SGL:
		return "PSDT_SGL_MPTR_SGL";
	}

	return "ENOSYS";
}

const char *
xnvme_spec_sgl_descriptor_subtype_str(enum xnvme_spec_sgl_descriptor_subtype eval)
{
	switch (eval) {
	case XNVME_SPEC_SGL_DESCR_SUBTYPE_ADDRESS:
		return "SGL_DESCR_SUBTYPE_ADDRESS";
	case XNVME_SPEC_SGL_DESCR_SUBTYPE_OFFSET:
		return "SGL_DESCR_SUBTYPE_OFFSET";
	}

	return "ENOSYS";
}

const char *
xnvme_spec_znd_cmd_mgmt_recv_action_str(enum xnvme_spec_zns_zone_management_receive_cdw13_zra eval)
{
	switch (eval) {
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRA_REPORT_ZONES:
		return "ZND_CMD_MGMT_RECV_ACTION_REPORT";
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRA_EXTENDED_REPORT_ZONES:
		return "ZND_CMD_MGMT_RECV_ACTION_REPORT_EXTENDED";
	}

	return "ENOSYS";
}

const char *
xnvme_spec_znd_cmd_mgmt_recv_action_sf_str(int eval)
{
	switch (eval) {
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRAS_LIST_ALL_ZONES:
		return "ZND_CMD_MGMT_RECV_SF_ALL";
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRAS_ZSC:
		return "ZND_CMD_MGMT_RECV_SF_CLOSED";
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRAS_ZSE:
		return "ZND_CMD_MGMT_RECV_SF_EMPTY";
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRAS_ZSEO:
		return "ZND_CMD_MGMT_RECV_SF_EOPEN";
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRAS_ZSF:
		return "ZND_CMD_MGMT_RECV_SF_FULL";
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRAS_ZSIO:
		return "ZND_CMD_MGMT_RECV_SF_IOPEN";
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRAS_ZSO:
		return "ZND_CMD_MGMT_RECV_SF_OFFLINE";
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRAS_ZSRO:
		return "ZND_CMD_MGMT_RECV_SF_RONLY";
	}

	return "ENOSYS";
}

const char *
xnvme_spec_znd_cmd_mgmt_send_action_str(enum xnvme_spec_zns_zone_management_send_cdw13_zsa eval)
{
	switch (eval) {
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZSA_CLOSE_ZONE:
		return "ZND_CMD_MGMT_SEND_CLOSE";
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZSA_FLUSH_EXPLICIT_ZRWA_RANGE:
		return "ZND_CMD_MGMT_SEND_FLUSH";
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZSA_SET_ZONE_DESCRIPTOR_EXTENSION:
		return "ZND_CMD_MGMT_SEND_DESCRIPTOR";
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZSA_FINISH_ZONE:
		return "ZND_CMD_MGMT_SEND_FINISH";
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZSA_OFFLINE_ZONE:
		return "ZND_CMD_MGMT_SEND_OFFLINE";
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZSA_OPEN_ZONE:
		return "ZND_CMD_MGMT_SEND_OPEN";
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZSA_RESET_ZONE:
		return "ZND_CMD_MGMT_SEND_RESET";
	}

	return "ENOSYS";
}

const char *
xnvme_spec_znd_opc_str(enum xnvme_spec_zns_opcode eval)
{
	switch (eval) {
	case XNVME_SPEC_ZNS_ZONE_APPEND_OPCODE:
		return "ZND_CMD_OPC_APPEND";
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_OPCODE:
		return "ZND_OPC_MGMT_RECV";
	case XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_OPCODE:
		return "ZND_OPC_MGMT_SEND";
	default:
		break;
	}

	return "ENOSYS";
}

const char *
xnvme_spec_znd_status_code_str(int eval)
{
	switch (eval) {
	case XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_INVALID_ZONE_OPERATION_REQUEST:
		return "XNVME_SPEC_ZND_SC_INVALID_ZONE_OP";
	case XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_ZRWA_RESOURCES_UNAVAILABLE:
		return "XNVME_SPEC_ZND_SC_NOZRWA";
	case XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_ZONE_BOUNDARY_ERROR:
		return "ZND_SC_BOUNDARY_ERROR";
	case XNVME_SPEC_ZND_SC_INVALID_FORMAT:
		return "ZND_SC_INVALID_FORMAT";
	case XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_INVALID_ZONE_STATE_TRANSITION:
		return "ZND_SC_INVALID_TRANS";
	case XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_ZONE_INVALID_WRITE:
		return "ZND_SC_INVALID_WRITE";
	case XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_ZONE_IS_FULL:
		return "ZND_SC_IS_FULL";
	case XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_ZONE_IS_OFFLINE:
		return "ZND_SC_IS_OFFLINE";
	case XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_ZONE_IS_READ_ONLY:
		return "ZND_SC_IS_READONLY";
	case XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_TOO_MANY_ACTIVE_ZONES:
		return "ZND_SC_TOO_MANY_ACTIVE";
	case XNVME_SPEC_ZNS_COMMAND_SPECIFIC_STATUS_TOO_MANY_OPEN_ZONES:
		return "ZND_SC_TOO_MANY_OPEN";
	}

	return "ENOSYS";
}

const char *
xnvme_spec_znd_state_str(enum xnvme_spec_zns_zone_descriptor_zsattr_zs eval)
{
	switch (eval) {
	case XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZSATTR_ZS_CLOSED:
		return "ZND_STATE_CLOSED";
	case XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZSATTR_ZS_EMPTY:
		return "ZND_STATE_EMPTY";
	case XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZSATTR_ZS_EXPLICITLY_OPENED:
		return "ZND_STATE_EOPEN";
	case XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZSATTR_ZS_FULL:
		return "ZND_STATE_FULL";
	case XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZSATTR_ZS_IMPLICITLY_OPENED:
		return "ZND_STATE_IOPEN";
	case XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZSATTR_ZS_OFFLINE:
		return "ZND_STATE_OFFLINE";
	case XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZSATTR_ZS_READ_ONLY:
		return "ZND_STATE_RONLY";
	}

	return "ENOSYS";
}

const char *
xnvme_spec_znd_type_str(enum xnvme_spec_zns_zone_descriptor_ztattr_zt eval)
{
	switch (eval) {
	case XNVME_SPEC_ZNS_ZONE_DESCRIPTOR_ZTATTR_ZT_SEQUENTIAL_WRITE_REQUIRED:
		return "ZND_TYPE_SEQWR";
	}

	return "ENOSYS";
}

int
xnvme_spec_ctrlr_bar_fpr(FILE *stream, struct xnvme_spec_ctrlr_bar *bar, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_spec_ctrlr_bar:");
	if (!bar) {
		wrtn += fprintf(stream, "~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");

	wrtn += fprintf(stream, "  cap: 0x%016" PRIx64 "\n", bar->cap);
	wrtn += fprintf(stream, "  vs: %" PRIu32 "\n", bar->vs);
	wrtn += fprintf(stream, "  intms: %" PRIu32 "\n", bar->intms);
	wrtn += fprintf(stream, "  intmc: %" PRIu32 "\n", bar->intmc);
	wrtn += fprintf(stream, "  cc: %" PRIu32 "\n", bar->cc);
	wrtn += fprintf(stream, "  csts: %" PRIu32 "\n", bar->csts);
	wrtn += fprintf(stream, "  nssr: %" PRIu32 "\n", bar->nssr);
	wrtn += fprintf(stream, "  aqa: %" PRIu32 "\n", bar->aqa);

	wrtn += fprintf(stream, "  asq: %" PRIu64 "\n", bar->asq);
	wrtn += fprintf(stream, "  acq: %" PRIu64 "\n", bar->acq);

	wrtn += fprintf(stream, "  cmbloc: %" PRIu32 "\n", bar->cmbloc);
	wrtn += fprintf(stream, "  cmbsz: %" PRIu32 "\n", bar->cmbsz);
	wrtn += fprintf(stream, "  bpinfo: %" PRIu32 "\n", bar->bpinfo);
	wrtn += fprintf(stream, "  bprsel: %" PRIu32 "\n", bar->bprsel);
	wrtn += fprintf(stream, "  bpmbl: %" PRIu64 "\n", bar->bpmbl);
	wrtn += fprintf(stream, "  cmbmsc: %" PRIu64 "\n", bar->cmbmsc);
	wrtn += fprintf(stream, "  cmbsts: %" PRIu32 "\n", bar->cmbsts);
	wrtn += fprintf(stream, "  pmrcap: %" PRIu32 "\n", bar->pmrcap);
	wrtn += fprintf(stream, "  pmrctl: %" PRIu32 "\n", bar->pmrctl);
	wrtn += fprintf(stream, "  pmrsts: %" PRIu32 "\n", bar->pmrsts);
	wrtn += fprintf(stream, "  pmrebs: %" PRIu32 "\n", bar->pmrebs);
	wrtn += fprintf(stream, "  pmrswtp: %" PRIu32 "\n", bar->pmrswtp);
	wrtn += fprintf(stream, "  pmrmscl: %" PRIu32 "\n", bar->pmrmscl);
	wrtn += fprintf(stream, "  pmrmscu: %" PRIu32 "\n", bar->pmrmscu);

	return wrtn;
}

int
xnvme_spec_ctrlr_bar_pp(struct xnvme_spec_ctrlr_bar *bar, int opts)
{
	return xnvme_spec_ctrlr_bar_fpr(stdout, bar, opts);
}
