/**
 * libxnvme_pp - pretty-printers of the types of the other headers
 *
 * The printers of the API are declared here, grouped by the header defining
 * what they print, so that a binding can leave them out and print in its own
 * language. The command-line library keeps its printers in libxnvme_cli.h,
 * and the static inline timer printers stay in libxnvme_util.h. As this
 * header names types of the others, libxnvme.h includes it after them.
 *
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_pp.h
 */

/**
 * Options for pretty-printer (``*_pr``, ``*_fpr``) functions
 *
 * @see libxnvme_pp.h
 *
 * @enum xnvme_pr
 */
enum xnvme_pr {
	XNVME_PR_DEF   = 0x0, ///< XNVME_PR_DEF: Default options
	XNVME_PR_YAML  = 0x1, ///< XNVME_PR_YAML: Print formatted as YAML
	XNVME_PR_TERSE = 0x2  ///< XNVME_PR_TERSE: Print without formatting
};

/* Printers of libxnvme_opts.h */

/**
 * Prints a humanly readable representation of the given ::xnvme_opts
 *
 * @param opts Pointer to the #xnvme_opts to print
 * @param pr_opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_opts_pr(const struct xnvme_opts *opts, int pr_opts);

/* Printers of libxnvme_dev.h */

/**
 * Prints the given ::xnvme_dev to the given output stream
 *
 * @param stream output stream used for printing
 * @param dev pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_dev_fpr(FILE *stream, const struct xnvme_dev *dev, int opts);

/**
 * Prints the given ::xnvme_dev to stdout
 *
 * @param dev pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_dev_pr(const struct xnvme_dev *dev, int opts);

/* Printers of libxnvme_be.h */

/**
 * Prints the given backend attribute to the given output stream
 *
 * @param stream output stream used for printing
 * @param attr Pointer to the ::xnvme_be_attr to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_be_attr_fpr(FILE *stream, const struct xnvme_be_attr *attr, int opts);

/**
 * Prints the given backend attribute to stdout
 *
 * @param attr Pointer to the ::xnvme_be_attr to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_be_attr_pr(const struct xnvme_be_attr *attr, int opts);

/**
 * Prints attributes of backend configurations to the given output stream
 *
 * @param stream output stream used for printing
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_be_attr_list_fpr(FILE *stream, enum xnvme_pr opts);

/**
 * Prints attributes of backend configurations to stdout
 *
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_be_attr_list_pr(enum xnvme_pr opts);

/* Printers of libxnvme_buf.h */

/**
 * Prints the number and value of bytes where expected is different from actual
 *
 * @param expected Pointer to buffer of "expected" content
 * @param actual Pointer to buffer to compare to "expected"
 * @param nbytes Amount of bytes to compare
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_buf_diff_pr(const void *expected, const void *actual, size_t nbytes, int opts);

/* Printers of libxnvme_geo.h */

/**
 * Prints the given ::xnvme_geo to the given output stream
 *
 * @param stream output stream used for printing
 * @param geo pointer to the the ::xnvme_geo to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_geo_fpr(FILE *stream, const struct xnvme_geo *geo, int opts);

/**
 * Prints the given ::xnvme_geo to stdout
 *
 * @param geo pointer to the the ::xnvme_geo to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_geo_pr(const struct xnvme_geo *geo, int opts);

/* Printers of libxnvme_ident.h */

/**
 * Writes a YAML-representation of the given 'ident' to stream
 */
int
xnvme_ident_yaml(FILE *stream, const struct xnvme_ident *ident, int indent, const char *sep,
		 int head);

/**
 * Prints the given ::xnvme_ident to the given output stream
 *
 * @param stream output stream used for printing
 * @param ident pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_ident_fpr(FILE *stream, const struct xnvme_ident *ident, int opts);

/**
 * Prints the given ::xnvme_ident to stdout
 *
 * @param ident pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_ident_pr(const struct xnvme_ident *ident, int opts);

/* Printers of the structures of the generated libxnvme_spec_<doc>.h */

/**
 * Prints the given :;xnvme_spec_base_log_smart_health_information to the given output stream
 *
 * @param stream output stream used for printing
 * @param log pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_health_fpr(FILE *stream,
			  const struct xnvme_spec_base_log_smart_health_information *log,
			  int opts);

/**
 * Prints the given :;xnvme_spec_base_log_smart_health_information to stdout
 *
 * @param log
 * @param opts
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_health_pr(const struct xnvme_spec_base_log_smart_health_information *log, int opts);

/**
 * Prints the given :;xnvme_spec_base_log_sanitize_status to the given output stream
 *
 * @param stream output stream used for printing
 * @param log pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_sanitize_fpr(FILE *stream, const struct xnvme_spec_base_log_sanitize_status *log,
			    int opts);

/**
 * Prints the given :;xnvme_spec_base_log_sanitize_status to stdout
 *
 * @param log pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_sanitize_pr(const struct xnvme_spec_base_log_sanitize_status *log, int opts);

/**
 * Prints the given :;xnvme_spec_base_error_information_log_entry to stdout
 *
 * @param stream output stream used for printing
 * @param log
 * @param limit
 * @param opts
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_erri_fpr(FILE *stream,
			const struct xnvme_spec_base_error_information_log_entry *log, int limit,
			int opts);

/**
 * Prints the given :;xnvme_spec_base_error_information_log_entry to stdout
 *
 * @param log
 * @param limit
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_erri_pr(const struct xnvme_spec_base_error_information_log_entry *log, int limit,
		       int opts);

/**
 * Prints the given :;xnvme_spec_base_log_fdp_configurations to stdout
 *
 * @param log
 * @param opts
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_fdp_conf_pr(const struct xnvme_spec_base_log_fdp_configurations *log, int opts);

/**
 * Prints the given :;xnvme_spec_base_log_fdp_statistics to stdout
 *
 * @param log
 * @param opts
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_fdp_stats_pr(const struct xnvme_spec_base_log_fdp_statistics *log, int opts);

/**
 * Prints the given ::xnvme_spec_base_log_fdp_events to stdout
 *
 * @param log
 * @param limit
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_fdp_events_pr(const struct xnvme_spec_base_log_fdp_events *log, int limit,
			     int opts);

/**
 * Prints the given :;xnvme_spec_base_log_reclaim_unit_handle_usage to stdout
 *
 * @param log
 * @param limit
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_ruhu_pr(const struct xnvme_spec_base_log_reclaim_unit_handle_usage *log, int limit,
		       int opts);

/**
 * Prints the given :;xnvme_spec_base_reclaim_unit_handle_status to stdout
 *
 * @param ruhs
 * @param limit
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_ruhs_pr(const struct xnvme_spec_base_reclaim_unit_handle_status *ruhs, int limit,
		   int opts);

/**
 * Prints the given :;xnvme_spec_nvm_identify_namespace to the given output stream
 *
 * @param stream output stream used for printing
 * @param idfy pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_idfy_ns_fpr(FILE *stream, const struct xnvme_spec_nvm_identify_namespace *idfy,
		       int opts);

/**
 * Prints the given :;xnvme_spec_nvm_identify_namespace to stdout
 *
 * @param idfy pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_idfy_ns_pr(const struct xnvme_spec_nvm_identify_namespace *idfy, int opts);

/**
 * Prints the given :;xnvme_spec_base_identify_controller to the given output stream
 *
 * @param stream output stream used for printing
 * @param idfy pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_idfy_ctrlr_fpr(FILE *stream, const struct xnvme_spec_base_identify_controller *idfy,
			  int opts);

/**
 * Prints the given :;xnvme_spec_base_identify_controller to stdout
 *
 * @param idfy pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_idfy_ctrlr_pr(const struct xnvme_spec_base_identify_controller *idfy, int opts);

int
xnvme_spec_idfy_cs_fpr(FILE *stream, const struct xnvme_spec_base_identify_io_command_set *idfy,
		       int opts);

int
xnvme_spec_idfy_cs_pr(const struct xnvme_spec_base_identify_io_command_set *idfy, int opts);

/**
 * Prints the attributes of the given feature to the given output stream
 *
 * @param stream output stream used for printing
 * @param fid feature identifier, a value of ::xnvme_spec_base_feature_identifier
 * @param dw0 the attributes, as Get Features returns them in Dword 0 of the completion
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_feat_fpr(FILE *stream, uint8_t fid, uint32_t dw0, int opts);

/**
 * Prints the attributes of the given feature to stdout
 *
 * @param fid feature identifier, a value of ::xnvme_spec_base_feature_identifier
 * @param dw0 the attributes, as Get Features returns them in Dword 0 of the completion
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_feat_pr(uint8_t fid, uint32_t dw0, int opts);

/**
 * Prints the :;xnvme_spec_base_supported_fdp_event_descriptor to the given output stream
 *
 * @param buf Buffer which contains xnvme_spec_base_supported_fdp_event_descriptor
 * @param dw0 Dword 0 of the completion of Get Features of FDP Events, the number of events
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_feat_fdp_events_pr(void *buf, uint32_t dw0, int opts);

/**
 * Prints the given :;xnvme_cmd_sqe to the given output stream
 *
 * @param stream output stream used for printing
 * @param cmd pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_sqe_fpr(FILE *stream, union xnvme_cmd_sqe *cmd, int opts);

/**
 * Prints the given :;xnvme_cmd_sqe to stdout
 *
 * @param cmd pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_sqe_pr(union xnvme_cmd_sqe *cmd, int opts);

/**
 * Prints the given ::xnvme_spec_base_identify_directive_return_parameters to stdout
 *
 * @param idfy pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_drecv_idfy_pr(struct xnvme_spec_base_identify_directive_return_parameters *idfy,
			 int opts);

/**
 * Prints the given :;xnvme_spec_base_streams_directive_return_parameters to stdout
 *
 * @param srp pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_drecv_srp_pr(struct xnvme_spec_base_streams_directive_return_parameters *srp, int opts);

/**
 * Prints the given :;xnvme_spec_base_get_status to stdout
 *
 * @param sgs pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_drecv_sgs_pr(struct xnvme_spec_base_get_status *sgs, int opts);

/**
 * Prints Dword 0 of the completion of Directive Receive Allocate Resources to stdout
 *
 * @param dw0 Dword 0 of the completion queue entry
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_drecv_sar_pr(uint32_t dw0, int opts);

int
xnvme_spec_nvm_scopy_fmt_zero_fpr(FILE *stream, const struct xnvme_spec_nvm_source_range *entry,
				  int opts);

/**
 * Prints the given ::xnvme_spec_nvm_source_range
 * to stdout
 *
 * @param entry pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_scopy_fmt_zero_pr(const struct xnvme_spec_nvm_source_range *entry, int opts);

/**
 * Prints the given Source Range entries of Copy to the given output stream
 *
 * @param stream output stream used for printing
 * @param srange pointer to the first of the entries to print
 * @param nr zero-based number of entries, as the NR field of Copy
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_scopy_source_range_fpr(FILE *stream,
				      const struct xnvme_spec_nvm_source_range *srange, uint8_t nr,
				      int opts);

/**
 * Prints the given Source Range entries of Copy to stdout
 *
 * @param srange pointer to the first of the entries to print
 * @param nr zero-based number of entries, as the NR field of Copy
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_scopy_source_range_pr(const struct xnvme_spec_nvm_source_range *srange, uint8_t nr,
				     int opts);

/**
 * Prints the given ::xnvme_spec_nvm_io_command_set_specific_identify_controller to the given
 * output stream
 *
 * @param stream output stream used for printing
 * @param idfy pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_idfy_ctrlr_fpr(
	FILE *stream, struct xnvme_spec_nvm_io_command_set_specific_identify_controller *idfy,
	int opts);

/**
 * Prints the given ::xnvme_spec_nvm_io_command_set_specific_identify_controller to stdout
 *
 * @param idfy pointer to ::xnvme_spec_nvm_io_command_set_specific_identify_controller
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_idfy_ctrlr_pr(
	struct xnvme_spec_nvm_io_command_set_specific_identify_controller *idfy, int opts);

/**
 * Prints the given
 * ::xnvme_spec_nvm_io_command_set_specific_identify_namespace to
 * the given output stream
 *
 * Only fields specific to Logical Block Namespaces are printed by this function
 *
 * @param stream output stream used for printing
 * @param idfy pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_idfy_ns_fpr(FILE *stream,
			   struct xnvme_spec_nvm_io_command_set_specific_identify_namespace *idfy,
			   int opts);

/**
 * Prints the given
 * ::xnvme_spec_nvm_io_command_set_specific_identify_namespace to
 * stdout
 *
 * @param idfy pointer to
 * ::xnvme_spec_nvm_io_command_set_specific_identify_namespace
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_idfy_ns_pr(struct xnvme_spec_nvm_io_command_set_specific_identify_namespace *idfy,
			  int opts);

/**
 * Prints the given ::xnvme_spec_zns_identify_controller to the given
 * output stream
 *
 * Only fields specific to Zoned Namespaces are printed by this function
 *
 * @param stream output stream used for printing
 * @param zctrlr pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_znd_idfy_ctrlr_fpr(FILE *stream, struct xnvme_spec_zns_identify_controller *zctrlr,
			      int opts);

/**
 * Prints the given ::xnvme_spec_zns_identify_controller to the given
 * output stream
 *
 * Only fields specific to Zoned Namespaces are printed by this function
 *
 * @param zctrlr pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_znd_idfy_ctrlr_pr(struct xnvme_spec_zns_identify_controller *zctrlr, int opts);

/**
 * Prints the given ::xnvme_spec_zns_lba_format_extension to the given output stream
 *
 * @param stream output stream used for printing
 * @param zonef pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_znd_idfy_lbafe_fpr(FILE *stream, struct xnvme_spec_zns_lba_format_extension *zonef,
			      int opts);

/**
 * Prints the given ::xnvme_spec_zns_identify_namespace to the given output
 * stream
 *
 * Only fields specific to Zoned Namespaces are printed by this function
 *
 * @param stream output stream used for printing
 * @param zns pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_znd_idfy_ns_fpr(FILE *stream, struct xnvme_spec_zns_identify_namespace *zns, int opts);

/**
 * Prints the given ::xnvme_spec_zns_identify_namespace to stdout
 *
 * @param zns pointer to ::xnvme_spec_zns_identify_namespace
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_znd_idfy_ns_pr(struct xnvme_spec_zns_identify_namespace *zns, int opts);

/**
 * Prints the given ::xnvme_spec_zns_log_changed_zone_list to the given output stream
 *
 * @param stream output stream used for printing
 * @param changes pointer to subject to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_znd_log_changes_fpr(FILE *stream,
			       const struct xnvme_spec_zns_log_changed_zone_list *changes,
			       int opts);

/**
 * Pretty-printer of ::xnvme_spec_zns_log_changed_zone_list
 *
 * @param changes pointer to subject to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_znd_log_changes_pr(const struct xnvme_spec_zns_log_changed_zone_list *changes,
			      int opts);

/**
 * Prints the given ::xnvme_spec_zns_zone_descriptor to the given stream
 *
 * @param stream output stream used for printing
 * @param descr the struct to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_znd_descr_fpr(FILE *stream, const struct xnvme_spec_zns_zone_descriptor *descr,
			 int opts);

/**
 * Prints the given ::xnvme_spec_zns_zone_descriptor to stdout
 *
 * @param descr the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_znd_descr_pr(const struct xnvme_spec_zns_zone_descriptor *descr, int opts);

/**
 * Print the given ::xnvme_spec_zns_report_zones to the given output stream
 *
 * @param stream output stream used for printing
 * @param hdr the struct to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_znd_report_hdr_fpr(FILE *stream, const struct xnvme_spec_zns_report_zones *hdr,
			      int opts);

/**
 * Print the given ::xnvme_spec_zns_report_zones to the stdout
 *
 * @param hdr the struct to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_znd_report_hdr_pr(const struct xnvme_spec_zns_report_zones *hdr, int opts);

int
xnvme_spec_znd_descr_fpr_yaml(FILE *stream, const struct xnvme_spec_zns_zone_descriptor *descr,
			      int indent, const char *sep);

/**
 * Prints the given ::xnvme_spec_kv_identify_namespace to the given output
 * stream
 *
 * @param stream output stream used for printing
 * @param idfy pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_kvs_idfy_ns_fpr(FILE *stream, const struct xnvme_spec_kv_identify_namespace *idfy,
			   int opts);

/**
 * Prints the given ::xnvme_spec_kv_identify_namespace to stdout
 *
 * @param idfy pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_kvs_idfy_ns_pr(const struct xnvme_spec_kv_identify_namespace *idfy, int opts);

/**
 * Produces a string representation of the given ::xnvme_spec_admin_opcode
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_adm_opc_str(enum xnvme_spec_admin_opcode eval);

/**
 * Produces a string representation of the given ::xnvme_spec_base_csi
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_csi_str(enum xnvme_spec_base_csi eval);

/**
 * Produces a string representation of the given ::xnvme_spec_base_feature_identifier
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_feat_id_str(enum xnvme_spec_base_feature_identifier eval);

/**
 * Produces a string representation of the given ::xnvme_spec_admin_get_features_cdw10_sel
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_feat_sel_str(enum xnvme_spec_admin_get_features_cdw10_sel eval);

/**
 * Produces a string representation of the given value of ::xnvme_spec_base_cns
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_idfy_cns_str(int eval);

/**
 * Produces a string representation of the given value of
 * ::xnvme_spec_base_log_page_identifier
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_log_lpi_str(int eval);

/**
 * Produces a string representation of the given value of ::xnvme_spec_base_log_page_identifier
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_znd_log_lid_str(int eval);

/**
 * Produces a string representation of the given ::xnvme_spec_nvm_command_specific_status
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_nvm_cmd_cpl_sc_str(int eval);

/**
 * Produces a string representation of the given opcode, one of ::xnvme_spec_nvm_opcode,
 * ::xnvme_spec_io_opcode, or the Format NVM and Sanitize values of ::xnvme_spec_admin_opcode
 *
 * @param eval the opcode to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_nvm_opc_str(int eval);

/**
 * Produces a string representation of the given ::xnvme_spec_base_psdt
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_psdt_str(enum xnvme_spec_base_psdt eval);

/**
 * Produces a string representation of the given
 * ::xnvme_spec_base_sgldst
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_sgl_descriptor_subtype_str(enum xnvme_spec_base_sgldst eval);

/**
 * Produces a string representation of the given ::xnvme_spec_zns_zra
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_znd_cmd_mgmt_recv_action_str(enum xnvme_spec_zns_zra eval);

/**
 * Produces a string representation of the given value of
 * ::xnvme_spec_zns_zone_management_receive_cdw13_zras or
 * XNVME_SPEC_ZNS_ZONE_MANAGEMENT_RECEIVE_CDW13_ZRAS_LIST_ALL_ZONES
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_znd_cmd_mgmt_recv_action_sf_str(int eval);

/**
 * Produces a string representation of the given ::xnvme_spec_zns_zsa
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_znd_cmd_mgmt_send_action_str(enum xnvme_spec_zns_zsa eval);

/**
 * Produces a string representation of the given ::xnvme_spec_zns_opcode
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_znd_opc_str(enum xnvme_spec_zns_opcode eval);

/**
 * Produces a string representation of the given ::xnvme_spec_zns_command_specific_status
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_znd_status_code_str(int eval);

/**
 * Produces a string representation of the given ::xnvme_spec_zns_zs
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_znd_state_str(enum xnvme_spec_zns_zs eval);

/**
 * Produces a string representation of the given ::xnvme_spec_zns_zt
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_znd_type_str(enum xnvme_spec_zns_zt eval);

int
xnvme_spec_ctrlr_bar_fpr(FILE *stream, struct xnvme_spec_base_registers *bar, int opts);

int
xnvme_spec_ctrlr_bar_pp(struct xnvme_spec_base_registers *bar, int opts);

/* Printers of libxnvme_cmd.h */

/**
 * Prints a humanly readable representation the given #xnvme_cmd_ctx
 *
 * @param ctx Pointer to the #xnvme_cmd_ctx to print
 * @param opts Printer options, unused
 */
void
xnvme_cmd_ctx_pr(const struct xnvme_cmd_ctx *ctx, int opts);

/* Printers of libxnvme_ver.h */

/**
 * Prints the library version to the given 'stream'
 *
 * @param stream output stream used for printing
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_ver_fpr(FILE *stream, int opts);

/**
 * Prints the library version to stdout
 *
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_ver_pr(int opts);

/* Printers of libxnvme_znd.h */

/**
 * Prints the given ::xnvme_znd_report to the given stream
 *
 * @param stream output stream used for printing
 * @param report pointer to the the ::xnvme_znd_report to print
 * @param flags printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_znd_report_fpr(FILE *stream, const struct xnvme_znd_report *report, int flags);

/**
 * Prints the given ::xnvme_znd_report to stdout
 *
 * @param report
 * @param flags
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_znd_report_pr(const struct xnvme_znd_report *report, int flags);

/* Printers of libxnvme_libconf.h */

/**
 * Prints the given array of version-strings to the given output stream
 *
 * @param stream Output stream used for printing
 * @param libconf Pointer to the ::xnvme_libconf to print
 * @param opts Printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_libconf_fpr(FILE *stream, const struct xnvme_libconf *libconf, enum xnvme_pr opts);

/**
 * Prints the given array of version-strings to stdout
 *
 * @param libconf Pointer to the ::xnvme_libconf to print
 * @param opts Printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_libconf_pr(const struct xnvme_libconf *libconf, enum xnvme_pr opts);
