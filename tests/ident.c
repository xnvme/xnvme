// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvmec.h>

#define OK 0
#define ERR -1

// NOTE: thse are internal backend identifiers... should not be exposed...
#define SPDK_BID 0x59DA
#define LIOC_BID 0xDEAD
#define LIOU_BID 0xBEAF
#define FIOC_BID 0xFB5D
#define ANY_BID 0x0

struct test_input {
	const char *uri;		///< The uri to parse
	struct xnvme_ident	exp;	///< Expected ident
	int ret;			///< Expected return-value
	int err;			///< Expected errno
};

static struct test_input input[] = {
	{
		"pci://1000:20:1d.e", {
			.bid = SPDK_BID,
			.type = XNVME_IDENT_CTRLR
		},	OK,	OK
	},
	{
		"pcie://1000:20:1d.e", {
			.bid = SPDK_BID,
			.type = XNVME_IDENT_CTRLR
		},	OK,	OK
	},
	{
		"spdk://1000:20:1d.e", {
			.bid = SPDK_BID,
			.type = XNVME_IDENT_CTRLR
		},	OK,	OK
	},
	{
		"pci://1000:20:1d.e/1", {
			.bid = SPDK_BID,
			.type = XNVME_IDENT_NS,
			.nsid = 1
		},	OK,	OK
	},
	{
		"pcie://1000:20:1d.e/1", {
			.bid = SPDK_BID,
			.type = XNVME_IDENT_NS,
			.nsid = 1
		},	OK,	OK
	},
	{
		"spdk://1000:20:1d.e/1", {
			.bid = SPDK_BID,
			.type = XNVME_IDENT_NS,
			.nsid = 1
		},	OK,	OK
	},
	{
		"spdk://1000:20:1d.e/8", {
			.bid = SPDK_BID,
			.type = XNVME_IDENT_NS,
			.nsid = 8
		},	OK,	OK
	},
	{
		"://1000:20:1d.e",
		{ 0 },	ERR,	EINVAL
	},
	{
		"://20:1d.e",
		{ 0 },	ERR,	EINVAL
	},

	{
		"fioc:///dev/nvme0", {
			.bid = FIOC_BID,
			.type = XNVME_IDENT_CTRLR,
		},	OK,	OK
	},
	{
		"fioc:///dev/nvme2", {
			.bid = FIOC_BID,
			.type = XNVME_IDENT_CTRLR,
		},	OK,	OK
	},
	{
		"fioc:///dev/nvme0ns1", {
			.bid = FIOC_BID,
			.type = XNVME_IDENT_NS,
			.nsid = 1
		},	OK,	OK
	},
	{
		"fioc:///dev/nvme2ns3", {
			.bid = FIOC_BID,
			.type = XNVME_IDENT_NS,
			.nsid = 3
		},	OK,	OK
	},
	{
		"/dev/nvme0ns1", {
			.bid = FIOC_BID,
			.type = XNVME_IDENT_NS,
			.nsid = 1
		},	OK,	OK
	},
	{
		"fioc:///dev/nvme0n1",
		{ 0 },	ERR,	EINVAL
	},
	{
		"fioc:///dev/nvme2n3",
		{ 0 },	ERR,	EINVAL
	},

	{
		"liou:///dev/nvme0", {
			.bid = LIOU_BID,
			.type = XNVME_IDENT_CTRLR,
		},	OK,	OK
	},
	{
		"liou:///dev/nvme2", {
			.bid = LIOU_BID,
			.type = XNVME_IDENT_CTRLR,
		},	OK,	OK
	},
	{
		"liou:///dev/nvme3n4", {
			.bid = LIOU_BID,
			.type = XNVME_IDENT_NS,
			.nsid = 4
		},	OK,	OK
	},
	{
		"liou:///dev/nvme0n1", {
			.bid = LIOU_BID,
			.type = XNVME_IDENT_NS,
			.nsid = 1
		},	OK,	OK
	},

	{
		"lioc:///dev/nvme0", {
			.bid = LIOC_BID,
			.type = XNVME_IDENT_CTRLR,
		},	OK,	OK
	},
	{
		"lioc:///dev/nvme2", {
			.bid = LIOC_BID,
			.type = XNVME_IDENT_CTRLR,
		},	OK,	OK
	},
	{
		"lioc:///dev/nvme3n4", {
			.bid = LIOC_BID,
			.type = XNVME_IDENT_NS,
			.nsid = 4
		},	OK,	OK
	},
	{
		"lioc:///dev/nvme0n1", {
			.bid = LIOC_BID,
			.type = XNVME_IDENT_NS,
			.nsid = 1
		},	OK,	OK
	},

	// Should be one of: fioc, lioc, liou
	{
		"/dev/nvme2", {
			.bid = ANY_BID,
			.type = XNVME_IDENT_CTRLR,
		},	OK,	OK
	},

	// Should be one of: lioc, liou
	{
		"/dev/nvme0n1", {
			.bid = ANY_BID,
			.type = XNVME_IDENT_NS,
			.nsid = 1
		},	OK,	OK
	},
};
static int ninput = sizeof(input) / sizeof(input[0]);

static int
test_ident_from_uri(struct xnvmec *XNVME_UNUSED(cli))
{
	int nerr = 0;
	struct xnvme_ident ident = { 0 };

	for (int i = 0; i < ninput; ++i) {
		struct test_input *test = &input[i];
		int failed = 0;
		int ret;

		memset(&ident, 0, sizeof(ident));

		xnvmec_pinfo("TEST [%d/%d] ENTER uri: '%s'", i + 1, ninput,
			     test->uri);

		ret = xnvme_ident_from_uri(test->uri, &ident);
		if (ret != test->ret) {
			failed = 1;
			XNVME_DEBUG("ret: %d != test->ret: %d",
				    ret, test->ret);
		}
		if ((ret) && (errno != test->err)) {
			failed = 1;
			XNVME_DEBUG("errno: %d != inp->err: %d",
				    errno, test->err);
		}
		if ((!ret) && test->exp.bid && (ident.bid != test->exp.bid)) {
			failed = 1;
			XNVME_DEBUG("ident.bid: 0x%x != test->exp.bid: 0x%x",
				    ident.bid, test->exp.bid);
		}

		if ((!ret) && (ident.type != test->exp.type)) {
			failed = 1;
			XNVME_DEBUG("ident.type: 0x%x != test->exp.type: 0x%x",
				    ident.type, test->exp.type);
		}
		if ((!ret) && (ident.nsid != test->exp.nsid)) {
			failed = 1;
			XNVME_DEBUG("ident.nsid: 0x%x != test->exp.nsid: 0x%x",
				    ident.nsid, test->exp.nsid);
		}
		if ((!ret) && strncmp(ident.uri, test->exp.uri,
				      strnlen(test->exp.uri, XNVME_IDENT_URI_LEN))) {
			failed = 1;
			XNVME_DEBUG("ident.uri: '%s' != test->exp.uri: '%s'",
				    ident.uri, test->exp.uri);
		}

		if (failed) {
			++nerr;
			XNVME_DEBUG_FCALL(xnvme_ident_pr(&ident, XNVME_PR_DEF));
		}

		xnvmec_pinfo("TEST [%d/%d] %s uri: %s", i + 1, ninput,
			     failed ? "FAILED" : "PASSED", test->uri);
	}

	xnvmec_pinfo("nerr: %d", nerr);

	return nerr ? 1 : 0;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvmec_sub subs[] = {
	{
		"ident_from_uri",
		"Test 'xnvme_ident_from_uri'",
		"Test 'xnvme_ident_from_uri'", test_ident_from_uri, {
			{ 0 },
		}
	},
};

static struct xnvmec cli = {
	.title = "Test 'xnvme_ident' and 'xnvme_enumerate' helpers",
	.descr_short = "Test 'xnvme_ident' and 'xnvme_enumerate' helpers",
	.subs = subs,
	.nsubs = sizeof(subs) / sizeof(subs[0]),
};

int
main(int argc, char **argv)
{
	return xnvmec(&cli, argc, argv, XNVMEC_INIT_NONE);
}
