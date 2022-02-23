// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>
#include <libxnvme_spec_pp.h>
#include <libxnvme_adm.h>
#include <libxnvme_kvs.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_spec.h>
#include <libxnvmec.h>

static inline void
kvs_put_key_into_cmd(struct xnvme_spec_cmd *cmd, const void *key, uint8_t key_len)
{
	uint8_t *key_ptr = (uint8_t *)key;
	cmd->kvs.cdw11.kl = key_len;

	memcpy((void *)&cmd->kvs.key, key, XNVME_MIN(key_len, 8));

	if (key_len > 8) {
		memcpy((void *)&cmd->kvs.key_hi, key_ptr + 8, XNVME_MIN(key_len - 8, 8));
	}
}

int
xnvme_kvs_retrieve(struct xnvme_cmd_ctx *ctx, uint32_t nsid, const void *key, uint8_t key_len,
		   uint8_t opt, const void *dbuf, size_t dbuf_nbytes)
{
	void *cdbuf = (void *)dbuf;

	ctx->cmd.common.opcode = XNVME_SPEC_KV_OPC_RETRIEVE;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.kvs.cdw10 = dbuf_nbytes;

	kvs_put_key_into_cmd(&ctx->cmd, key, key_len);

	if (opt & XNVME_KVS_RETRIEVE_OPT_RETRIEVE_RAW) {
		ctx->cmd.kvs.cdw11.ro = opt;
	}

	return xnvme_cmd_pass(ctx, cdbuf, dbuf_nbytes, NULL, 0);
}

int
xnvme_kvs_store(struct xnvme_cmd_ctx *ctx, uint32_t nsid, const void *key, uint8_t key_len,
		uint8_t opt, const void *dbuf, size_t dbuf_nbytes)
{
	void *cdbuf = (void *)dbuf;

	ctx->cmd.common.opcode = XNVME_SPEC_KV_OPC_STORE;
	ctx->cmd.common.nsid = nsid;

	// Value Size
	ctx->cmd.kvs.cdw10 = dbuf_nbytes;

	if (opt & XNVME_KVS_STORE_OPT_DONT_STORE_IF_KEY_NOT_EXISTS) {
		ctx->cmd.kvs.cdw11.ro = opt;
	}

	if (opt & XNVME_KVS_STORE_OPT_DONT_STORE_IF_KEY_EXISTS) {
		ctx->cmd.kvs.cdw11.ro = opt;
	}

	if (opt & XNVME_KVS_STORE_OPT_COMPRESS) {
		ctx->cmd.kvs.cdw11.ro = ctx->cmd.kvs.cdw11.ro | XNVME_KVS_STORE_OPT_COMPRESS;
	}

	kvs_put_key_into_cmd(&ctx->cmd, key, key_len);

	return xnvme_cmd_pass(ctx, cdbuf, dbuf_nbytes, NULL, 0);
}

int
xnvme_kvs_delete(struct xnvme_cmd_ctx *ctx, uint32_t nsid, const void *key, uint8_t key_len)
{
	ctx->cmd.common.opcode = XNVME_SPEC_KV_OPC_DELETE;
	ctx->cmd.common.nsid = nsid;

	kvs_put_key_into_cmd(&ctx->cmd, key, key_len);

	return xnvme_cmd_pass(ctx, NULL, 0, NULL, 0);
}

int
xnvme_kvs_exist(struct xnvme_cmd_ctx *ctx, uint32_t nsid, const void *key, uint8_t key_len)
{
	ctx->cmd.common.opcode = XNVME_SPEC_KV_OPC_EXIST;
	ctx->cmd.common.nsid = nsid;

	kvs_put_key_into_cmd(&ctx->cmd, key, key_len);

	return xnvme_cmd_pass(ctx, NULL, 0, NULL, 0);
}

int
xnvme_kvs_list(struct xnvme_cmd_ctx *ctx, uint32_t nsid, const void *key, uint8_t key_len,
	       const void *dbuf, size_t dbuf_nbytes)
{
	void *cdbuf = (void *)dbuf;

	ctx->cmd.common.opcode = XNVME_SPEC_KV_OPC_LIST;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.kvs.cdw10 = dbuf_nbytes;

	kvs_put_key_into_cmd(&ctx->cmd, key, key_len);

	return xnvme_cmd_pass(ctx, cdbuf, dbuf_nbytes, NULL, 0);
}