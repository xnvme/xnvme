#include <stdlib.h>
#include <string.h>
#include <libznd.h>
#include <libxnvmec.h>

static int
cmd_enumerate(struct xnvmec *XNVME_UNUSED(cli))
{
	struct xnvme_enumeration *listing = NULL;

	// TODO: Have this enumeration only show namespaces of zones namespace
	// type

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
cmd_info(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;

	xnvme_dev_pr(dev, XNVME_PR_DEF);

	return 0;
}

static inline int
cmd_idfy_ns(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint32_t nsid = xnvme_dev_get_nsid(cli->args.dev);
	enum xnvme_spec_nstype nstype = XNVME_SPEC_NSTYPE_ZONED;
	struct znd_idfy *idfy = NULL;
	struct xnvme_req req = { 0 };
	int err;

	xnvmec_pinfo("xnvme_cmd_idfy_ns: {nsid: 0x%x, nstype: %s}", nsid,
		     xnvme_spec_nstype_str(nstype));

	idfy = xnvme_buf_alloc(dev, sizeof(*idfy), NULL);
	if (!idfy) {
		xnvmec_perror("xnvme_buf_alloc()");
		return -1;
	}

	err = xnvme_cmd_idfy_ns(dev, nsid, XNVME_SPEC_NSTYPE_ZONED, &idfy->base,
				&req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_perror("xnvme_cmd_idfy_ns()");
		xnvme_req_pr(&req, XNVME_PR_DEF);
		xnvme_buf_free(dev, idfy);
		return -1;
	}

	znd_idfy_ns_pr(&idfy->zns, XNVME_PR_DEF);

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
cmd_report(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint64_t zslba = cli->args.slba;
	uint64_t limit = cli->args.limit;

	struct znd_report *report = NULL;

	xnvmec_pinfo("Zone Information Report for lba: 0x%016lx, limit: %zu",
		     zslba, limit ? limit : cli->args.geo->nzone);

	report = znd_report_from_dev(dev, zslba, limit);
	if (!report) {
		xnvmec_perror("znd_report_from_dev()");
		return -1;
	}
	znd_report_pr(report, XNVME_PR_DEF);

	if (cli->args.data_output) {
		xnvmec_pinfo("dumping to: '%s'", cli->args.data_output);
		xnvmec_pinfo("NOTE: log-header is omitted, only entries");
		if (xnvmec_buf_to_file((char *) report->storage,
				       report->report_nbytes,
				       cli->args.data_output)) {
			xnvmec_perror("xnvmec_buf_to_file()");
		}
	}

	xnvmec_pinfo("Completed without errors");

	xnvme_buf_virt_free(report);

	return 0;
}

static int
cmd_changes(struct xnvmec *cli)
{
	struct znd_changes *changes = NULL;

	xnvmec_pinfo("Retrieving the Changed Zone List");

	changes = znd_changes_from_dev(cli->args.dev);
	if (!changes) {
		xnvmec_perror("znd_changes_from_dev()");
		return -1;
	}

	znd_changes_pr(changes, XNVME_PR_DEF);

	xnvmec_pinfo("Completed without errors");

	xnvme_buf_free(cli->args.dev, changes);

	return 0;
}

static int
cmd_errors(struct xnvmec *cli)
{
	struct xnvme_spec_log_erri_entry *log = NULL;
	uint32_t nsid = cli->args.nsid;
	uint32_t log_nentries = 0;
	uint32_t log_nbytes = 0;

	struct xnvme_req req = { 0 };
	size_t nvalid = 0;
	int err = 0;

	log_nentries = (uint32_t) xnvme_dev_get_ctrlr(cli->args.dev)->elpe + 1;
	log_nbytes = log_nentries * sizeof(*log);

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	log = xnvme_buf_alloc(cli->args.dev, log_nbytes, NULL);
	if (!log) {
		xnvmec_perror("xnvme_buf_alloc()");
		return 1;
	}

	err = xnvme_cmd_log(cli->args.dev, XNVME_SPEC_LOG_ERRI, nsid, 0, 0, 0,
			    log, log_nbytes, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_perror("xnvme_cmd_log(XNVME_SPEC_LOG_ERRI)");
		xnvme_req_pr(&req, XNVME_PR_DEF);
		xnvme_buf_free(cli->args.dev, log);
		return 1;
	}

	for (size_t i = 0; i < log_nentries; ++i) {
		if (!log[i].ecnt) {
			break;
		}
		nvalid++;
	}

	xnvmec_pinfo("Error-Information-Log has %zu valid entries", nvalid);
	xnvme_spec_log_erri_pr(log, log_nentries, XNVME_PR_DEF);

	xnvmec_pinfo("Completed without errors");

	xnvme_buf_free(cli->args.dev, log);

	return 0;
}

static int
cmd_read(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const uint64_t slba = cli->args.slba;
	const uint64_t nlb = cli->args.nlb;
	uint32_t nsid = cli->args.nsid;

	size_t dbuf_nbytes = cli->args.geo->nbytes * (nlb + 1);
	char *dbuf = NULL;

	struct xnvme_req req = { 0 };
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinfo("Read nlb: %zu starting at slba: 0x%016x",
		     nlb, slba);

	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes, NULL);
	if (!dbuf) {
		xnvmec_perror("xnvme_buf_alloc()");
		return -1;
	}
	xnvmec_buf_fill(dbuf, dbuf_nbytes);

	err = xnvme_cmd_read(dev, nsid, slba, nlb, dbuf, NULL,
			     XNVME_CMD_SYNC, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_perror("xnvme_cmd_read()");
		xnvme_req_pr(&req, XNVME_PR_DEF);
		xnvme_buf_free(dev, dbuf);
		return -1;
	}

	if (cli->args.data_output) {
		xnvmec_pinfo("dumping to: '%s'", cli->args.data_output);
		if (xnvmec_buf_to_file(dbuf, dbuf_nbytes,
				       cli->args.data_output)) {
			xnvmec_perror("xnvmec_buf_to_file()");
		}
	}

	xnvmec_pinfo("Completed without errors");

	xnvme_buf_free(dev, dbuf);

	return 0;
}

static int
cmd_write(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint32_t nsid = cli->args.nsid;
	const uint64_t zslba = cli->args.slba;
	uint16_t nlb = cli->args.nlb;

	size_t dbuf_nbytes = cli->args.geo->nbytes * (nlb + 1);
	char *dbuf = NULL;

	struct xnvme_req req = { 0 };
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinfo("Write zslba: 0x%016x with nlb: %zu",
		     zslba, nlb);

	xnvmec_pinfo("Allocating dbuf");
	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes, NULL);
	if (!dbuf) {
		xnvmec_perror("xnvme_buf_alloc()");
		return -1;
	}

	if (cli->args.data_input) {
		xnvmec_pinfo("Filling dbuf with: %s", cli->args.data_input);
		if (xnvmec_buf_from_file(dbuf, dbuf_nbytes,
					 cli->args.data_input)) {
			xnvmec_perror("xnvmec_buf_from_file()");
			xnvme_buf_free(dev, dbuf);
			return -1;
		}
	} else {
		xnvmec_pinfo("Filling dbuf with random data");
		xnvmec_buf_fill(dbuf, dbuf_nbytes);
	}

	err = xnvme_cmd_write(dev, nsid, zslba, nlb, dbuf, NULL,
			      XNVME_CMD_SYNC, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_perror("xnvme_cmd_write()");
		xnvme_req_pr(&req, XNVME_PR_DEF);
		xnvme_buf_free(dev, dbuf);
		return -1;
	}
	xnvmec_pinfo("Completed without errors");

	xnvme_buf_free(dev, dbuf);

	return 0;
}

static int
cmd_append(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint64_t nsid = cli->args.nsid;
	const uint64_t zslba = cli->args.slba;
	uint16_t nlb = cli->args.nlb;

	size_t dbuf_nbytes = cli->args.geo->nbytes * (nlb + 1);
	char *dbuf = NULL;

	struct xnvme_req req = { 0 };
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinfo("Zone Append nlb: %zu to zslba: 0x%016x",
		     nlb, zslba);

	xnvmec_pinfo("Allocating dbuf");
	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes, NULL);
	if (!dbuf) {
		xnvmec_perror("xnvme_buf_alloc()");
		return -1;
	}

	if (cli->args.data_input) {
		xnvmec_pinfo("Filling dbuf with: %s", cli->args.data_input);
		if (xnvmec_buf_from_file(dbuf, dbuf_nbytes,
					 cli->args.data_input)) {
			xnvmec_perror("xnvmec_buf_from_file()");
			xnvme_buf_free(dev, dbuf);
			return -1;
		}
	} else {
		xnvmec_pinfo("Filling dbuf with random data");
		xnvmec_buf_fill(dbuf, dbuf_nbytes);
	}

	err = znd_cmd_append(dev, nsid, zslba, nlb, dbuf, NULL,
			     XNVME_CMD_SYNC, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_perror("znd_cmd_append()");
		xnvme_req_pr(&req, XNVME_PR_DEF);
		xnvme_buf_free(cli->args.dev, dbuf);
		return -1;
	}

	xnvmec_pinfo("Completed without errors");
	xnvmec_pinfo("Appended to slba: 0x%016x", req.cpl.cdw0);

	xnvme_buf_free(dev, dbuf);

	return 0;
}

static int
_cmd_mgmt(struct xnvmec *cli, uint8_t action)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint32_t nsid = cli->args.nsid;
	const uint64_t zslba = cli->args.slba;
	int asf = 0;

	//size_t dbuf_nbytes = xnvme_dev_get_ns(dev)->zds;
	size_t dbuf_nbytes = 0;
	char *dbuf = NULL;

	struct xnvme_req req = { 0 };
	int err;

	if (cli->given[XNVMEC_OPT_ALL]) {
		asf = ZND_SEND_SF_SALL;
	}

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinfo("MGMT: zslba: 0x%016x, action: 0x%x, str: %s", zslba,
		     action, znd_send_action_str(action));

	if ((action == ZND_SEND_DESCRIPTOR) && cli->args.data_input) {
		xnvmec_pinfo("Allocating dbuf");
		dbuf = xnvme_buf_alloc(dev, dbuf_nbytes, NULL);
		if (!dbuf) {
			xnvmec_perror("xnvme_buf_alloc()");
			return -1;
		}
		if (cli->args.data_input) {
			xnvmec_pinfo("Filling dbuf with: %s", cli->args.data_input);
			if (xnvmec_buf_from_file(dbuf, dbuf_nbytes, cli->args.data_input)) {
				xnvmec_perror("xnvmec_buf_from_file()");
				xnvme_buf_free(dev, dbuf);
				return -1;
			}
		} else {
			xnvmec_pinfo("Filling dbuf with random data");
			xnvmec_buf_fill(dbuf, dbuf_nbytes);
		}
	}

	err = znd_cmd_mgmt_send(dev, nsid, zslba, action, asf, dbuf,
				XNVME_CMD_SYNC, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_perror("znd_cmd_mgmt_send()");
		xnvme_req_pr(&req, XNVME_PR_DEF);
		return -1;
	}

	xnvmec_pinfo("Completed without errors");

	return 0;
}

static int
cmd_mgmt(struct xnvmec *cli)
{
	return _cmd_mgmt(cli, cli->args.action);
}

static int
cmd_mgmt_open(struct xnvmec *cli)
{
	return _cmd_mgmt(cli, ZND_SEND_OPEN);
}

static int
cmd_mgmt_finish(struct xnvmec *cli)
{
	return _cmd_mgmt(cli, ZND_SEND_FINISH);
}

static int
cmd_mgmt_close(struct xnvmec *cli)
{
	return _cmd_mgmt(cli, ZND_SEND_CLOSE);
}

static int
cmd_mgmt_reset(struct xnvmec *cli)
{
	return _cmd_mgmt(cli, ZND_SEND_RESET);
}

//
// Command-Line Interface (CLI) definition
//

static struct xnvmec_sub subs[] = {
	{
		"enum", "Enumerate Zoned Namespaces on the system",
		"Enumerate Zoned Namespaces on the system", cmd_enumerate, {
			{ 0 }
		}
	},
	{
		"info", "Retrieve device info",
		"Retrieve device info", cmd_info, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
		}
	},
	{
		"idfy-ns", "Identify the associated namespace",
		"Identify the associated namespace", cmd_idfy_ns, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
		}
	},
	{
		"report", "Retrieve Zone Information",
		"Retrieve Zone Information", cmd_report, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LOPT},
			{XNVMEC_OPT_LIMIT, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"changes", "Retrieve the Changed Zone list",
		"Retrieve the Changed Zone list", cmd_changes, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"errors", "Retrieve the Error-Log",
		"Retrieve the Error-Log", cmd_errors, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"read", "Execute a Read Command",
		"Execute a Read Command", cmd_read, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NLB, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_META_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"write", "Execute a Write Command",
		"Execute a Write Command", cmd_write, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NLB, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_META_INPUT, XNVMEC_LOPT},
		}
	},
	{
		"append", "Execute an Append Command",
		"Execute an Append Command", cmd_append, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NLB, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_META_INPUT, XNVMEC_LOPT},
		}
	},
	{
		"mgmt-open", "Open a Zone",
		"Open a Zone", cmd_mgmt_open, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_ALL, XNVMEC_LFLG},
		}
	},
	{
		"mgmt-close", "Close a Zone",
		"Close a Zone", cmd_mgmt_close, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_ALL, XNVMEC_LFLG},
		}
	},
	{
		"mgmt-finish", "Finish a Zone",
		"Finish a Zone", cmd_mgmt_finish, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_ALL, XNVMEC_LFLG},
		}
	},
	{
		"mgmt-reset", "Reset a Zone",
		"Reset a Zone", cmd_mgmt_reset, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_ALL, XNVMEC_LFLG},
		}
	},
	{
		"mgmt", "Zone Management Send Command with custom action",
		"Zone Management Send Command with custom action",
		cmd_mgmt, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_ACTION, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_ALL, XNVMEC_LFLG},
		}
	},
};

static struct xnvmec cli = {
	.title = "Zoned Namespace Utility",
	.descr_short = "Enumerate Zoned Namespaces, manage, inspect properties, state, and send IO to them",
	.subs = subs,
	.nsubs = sizeof(subs) / sizeof(subs[0]),
};

int
main(int argc, char **argv)
{
	return xnvmec(&cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
