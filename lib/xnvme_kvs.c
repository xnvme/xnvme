// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_spec.h>

static inline int
kvs_cmd_set_key(struct xnvme_spec_cmd *cmd, const void *key, uint8_t key_len)
{
	int err;
	uint8_t *key_ptr = (uint8_t *)key;

	cmd->kvs.cdw11.kl = key_len;

	err = xnvme_buf_memcpy((void *)&cmd->kvs.key, key, XNVME_MIN(key_len, 8));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_buf_memcpy(key), err: %d", err);
		return err;
	}

	if (key_len > 8) {
		err = xnvme_buf_memcpy((void *)&cmd->kvs.key_hi, key_ptr + 8,
				       XNVME_MIN(key_len - 8, 8));
		if (err) {
			XNVME_DEBUG("FAILED: xnvme_buf_memcpy(key_hi), err: %d", err);
			return err;
		}
	}

	return 0;
}

int
xnvme_kvs_retrieve(struct xnvme_cmd_ctx *ctx, uint32_t nsid, const void *key, uint8_t key_len,
		   const void *dbuf, uint32_t dbuf_nbytes, uint8_t opt)
{
	int err;
	void *cdbuf = (void *)dbuf;

	ctx->cmd.common.opcode = XNVME_SPEC_KV_OPC_RETRIEVE;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.kvs.cdw10 = dbuf_nbytes;

	err = kvs_cmd_set_key(&ctx->cmd, key, key_len);
	if (err) {
		XNVME_DEBUG("FAILED: kvs_cmd_set_key(), err: %d", err);
		return err;
	}

	if (opt & XNVME_KVS_RETRIEVE_OPT_RETRIEVE_RAW) {
		ctx->cmd.kvs.cdw11.ro = opt;
	}

	return xnvme_cmd_pass(ctx, cdbuf, dbuf_nbytes, NULL, 0);
}

int
xnvme_kvs_store(struct xnvme_cmd_ctx *ctx, uint32_t nsid, const void *key, uint8_t key_len,
		const void *dbuf, uint32_t dbuf_nbytes, uint8_t opt)
{
	int err;
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

	err = kvs_cmd_set_key(&ctx->cmd, key, key_len);
	if (err) {
		XNVME_DEBUG("FAILED: kvs_cmd_set_key(), err: %d", err);
		return err;
	}

	return xnvme_cmd_pass(ctx, cdbuf, dbuf_nbytes, NULL, 0);
}

int
xnvme_kvs_delete(struct xnvme_cmd_ctx *ctx, uint32_t nsid, const void *key, uint8_t key_len)
{
	int err;
	ctx->cmd.common.opcode = XNVME_SPEC_KV_OPC_DELETE;
	ctx->cmd.common.nsid = nsid;

	err = kvs_cmd_set_key(&ctx->cmd, key, key_len);
	if (err) {
		XNVME_DEBUG("FAILED: kvs_cmd_set_key(), err: %d", err);
		return err;
	}

	return xnvme_cmd_pass(ctx, NULL, 0, NULL, 0);
}

int
xnvme_kvs_exist(struct xnvme_cmd_ctx *ctx, uint32_t nsid, const void *key, uint8_t key_len)
{
	int err;
	ctx->cmd.common.opcode = XNVME_SPEC_KV_OPC_EXIST;
	ctx->cmd.common.nsid = nsid;

	err = kvs_cmd_set_key(&ctx->cmd, key, key_len);
	if (err) {
		XNVME_DEBUG("FAILED: kvs_cmd_set_key(), err: %d", err);
		return err;
	}

	return xnvme_cmd_pass(ctx, NULL, 0, NULL, 0);
}

int
xnvme_kvs_list(struct xnvme_cmd_ctx *ctx, uint32_t nsid, const void *key, uint8_t key_len,
	       const void *dbuf, uint32_t dbuf_nbytes)
{
	int err;
	void *cdbuf = (void *)dbuf;

	ctx->cmd.common.opcode = XNVME_SPEC_KV_OPC_LIST;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.kvs.cdw10 = dbuf_nbytes;

	err = kvs_cmd_set_key(&ctx->cmd, key, key_len);
	if (err) {
		XNVME_DEBUG("FAILED: kvs_cmd_set_key(), err: %d", err);
		return err;
	}

	return xnvme_cmd_pass(ctx, cdbuf, dbuf_nbytes, NULL, 0);
}
