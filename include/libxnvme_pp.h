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

/* Printers of libxnvme_spec.h */

/**
 * Prints the given :;xnvme_spec_log_health_entry to the given output stream
 *
 * @param stream output stream used for printing
 * @param log pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_health_fpr(FILE *stream, const struct xnvme_spec_log_health_entry *log, int opts);

/**
 * Prints the given :;xnvme_spec_log_health_entry to stdout
 *
 * @param log
 * @param opts
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_health_pr(const struct xnvme_spec_log_health_entry *log, int opts);

/**
 * Prints the given :;xnvme_spec_log_sanitize_entry to the given output stream
 *
 * @param stream output stream used for printing
 * @param log pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_sanitize_fpr(FILE *stream, const struct xnvme_spec_log_sanitize_entry *log,
			    int opts);

/**
 * Prints the given :;xnvme_spec_log_sanitize_entry to stdout
 *
 * @param log pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_sanitize_pr(const struct xnvme_spec_log_sanitize_entry *log, int opts);

/**
 * Prints the given :;xnvme_spec_log_erri_entry to stdout
 *
 * @param stream output stream used for printing
 * @param log
 * @param limit
 * @param opts
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_erri_fpr(FILE *stream, const struct xnvme_spec_log_erri_entry *log, int limit,
			int opts);

/**
 * Prints the given :;xnvme_spec_log_erri_entry to stdout
 *
 * @param log
 * @param limit
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_erri_pr(const struct xnvme_spec_log_erri_entry *log, int limit, int opts);

/**
 * Prints the given :;xnvme_spec_log_fdp_conf to stdout
 *
 * @param log
 * @param opts
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_fdp_conf_pr(const struct xnvme_spec_log_fdp_conf *log, int opts);

/**
 * Prints the given :;xnvme_spec_log_fdp_stats to stdout
 *
 * @param log
 * @param opts
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_fdp_stats_pr(const struct xnvme_spec_log_fdp_stats *log, int opts);

/**
 * Prints the given :;xnvme_spec_log_fdp_events_entry to stdout
 *
 * @param log
 * @param limit
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_fdp_events_pr(const struct xnvme_spec_log_fdp_events *log, int limit, int opts);

/**
 * Prints the given :;xnvme_spec_log_ruhu to stdout
 *
 * @param log
 * @param limit
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_ruhu_pr(const struct xnvme_spec_log_ruhu *log, int limit, int opts);

/**
 * Prints the given :;xnvme_spec_ruhs to stdout
 *
 * @param ruhs
 * @param limit
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_ruhs_pr(const struct xnvme_spec_ruhs *ruhs, int limit, int opts);

/**
 * Prints the given :;xnvme_spec_idfy_ns to the given output stream
 *
 * @param stream output stream used for printing
 * @param idfy pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_idfy_ns_fpr(FILE *stream, const struct xnvme_spec_idfy_ns *idfy, int opts);

/**
 * Prints the given :;xnvme_spec_idfy_ns to stdout
 *
 * @param idfy pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_idfy_ns_pr(const struct xnvme_spec_idfy_ns *idfy, int opts);

/**
 * Prints the given :;xnvme_spec_idfy_ctrlr to the given output stream
 *
 * @param stream output stream used for printing
 * @param idfy pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_idfy_ctrlr_fpr(FILE *stream, const struct xnvme_spec_idfy_ctrlr *idfy, int opts);

/**
 * Prints the given :;xnvme_spec_idfy_ctrlr to stdout
 *
 * @param idfy pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_idfy_ctrlr_pr(const struct xnvme_spec_idfy_ctrlr *idfy, int opts);

int
xnvme_spec_idfy_cs_fpr(FILE *stream, const struct xnvme_spec_idfy_cs *idfy, int opts);

int
xnvme_spec_idfy_cs_pr(const struct xnvme_spec_idfy_cs *idfy, int opts);

/**
 * Prints the given :;xnvme_spec_feat to the given output stream
 *
 * @param stream output stream used for printing
 * @param fid feature identifier
 * @param feat feature values
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_feat_fpr(FILE *stream, uint8_t fid, const union xnvme_spec_feat *feat, int opts);

/**
 * Prints the given :;xnvme_spec_feat to the given output stream
 *
 * @param fid feature identifier
 * @param feat feature values
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_feat_pr(uint8_t fid, const union xnvme_spec_feat *feat, int opts);

/**
 * Prints the :;xnvme_spec_fdp_event_desc to the given output stream
 *
 * @param buf Buffer which contains xnvme_spec_fdp_event_desc
 * @param feat to get number of events
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_feat_fdp_events_pr(void *buf, const union xnvme_spec_feat *feat, int opts);

/**
 * Prints the given :;xnvme_spec_sqe to the given output stream
 *
 * @param stream output stream used for printing
 * @param cmd pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_sqe_fpr(FILE *stream, union xnvme_spec_sqe *cmd, int opts);

/**
 * Prints the given :;xnvme_spec_sqe to stdout
 *
 * @param cmd pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_sqe_pr(union xnvme_spec_sqe *cmd, int opts);

/**
 * Prints the given :;xnvme_spec_idfy_dir_rp to stdout
 *
 * @param idfy pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_drecv_idfy_pr(struct xnvme_spec_idfy_dir_rp *idfy, int opts);

/**
 * Prints the given :;xnvme_spec_streams_dir_rp to stdout
 *
 * @param srp pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_drecv_srp_pr(struct xnvme_spec_streams_dir_rp *srp, int opts);

/**
 * Prints the given :;xnvme_spec_streams_dir_gs to stdout
 *
 * @param sgs pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_drecv_sgs_pr(struct xnvme_spec_streams_dir_gs *sgs, int opts);

/**
 * Prints the given :;xnvme_spec_alloc_resource to stdout
 *
 * @param sar structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_drecv_sar_pr(const struct xnvme_spec_alloc_resource *sar, int opts);

int
xnvme_spec_nvm_scopy_fmt_zero_fpr(FILE *stream, const struct xnvme_spec_nvm_scopy_fmt_zero *entry,
				  int opts);

/**
 * Prints the given ::xnvme_spec_nvm_scopy_fmt_zero to stdout
 *
 * @param entry pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_scopy_fmt_zero_pr(const struct xnvme_spec_nvm_scopy_fmt_zero *entry, int opts);

/**
 * Prints the given ::xnvme_spec_nvm_scopy_source_range to the given output stream
 *
 * @param stream output stream used for printing
 * @param srange pointer to structure to print
 * @param nr zero-based number of entries, at most ::XNVME_SPEC_NVM_SCOPY_NENTRY_MAX -1
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_scopy_source_range_fpr(FILE *stream,
				      const struct xnvme_spec_nvm_scopy_source_range *srange,
				      uint8_t nr, int opts);

/**
 * Prints the given ::xnvme_spec_nvm_scopy_source_range to stdout
 *
 * @param srange pointer to structure to print
 * @param nr nr=0: print ::XNVME_SPEC_NVM_SCOPY_NENTRY_MAX entries from the given , nr>0:
 * print no more than nr
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_scopy_source_range_pr(const struct xnvme_spec_nvm_scopy_source_range *srange,
				     uint8_t nr, int opts);

/**
 * Prints the given ::xnvme_spec_nvm_idfy_ctrlr to the given output stream
 *
 * @param stream output stream used for printing
 * @param idfy pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_idfy_ctrlr_fpr(FILE *stream, struct xnvme_spec_nvm_idfy_ctrlr *idfy, int opts);

/**
 * Prints the given ::xnvme_spec_nvm_idfy_ctrlr to stdout
 *
 * @param idfy pointer to ::xnvme_spec_nvm_idfy_ctrlr
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_idfy_ctrlr_pr(struct xnvme_spec_nvm_idfy_ctrlr *idfy, int opts);

/**
 * Prints the given ::xnvme_spec_nvm_idfy_ns to the given output stream
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
xnvme_spec_nvm_idfy_ns_fpr(FILE *stream, struct xnvme_spec_nvm_idfy_ns *idfy, int opts);

/**
 * Prints the given ::xnvme_spec_nvm_idfy_ns to stdout
 *
 * @param idfy pointer to ::xnvme_spec_nvm_idfy_ns
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_idfy_ns_pr(struct xnvme_spec_nvm_idfy_ns *idfy, int opts);

/**
 * Prints the given ::xnvme_spec_znd_idfy_ctrlr to the given output stream
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
xnvme_spec_znd_idfy_ctrlr_fpr(FILE *stream, struct xnvme_spec_znd_idfy_ctrlr *zctrlr, int opts);

/**
 * Prints the given ::xnvme_spec_znd_idfy_ctrlr to the given output stream
 *
 * Only fields specific to Zoned Namespaces are printed by this function
 *
 * @param zctrlr pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_znd_idfy_ctrlr_pr(struct xnvme_spec_znd_idfy_ctrlr *zctrlr, int opts);

/**
 * Prints the given ::xnvme_spec_znd_idfy_lbafe to the given output stream
 *
 * @param stream output stream used for printing
 * @param zonef pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_znd_idfy_lbafe_fpr(FILE *stream, struct xnvme_spec_znd_idfy_lbafe *zonef, int opts);

/**
 * Prints the given ::xnvme_spec_znd_idfy_ns to the given output stream
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
xnvme_spec_znd_idfy_ns_fpr(FILE *stream, struct xnvme_spec_znd_idfy_ns *zns, int opts);

/**
 * Prints the given ::xnvme_spec_znd_idfy_ns to stdout
 *
 * @param zns pointer to ::xnvme_spec_znd_idfy_ns
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_znd_idfy_ns_pr(struct xnvme_spec_znd_idfy_ns *zns, int opts);

/**
 * Prints the given ::xnvme_spec_znd_log_changes to the given output stream
 *
 * @param stream output stream used for printing
 * @param changes pointer to subject to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_znd_log_changes_fpr(FILE *stream, const struct xnvme_spec_znd_log_changes *changes,
			       int opts);

/**
 * Pretty-printer of ::xnvme_spec_znd_log_changes
 *
 * @param changes pointer to subject to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_znd_log_changes_pr(const struct xnvme_spec_znd_log_changes *changes, int opts);

/**
 * Prints the given ::xnvme_spec_znd_descr to the given stream
 *
 * @param stream output stream used for printing
 * @param descr the struct to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_znd_descr_fpr(FILE *stream, const struct xnvme_spec_znd_descr *descr, int opts);

/**
 * Prints the given ::xnvme_spec_znd_descr to stdout
 *
 * @param descr the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_znd_descr_pr(const struct xnvme_spec_znd_descr *descr, int opts);

/**
 * Print the given ::xnvme_spec_znd_report_hdr to the given output stream
 *
 * @param stream output stream used for printing
 * @param hdr the struct to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_znd_report_hdr_fpr(FILE *stream, const struct xnvme_spec_znd_report_hdr *hdr, int opts);

/**
 * Print the given ::xnvme_spec_znd_report_hdr to the stdout
 *
 * @param hdr the struct to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_znd_report_hdr_pr(const struct xnvme_spec_znd_report_hdr *hdr, int opts);

int
xnvme_spec_znd_descr_fpr_yaml(FILE *stream, const struct xnvme_spec_znd_descr *descr, int indent,
			      const char *sep);

/**
 * Prints the given xnvme_spec_kvs_idfy to the given output stream
 *
 * @param stream output stream used for printing
 * @param idfy pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_kvs_idfy_ns_fpr(FILE *stream, const struct xnvme_spec_kvs_idfy_ns *idfy, int opts);

/**
 * Prints the given :;xnvme_spec_kvs_idfy to stdout
 *
 * @param idfy pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_kvs_idfy_ns_pr(const struct xnvme_spec_kvs_idfy_ns *idfy, int opts);

/**
 * Produces a string representation of the given ::xnvme_spec_adm_opc
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_adm_opc_str(enum xnvme_spec_adm_opc eval);

/**
 * Produces a string representation of the given ::xnvme_spec_csi
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_csi_str(enum xnvme_spec_csi eval);

/**
 * Produces a string representation of the given ::xnvme_spec_feat_id
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_feat_id_str(enum xnvme_spec_feat_id eval);

/**
 * Produces a string representation of the given ::xnvme_spec_feat_sel
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_feat_sel_str(enum xnvme_spec_feat_sel eval);

/**
 * Produces a string representation of the given ::xnvme_spec_flag
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_flag_str(enum xnvme_spec_flag eval);

/**
 * Produces a string representation of the given ::xnvme_spec_idfy_cns
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_idfy_cns_str(enum xnvme_spec_idfy_cns eval);

/**
 * Produces a string representation of the given ::xnvme_spec_log_lpi
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_log_lpi_str(enum xnvme_spec_log_lpi eval);

/**
 * Produces a string representation of the given ::xnvme_spec_znd_log_lid
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_znd_log_lid_str(enum xnvme_spec_znd_log_lid eval);

/**
 * Produces a string representation of the given ::xnvme_spec_nvm_cmd_cpl_sc
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_nvm_cmd_cpl_sc_str(enum xnvme_spec_nvm_cmd_cpl_sc eval);

/**
 * Produces a string representation of the given ::xnvme_spec_nvm_opc
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_nvm_opc_str(enum xnvme_spec_nvm_opc eval);

/**
 * Produces a string representation of the given ::xnvme_spec_psdt
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_psdt_str(enum xnvme_spec_psdt eval);

/**
 * Produces a string representation of the given ::xnvme_spec_sgl_descriptor_subtype
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_sgl_descriptor_subtype_str(enum xnvme_spec_sgl_descriptor_subtype eval);

/**
 * Produces a string representation of the given ::xnvme_spec_znd_cmd_mgmt_recv_action
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_znd_cmd_mgmt_recv_action_str(enum xnvme_spec_znd_cmd_mgmt_recv_action eval);

/**
 * Produces a string representation of the given ::xnvme_spec_znd_cmd_mgmt_recv_action_sf
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_znd_cmd_mgmt_recv_action_sf_str(enum xnvme_spec_znd_cmd_mgmt_recv_action_sf eval);

/**
 * Produces a string representation of the given ::xnvme_spec_znd_cmd_mgmt_send_action
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_znd_cmd_mgmt_send_action_str(enum xnvme_spec_znd_cmd_mgmt_send_action eval);

/**
 * Produces a string representation of the given ::xnvme_spec_znd_opc
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_znd_opc_str(enum xnvme_spec_znd_opc eval);

/**
 * Produces a string representation of the given ::xnvme_spec_znd_mgmt_send_action_so
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_znd_mgmt_send_action_so_str(enum xnvme_spec_znd_mgmt_send_action_so eval);

/**
 * Produces a string representation of the given ::xnvme_spec_znd_status_code
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_znd_status_code_str(enum xnvme_spec_znd_status_code eval);

/**
 * Produces a string representation of the given ::xnvme_spec_znd_state
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_znd_state_str(enum xnvme_spec_znd_state eval);

/**
 * Produces a string representation of the given ::xnvme_spec_znd_type
 *
 * @param eval the enum value to produce a string representation of
 * @return On success, a string representation is returned. On error, the string
 * "ENOSYS" is returned.
 */
const char *
xnvme_spec_znd_type_str(enum xnvme_spec_znd_type eval);

int
xnvme_spec_ctrlr_bar_fpr(FILE *stream, struct xnvme_spec_ctrlr_bar *bar, int opts);

int
xnvme_spec_ctrlr_bar_pp(struct xnvme_spec_ctrlr_bar *bar, int opts);

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
