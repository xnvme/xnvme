#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libxnvme.h>
#include <libxnvmec.h>

static int
sub_enumerate(struct xnvmec *XNVME_UNUSED(cli))
{
	struct xnvme_enumeration *listing = NULL;

	xnvmec_pinfo("xnvme_enumerate()");

	listing = xnvme_enumerate(0x0);
	if (!listing) {
		xnvmec_perror("xnvme_enumerate()");
		return -1;
	}

	xnvme_enumeration_pr(listing, XNVME_PR_DEF);

	free(listing);

	return 0;
}

static int
sub_info(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;

	xnvme_dev_pr(dev, XNVME_PR_DEF);

	return 0;
}

static inline int
_sub_idfy(struct xnvmec *cli, uint8_t cns, uint16_t cntid, uint8_t nsid,
	  uint16_t nvmsetid, uint8_t uuid)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_spec_idfy *idfy = NULL;
	enum xnvme_spec_nstype nstype = XNVME_SPEC_NSTYPE_NOCHECK;
	struct xnvme_req req = { 0 };
	int err;

	xnvmec_pinfo("xnvme_cmd_idfy: {cns: 0x%x, cntid: 0x%x, nsid: 0x%x, "
		     "nstype: %s, nvmsetid: 0x%x, uuid: 0x%x}",
		     cns, cntid, nsid, xnvme_spec_nstype_str(nstype), nvmsetid,
		     uuid);

	idfy = xnvme_buf_alloc(dev, sizeof(*idfy), NULL);
	if (!idfy) {
		xnvmec_perror("xnvme_buf_alloc()");
		return -1;
	}

	err = xnvme_cmd_idfy(dev, cns, cntid, nsid, nstype, nvmsetid, uuid,
			     idfy, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_perror("xnvme_cmd_idfy()");
		xnvme_req_pr(&req, XNVME_PR_DEF);
		xnvme_buf_free(dev, idfy);
		return -1;
	}

	switch (cns) {
	case XNVME_SPEC_IDFY_NS:
		xnvme_spec_idfy_ns_pr(&idfy->ns, XNVME_PR_DEF);
		break;

	case XNVME_SPEC_IDFY_CTRLR:
		xnvme_spec_idfy_ctrl_pr(&idfy->ctrlr, XNVME_PR_DEF);
		break;

	default:
		xnvmec_pinfo("No pretty-printer for the given "
			     "cns(0%x0)\n"
			     "Use -o to dump the result to file",
			     cns);
		break;
	}

	if (cli->args.data_output) {
		xnvmec_pinfo("Dumping to: '%s'", cli->args.data_output);
		if (xnvmec_buf_to_file((char *) idfy, sizeof(*idfy),
				       cli->args.data_output)) {
			xnvmec_perror("xnvmec_buf_to_file()");
		}
	}

	xnvme_buf_free(dev, idfy);

	return 0;
}

static int
sub_idfy(struct xnvmec *cli)
{
	uint8_t cns = cli->args.cns;
	uint64_t cntid = cli->args.cntid;
	uint32_t nsid = cli->args.nsid;
	uint16_t nvmsetid = cli->args.setid;
	uint8_t uuid = cli->args.uuid;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	return _sub_idfy(cli, cns, cntid, nsid, nvmsetid, uuid);
}

static int
sub_idfy_ns(struct xnvmec *cli)
{
	uint8_t nsid = cli->args.nsid;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	return _sub_idfy(cli, XNVME_SPEC_IDFY_NS, 0x0, nsid, 0x0, 0x0);
}

static int
sub_idfy_ctrlr(struct xnvmec *cli)
{
	return _sub_idfy(cli, XNVME_SPEC_IDFY_CTRLR, 0x0, 0x0, 0x0, 0x0);
}

static int
sub_log_health(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint32_t nsid = cli->args.nsid;

	struct xnvme_spec_log_health_entry *log = NULL;
	const size_t log_nbytes = sizeof(*log);
	struct xnvme_req req = { 0 };
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinfo("Allocating and clearing buffer...");
	log = xnvme_buf_alloc(dev, log_nbytes, NULL);
	if (!log) {
		xnvmec_perror("xnvme_buf_alloc");
		return 1;
	}
	memset(log, 0, log_nbytes);

	xnvmec_pinfo("Retrieving SMART / health log page ...");
	err = xnvme_cmd_log(dev, XNVME_SPEC_LOG_HEALTH, 0x0, 0, nsid, 0, log,
			    log_nbytes, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_perror(
			"xnvme_cmd_log(..., XNVME_SPEC_LOG_HEALTH, ...)");
		xnvme_req_pr(&req, XNVME_PR_DEF);
		xnvme_buf_free(dev, log);
		return 1;
	}

	xnvme_spec_log_health_pr(log, XNVME_PR_DEF);

	xnvme_buf_free(dev, log);

	return 0;
}

static int
sub_log_erri(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint32_t limit = cli->args.limit;
	uint32_t nsid = cli->args.nsid;

	struct xnvme_spec_log_erri_entry *log = NULL;
	uint32_t log_nentries = 0;
	uint32_t log_nbytes = 0;
	struct xnvme_req req = { 0 };
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	// NOTE: The Error Log Page Entries (elpe) is a zero-based value
	log_nentries = (uint32_t) xnvme_dev_get_ctrlr(dev)->elpe + 1;
	if (limit) {
		log_nentries = limit;
	}
	log_nbytes = log_nentries * sizeof(*log);

	xnvmec_pinfo("Allocating and clearing buffer...");
	log = xnvme_buf_alloc(dev, log_nbytes, NULL);
	if (!log) {
		xnvmec_perror("xnvme_buf_alloc");
		return 1;
	}
	memset(log, 0, log_nbytes);

	xnvmec_pinfo("Retrieving error-info-log ...");
	err = xnvme_cmd_log(dev, XNVME_SPEC_LOG_ERRI, 0x0, 0x0, nsid, 0, log,
			    log_nbytes, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_perror("xnvme_cmd_log(XNVME_SPEC_LOG_ERRI)");
		xnvme_req_pr(&req, XNVME_PR_DEF);
		xnvme_buf_free(dev, log);
		return 1;
	}

	printf("# %d error log page entries:\n", log_nentries);
	xnvme_spec_log_erri_pr(log, log_nentries, XNVME_PR_DEF);

	xnvme_buf_free(dev, log);

	return 0;
}

static int
sub_log(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;

	uint8_t lid = cli->args.lid;
	uint8_t lsp = cli->args.lsp;
	uint64_t lpo_nbytes = cli->args.lpo_nbytes;
	uint32_t nsid = cli->args.nsid;
	uint8_t rae = cli->args.reset;
	uint32_t buf_nbytes = cli->args.data_nbytes;

	void *buf = NULL;
	struct xnvme_req req = { 0 };
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinfo("xnvme_cmd_log: {lid: 0x%x, lsp: 0x%x, "
		     "lpo_nbytes: %zu, nsid: 0x%x, rae: %d}",
		     lid, lsp, lpo_nbytes, nsid, rae);

	if (!buf_nbytes) {
		errno = EINVAL;
		xnvmec_perror("buf_nbytes");
		XNVME_DEBUG("buf_nbytes: %u", buf_nbytes);
		return 1;
	}

	buf = xnvme_buf_alloc(dev, buf_nbytes, NULL);
	if (!buf) {
		xnvmec_perror("xnvme_buf_alloc()");
		return 1;
	}

	err = xnvme_cmd_log(dev, lid, lsp, lpo_nbytes, nsid, rae, buf,
			    buf_nbytes, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_perror("xnvme_cmd_log()");
		xnvme_req_pr(&req, XNVME_PR_DEF);
		xnvme_buf_free(dev, buf);
		return 1;
	}

	xnvmec_pinfo("No printer for log-pages");

	if (!cli->args.data_output) {
		xnvmec_pinfo("Use -o to dump output to file");
	}

	// Generic buf-print and/or dump to file
	if (cli->args.data_output) {
		xnvmec_pinfo("Dumping to: '%s'", cli->args.data_output);
		if (xnvmec_buf_to_file(buf, buf_nbytes,
				       cli->args.data_output)) {
			xnvmec_perror("xnvmec_buf_to_file()");
		}
	}

	xnvme_buf_free(dev, buf);

	return 0;
}

static int
sub_gfeat(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint8_t fid = cli->args.fid;
	uint8_t sel = cli->args.sel;
	uint32_t nsid = cli->args.nsid;
	struct xnvme_req req = { 0 };
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinfo("cmd_gfeat: {nsid: 0x%x, fid: 0x%x, sel: 0x%x}",
		     nsid, fid, sel);

	err = xnvme_cmd_gfeat(dev, nsid, fid, sel, NULL, 0, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_perror("xnvme_cmd_gfeat()");
		xnvme_req_pr(&req, XNVME_PR_DEF);
		return -1;
	}

	{
		struct xnvme_spec_feat feat = { .val = req.cpl.cdw0 };
		xnvme_spec_feat_pr(fid, feat, XNVME_PR_DEF);
	}

	xnvmec_pinfo("Completed without errors");

	return 0;
}

static int
sub_sfeat(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint8_t fid = cli->args.fid;
	uint32_t feat = cli->args.feat;
	uint8_t save = cli->args.save;
	uint32_t nsid = cli->args.nsid;
	struct xnvme_req req = { 0 };
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinfo("cmd_sfeat: {nsid: 0%x, fid: 0x%x, feat: 0x%x, 0x%x}",
		     nsid, fid, feat, save);

	err = xnvme_cmd_sfeat(dev, 0, fid, feat, 0, NULL, 0, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_perror("xnvme_cmd_gfeat()");
		xnvme_req_pr(&req, XNVME_PR_DEF);
		return -1;
	}

	xnvmec_pinfo("Completed without errors");

	return 0;
}

static int
sub_format(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint32_t nsid = cli->args.nsid;
	uint8_t lbaf = cli->args.lbaf;
	uint8_t zf = cli->args.zf;
	uint8_t mset = cli->args.mset;
	uint8_t ses = cli->args.ses;
	uint8_t pi = cli->args.pi;
	uint8_t pil = cli->args.pil;
	struct xnvme_req req = { 0 };
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinfo("xnvme_cmd_format: {nsid: 0x%08x, lbaf: 0x%x, zf: 0x%x, "
		     "mset: 0x%x, ses: 0x%x, pi: 0x%x, pil: 0x%x}", nsid,
		     lbaf, zf, mset, ses, pi, pil);

	err = xnvme_cmd_format(dev, nsid, lbaf, zf, mset, ses, pi, pil, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_perror("xnvme_cmd_format()");
		xnvme_req_pr(&req, XNVME_PR_DEF);
		return -1;
	}

	xnvmec_pinfo("Completed without errors");

	return 0;
}

static int
sub_sanitize(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint8_t sanact = cli->args.action;
	uint8_t ause = cli->args.ause;
	uint32_t ovrpat = cli->args.ovrpat;
	uint8_t owpass = cli->args.owpass;
	uint8_t oipbp = cli->args.oipbp;
	uint8_t nodas = cli->args.nodas;
	struct xnvme_req req = { 0 };
	int err;

	xnvmec_pinfo("xnvme_cmd_sanitize: {sanact: 0x%x, ause: 0x%x, "
		     "ovrpat: 0x%x, owpass: 0x%x, oipbp: 0x%x, "
		     "nodas: 0x%x}", sanact, ause, ovrpat, owpass, oipbp,
		     nodas);

	err = xnvme_cmd_sanitize(dev, sanact, ause, ovrpat, owpass, oipbp,
				 nodas,
				 &req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_perror("xnvme_cmd_format()");
		xnvme_req_pr(&req, XNVME_PR_DEF);
		return -1;
	}

	xnvmec_pinfo("Completed without errors");

	return 0;
}

static int
sub_pass(struct xnvmec *cli, int opts, int admin)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_spec_cmd cmd = { 0 };
	struct xnvme_req req = { 0 };
	void *data_buf = NULL;
	size_t data_nbytes = cli->args.data_nbytes;
	void *meta_buf = NULL;
	size_t meta_nbytes = cli->args.meta_nbytes;
	int err;

	errno = 0;

	xnvmec_pinfo("xnvme_cmd_pass(..., CMD_OPTS=0x%x, ...)", opts);

	err = xnvmec_buf_from_file(&cmd, sizeof(cmd), cli->args.cmd_input);
	if (err) {
		xnvmec_perror("xnvmec_buf_from_file()");
		xnvmec_pinfo("Error reading: '%s'", cli->args.cmd_input);
		goto exit;
	}

	if (data_nbytes) {
		data_buf = xnvme_buf_alloc(dev, data_nbytes, NULL);
		if (!data_buf) {
			xnvmec_perror("xnvme_buf_alloc()");
			goto exit;
		}

		if (cli->args.data_input) {
			xnvmec_pinfo("Reading data(%s)", cli->args.data_input);
			xnvmec_buf_from_file(data_buf, data_nbytes,
					     cli->args.data_input);
		}
	}

	if (meta_nbytes) {
		meta_buf = xnvme_buf_alloc(dev, meta_nbytes, NULL);
		if (!meta_buf) {
			xnvmec_perror("xnvme_buf_alloc()");
			goto exit;
		}

		if (cli->args.meta_input) {
			xnvmec_pinfo("Reading meta(%s)", cli->args.meta_input);
			xnvmec_buf_from_file(meta_buf, meta_nbytes,
					     cli->args.meta_input);
		}
	}

	if (cli->args.verbose) {
		xnvme_spec_cmd_pr(&cmd, XNVME_PR_DEF);
	}

	if (admin) {
		err = xnvme_cmd_pass_admin(dev, &cmd, data_buf, data_nbytes,
					   meta_buf, meta_nbytes, opts, &req);
	} else {
		err = xnvme_cmd_pass(dev, &cmd, data_buf, data_nbytes, meta_buf,
				     meta_nbytes, opts, &req);
	}
	if (err) {
		xnvmec_perror("xnvme_cmd_pass[_admin]()");
		xnvme_req_pr(&req, XNVME_PR_DEF);
		goto exit;
	}

	if (data_nbytes && cli->args.data_output) {
		xnvmec_pinfo("Dumping data(%s)", cli->args.data_output);
		xnvmec_buf_to_file(data_buf, data_nbytes,
				   cli->args.data_output);
	}

	if (meta_nbytes && cli->args.meta_output) {
		xnvmec_pinfo("Dumping meta(%s)", cli->args.meta_output);
		xnvmec_buf_to_file(meta_buf, meta_nbytes,
				   cli->args.meta_output);
	}

exit:
	xnvme_buf_free(dev, data_buf);
	xnvme_buf_free(dev, meta_buf);

	return errno;
}

static int
sub_pioc(struct xnvmec *cli)
{
	return sub_pass(cli, XNVME_CMD_SYNC, 0);
}

static int
sub_padc(struct xnvmec *cli)
{
	return sub_pass(cli, 0x0, 1);
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvmec_sub subs[] = {
	{
		"enum", "Enumerate devices on the system",
		"Enumerate devices on the system", sub_enumerate, {
			{ 0 }
		}
	},
	{
		"info", "Retrieve derived information for given device",
		"Retrieve derived information for given device", sub_info, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
		}
	},
	{
		"idfy", "Execute an User-defined Identify Command",
		"Execute an User-defined Identify Command", sub_idfy, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_CNS, XNVMEC_LREQ},
			{XNVMEC_OPT_CNTID, XNVMEC_LOPT},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_SETID, XNVMEC_LOPT},
			{XNVMEC_OPT_UUID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"idfy-ns", "Identify the given Namespace",
		"Identify the given Namespace", sub_idfy_ns, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"idfy-ctrlr", "Identify the given Controller",
		"Identify the given Controller", sub_idfy_ctrlr, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"log", "Retrieve a User-defined Log",
		"Retrieve and print log", sub_log, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_LID, XNVMEC_LREQ},
			{XNVMEC_OPT_LSP, XNVMEC_LOPT},
			{XNVMEC_OPT_LPO_NBYTES, XNVMEC_LOPT},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_RAE, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_NBYTES, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"log-erri", "Retrieve the error-information log",
		"Retrieve and print log", sub_log_erri, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_LIMIT, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"log-health", "Retrieve the S.M.A.R.T. / Health information log",
		"Retrieve and print log", sub_log_health, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"feature-get", "Execute a Get-Features Command",
		"Execute a Get Features Command", sub_gfeat, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_FID, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_SEL, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"feature-set", "Execute a Set-Features Command",
		"Execute a Set-Features Command", sub_sfeat, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_FID, XNVMEC_LREQ},
			{XNVMEC_OPT_FEAT, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_SAVE, XNVMEC_LFLG},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
		}
	},
	{
		"format", "Format a NVM namespace",
		"Format a NVM namespace", sub_format, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
		}
	},
	{
		"sanitize", "Sanitize...", "Sanitize...", sub_sanitize, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
		}
	},
	{
		"pioc", "Pass a used-defined IO Command through",
		"Pass a used-defined IO Command through", sub_pioc, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_CMD_INPUT, XNVMEC_LREQ},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_NBYTES, XNVMEC_LOPT},
			{XNVMEC_OPT_META_INPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_META_OUTPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_META_NBYTES, XNVMEC_LOPT},
		}
	},
	{
		"padc", "Pass a user-defined ADmin Command through",
		"Pass a user-defined ADmin Command through", sub_padc, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_CMD_INPUT, XNVMEC_LREQ},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_NBYTES, XNVMEC_LOPT},
			{XNVMEC_OPT_META_INPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_META_OUTPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_META_NBYTES, XNVMEC_LOPT},
		}
	},
};

static struct xnvmec cli = {
	.title = "xNVMe - Cross-platform NVMe utility",
	.descr_short = "Construct and execute NVMe Commands",
	.descr_long = "",
	.subs = subs,
	.nsubs = sizeof(subs) / sizeof(subs[0]),
};

int
main(int argc, char **argv)
{
	return xnvmec(&cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
