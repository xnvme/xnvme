// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libxnvme.h>
#include <libxnvme_ident.h>
#include <xnvme_be.h>
#include <libxnvme_dev.h>
#include <libxnvme_spec_pp.h>
#include <libxnvme_adm.h>
#include <libxnvme_nvm.h>
#include <libxnvme_kvs.h>
#include <libxnvmec.h>

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

static int
cmd_enumerate(struct xnvmec *cli)
{
	struct xnvme_opts opts = {0};
	uint32_t ns_count = 0;
	int err = 0;

	err = xnvmec_cli_to_opts(cli, &opts);
	if (err) {
		xnvmec_perr("xnvmec_cli_to_opts()", err);
		return err;
	}

	fprintf(stdout, "xnvme_enumeration:");

	err = xnvme_enumerate(cli->args.sys_uri, &opts, *enumerate_cb, &ns_count);
	if (err) {
		xnvmec_perr("xnvme_enumerate()", err);
		return err;
	}

	if (ns_count == 0) {
		fprintf(stdout, "~\n");
	}

	return 0;
}

static inline int
cmd_idfy_ns(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t nsid = xnvme_dev_get_nsid(cli->args.dev);
	enum xnvme_spec_csi csi = XNVME_SPEC_CSI_KV;
	struct xnvme_spec_kvs_idfy *idfy = NULL;
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinf("xnvme_adm_idfy_ns: {nsid: 0x%x, csi: %s}", nsid, xnvme_spec_csi_str(csi));

	idfy = xnvme_buf_alloc(dev, sizeof(*idfy));
	if (!idfy) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_adm_idfy_ns_csi(&ctx, nsid, XNVME_SPEC_CSI_KV, &idfy->base);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_adm_idfy_ns_csi()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_spec_kvs_idfy_ns_pr(&idfy->ns, XNVME_PR_DEF);

	if (cli->args.data_output) {
		xnvmec_pinf("Dumping to: '%s'", cli->args.data_output);
		err = xnvmec_buf_to_file((char *)idfy, sizeof(*idfy), cli->args.data_output);
		if (err) {
			xnvmec_perr("xnvmec_buf_to_file()", err);
		}
	}

exit:
	xnvme_buf_free(dev, idfy);

	return err;
}

static int
cmd_retrieve(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint8_t nsid = cli->args.nsid;
	void *dbuf = NULL;
	size_t dbuf_nbytes = 4096;
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinf("Alloc/clear dbuf, dbuf_nbytes: %zu", dbuf_nbytes);

	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	memset(dbuf, 0, dbuf_nbytes);

	xnvmec_pinf("Sending xnvme_kvs_retrieve command");
	err = xnvme_kvs_retrieve(&ctx, nsid, cli->args.kv_key, strlen(cli->args.kv_key), dbuf,
				 dbuf_nbytes, 0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_kvs_retrieve()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	if (cli->args.data_output) {
		xnvmec_pinf("dumping to: '%s'", cli->args.data_output);
		err = xnvmec_buf_to_file(dbuf, dbuf_nbytes, cli->args.data_output);
		if (err) {
			xnvmec_perr("xnvmec_buf_to_file()", err);
		}
	} else {
		xnvmec_pinf("KV Value retrieved: '%s'", dbuf);
	}

exit:
	xnvme_buf_free(dev, dbuf);

	return err;
}

static int
cmd_store(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint8_t nsid = cli->args.nsid;
	uint8_t opt = 0x0;
	void *dbuf = NULL;
	size_t dbuf_nbytes = strlen(cli->args.kv_val) + 1;
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}
	xnvmec_pinf("Alloc/fill dbuf, dbuf_nbytes: %zu", dbuf_nbytes);

	if (cli->args.kv_store_add) {
		opt = opt | XNVME_KVS_STORE_OPT_DONT_STORE_IF_KEY_EXISTS;
	} else if (cli->args.kv_store_update) {
		opt = opt | XNVME_KVS_STORE_OPT_DONT_STORE_IF_KEY_NOT_EXISTS;
	}

	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	memcpy(dbuf, cli->args.kv_val, dbuf_nbytes);

	err = xnvme_kvs_store(&ctx, nsid, cli->args.kv_key, strlen(cli->args.kv_key), dbuf,
			      dbuf_nbytes, opt);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_kvs_store()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvmec_pinf("KV Value is written under a key: \"%s\"\n", cli->args.kv_key);
exit:
	xnvme_buf_free(dev, dbuf);

	return err;
}

static int
cmd_delete(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint8_t nsid = cli->args.nsid;
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinf("Sending xnvme_kvs_delete command");

	err = xnvme_kvs_delete(&ctx, nsid, cli->args.kv_key, strlen(cli->args.kv_key));
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_kv_delete()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvmec_pinf("Removed key: '%s'", cli->args.kv_key);

exit:
	return err;
}

static int
cmd_exist(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint8_t nsid = cli->args.nsid;
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinf("Sending xnvme_kvs_exist command");
	err = xnvme_kvs_exist(&ctx, nsid, cli->args.kv_key, strlen(cli->args.kv_key));
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_kvs_exist()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvmec_pinf("Key '%s' exists", cli->args.kv_key);

exit:
	return err;
}

static int
cmd_list(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint8_t nsid = cli->args.nsid;
	void *dbuf = NULL;
	size_t stdio_wrtn, dbuf_nbytes = 4096;
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	XNVME_DEBUG("Alloc/clear dbuf, dbuf_nbytes: %zu", dbuf_nbytes);

	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	memset(dbuf, 0, dbuf_nbytes);

	XNVME_DEBUG("Sending xnvme_kvs_list command");

	err = xnvme_kvs_list(&ctx, nsid, cli->args.kv_key, strlen(cli->args.kv_key), dbuf,
			     dbuf_nbytes);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_kvs_list()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	XNVME_DEBUG("KV List retrieved with %u keys\n", *(uint32_t *)dbuf);

	stdio_wrtn = fwrite(dbuf, sizeof(uint8_t), dbuf_nbytes, stdout);

	if (stdio_wrtn != dbuf_nbytes) {
		err = (int)stdio_wrtn;
		xnvmec_perr("fwrite()", err);
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

static struct xnvmec_sub g_subs[] = {
	{"enum",
	 "Enumerate Logical Block Namespaces on the system",
	 "Enumerate Logical Block Namespaces on the system",
	 cmd_enumerate,
	 {
		 {XNVMEC_OPT_SYS_URI, XNVMEC_LOPT},
		 {XNVMEC_OPT_FLAGS, XNVMEC_LOPT},

		 {XNVMEC_OPT_DEV_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_BE, XNVMEC_LOPT},
		 {XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
		 {XNVMEC_OPT_SYNC, XNVMEC_LOPT},
	 }},
	{"idfy-ns",
	 "Zoned Command Set specific identify-controller",
	 "KV Command Set specific identify-controller",
	 cmd_idfy_ns,
	 {
		 {XNVMEC_OPT_URI, XNVMEC_POSA},
		 {XNVMEC_OPT_NSID, XNVMEC_LOPT},

		 {XNVMEC_OPT_DEV_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_BE, XNVMEC_LOPT},
		 {XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
		 {XNVMEC_OPT_SYNC, XNVMEC_LOPT},
	 }},
	{"retrieve",
	 "Execute a KV Retrieve Command",
	 "Execute a KV Retrieve Command",
	 cmd_retrieve,
	 {
		 {XNVMEC_OPT_URI, XNVMEC_POSA},
		 {XNVMEC_OPT_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_KV_KEY, XNVMEC_LREQ},
		 {XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},

		 {XNVMEC_OPT_DEV_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_BE, XNVMEC_LOPT},
		 {XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
		 {XNVMEC_OPT_SYNC, XNVMEC_LOPT},
	 }},
	{"store",
	 "Execute a KV Store Command",
	 "Execute a KV Store Command",
	 cmd_store,
	 {
		 {XNVMEC_OPT_URI, XNVMEC_POSA},
		 {XNVMEC_OPT_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_KV_KEY, XNVMEC_LREQ},
		 {XNVMEC_OPT_KV_VAL, XNVMEC_LREQ},

		 {XNVMEC_OPT_KV_STORE_UPDATE, XNVMEC_LFLG},
		 {XNVMEC_OPT_KV_STORE_ADD, XNVMEC_LFLG},
		 {XNVMEC_OPT_KV_STORE_COMPRESS, XNVMEC_LFLG},

		 {XNVMEC_OPT_DEV_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_BE, XNVMEC_LOPT},
		 {XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
		 {XNVMEC_OPT_SYNC, XNVMEC_LOPT},
	 }},
	{"delete",
	 "Execute a KV Delete Command",
	 "Execute a KV Delete Command",
	 cmd_delete,
	 {
		 {XNVMEC_OPT_URI, XNVMEC_POSA},
		 {XNVMEC_OPT_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_KV_KEY, XNVMEC_LREQ},

		 {XNVMEC_OPT_DEV_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_BE, XNVMEC_LOPT},
		 {XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
		 {XNVMEC_OPT_SYNC, XNVMEC_LOPT},
	 }},
	{"exist",
	 "Execute a KV Exist Command",
	 "Execute a KV Exist Command",
	 cmd_exist,
	 {
		 {XNVMEC_OPT_URI, XNVMEC_POSA},
		 {XNVMEC_OPT_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_KV_KEY, XNVMEC_LREQ},

		 {XNVMEC_OPT_DEV_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_BE, XNVMEC_LOPT},
		 {XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
		 {XNVMEC_OPT_SYNC, XNVMEC_LOPT},
	 }},
	{"list",
	 "Execute a KV List Command",
	 "Execute a KV List Command",
	 cmd_list,
	 {
		 {XNVMEC_OPT_URI, XNVMEC_POSA},
		 {XNVMEC_OPT_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_KV_KEY, XNVMEC_LREQ},

		 {XNVMEC_OPT_DEV_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_BE, XNVMEC_LOPT},
		 {XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
		 {XNVMEC_OPT_SYNC, XNVMEC_LOPT},
	 }},
};

static struct xnvmec g_cli = {
	.title = "KV Utility",
	.descr_short = "Retrieve KV value",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}