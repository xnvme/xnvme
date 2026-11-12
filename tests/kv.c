// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

int
enumerate_cb(struct xnvme_dev *dev, void *cb_args)
{
	uint32_t *ns_count_ref = cb_args;
	const struct xnvme_ident *ident;

	ident = xnvme_dev_get_ident(dev);
	if (ident->csi != XNVME_SPEC_CSI_KV) {
		return XNVME_ENUMERATE_DEV_CLOSE;
	}

	if (*ns_count_ref == 0) {
		fprintf(stdout, "\n");
	}

	fprintf(stdout, "  - {");
	xnvme_ident_yaml(stdout, ident, 0, ", ", 0);
	fprintf(stdout, "}\n");

	*ns_count_ref = *ns_count_ref + 1;

	return XNVME_ENUMERATE_DEV_CLOSE;
}

/**
 * Copy up to 16 bytes of 'key' into 'kv_key', returning its length, the Key Length of the command
 */
static uint8_t
key_of(uint8_t kv_key[16], const char *key)
{
	size_t len = strlen(key);

	memcpy(kv_key, key, XNVME_MIN(len, 16));

	return (uint8_t)len;
}

static int
cmd_enumerate(struct xnvme_cli *cli)
{
	struct xnvme_opts opts = {0};
	uint32_t ns_count = 0;
	int err = 0;

	err = xnvme_cli_to_opts(cli, &opts);
	if (err) {
		xnvme_cli_perr("xnvme_cli_to_opts()", err);
		return err;
	}

	fprintf(stdout, "xnvme_enumeration:");

	err = xnvme_enumerate(cli->args.sys_uri, &opts, *enumerate_cb, &ns_count);
	if (err) {
		xnvme_cli_perr("xnvme_enumerate()", err);
		return err;
	}

	if (ns_count == 0) {
		fprintf(stdout, "~\n");
	}

	return 0;
}

static inline int
cmd_idfy_ns(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = {0};
	uint32_t nsid = xnvme_dev_get_nsid(cli->args.dev);
	enum xnvme_spec_csi csi = XNVME_SPEC_CSI_KV;
	union xnvme_spec_kvs_idfy *idfy = NULL;
	int err;

	xnvme_dev_get_cmd_ctx(dev, &ctx);

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvme_cli_pinf("identify: {nsid: 0x%x, csi: %s}", nsid, xnvme_spec_csi_str(csi));

	idfy = xnvme_buf_alloc(dev, sizeof(*idfy));
	if (!idfy) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	struct xnvme_cmd_prep_adm_identify_args args = {
		.args_nbytes = sizeof(args),
		.nsid = nsid ? nsid : xnvme_dev_get_nsid(dev),
		.cns = XNVME_SPEC_IDFY_NS_IOCS,
		.csi = XNVME_SPEC_CSI_KV,
	};

	err = xnvme_cmd_prep_adm_identify(&ctx.cmd, &args);
	if (!err) {
		err = xnvme_cmd_pass_admin(&ctx, &idfy->base, sizeof(idfy->base), NULL, 0);
	}
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("identify", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_spec_kvs_idfy_ns_pr(&idfy->ns, XNVME_PR_DEF);

	if (cli->args.data_output) {
		xnvme_cli_pinf("Dumping to: '%s'", cli->args.data_output);
		err = xnvme_buf_to_file((char *)idfy, sizeof(*idfy), cli->args.data_output);
		if (err) {
			xnvme_cli_perr("xnvme_buf_to_file()", err);
		}
	}

exit:
	xnvme_buf_free(dev, idfy);

	return err;
}

static int
cmd_retrieve(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = {0};
	uint8_t nsid = cli->args.nsid;
	void *dbuf = NULL;
	size_t dbuf_nbytes = 4096;
	int err;

	xnvme_dev_get_cmd_ctx(dev, &ctx);

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvme_cli_pinf("Alloc/clear dbuf, dbuf_nbytes: %zu", dbuf_nbytes);

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

	struct xnvme_cmd_prep_kv_retrieve_args args = {
		.args_nbytes = sizeof(args),
		.nsid = nsid,
		.hbs = (uint32_t)dbuf_nbytes,
	};

	args.kl = key_of(args.kv_key, cli->args.kv_key);

	xnvme_cli_pinf("Sending the Retrieve command");
	err = xnvme_cmd_prep_kv_retrieve(&ctx.cmd, &args);
	if (!err) {
		err = xnvme_cmd_pass(&ctx, dbuf, dbuf_nbytes, NULL, 0);
	}
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("retrieve", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	if (cli->args.data_output) {
		xnvme_cli_pinf("dumping to: '%s'", cli->args.data_output);
		err = xnvme_buf_to_file(dbuf, dbuf_nbytes, cli->args.data_output);
		if (err) {
			xnvme_cli_perr("xnvme_buf_to_file()", err);
		}
	} else {
		xnvme_cli_pinf("KV Value retrieved: '%s'", dbuf);
	}

exit:
	xnvme_buf_free(dev, dbuf);

	return err;
}

static int
cmd_store(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = {0};
	uint8_t nsid = cli->args.nsid;
	uint8_t opt = 0x0;
	void *dbuf = NULL;
	size_t dbuf_nbytes = strlen(cli->args.kv_val) + 1;
	int err;

	xnvme_dev_get_cmd_ctx(dev, &ctx);

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}
	xnvme_cli_pinf("Alloc/fill dbuf, dbuf_nbytes: %zu", dbuf_nbytes);

	if (cli->args.kv_store_add) {
		opt = opt | XNVME_KVS_STORE_OPT_SINKE;
	} else if (cli->args.kv_store_update) {
		opt = opt | XNVME_KVS_STORE_OPT_SIKE;
	}

	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_buf_memcpy(dbuf, cli->args.kv_val, dbuf_nbytes);
	if (err) {
		xnvme_cli_perr("xnvme_buf_memcpy()", err);
		goto exit;
	}

	struct xnvme_cmd_prep_kv_store_args args = {
		.args_nbytes = sizeof(args),
		.nsid = nsid,
		.vs = (uint32_t)dbuf_nbytes,
		.so = opt,
	};

	args.kl = key_of(args.kv_key, cli->args.kv_key);

	err = xnvme_cmd_prep_kv_store(&ctx.cmd, &args);
	if (!err) {
		err = xnvme_cmd_pass(&ctx, dbuf, dbuf_nbytes, NULL, 0);
	}
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("store", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_cli_pinf("KV Value is written under a key: \"%s\"\n", cli->args.kv_key);
exit:
	xnvme_buf_free(dev, dbuf);

	return err;
}

static int
cmd_delete(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = {0};
	uint8_t nsid = cli->args.nsid;
	int err;

	xnvme_dev_get_cmd_ctx(dev, &ctx);

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	struct xnvme_cmd_prep_kv_delete_args args = {
		.args_nbytes = sizeof(args),
		.nsid = nsid,
	};

	args.kl = key_of(args.kv_key, cli->args.kv_key);

	xnvme_cli_pinf("Sending the Delete command");
	err = xnvme_cmd_prep_kv_delete(&ctx.cmd, &args);
	if (!err) {
		err = xnvme_cmd_pass(&ctx, NULL, 0, NULL, 0);
	}
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("delete", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_cli_pinf("Removed key: '%s'", cli->args.kv_key);

exit:
	return err;
}

static int
cmd_exist(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = {0};
	uint8_t nsid = cli->args.nsid;
	int err;

	xnvme_dev_get_cmd_ctx(dev, &ctx);

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	struct xnvme_cmd_prep_kv_exist_args args = {
		.args_nbytes = sizeof(args),
		.nsid = nsid,
	};

	args.kl = key_of(args.kv_key, cli->args.kv_key);

	xnvme_cli_pinf("Sending the Exist command");
	err = xnvme_cmd_prep_kv_exist(&ctx.cmd, &args);
	if (!err) {
		err = xnvme_cmd_pass(&ctx, NULL, 0, NULL, 0);
	}
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("exist", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_cli_pinf("Key '%s' exists", cli->args.kv_key);

exit:
	return err;
}

static int
cmd_list(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = {0};
	uint8_t nsid = cli->args.nsid;
	void *dbuf = NULL;
	size_t stdio_wrtn, dbuf_nbytes = 4096;
	int err;

	xnvme_dev_get_cmd_ctx(dev, &ctx);

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	XNVME_DEBUG("Alloc/clear dbuf, dbuf_nbytes: %zu", dbuf_nbytes);

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

	struct xnvme_cmd_prep_kv_list_args args = {
		.args_nbytes = sizeof(args),
		.nsid = nsid,
		.hbs = (uint32_t)dbuf_nbytes,
	};

	args.kl = key_of(args.kv_key, cli->args.kv_key);

	XNVME_DEBUG("Sending the List command");
	err = xnvme_cmd_prep_kv_list(&ctx.cmd, &args);
	if (!err) {
		err = xnvme_cmd_pass(&ctx, dbuf, dbuf_nbytes, NULL, 0);
	}
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("list", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	XNVME_DEBUG("KV List retrieved with %u keys\n", *(uint32_t *)dbuf);

	stdio_wrtn = fwrite(dbuf, sizeof(uint8_t), dbuf_nbytes, stdout);

	if (stdio_wrtn != dbuf_nbytes) {
		err = (int)stdio_wrtn;
		xnvme_cli_perr("fwrite()", err);
		err = err ? err : -EIO;
		goto exit;
	}

	return 0;
exit:
	xnvme_buf_free(dev, dbuf);

	return err;
}

//
// Command-Line Interface (CLI) definition
//

static struct xnvme_cli_sub g_subs[] = {
	{"enum",
	 "Enumerate Logical Block Namespaces on the system",
	 "Enumerate Logical Block Namespaces on the system",
	 cmd_enumerate,
	 {
		 {XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},

		 {XNVME_CLI_OPT_SYS_URI, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_FLAGS, XNVME_CLI_LOPT},

		 {XNVME_CLI_OPT_DEV_NSID, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_BE, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_ADMIN, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_SYNC, XNVME_CLI_LOPT},
	 }},
	{"idfy-ns",
	 "Zoned Command Set specific identify-controller",
	 "KV Command Set specific identify-controller",
	 cmd_idfy_ns,
	 {
		 {XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},

		 {XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
		 {XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},

		 {XNVME_CLI_OPT_DEV_NSID, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_BE, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_ADMIN, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_SYNC, XNVME_CLI_LOPT},
	 }},
	{"retrieve",
	 "Execute a KV Retrieve Command",
	 "Execute a KV Retrieve Command",
	 cmd_retrieve,
	 {
		 {XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},

		 {XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
		 {XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_KV_KEY, XNVME_CLI_LREQ},
		 {XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},

		 {XNVME_CLI_OPT_DEV_NSID, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_BE, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_ADMIN, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_SYNC, XNVME_CLI_LOPT},
	 }},
	{"store",
	 "Execute a KV Store Command",
	 "Execute a KV Store Command",
	 cmd_store,
	 {
		 {XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},

		 {XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
		 {XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_KV_KEY, XNVME_CLI_LREQ},
		 {XNVME_CLI_OPT_KV_VAL, XNVME_CLI_LREQ},

		 {XNVME_CLI_OPT_KV_STORE_UPDATE, XNVME_CLI_LFLG},
		 {XNVME_CLI_OPT_KV_STORE_ADD, XNVME_CLI_LFLG},
		 {XNVME_CLI_OPT_KV_STORE_COMPRESS, XNVME_CLI_LFLG},

		 {XNVME_CLI_OPT_DEV_NSID, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_BE, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_ADMIN, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_SYNC, XNVME_CLI_LOPT},
	 }},
	{"delete",
	 "Execute a KV Delete Command",
	 "Execute a KV Delete Command",
	 cmd_delete,
	 {
		 {XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},

		 {XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
		 {XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_KV_KEY, XNVME_CLI_LREQ},

		 {XNVME_CLI_OPT_DEV_NSID, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_BE, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_ADMIN, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_SYNC, XNVME_CLI_LOPT},
	 }},
	{"exist",
	 "Execute a KV Exist Command",
	 "Execute a KV Exist Command",
	 cmd_exist,
	 {
		 {XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},

		 {XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
		 {XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_KV_KEY, XNVME_CLI_LREQ},

		 {XNVME_CLI_OPT_DEV_NSID, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_BE, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_ADMIN, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_SYNC, XNVME_CLI_LOPT},
	 }},
	{"list",
	 "Execute a KV List Command",
	 "Execute a KV List Command",
	 cmd_list,
	 {
		 {XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},

		 {XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
		 {XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_KV_KEY, XNVME_CLI_LREQ},

		 {XNVME_CLI_OPT_DEV_NSID, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_BE, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_ADMIN, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_SYNC, XNVME_CLI_LOPT},
	 }},
};

static struct xnvme_cli g_cli = {
	.title = "KV Utility",
	.descr_short = "Store, retrieve, delete, check and list key-value pairs",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
