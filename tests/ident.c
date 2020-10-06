// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvmec.h>

#define SUCCESS 0

#define SPDK_NAME "SPDK"
#define FIOC_NAME "FIOC"
#define LIOC_NAME "LIOC"
#define LIOU_NAME "LIOU"

#define ANY ""

// NOTE: thse are internal backend identifiers... should not be exposed...
struct test_input {
	const char *uri;		///< The uri to parse
	struct xnvme_ident	exp;	///< Expected ident
	int ret;			///< Expected return-value
};

#define EXPECTED(_scheme, _trgt, _opts) {\
		.uri	= "",		\
		.schm	= _scheme,	\
		.trgt	= _trgt,	\
		.opts	= _opts,	\
	}

static struct test_input input[] = {
	{
		"pci:1000:20:1d.e",
		EXPECTED("pci", "1000:20:1d.e", ""),
		SUCCESS
	},
	{
		"spdk:1000:20:1d.e",
		EXPECTED("spdk", "1000:20:1d.e", ""),
		SUCCESS
	},
	{
		"pci:1000:20:1d.e?nsid=1",
		EXPECTED("pci", "1000:20:1d.e", "?nsid=1"),
		SUCCESS
	},
	{
		"pcie:1000:20:1d.e?nsid=1",
		EXPECTED("pcie", "1000:20:1d.e", "?nsid=1"),
		SUCCESS
	},
	{
		"spdk:1000:20:1d.e?nsid=1",
		EXPECTED("spdk", "1000:20:1d.e", "?nsid=1"),
		SUCCESS
	},
	{
		"spdk:1000:20:1d.e?nsid=8",
		EXPECTED("spdk", "1000:20:1d.e", "?nsid=8"),
		SUCCESS
	},
	{
		":1000:20:1d.e",
		EXPECTED(ANY, ANY, ANY),
		-EINVAL
	},
	{
		"://20:1d.e",
		EXPECTED(ANY, ANY, ANY),
		-EINVAL
	},

	{
		"fioc:/dev/nvme0",
		EXPECTED("fioc", "/dev/nvme0", ""),
		SUCCESS
	},
	{
		"fioc:/dev/nvme2",
		EXPECTED("fioc", "/dev/nvme2", ""),
		SUCCESS
	},
	{
		"fioc:/dev/nvme0ns1",
		EXPECTED("fioc", "/dev/nvme0ns1", ""),
		SUCCESS
	},
	{
		"fioc:/dev/nvme2ns3",
		EXPECTED("fioc", "/dev/nvme2ns3", ""),
		SUCCESS
	},
	{
		"/dev/nvme0ns1",
		EXPECTED("file", "/dev/nvme0ns1", ""),
		SUCCESS
	},
	{
		"fioc:///dev/nvme0n1",
		EXPECTED(ANY, ANY, ANY),
		SUCCESS
	},
	{
		"fioc:///dev/nvme2n3",
		EXPECTED(ANY, ANY, ANY),
		SUCCESS
	},

	{
		"liou:/dev/nvme0",
		EXPECTED("liou", "/dev/nvme0", ""),
		SUCCESS
	},
	{
		"liou:/dev/nvme2",
		EXPECTED("liou", "/dev/nvme2", ""),
		SUCCESS
	},
	{
		"liou:/dev/nvme3n4",
		EXPECTED("liou", "/dev/nvme3n4", ""),
		SUCCESS
	},
	{
		"liou:/dev/nvme0n1",
		EXPECTED("liou", "/dev/nvme0n1", ""),
		SUCCESS
	},

	{
		"lioc:/dev/nvme0",
		EXPECTED("lioc", "/dev/nvme0", ""),
		SUCCESS
	},
	{
		"lioc:/dev/nvme2",
		EXPECTED("lioc", "/dev/nvme2", ""),
		SUCCESS
	},
	{
		"lioc:/dev/nvme3n4",
		EXPECTED("lioc", "/dev/nvme3n4", ""),
		SUCCESS
	},
	{
		"lioc:/dev/nvme0n1",
		EXPECTED("lioc", "/dev/nvme0n1", ""),
		SUCCESS
	},

	// Should be one of: fioc, lioc, liou
	{
		"/dev/nvme2",
		EXPECTED(ANY, "/dev/nvme0n1", ""),
		SUCCESS
	},

	// Should be one of: lioc, liou
	{
		"/dev/nvme0n1",
		EXPECTED(ANY, "/dev/nvme0n1", ""),
		SUCCESS
	},
};
static int ninput = sizeof input / sizeof(*input);

static int
test_ident_from_uri(struct xnvmec *XNVME_UNUSED(cli))
{
	int nerr = 0;
	struct xnvme_ident ident = { 0 };

	for (int i = 0; i < ninput; ++i) {
		struct test_input *test = &input[i];
		int failed = 0;
		int err;

		memset(&ident, 0, sizeof(ident));

		xnvmec_pinf("TEST [%d/%d] ENTER uri: '%s'", i + 1, ninput,
			    test->uri);

		err = xnvme_ident_from_uri(test->uri, &ident);
		if (err != test->ret) {
			failed = 1;
			XNVME_DEBUG("err: %d != test->ret: %d", err, test->ret);
		}

		if (!err) {
			if (strncmp(ident.uri, test->exp.uri,
				    strnlen(test->exp.uri,
					    XNVME_IDENT_URI_LEN))) {
				failed = 1;
				XNVME_DEBUG("uri: '%s' != exp.uri: '%s'",
					    ident.uri, test->exp.uri);
			}
			if (strlen(test->exp.schm) && \
			    (strncmp(ident.schm, test->exp.schm,
				     XNVME_IDENT_SCHM_LEN))) {
				failed = 1;
				XNVME_DEBUG("schm: '%s' != exp.schm: '%s'",
					    ident.schm, test->exp.schm);
			}
		}

		if (failed) {
			++nerr;
			XNVME_DEBUG_FCALL(xnvme_ident_pr(&ident, XNVME_PR_DEF));
			XNVME_DEBUG("err: %d, exp.err: %d", err, test->ret);
		}

		xnvmec_pinf("TEST [%d/%d] %s uri: %s", i + 1, ninput,
			    failed ? "FAILED" : "PASSED", test->uri);
	}

	xnvmec_pinf("nerr: %d", nerr);

	return nerr ? 1 : 0;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvmec_sub g_subs[] = {
	{
		"ident_from_uri",
		"Test 'xnvme_ident_from_uri'",
		"Test 'xnvme_ident_from_uri'", test_ident_from_uri, {
			{ 0 },
		}
	},
};

static struct xnvmec g_cli = {
	.title = "Test 'xnvme_ident' and 'xnvme_enumerate' helpers",
	.descr_short = "Test 'xnvme_ident' and 'xnvme_enumerate' helpers",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_NONE);
}
