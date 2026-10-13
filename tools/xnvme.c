// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libxnvme.h>
#include <xnvme_vcs.h>

#define SET_EVENT_TYPES ((uint8_t[]){0x0, 0x1, 0x2, 0x3, 0x80, 0x81})
#define SET_EVENT_BUF_SIZE sizeof(SET_EVENT_TYPES)

int
enumerate_cb(struct xnvme_dev *dev, void *cb_args)
{
	uint32_t *ns_count_ref = cb_args;
	const struct xnvme_ident *ident;

	if (*ns_count_ref == 0) {
		fprintf(stdout, "\n");
	}

	ident = xnvme_dev_get_ident(dev);
	fprintf(stdout, "  - {");
	xnvme_ident_yaml(stdout, ident, 0, ", ", 0);
	fprintf(stdout, "}\n");

	*ns_count_ref = *ns_count_ref + 1;

	return XNVME_ENUMERATE_DEV_CLOSE;
}

int
listing_cb(struct xnvme_dev *dev, void *cb_args)
{
	struct xnvme_cli_enumeration *list = cb_args;
	const struct xnvme_ident *ident;

	ident = xnvme_dev_get_ident(dev);
	if (xnvme_cli_enumeration_append(list, ident)) {
		XNVME_DEBUG("FAILED: adding ident");
	}

	return XNVME_ENUMERATE_DEV_CLOSE;
}

static int
sub_listing(struct xnvme_cli *cli)
{
	struct xnvme_cli_enumeration *listing = NULL;
	struct xnvme_opts opts = {0};
	int err;

	err = xnvme_cli_to_opts(cli, &opts);
	if (err) {
		xnvme_cli_perr("xnvme_cli_to_opts()", err);
		return err;
	}

	err = xnvme_cli_enumeration_alloc(&listing, 100);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_cli_enumeration_alloc()");
		return err;
	}

	err = xnvme_enumerate(cli->args.sys_uri, &opts, *listing_cb, listing);
	if (err) {
		xnvme_cli_perr("xnvme_enumerate()", err);
		goto exit;
	}

	xnvme_cli_enumeration_pp(listing, XNVME_PR_DEF);

exit:
	xnvme_buf_virt_free(listing);

	return err;
}

static int
sub_enumerate(struct xnvme_cli *cli)
{
	struct xnvme_opts opts = {0};
	uint32_t ns_count = 0;
	int err = 0;

	err = xnvme_cli_to_opts(cli, &opts);
	if (err) {
		xnvme_cli_perr("xnvme_cli_to_opts()", err);
		return err;
	}

	fprintf(stdout, "xnvme_cli_enumeration:");

	err = xnvme_enumerate(cli->args.sys_uri, &opts, *enumerate_cb, &ns_count);
	if (err) {
		xnvme_cli_perr("xnvme_enumerate()", err);
		return err;
	}

	if (ns_count == 0) {
		fprintf(stdout, " ~\n");
	}

	return 0;
}

static int
scan_cb(const struct xnvme_ident *ident, void *cb_args)
{
	uint32_t *count_ref = cb_args;

	if (*count_ref == 0) {
		fprintf(stdout, "\n");
	}

	fprintf(stdout, "  - {");
	xnvme_ident_yaml(stdout, ident, 0, ", ", 0);
	fprintf(stdout, "}\n");

	*count_ref = *count_ref + 1;

	return 0;
}

static int
sub_scan(struct xnvme_cli *cli)
{
	struct xnvme_opts opts = {0};
	uint32_t count = 0;
	int err = 0;

	err = xnvme_cli_to_opts(cli, &opts);
	if (err) {
		xnvme_cli_perr("xnvme_cli_to_opts()", err);
		return err;
	}

	fprintf(stdout, "xnvme_scan:");

	err = xnvme_scan(cli->args.sys_uri, &opts, scan_cb, &count);
	if (err) {
		xnvme_cli_perr("xnvme_scan()", err);
		return err;
	}

	if (count == 0) {
		fprintf(stdout, " ~\n");
	}

	return 0;
}

static int
sub_info(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	int err;

	err = xnvme_dev_derive_geo(dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_dev_derive_geo(), err: %d", err);
		return err;
	}

	xnvme_dev_pr(dev, XNVME_PR_DEF);

	return 0;
}

static inline int
_sub_idfy(struct xnvme_cli *cli, uint8_t cns, uint16_t cntid, uint8_t nsid, uint16_t nvmsetid,
	  uint8_t uuid)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	struct xnvme_spec_idfy *dbuf = NULL;
	size_t dbuf_nbytes = sizeof(*dbuf);
	int err;

	// Setup command
	ctx.cmd.common.opcode = XNVME_SPEC_ADM_OPC_IDFY;
	ctx.cmd.common.nsid = nsid;
	ctx.cmd.idfy.cns = cns;
	ctx.cmd.idfy.cntid = cntid;
	ctx.cmd.idfy.nvmsetid = nvmsetid;
	ctx.cmd.idfy.uuid = uuid;

	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_cmd_pass_admin(&ctx, dbuf, dbuf_nbytes, NULL, 0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_cmd_pass_admin()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	switch (cns) {
	case XNVME_SPEC_IDFY_NS:
		xnvme_spec_idfy_ns_pr(&dbuf->ns, XNVME_PR_DEF);
		break;

	case XNVME_SPEC_IDFY_CTRLR:
		xnvme_spec_idfy_ctrlr_pr(&dbuf->ctrlr, XNVME_PR_DEF);
		break;

	case XNVME_SPEC_IDFY_IOCS:
		xnvme_spec_idfy_cs_pr(&dbuf->cs, XNVME_PR_DEF);
		break;

	default:
		xnvme_cli_pinf("No pretty-printer for the given "
			       "cns(0%x0)\n"
			       "Use --data-output to dump the result to file",
			       cns);
		break;
	}

	if (cli->args.data_output) {
		xnvme_cli_pinf("Dumping to: '%s'", cli->args.data_output);
		err = xnvme_buf_to_file((char *)dbuf, dbuf_nbytes, cli->args.data_output);
		if (err) {
			xnvme_cli_perr("xnvme_buf_to_file()", err);
		}
	}

exit:
	xnvme_buf_free(dev, dbuf);

	return err;
}

static int
sub_idfy(struct xnvme_cli *cli)
{
	uint8_t cns = cli->args.cns;
	uint64_t cntid = cli->args.cntid;
	uint32_t nsid = cli->args.nsid;
	uint16_t nvmsetid = cli->args.setid;
	uint8_t uuid = cli->args.uuid;

	switch (cns) {
	case XNVME_SPEC_IDFY_NS:
	case XNVME_SPEC_IDFY_NSLIST:
	case XNVME_SPEC_IDFY_NSDSCR:
	case XNVME_SPEC_IDFY_NS_IOCS:
	case XNVME_SPEC_IDFY_NSLIST_IOCS:
	case XNVME_SPEC_IDFY_NSLIST_ALLOC:
	case XNVME_SPEC_IDFY_NS_ALLOC:
	case XNVME_SPEC_IDFY_CTRLR_NS:
	case XNVME_SPEC_IDFY_NSLIST_ALLOC_IOCS:
	case XNVME_SPEC_IDFY_NS_ALLOC_IOCS:
		// Only add nsid if needed
		if (!cli->given[XNVME_CLI_OPT_NSID]) {
			nsid = xnvme_dev_get_nsid(cli->args.dev);
		}
	}

	return _sub_idfy(cli, cns, cntid, nsid, nvmsetid, uuid);
}

static int
sub_idfy_ns(struct xnvme_cli *cli)
{
	uint8_t nsid = cli->args.nsid;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	return _sub_idfy(cli, XNVME_SPEC_IDFY_NS, 0x0, nsid, 0x0, 0x0);
}

static int
sub_idfy_ctrlr(struct xnvme_cli *cli)
{
	return _sub_idfy(cli, XNVME_SPEC_IDFY_CTRLR, 0x0, 0x0, 0x0, 0x0);
}

static int
sub_idfy_cs(struct xnvme_cli *cli)
{
	return _sub_idfy(cli, XNVME_SPEC_IDFY_IOCS, 0xFFFF, 0x0, 0x0, 0x0);
}

static int
sub_log_health(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t nsid = cli->args.nsid;

	struct xnvme_spec_log_health_entry *log = NULL;
	const size_t log_nbytes = sizeof(*log);
	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		// Perform log-health on controller as the default
		nsid = 0xFFFFFFFF;
	}

	xnvme_cli_pinf("Allocating and clearing buffer...");
	log = xnvme_buf_alloc(dev, log_nbytes);
	if (!log) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_buf_clear(log, log_nbytes);
	if (err) {
		xnvme_cli_perr("xnvme_buf_clear()", err);
		goto exit;
	}

	xnvme_cli_pinf("Retrieving SMART / health log page ...");
	err = xnvme_adm_log(&ctx, XNVME_SPEC_LOG_HEALTH, 0x0, 0, nsid, 0, log, log_nbytes);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_log(XNVME_SPEC_LOG_HEALTH)", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_spec_log_health_pr(log, XNVME_PR_DEF);

exit:
	xnvme_buf_free(dev, log);

	return err;
}

static int
sub_log_erri(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t limit = cli->args.limit;

	struct xnvme_spec_log_erri_entry *log = NULL;
	uint32_t log_nentries = 0;
	uint32_t log_nbytes = 0;
	int err;

	// NOTE: The Error Log Page Entries (elpe) is a zero-based value
	log_nentries = (uint32_t)xnvme_dev_get_ctrlr(dev)->elpe + 1;
	if (limit) {
		log_nentries = limit;
	}
	log_nbytes = log_nentries * sizeof(*log);

	xnvme_cli_pinf("Allocating and clearing buffer...");
	log = xnvme_buf_alloc(dev, log_nbytes);
	if (!log) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_buf_clear(log, log_nbytes);
	if (err) {
		xnvme_cli_perr("xnvme_buf_clear()", err);
		goto exit;
	}

	xnvme_cli_pinf("Retrieving error-info-log ...");
	err = xnvme_adm_log(&ctx, XNVME_SPEC_LOG_ERRI, 0x0, 0x0, 0xFFFFFFFF, 0, log, log_nbytes);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_log(XNVME_SPEC_LOG_ERRI)", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	printf("# %" PRIu32 " error log page entries:\n", log_nentries);
	xnvme_spec_log_erri_pr(log, log_nentries, XNVME_PR_DEF);

exit:
	xnvme_buf_free(dev, log);

	return err;
}

static int
sub_log_sanitize(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t nsid = cli->args.nsid;

	struct xnvme_spec_log_sanitize_entry *log = NULL;
	const size_t log_nbytes = sizeof(*log);
	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvme_cli_pinf("Allocating and clearing buffer...");
	log = xnvme_buf_alloc(dev, log_nbytes);
	if (!log) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_buf_clear(log, log_nbytes);
	if (err) {
		xnvme_cli_perr("xnvme_buf_clear()", err);
		goto exit;
	}

	xnvme_cli_pinf("Retrieving Sanitize log page ...");
	err = xnvme_adm_log(&ctx, XNVME_SPEC_LOG_SANITIZE, 0x0, 0, nsid, 0, log, log_nbytes);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_log(XNVME_SPEC_LOG_SANITIZE)", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_spec_log_sanitize_pr(log, XNVME_PR_DEF);

exit:
	xnvme_buf_free(dev, log);

	return err;
}

static int
sub_log_fdp_config(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t nsid = cli->args.nsid;
	uint32_t egi = cli->args.lsi;

	struct xnvme_spec_log_fdp_conf *log = NULL;
	uint32_t log_nbytes = cli->args.data_nbytes;
	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvme_cli_pinf("Allocating and clearing buffer...");
	if (!log_nbytes) {
		err = -EINVAL;
		xnvme_cli_perr("!arg:data_nbytes", err);
		goto exit;
	}

	log = xnvme_buf_alloc(dev, log_nbytes);
	if (!log) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_buf_clear(log, log_nbytes);
	if (err) {
		xnvme_cli_perr("xnvme_buf_clear()", err);
		goto exit;
	}

	xnvme_prep_adm_log(&ctx, XNVME_SPEC_LOG_FDPCONF, 0x0, 0, nsid, 0, log_nbytes);
	ctx.cmd.log.lsi = egi;

	xnvme_cli_pinf("Retrieving FDP configurations log page ...");
	err = xnvme_cmd_pass_admin(&ctx, log, log_nbytes, NULL, 0x0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_log(XNVME_SPEC_LOG_FDPCONF)", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}
	xnvme_spec_log_fdp_conf_pr(log, XNVME_PR_DEF);

exit:
	xnvme_buf_free(dev, log);

	return err;
}

static int
sub_log_ruhu(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t limit = cli->args.limit;
	uint32_t nsid = cli->args.nsid;
	uint32_t egi = cli->args.lsi;

	struct xnvme_spec_log_ruhu *log = NULL;
	uint32_t log_nbytes = 0;
	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	log_nbytes = sizeof(*log) + limit * sizeof(struct xnvme_spec_ruhu_desc);

	xnvme_cli_pinf("Allocating and clearing buffer...");
	log = xnvme_buf_alloc(dev, log_nbytes);
	if (!log) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_buf_clear(log, log_nbytes);
	if (err) {
		xnvme_cli_perr("xnvme_buf_clear()", err);
		goto exit;
	}

	xnvme_cli_pinf("Retrieving ruhu-log ...");
	xnvme_prep_adm_log(&ctx, XNVME_SPEC_LOG_FDPRUHU, 0x0, 0, nsid, 0, log_nbytes);
	ctx.cmd.log.lsi = egi;

	err = xnvme_cmd_pass_admin(&ctx, log, log_nbytes, NULL, 0x0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_log(XNVME_SPEC_LOG_FDPRUHU)", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	printf("# %" PRIu32 " reclaim unit handle usage:\n", limit);
	xnvme_spec_log_ruhu_pr(log, limit, XNVME_PR_DEF);

exit:
	xnvme_buf_free(dev, log);

	return err;
}

static int
sub_log_fdp_stats(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t nsid = cli->args.nsid;
	uint32_t egi = cli->args.lsi;

	struct xnvme_spec_log_fdp_stats *log = NULL;

	const size_t log_nbytes = sizeof(*log);
	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvme_cli_pinf("Allocating and clearing buffer...");
	log = xnvme_buf_alloc(dev, log_nbytes);
	if (!log) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_buf_clear(log, log_nbytes);
	if (err) {
		xnvme_cli_perr("xnvme_buf_clear()", err);
		goto exit;
	}

	xnvme_prep_adm_log(&ctx, XNVME_SPEC_LOG_FDPSTATS, 0x0, 0, nsid, 0, log_nbytes);
	ctx.cmd.log.lsi = egi;

	xnvme_cli_pinf("Retrieving FDP statistics log page ...");
	err = xnvme_cmd_pass_admin(&ctx, log, log_nbytes, NULL, 0x0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_log(XNVME_SPEC_LOG_FDPSTATS)", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}
	xnvme_spec_log_fdp_stats_pr(log, XNVME_PR_DEF);

exit:
	xnvme_buf_free(dev, log);

	return err;
}

static int
sub_log_fdp_events(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t limit = cli->args.limit;
	uint32_t nsid = cli->args.nsid;
	uint32_t egi = cli->args.lsi;
	uint32_t lsp = cli->args.lsp;

	struct xnvme_spec_log_fdp_events *log = NULL;
	uint32_t log_nbytes = 0;
	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	log_nbytes = sizeof(*log) + limit * sizeof(struct xnvme_spec_fdp_event);

	xnvme_cli_pinf("Allocating and clearing buffer...");
	log = xnvme_buf_alloc(dev, log_nbytes);
	if (!log) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_buf_clear(log, log_nbytes);
	if (err) {
		xnvme_cli_perr("xnvme_buf_clear()", err);
		goto exit;
	}

	xnvme_cli_pinf("Retrieving fdp-events-log ...");
	xnvme_prep_adm_log(&ctx, XNVME_SPEC_LOG_FDPEVENTS, 0x0, 0, nsid, 0, log_nbytes);
	ctx.cmd.log.lsi = egi;
	ctx.cmd.log.lsp = lsp;

	err = xnvme_cmd_pass_admin(&ctx, log, log_nbytes, NULL, 0x0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_log(XNVME_SPEC_LOG_FDPEVENTS)", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	printf("# %" PRIu32 " fdp events log page entries:\n", limit);
	xnvme_spec_log_fdp_events_pr(log, limit, XNVME_PR_DEF);

exit:
	xnvme_buf_free(dev, log);

	return err;
}

static int
sub_log(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);

	uint8_t lid = cli->args.lid;
	uint8_t lsp = cli->args.lsp;
	uint64_t lpo_nbytes = cli->args.lpo_nbytes;
	uint32_t nsid = cli->args.nsid;
	uint8_t rae = cli->args.reset;
	uint32_t buf_nbytes = cli->args.data_nbytes;

	void *buf = NULL;
	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvme_cli_pinf("xnvme_adm_log: {lid: 0x%x, lsp: 0x%x, "
		       "lpo_nbytes: %zu, nsid: 0x%x, rae: %d}",
		       lid, lsp, lpo_nbytes, nsid, rae);

	if (!buf_nbytes) {
		err = -EINVAL;
		xnvme_cli_perr("!arg:data_nbytes", err);
		goto exit;
	}

	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_adm_log(&ctx, lid, lsp, lpo_nbytes, nsid, rae, buf, buf_nbytes);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_log()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_cli_pinf("No printer for log-pages");

	if (!cli->args.data_output) {
		xnvme_cli_pinf("Use -o to dump output to file");
	}

	// Generic buf-print and/or dump to file
	if (cli->args.data_output) {
		xnvme_cli_pinf("Dumping to: '%s'", cli->args.data_output);
		err = xnvme_buf_to_file(buf, buf_nbytes, cli->args.data_output);
		if (err) {
			xnvme_cli_perr("xnvme_buf_to_file()", err);
		}
	}

exit:
	xnvme_buf_free(dev, buf);

	return err;
}

static int
sub_gfeat(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint8_t fid = cli->args.fid;
	uint8_t sel = cli->args.sel;
	uint32_t nsid = cli->args.nsid;
	uint32_t cdw11 = cli->args.cdw[11];
	void *dbuf = NULL;
	size_t dbuf_nbytes = cli->args.data_nbytes;
	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	if (dbuf_nbytes) {
		dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
		if (!dbuf) {
			err = -errno;
			xnvme_cli_perr("xnvme_buf_alloc()", err);
			goto exit;
		}

		err = xnvme_buf_clear(dbuf, dbuf_nbytes);
		if (err) {
			xnvme_cli_perr("xnvme_buf_clear()", err);
			goto exit;
		}
	}

	xnvme_cli_pinf("cmd_gfeat: {nsid: 0x%x, fid: 0x%x, sel: 0x%x}", nsid, fid, sel);

	xnvme_prep_adm_gfeat(&ctx, nsid, fid, sel);
	ctx.cmd.gfeat.cdw11 = cdw11;

	err = xnvme_cmd_pass_admin(&ctx, dbuf, dbuf_nbytes, NULL, 0x0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_cmd_pass_admin()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	{
		struct xnvme_spec_feat feat = {.val = ctx.cpl.cdw0};
		xnvme_spec_feat_pr(fid, feat, XNVME_PR_DEF);

		if (fid == XNVME_SPEC_FEAT_FDP_EVENTS) {
			xnvme_spec_feat_fdp_events_pr(dbuf, feat, XNVME_PR_DEF);
		}
	}

	if (cli->args.data_output) {
		xnvme_cli_pinf("dumping to: '%s'", cli->args.data_output);
		err = xnvme_buf_to_file(dbuf, dbuf_nbytes, cli->args.data_output);
		if (err) {
			xnvme_cli_perr("xnvme_buf_to_file()", err);
		}
	}

exit:
	if (dbuf)
		xnvme_buf_free(dev, dbuf);
	return 0;
}

static int
sub_sfeat(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint8_t fid = cli->args.fid;
	uint32_t feat = cli->args.feat;
	uint8_t save = cli->args.save;
	uint32_t nsid = cli->args.nsid;
	uint32_t cdw12 = cli->args.cdw[12];
	void *dbuf = NULL;
	size_t dbuf_nbytes = 0;
	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	if (cli->args.data_input) {
		if (!cli->args.data_nbytes) {
			err = -EINVAL;
			xnvme_cli_perr("require data bytes", err);
			goto exit;
		}

		dbuf_nbytes = cli->args.data_nbytes;
		dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
		if (!dbuf) {
			err = -errno;
			xnvme_cli_perr("xnvme_buf_alloc()", err);
			goto exit;
		}

		err = xnvme_buf_fill(dbuf, dbuf_nbytes, cli->args.data_input);
		if (err) {
			xnvme_cli_perr("xnvme_buf_fill()", err);
			goto exit;
		}
	}

	xnvme_cli_pinf("cmd_sfeat: {nsid: 0%x, fid: 0x%x, save: 0x%x, feat: 0x%x, cdw12: 0x%x}",
		       nsid, fid, save, feat, cdw12);

	xnvme_prep_adm_sfeat(&ctx, 0, fid, feat, save);
	ctx.cmd.sfeat.cdw12 = cdw12;

	err = xnvme_cmd_pass_admin(&ctx, dbuf, dbuf_nbytes, NULL, 0x0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_cmd_pass_admin()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
	}

exit:
	if (dbuf)
		xnvme_buf_free(dev, dbuf);

	return err;
}

static int
sub_set_fdp_events(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint8_t fid = cli->args.fid;
	uint32_t feat = cli->args.feat;
	uint8_t save = cli->args.save;
	uint32_t nsid = cli->args.nsid;
	uint32_t cdw12 = cli->args.cdw[12];
	uint8_t *dbuf = NULL;
	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	dbuf = xnvme_buf_alloc(dev, SET_EVENT_BUF_SIZE);
	if (!dbuf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_buf_memcpy(dbuf, SET_EVENT_TYPES, SET_EVENT_BUF_SIZE);
	if (err) {
		xnvme_cli_perr("xnvme_buf_memcpy()", err);
		goto exit;
	}

	xnvme_cli_pinf("cmd_sfeat: {nsid: 0%x, fid: 0x%x, save: 0x%x, feat: 0x%x, cdw12: 0x%x}",
		       nsid, fid, save, feat, cdw12);

	xnvme_prep_adm_sfeat(&ctx, nsid, fid, feat, save);
	ctx.cmd.sfeat.cdw12 = cdw12;

	err = xnvme_cmd_pass_admin(&ctx, (void *)dbuf, SET_EVENT_BUF_SIZE, NULL, 0x0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_cmd_pass_admin()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
	}

exit:
	if (dbuf)
		xnvme_buf_free(dev, dbuf);

	return err;
}

static int
sub_format(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t nsid = cli->args.nsid;
	uint8_t lbafl = cli->args.lbafl;
	uint8_t lbafu = cli->args.lbafu;
	uint8_t mset = cli->args.mset;
	uint8_t ses = cli->args.ses;
	uint8_t pi = cli->args.pi;
	uint8_t pil = cli->args.pil;
	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvme_cli_pinf("xnvme_adm_format: {nsid: 0x%08x, lbafl: 0x%x, lbafu: 0x%x, "
		       "mset: 0x%x, ses: 0x%x, pi: 0x%x, pil: 0x%x}",
		       nsid, lbafl, lbafu, mset, ses, pi, pil);

	err = xnvme_adm_format(&ctx, nsid, lbafl, lbafu, mset, ses, pi, pil);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_format()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
	}

	return err;
}

static int
sub_sanitize(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint8_t sanact = cli->args.sanact;
	bool ause = cli->args.ause;
	uint32_t ovrpat = cli->args.ovrpat;
	uint8_t owpass = cli->args.owpass;
	bool oipbp = cli->args.oipbp;
	bool nodas = cli->args.nodas;
	int err;

	xnvme_cli_pinf("xnvme_nvm_sanitize: {sanact: 0x%x, ause: 0x%x, "
		       "ovrpat: 0x%x, owpass: 0x%x, oipbp: 0x%x, "
		       "nodas: 0x%x}",
		       sanact, ause, ovrpat, owpass, oipbp, nodas);

	err = xnvme_nvm_sanitize(&ctx, sanact, ause, ovrpat, owpass, oipbp, nodas);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_nvm_sanitize()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
	}

	return err;
}

static int
sub_ruhs(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t limit = cli->args.limit;
	uint32_t nsid = cli->args.nsid;

	struct xnvme_spec_ruhs *ruhs = NULL;
	uint32_t ruhs_nbytes = 0;
	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	ruhs_nbytes = sizeof(*ruhs) + limit * sizeof(struct xnvme_spec_ruhs_desc);

	xnvme_cli_pinf("Allocating and clearing buffer...");
	ruhs = xnvme_buf_alloc(dev, ruhs_nbytes);
	if (!ruhs) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_buf_clear(ruhs, ruhs_nbytes);
	if (err) {
		xnvme_cli_perr("xnvme_buf_clear()", err);
		goto exit;
	}

	xnvme_cli_pinf("Retrieving ruhs ...");
	err = xnvme_nvm_mgmt_recv(&ctx, nsid, XNVME_SPEC_IO_MGMT_RECV_RUHS, 0, ruhs, ruhs_nbytes);

	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_nvm_mgmt_recv(XNVME_SPEC_IO_MGMT_RECV_RUHS)", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	printf("# %" PRIu32 " reclaim unit handle status:\n", limit);
	xnvme_spec_ruhs_pr(ruhs, limit, XNVME_PR_DEF);

exit:
	xnvme_buf_free(dev, ruhs);

	return err;
}

static int
sub_ruhu(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t pi = cli->args.pi;
	uint32_t nsid = cli->args.nsid;
	uint16_t *pid_list = NULL;
	uint32_t npids = 1;

	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	pid_list = xnvme_buf_alloc(dev, npids * 2);
	if (!pid_list) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_buf_memcpy(pid_list, &pi, npids * 2);
	if (err) {
		xnvme_cli_perr("xnvme_buf_memcpy()", err);
		goto exit;
	}

	xnvme_cli_pinf("Updating ruh ...");
	err = xnvme_nvm_mgmt_send(&ctx, nsid, XNVME_SPEC_IO_MGMT_SEND_RUHU, npids - 1, pid_list,
				  npids * 2);

	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_nvm_mgmt_send(XNVME_SPEC_IO_MGMT_SEND_RUHU)", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

exit:
	xnvme_buf_free(dev, pid_list);

	return err;
}

static int
sub_pass(struct xnvme_cli *cli, int admin)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	void *data_buf = NULL;
	size_t data_nbytes = cli->args.data_nbytes;
	void *meta_buf = NULL;
	size_t meta_nbytes = cli->args.meta_nbytes;
	int err;

	for (int i = 0; i < 16; ++i) { // Setup using --cdwXX arguments
		uint32_t *cdw = (void *)&ctx.cmd;

		if (!cli->given[XNVME_CLI_OPT_CDW00 + i]) {
			continue;
		}
		cdw[i] = cli->args.cdw[i];
	}
	if (cli->given[XNVME_CLI_OPT_NSID]) { // Setup opcode and nsid
		ctx.cmd.common.nsid = cli->args.nsid;
	}
	if (cli->given[XNVME_CLI_OPT_OPCODE]) {
		ctx.cmd.common.opcode = cli->args.opcode;
	}

	if (data_nbytes) {
		data_buf = xnvme_buf_alloc(dev, data_nbytes);
		if (!data_buf) {
			err = -errno;
			xnvme_cli_perr("xnvme_buf_alloc()", err);
			goto exit;
		}

		if (cli->args.data_input) {
			if (cli->args.verbose) {
				xnvme_cli_pinf("Loading data-input(%s)", cli->args.data_input);
			}
			xnvme_buf_from_file(data_buf, data_nbytes, cli->args.data_input);
		}
	}

	if (meta_nbytes) {
		meta_buf = xnvme_buf_alloc(dev, meta_nbytes);
		if (!meta_buf) {
			err = -errno;
			xnvme_cli_perr("xnvme_buf_alloc()", err);
			goto exit;
		}

		if (cli->args.meta_input) {
			if (cli->args.verbose) {
				xnvme_cli_pinf("Loading meta-input(%s)", cli->args.meta_input);
			}
			xnvme_buf_from_file(meta_buf, meta_nbytes, cli->args.meta_input);
		}
	}

	if (cli->args.verbose) {
		xnvme_cli_pinf("Submitting ...");
		xnvme_spec_cmd_pr(&ctx.cmd, XNVME_PR_DEF);
	}

	if (admin) {
		err = xnvme_cmd_pass_admin(&ctx, data_buf, data_nbytes, meta_buf, meta_nbytes);
	} else {
		err = xnvme_cmd_pass(&ctx, data_buf, data_nbytes, meta_buf, meta_nbytes);
	}
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("Failed xnvme_cmd_pass[_admin]()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}
	if (cli->args.verbose) {
		xnvme_cli_pinf("No submission nor completion errors");
	}

	if (data_nbytes && cli->args.data_output) {
		if (cli->args.verbose) {
			xnvme_cli_pinf("Dumping data-output(%s)", cli->args.data_output);
		}
		err = xnvme_buf_to_file(data_buf, data_nbytes, cli->args.data_output);
		if (err) {
			xnvme_cli_perr("xnvme_buf_to_file()", err);
		}
	}
	if (meta_nbytes && cli->args.meta_output) {
		if (cli->args.verbose) {
			xnvme_cli_pinf("Dumping meta-output(%s)", cli->args.meta_output);
		}
		err = xnvme_buf_to_file(meta_buf, meta_nbytes, cli->args.meta_output);
		if (err) {
			xnvme_cli_perr("xnvme_buf_to_file()", err);
		}
	}

exit:
	xnvme_buf_free(dev, data_buf);
	xnvme_buf_free(dev, meta_buf);

	return err;
}

static int
sub_pioc(struct xnvme_cli *cli)
{
	return sub_pass(cli, 0);
}

static int
sub_padc(struct xnvme_cli *cli)
{
	return sub_pass(cli, 1);
}

static int
sub_library_info(struct xnvme_cli *XNVME_UNUSED(cli))
{
	xnvme_cli_pinf("xNVMe Library Information");
	xnvme_ver_pr(XNVME_PR_DEF);
	printf("\n");

	xnvme_libconf_pr(xnvme_libconf_get(), XNVME_PR_DEF);
	printf("\n");

	xnvme_be_attr_list_pr(XNVME_PR_DEF);
	printf("\n");

	return 0;
}

static int
sub_dsm(struct xnvme_cli *cli)
{
	// We can only define one range in CLI
	struct xnvme_spec_dsm_range *dsm_range;

	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t nsid = cli->args.nsid;
	uint64_t slba = cli->args.slba;
	uint32_t llb = cli->args.llb;
	bool ad = cli->args.ad;
	bool idw = cli->args.idw;
	bool idr = cli->args.idr;
	int err;

	xnvme_cli_pinf("xNVMe DSM");
	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	dsm_range = xnvme_buf_alloc(dev, sizeof(*dsm_range));
	if (!dsm_range) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	dsm_range->cattr = 0;
	dsm_range->llb = llb;
	dsm_range->slba = slba;

	err = xnvme_nvm_dsm(&ctx, nsid, dsm_range, 0, ad, idw, idr);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_nvm_dsm()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}
exit:
	return err;
}

static int
show_regs(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	struct xnvme_spec_ctrlr_bar bar = {0};
	int err;

	err = xnvme_controller_get_registers(dev, &bar);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_controller_get_registers()", err);
		return err;
	}

	xnvme_spec_ctrlr_bar_pp(&bar, XNVME_PR_DEF);

	return 0;
}

static int
subsystem_reset(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	int err;

	err = xnvme_subsystem_reset(dev);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_subsystem_reset()", err);
		return err;
	}

	return 0;
}

static int
ctrlr_reset(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	int err;

	err = xnvme_controller_reset(dev);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_controller_reset()", err);
		return err;
	}

	return 0;
}

static int
ns_rescan(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	int err;

	err = xnvme_namespace_rescan(dev);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_namespace_rescan()", err);
		return err;
	}

	return 0;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{
		"list",
		"List devices on the system",
		"List devices on the system",
		sub_listing,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SYS_URI, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_FLAGS, XNVME_CLI_LOPT},

			XNVME_CLI_CORE_OPTS,
		},
	},
	{
		"enum",
		"Enumerate devices on the system",
		"Enumerate devices on the system",
		sub_enumerate,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SYS_URI, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_FLAGS, XNVME_CLI_LOPT},

			XNVME_CLI_CORE_OPTS,
		},
	},
	{
		"scan",
		"Scan for devices without opening them",
		"Scan for devices without opening them",
		sub_scan,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SYS_URI, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_FLAGS, XNVME_CLI_LOPT},

			XNVME_CLI_CORE_OPTS,
		},
	},
	{
		"info",
		"Retrieve derived information for given device",
		"Retrieve derived information for given device",
		sub_info,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_ASYNC_OPTS,
		},
	},
	{
		"idfy",
		"Execute an User-defined Identify Command",
		"Execute an User-defined Identify Command",
		sub_idfy,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_CNS, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_CNTID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_SETID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_UUID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"idfy-ns",
		"Identify the given Namespace",
		"Identify the given Namespace",
		sub_idfy_ns,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"idfy-ctrlr",
		"Identify the given Controller",
		"Identify the given Controller",
		sub_idfy_ctrlr,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"idfy-cs",
		"Identify the Command Sets supported by the controller",
		"Identify the Command Sets supported by the controller",
		sub_idfy_cs,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"log",
		"Retrieve a User-defined Log",
		"Retrieve and print log",
		sub_log,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_LID, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_LSP, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_LPO_NBYTES, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_RAE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_NBYTES, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"log-erri",
		"Retrieve the error-information log",
		"Retrieve and print log",
		sub_log_erri,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_LIMIT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"log-health",
		"Retrieve the S.M.A.R.T. / Health information log",
		"Retrieve and print log",
		sub_log_health,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"log-sanitize",
		"Retrieve the Sanitize Status log",
		"Retrieve and print log",
		sub_log_sanitize,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"log-fdp-config",
		"Retrieve the FDP configurations log",
		"Retrieve and print log",
		sub_log_fdp_config,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_NBYTES, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_LSI, XNVME_CLI_LREQ},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"log-ruhu",
		"Retrieve the reclaim unit handle usage log",
		"Retrieve and print log",
		sub_log_ruhu,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_LIMIT, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_LSI, XNVME_CLI_LREQ},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"log-fdp-stats",
		"Retrieve the FDP statistics log",
		"Retrieve and print log",
		sub_log_fdp_stats,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_LSI, XNVME_CLI_LREQ},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"log-fdp-events",
		"Retrieve the fdp-events log",
		"Retrieve and print log",
		sub_log_fdp_events,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_LIMIT, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_LSI, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_LSP, XNVME_CLI_LREQ},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"feature-get",
		"Execute a Get-Features Command",
		"Execute a Get Features Command",
		sub_gfeat,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_FID, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_SEL, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW11, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_NBYTES, XNVME_CLI_LOPT},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"feature-set",
		"Execute a Set-Features Command",
		"Execute a Set-Features Command",
		sub_sfeat,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_FID, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_FEAT, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_SAVE, XNVME_CLI_LFLG},
			{XNVME_CLI_OPT_CDW12, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_INPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_NBYTES, XNVME_CLI_LOPT},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"set-fdp-events",
		"Enable or disable all events",
		"Enable or disable all events",
		sub_set_fdp_events,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_FID, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_FEAT, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_SAVE, XNVME_CLI_LFLG},
			{XNVME_CLI_OPT_CDW12, XNVME_CLI_LREQ},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"format",
		"Format a NVM namespace",
		"Format a NVM namespace",
		sub_format,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_LBAFL, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_LBAFU, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_MSET, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_SES, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_PI, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_PIL, XNVME_CLI_LOPT},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"sanitize",
		"Start a Sanitize Operation",
		"Start a Sanitize Operation",
		sub_sanitize,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_SANACT, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_AUSE, XNVME_CLI_LFLG},
			{XNVME_CLI_OPT_OVRPAT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_OWPASS, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_OIPBP, XNVME_CLI_LFLG},
			{XNVME_CLI_OPT_NODAS, XNVME_CLI_LFLG},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"fdp-ruhs",
		"Retrieve the Reclaim Unit Handle Status",
		"Retrieve and print log",
		sub_ruhs,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_LIMIT, XNVME_CLI_LREQ},

			XNVME_CLI_SYNC_OPTS,
		},
	},
	{
		"fdp-ruhu",
		"Reclaim Unit Handle Update for a Placement Identifier",
		"Reclaim Unit Handle Update for a Placement Identifier",
		sub_ruhu,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_PID, XNVME_CLI_LREQ},

			XNVME_CLI_SYNC_OPTS,
		},
	},
	{
		"pioc",
		"Pass a used-defined IO Command through",
		"Pass a used-defined IO Command through",
		sub_pioc,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},

			{XNVME_CLI_OPT_CDW00, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW01, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW02, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW03, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW04, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW04, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW05, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW06, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW07, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW08, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW09, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW10, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW11, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW12, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW13, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW14, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW15, XNVME_CLI_LOPT},

			{XNVME_CLI_OPT_OPCODE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},

			{XNVME_CLI_OPT_DATA_INPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_NBYTES, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_META_INPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_META_OUTPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_META_NBYTES, XNVME_CLI_LOPT},

			{XNVME_CLI_OPT_VERBOSE, XNVME_CLI_LFLG},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"padc",
		"Pass a user-defined ADmin Command through",
		"Pass a user-defined ADmin Command through",
		sub_padc,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},

			{XNVME_CLI_OPT_CDW00, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW01, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW02, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW03, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW04, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW04, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW05, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW06, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW07, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW08, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW09, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW10, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW11, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW12, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW13, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW14, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW15, XNVME_CLI_LOPT},

			{XNVME_CLI_OPT_OPCODE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},

			{XNVME_CLI_OPT_DATA_INPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_NBYTES, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_META_INPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_META_OUTPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_META_NBYTES, XNVME_CLI_LOPT},

			{XNVME_CLI_OPT_VERBOSE, XNVME_CLI_LFLG},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"library-info",
		"Produce information about the library",
		"Produce information about the library",
		sub_library_info,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
		},
	},
	{
		"dsm",
		"Dataset Management",
		"Dataset Management",
		sub_dsm,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_AD, XNVME_CLI_LFLG},
			{XNVME_CLI_OPT_IDW, XNVME_CLI_LFLG},
			{XNVME_CLI_OPT_IDR, XNVME_CLI_LFLG},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_LLB, XNVME_CLI_LREQ},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"subsystem-reset",
		"Resets the subsystem",
		"Resets the subsystem",
		subsystem_reset,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"show-regs",
		"Show controller-registers",
		"Show controller-registers",
		show_regs,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"ctrlr-reset",
		"Resets the controller",
		"Resets the controller",
		ctrlr_reset,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"ns-rescan",
		"Rescans the nvme namespaces",
		"Rescans the nvme namespaces",
		ns_rescan,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "xNVMe - Cross-platform NVMe utility",
	.vcs = XNVME_VCS_TAG,
	.descr_short = "Construct and execute NVMe Commands",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

/**
 * Front-end: bare 'xnvme' and a group name print the command tree one level at a time, and a
 * command that lives in another program is looked up on the exec path and run.
 *
 * The exec path is XNVME_EXEC_PATH when set, a list of directories, else the directory the
 * programs are installed into.
 */
#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
#include <process.h>
#define CLI_PATH_SEP ';'
#define CLI_EXE_SUFFIX ".exe"
#else
#define CLI_PATH_SEP ':'
#define CLI_EXE_SUFFIX ""
#endif

#define CLI_TESTS_PREFIX "xnvme_tests_"
#define CLI_NAMES_MAX 256

struct cli_ext {
	const char *name;  ///< Command name under 'xnvme'
	const char *prog;  ///< Program the command runs
	const char *descr; ///< Short description, shown by the overview
	bool listed;       ///< Whether the overview lists it; unlisted ones resolve all the same
};

static const struct cli_ext g_cli_exts[] = {
	{"perf", "xnvmeperf", "Synthetic workloads, on every backend", true},
	{"homi", "homi", "Share controllers between processes", false},
	{"qublk", "qublk", "Expose an xNVMe device as a Linux block device", false},
};

/// Built-ins the front-end lists itself, or keeps out of the 'nvme' group
static const char *g_cli_not_nvme[] = {"list", "scan", "enum", "info", "library-info", NULL};

static const char *
cli_exec_path(void)
{
	const char *env = getenv("XNVME_EXEC_PATH");

	return (env && env[0]) ? env : XNVME_BINDIR;
}

/**
 * Find 'prog' in the directories of the exec path, writing its path to 'path'
 *
 * @return On success, 0 is returned. When no directory holds it, -ENOENT is returned.
 */
static int
cli_resolve(const char *prog, char *path, size_t path_len)
{
	const char *seg = cli_exec_path();

	while (*seg) {
		const char *end = strchr(seg, CLI_PATH_SEP);
		int seg_len = end ? (int)(end - seg) : (int)strlen(seg);

		if (seg_len) {
			struct stat st;

			snprintf(path, path_len, "%.*s/%s%s", seg_len, seg, prog, CLI_EXE_SUFFIX);
			// Not a directory of the same name, as the build tree has for xnvmeperf
			if (!stat(path, &st) && S_ISREG(st.st_mode)) {
				return 0;
			}
		}
		if (!end) {
			break;
		}
		seg = end + 1;
	}

	return -ENOENT;
}

/**
 * Run the program at 'path' with 'argv0' followed by the 'argc' arguments in 'argv'
 *
 * @return Only returns on failure, with 1, except on Windows, where the exit status of the
 * program is returned
 */
static int
cli_run(const char *path, char *argv0, int argc, char **argv)
{
	char **nargv = calloc(argc + 2, sizeof(*nargv));
	int err;

	if (!nargv) {
		fprintf(stderr, "xnvme: out of memory\n");
		return 1;
	}
	nargv[0] = argv0;
	for (int i = 0; i < argc; ++i) {
		nargv[i + 1] = argv[i];
	}

#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
	// Windows has no exec that replaces the process, and joins argv into one command line
	// without quoting, so the program keeps its own name rather than 'xnvme <command>'
	nargv[0] = (char *)path;
	err = (int)_spawnv(_P_WAIT, path, (const char *const *)nargv);
	if (err >= 0) {
		free(nargv);
		return err;
	}
#else
	execv(path, nargv);
#endif
	err = errno;
	fprintf(stderr, "xnvme: cannot run '%s': %s\n", path, strerror(err));
	free(nargv);

	return 1;
}

static const struct xnvme_cli_sub *
cli_builtin(struct xnvme_cli *cli, const char *name)
{
	for (int i = 0; i < cli->nsubs; ++i) {
		if (!strcmp(cli->subs[i].name, name)) {
			return &cli->subs[i];
		}
	}

	return NULL;
}

static const struct cli_ext *
cli_ext(const char *name)
{
	for (size_t i = 0; i < sizeof g_cli_exts / sizeof(*g_cli_exts); ++i) {
		if (!strcmp(g_cli_exts[i].name, name)) {
			return &g_cli_exts[i];
		}
	}

	return NULL;
}

static bool
cli_not_nvme(const char *name)
{
	for (int i = 0; g_cli_not_nvme[i]; ++i) {
		if (!strcmp(g_cli_not_nvme[i], name)) {
			return true;
		}
	}

	return false;
}

static int
cli_cmp_names(const void *a, const void *b)
{
	return strcmp(*(char *const *)a, *(char *const *)b);
}

/**
 * Collect the test names found on the exec path: the program names without CLI_TESTS_PREFIX,
 * with '-' for '_' as 'xnvme test' spells them
 *
 * @return The number of names stored in 'names', sorted and without duplicates
 */
static int
cli_tests(char **names, int names_max)
{
	const char *seg = cli_exec_path();
	size_t prefix_len = strlen(CLI_TESTS_PREFIX);
	size_t suffix_len = strlen(CLI_EXE_SUFFIX);
	int count = 0;

	while (*seg) {
		const char *end = strchr(seg, CLI_PATH_SEP);
		int seg_len = end ? (int)(end - seg) : (int)strlen(seg);
		char dpath[4096];
		struct dirent *ent;
		DIR *dir;

		snprintf(dpath, sizeof(dpath), "%.*s", seg_len, seg);
		dir = seg_len ? opendir(dpath) : NULL;
		while (dir && (ent = readdir(dir)) && count < names_max) {
			size_t len = strlen(ent->d_name);
			char fpath[4096 + 256];
			struct stat st;

			if (strncmp(ent->d_name, CLI_TESTS_PREFIX, prefix_len) ||
			    len <= prefix_len) {
				continue;
			}
			if (suffix_len &&
			    (len <= suffix_len ||
			     strcmp(ent->d_name + len - suffix_len, CLI_EXE_SUFFIX))) {
				continue;
			}
			snprintf(fpath, sizeof(fpath), "%s/%s", dpath, ent->d_name);
			if (stat(fpath, &st) || !S_ISREG(st.st_mode)) {
				continue;
			}
			len -= prefix_len + suffix_len;
			names[count] = malloc(len + 1);
			if (names[count]) {
				memcpy(names[count], ent->d_name + prefix_len, len);
				names[count][len] = '\0';
				for (char *c = names[count]; *c; ++c) {
					*c = (*c == '_') ? '-' : *c;
				}
				count += 1;
			}
		}
		if (dir) {
			closedir(dir);
		}
		if (!end) {
			break;
		}
		seg = end + 1;
	}

	qsort(names, count, sizeof(*names), cli_cmp_names);
	for (int i = 1; i < count; ++i) {
		if (!strcmp(names[i - 1], names[i])) {
			free(names[i]);
			memmove(&names[i], &names[i + 1], (count - i - 1) * sizeof(*names));
			count -= 1;
			i -= 1;
		}
	}

	return count;
}

static void
cli_tests_free(char **names, int count)
{
	for (int i = 0; i < count; ++i) {
		free(names[i]);
	}
}

static void
cli_row(const char *cmd, const char *descr)
{
	printf("  %-24s %s\n", cmd, descr);
}

static int
cli_overview(struct xnvme_cli *cli)
{
	char path[4096];

	printf("Usage: xnvme <command> [<args>] [--help]\n");
	printf("\n");
	printf("%s\n", cli->descr_short);
	printf("\n");
	printf("Start here:\n");
	cli_row("xnvme list", cli_builtin(cli, "list")->descr_short);
	cli_row("xnvme info <uri>", cli_builtin(cli, "info")->descr_short);
	cli_row("xnvme enum", cli_builtin(cli, "enum")->descr_short);
	cli_row("xnvme library-info", cli_builtin(cli, "library-info")->descr_short);
	for (size_t i = 0; i < sizeof g_cli_exts / sizeof(*g_cli_exts); ++i) {
		if (g_cli_exts[i].listed && !cli_resolve(g_cli_exts[i].prog, path, sizeof(path))) {
			char cmd[64];

			snprintf(cmd, sizeof(cmd), "xnvme %s", g_cli_exts[i].name);
			cli_row(cmd, g_cli_exts[i].descr);
		}
	}
	printf("\n");
	printf("Groups, run one to list its commands:\n");
	cli_row("xnvme test", "installed test programs");
	cli_row("xnvme nvme", "NVMe admin commands; see nvme-cli for kernel-bound devices");
	cli_row("xnvme --help", "the device commands list, enum, scan and info");

	return 0;
}

static int
cli_group_nvme(struct xnvme_cli *cli)
{
	printf("Commands in 'xnvme nvme':\n");
	printf("\n");
	for (int i = 0; i < cli->nsubs; ++i) {
		if (!cli_not_nvme(cli->subs[i].name)) {
			cli_row(cli->subs[i].name, cli->subs[i].descr_short);
		}
	}
	printf("\n");
	printf("See 'xnvme <command> --help' for the description of [<args>]\n");

	return 0;
}

static int
cli_group_test(void)
{
	char *names[CLI_NAMES_MAX];
	int count = cli_tests(names, CLI_NAMES_MAX);

	if (!count) {
		printf("No test programs found on the exec path: %s\n", cli_exec_path());
		return 0;
	}

	printf("Test programs, run one to list its subcommands:\n");
	printf("\n");
	for (int i = 0; i < count; ++i) {
		printf("  xnvme test %s\n", names[i]);
	}
	printf("\n");
	printf("See 'xnvme test <name> <subcommand> --help' for the description of [<args>]\n");
	cli_tests_free(names, count);

	return 0;
}

static int
cli_test(int argc, char **argv)
{
	char prog[256];
	char path[4096];
	char argv0[512];

	snprintf(prog, sizeof(prog), "%s%s", CLI_TESTS_PREFIX, argv[0]);
	for (char *c = prog; *c; ++c) {
		*c = (*c == '-') ? '_' : *c;
	}
	if (cli_resolve(prog, path, sizeof(path))) {
		fprintf(stderr, "xnvme: no test program '%s' on the exec path; see 'xnvme test'\n",
			argv[0]);
		return 1;
	}
	snprintf(argv0, sizeof(argv0), "xnvme test %s", argv[0]);

	return cli_run(path, argv0, argc - 1, argv + 1);
}

static int
cli_external(const struct cli_ext *ext, int argc, char **argv)
{
	char path[4096];
	char argv0[512];

	if (cli_resolve(ext->prog, path, sizeof(path))) {
		fprintf(stderr, "xnvme: '%s' runs '%s', which is not on the exec path: %s\n",
			ext->name, ext->prog, cli_exec_path());
		return 1;
	}
	snprintf(argv0, sizeof(argv0), "xnvme %s", ext->name);

	return cli_run(path, argv0, argc, argv);
}

/**
 * Handle what the front-end owns: the overview, the groups, '--exec-path', and the
 * commands run by other programs
 *
 * @return The exit status of a command the front-end handled, or -1 for a command it leaves
 * to xnvme_cli_run(), a built-in subcommand or one that does not exist
 */
static int
cli_main(struct xnvme_cli *cli, int argc, char **argv)
{
	const char *cmd = argc > 1 ? argv[1] : NULL;
	const struct cli_ext *ext;

	if (!cmd) {
		return cli_overview(cli);
	}
	if (cli_builtin(cli, cmd)) {
		return -1;
	}
	if (!strcmp(cmd, "--exec-path")) {
		printf("%s\n", cli_exec_path());
		return 0;
	}
	if (!strcmp(cmd, "nvme")) {
		return cli_group_nvme(cli);
	}
	if (!strcmp(cmd, "test")) {
		return argc > 2 ? cli_test(argc - 2, argv + 2) : cli_group_test();
	}
	if (!strcmp(cmd, "help")) {
		char *help[] = {"--help"};

		if (argc < 3 || !strcmp(argv[2], "help") || !strcmp(argv[2], "--help")) {
			return cli_overview(cli);
		}
		if (cli_builtin(cli, argv[2])) {
			char *nargv[] = {argv[0], argv[2], "--help", NULL};

			return xnvme_cli_run(cli, 3, nargv, XNVME_CLI_INIT_NONE);
		}
		if (!strcmp(argv[2], "nvme") || !strcmp(argv[2], "test")) {
			char *nargv[] = {argv[0], argv[2], NULL};

			return cli_main(cli, 2, nargv);
		}
		ext = cli_ext(argv[2]);
		if (ext) {
			return cli_external(ext, 1, help);
		}
		fprintf(stderr, "xnvme: no command '%s'; see 'xnvme'\n", argv[2]);
		return 1;
	}
	ext = cli_ext(cmd);
	if (ext) {
		return cli_external(ext, argc - 2, argv + 2);
	}

	return -1;
}

int
main(int argc, char **argv)
{
	int err = cli_main(&g_cli, argc, argv);

	if (err >= 0) {
		return err;
	}

	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
