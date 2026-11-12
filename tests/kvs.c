// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

static int
kvs_io(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = {0};
	uint8_t nsid = cli->args.dev_nsid;
	const void *kv_key;
	const void *kv_val;
	void *dbuf = NULL;
	void *rbuf = NULL;
	uint8_t kv_key_nbytes = 0;
	size_t kv_val_nbytes = 0;
	size_t diff = 0;
	int err;

	xnvme_dev_get_cmd_ctx(dev, &ctx);

	if (cli->given[XNVME_CLI_OPT_KV_KEY]) {
		kv_key = cli->args.kv_key;
	} else {
		kv_key = "marco";
	}

	if (cli->given[XNVME_CLI_OPT_KV_VAL]) {
		kv_val = cli->args.kv_val;
	} else {
		kv_val = "polo";
	}

	kv_key_nbytes = strlen(kv_key);
	kv_val_nbytes = strlen(kv_val) + 1;

	if (!cli->given[XNVME_CLI_OPT_DEV_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvme_cli_pinf("Alloc/fill dbuf, kv_val_nbytes: %zu", kv_val_nbytes);

	dbuf = xnvme_buf_alloc(dev, kv_val_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_buf_memcpy(dbuf, kv_val, kv_val_nbytes);
	if (err) {
		xnvme_cli_perr("xnvme_buf_memcpy()", err);
		goto exit;
	}

	struct xnvme_cmd_prep_kv_store_args st = {
		.args_nbytes = sizeof(st),
		.nsid = nsid,
		.vs = (uint32_t)kv_val_nbytes,
		.kl = (uint8_t)kv_key_nbytes,
	};

	memcpy(st.kv_key, kv_key, XNVME_MIN(kv_key_nbytes, sizeof(st.kv_key)));
	err = xnvme_cmd_prep_kv_store(&ctx.cmd, &st);
	if (!err) {
		err = xnvme_cmd_pass(&ctx, dbuf, kv_val_nbytes, NULL, 0);
	}
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("store", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_cli_pinf("KV Value is written under a key: \"%s\"\n", kv_key);

	rbuf = xnvme_buf_alloc(dev, kv_val_nbytes);
	if (!rbuf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_buf_clear(rbuf, kv_val_nbytes);
	if (err) {
		xnvme_cli_perr("xnvme_buf_clear()", err);
		goto exit;
	}

	struct xnvme_cmd_prep_kv_retrieve_args rt = {
		.args_nbytes = sizeof(rt),
		.nsid = nsid,
		.hbs = (uint32_t)kv_val_nbytes,
		.kl = (uint8_t)kv_key_nbytes,
	};

	memcpy(rt.kv_key, kv_key, XNVME_MIN(kv_key_nbytes, sizeof(rt.kv_key)));
	xnvme_cli_pinf("Sending the Retrieve command");
	err = xnvme_cmd_prep_kv_retrieve(&ctx.cmd, &rt);
	if (!err) {
		err = xnvme_cmd_pass(&ctx, rbuf, kv_val_nbytes, NULL, 0);
	}
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("retrieve", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_cli_pinf("KV Value retrieved: '%s'", rbuf);

	xnvme_cli_pinf("Comparing wbuf and rbuf");
	err = xnvme_buf_diff(dbuf, rbuf, kv_val_nbytes, &diff);
	if (err) {
		xnvme_cli_perr("xnvme_buf_diff()", err);
		goto exit;
	}
	if (diff) {
		err = xnvme_buf_diff_pr(dbuf, rbuf, kv_val_nbytes, XNVME_PR_DEF);
		if (err) {
			xnvme_cli_perr("xnvme_buf_diff_pr()", err);
			goto exit;
		}
		err = -EIO;
		goto exit;
	}
exit:
	xnvme_buf_free(dev, dbuf);
	xnvme_buf_free(dev, rbuf);

	return err;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{"kvs_io",
	 "Basic Verification of being able to read, what was written",
	 "Basic Verification of being able to read, what was written",
	 kvs_io,
	 {
		 {XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
		 {XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
		 {XNVME_CLI_OPT_KV_KEY, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_KV_VAL, XNVME_CLI_LOPT},
		 XNVME_CLI_SYNC_OPTS,
	 }},
};

static struct xnvme_cli g_cli = {
	.title = "Basic KV Verification",
	.descr_short = "Basic KV Verification",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
