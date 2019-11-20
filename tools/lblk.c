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

	// TODO: only show namespaces of logical block type

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
sub_idfy(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_spec_idfy *idfy = NULL;
	uint32_t nsid = xnvme_dev_get_nsid(cli->args.dev);
	struct xnvme_req req = { 0 };
	int err;

	xnvmec_pinfo("xnvme_cmd_idfy: {nsid: 0x%x}", nsid);

	idfy = xnvme_buf_alloc(dev, sizeof(*idfy), NULL);
	if (!idfy) {
		xnvmec_perror("xnvme_buf_alloc()");
		return -1;
	}

	err = xnvme_cmd_idfy_ns(dev, nsid, XNVME_SPEC_NSTYPE_LBLK, idfy, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_perror("xnvme_cmd_idfy()");
		xnvme_req_pr(&req, XNVME_PR_DEF);
		xnvme_buf_free(dev, idfy);
		return -1;
	}

	xnvme_spec_idfy_ns_pr(&idfy->ns, XNVME_PR_DEF);

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
sub_read(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	const uint64_t slba = cli->args.slba;
	const size_t nlb = cli->args.nlb;
	uint8_t nsid = cli->args.nsid;

	void *dbuf = NULL;
	void *mbuf = NULL;
	size_t dbuf_nbytes = (nlb + 1) * geo->nbytes;
	size_t mbuf_nbytes = (nlb + 1) * geo->nbytes_oob;

	struct xnvme_req req = { 0 };
	int err = 0;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinfo("Reading nsid: 0x%x, slba: 0x%016x, nlb: %zu",
		     nsid, slba, nlb);

	xnvmec_pinfo("Allocating dbuf_nbytes: %zu", dbuf_nbytes);
	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes, NULL);
	if (!dbuf) {
		xnvmec_perror("xnvme_buf_alloc()");
		return -1;
	}
	xnvmec_pinfo("Clearing dbuf");
	memset(dbuf, 0, dbuf_nbytes);

	if (mbuf_nbytes) {
		xnvmec_pinfo("Allocating mbuf_nbytes: %zu", mbuf_nbytes);
		mbuf = xnvme_buf_alloc(dev, mbuf_nbytes, NULL);
		if (!mbuf) {
			xnvmec_perror("xnvme_buf_alloc()");
			xnvme_buf_free(dev, dbuf);
			return -1;
		}
		xnvmec_pinfo("Clearing mbuf");
		memset(mbuf, 0, mbuf_nbytes);
	}

	xnvmec_pinfo("Sending the command...");
	err = xnvme_cmd_read(dev, nsid, slba, nlb, dbuf, mbuf, 0x0, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_perror("xnvme_cmd_read()");
		xnvme_req_pr(&req, XNVME_PR_DEF);
		goto exit;
	}
	xnvmec_pinfo("Completed without errors");

	if (cli->args.data_output) {
		xnvmec_pinfo("dumping to: '%s'", cli->args.data_output);
		if (xnvmec_buf_to_file(dbuf, dbuf_nbytes,
				       cli->args.data_output)) {
			xnvmec_perror("xnvmec_buf_to_file()");
		}
	}

exit:
	xnvme_buf_free(dev, dbuf);
	xnvme_buf_free(dev, mbuf);

	return err;
}

static int
sub_write(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	const uint64_t slba = cli->args.slba;
	const size_t nlb = cli->args.nlb;
	uint32_t nsid = cli->args.nsid;

	int dbuf_nbytes = (nlb + 1) * geo->nbytes;
	char *dbuf = NULL;
	int mbuf_nbytes = (nlb + 1) * geo->nbytes_oob;
	char *mbuf = NULL;

	struct xnvme_req req = { 0 };
	int err = 0;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinfo("Writing nsid: 0x%x, slba: 0x%016x, nlb: %zu",
		     nsid, slba, nlb);

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

	if (mbuf_nbytes) {
		xnvmec_pinfo("Allocating mbuf");
		mbuf = xnvme_buf_alloc(dev, mbuf_nbytes, NULL);
		if (!mbuf) {
			xnvmec_perror("xnvme_buf_alloc()");
			xnvme_buf_free(dev, dbuf);
			return -1;
		}

		xnvmec_pinfo("Filling mbuf with random data");
		xnvmec_buf_fill(mbuf, mbuf_nbytes);
	}

	xnvmec_pinfo("Sending the command...");
	err = xnvme_cmd_write(dev, nsid, slba, nlb, dbuf, mbuf, 0x0, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_perror("xnvme_cmd_write()");
		xnvme_req_pr(&req, XNVME_PR_DEF);
		goto exit;
	}
	xnvmec_pinfo("Completed without errors");

exit:
	xnvme_buf_free(dev, dbuf);
	xnvme_buf_free(dev, mbuf);

	return err;
}

static int
sub_write_zeroes(struct xnvmec *XNVME_UNUSED(cli))
{
	xnvmec_pinfo("Not implemented");

	return -1;
}

static int
sub_write_uncor(struct xnvmec *XNVME_UNUSED(cli))
{
	xnvmec_pinfo("Not implemented");

	return -1;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvmec_sub subs[] = {
	{
		"enum", "Enumerate Logical Block Namespaces on the system",
		"Enumerate Logical Block Namespaces on the system", sub_enumerate, {
			{ 0 }
		}
	},
	{
		"info", "Retrieve derived information for the given URI",
		"Retrieve derived information for the given URI", sub_info, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
		}
	},
	{
		"idfy", "Identify the namespace for the given URI",
		"Identify the namespace for the given URI", sub_idfy, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"read", "Read data and optionally metadata",
		"Read data and optionally metadata", sub_read, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NLB, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_META_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"write", "Writes data and optionally metadata",
		"Writes data and optionally metadata", sub_write, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NLB, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_META_INPUT, XNVMEC_LOPT},
		}
	},
	{
		"write-zeros", "Set a range of logical blocks to zero",
		"Set a range of logical blocks to zero", sub_write_zeroes, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NLB, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_META_INPUT, XNVMEC_LOPT},
		}
	},
	{
		"write-uncor", "Mark a range of logical blocks as invalid",
		"Mark a range of logical blocks as invalid", sub_write_uncor, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NLB, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_META_INPUT, XNVMEC_LOPT},
		}
	},
};

static struct xnvmec cli = {
	.title = "Logical Block Namespace Utility",
	.descr_short = "Logical Block Namespace Utility",
	.descr_long = "",
	.subs = subs,
	.nsubs = sizeof(subs) / sizeof(subs[0]),
};

int
main(int argc, char **argv)
{
	return xnvmec(&cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
