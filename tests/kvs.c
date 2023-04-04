#include <errno.h>
#include <libxnvmec.h>
#include <libxnvme_kvs.h>

static int
kvs_io(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint8_t nsid = cli->args.nsid;
	void *kv_key = NULL;
	void *kv_val = NULL;
	void *dbuf = NULL;
	void *rbuf = NULL;
	uint8_t kv_key_nbytes = 0;
	size_t kv_val_nbytes = 0;
	int err;

	if (!cli->given[XNVMEC_OPT_KV_KEY]) {
		kv_key = "marco";
	}

	if (!cli->given[XNVMEC_OPT_KV_VAL]) {
		kv_val = "polo";
	}

	kv_key_nbytes = strlen(kv_key);
	kv_val_nbytes = strlen(kv_val) + 1;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinf("Alloc/fill dbuf, kv_val_nbytes: %zu", kv_val_nbytes);

	dbuf = xnvme_buf_alloc(dev, kv_val_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	memcpy(dbuf, kv_val, kv_val_nbytes);

	err = xnvme_kvs_store(&ctx, nsid, kv_key, kv_key_nbytes, 0, dbuf, kv_val_nbytes);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_kvs_store()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvmec_pinf("KV Value is written under a key: \"%s\"\n", kv_key);

	rbuf = xnvme_buf_alloc(dev, kv_val_nbytes);
	if (!rbuf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	memset(rbuf, 0, kv_val_nbytes);

	xnvmec_pinf("Sending xnvme_kvs_retrieve command");
	err = xnvme_kvs_retrieve(&ctx, nsid, kv_key, kv_key_nbytes, 0, rbuf, kv_val_nbytes);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_kvs_retrieve()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvmec_pinf("KV Value retrieved: '%s'", rbuf);

	xnvmec_pinf("Comparing wbuf and rbuf");
	if (xnvmec_buf_diff(dbuf, rbuf, kv_val_nbytes)) {
		xnvmec_buf_diff_pr(dbuf, rbuf, kv_val_nbytes, XNVME_PR_DEF);
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
static struct xnvmec_sub g_subs[] = {
	{"kvs_io",
	 "Basic Verification of being able to read, what was written",
	 "Basic Verification of being able to read, what was written",
	 kvs_io,
	 {
		 {XNVMEC_OPT_POSA_TITLE, XNVMEC_SKIP},
		 {XNVMEC_OPT_URI, XNVMEC_POSA},
		 {XNVMEC_OPT_KV_KEY, XNVMEC_LOPT},
		 {XNVMEC_OPT_KV_VAL, XNVMEC_LOPT},
		 XNVMEC_SYNC_OPTS,
	 }},
};

static struct xnvmec g_cli = {
	.title = "Basic KV Verification",
	.descr_short = "Basic KV Verification",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
