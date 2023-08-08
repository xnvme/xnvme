/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @headerfile libxnvme_kv.h
 */

enum xnvme_retreive_opts {
	XNVME_KVS_RETRIEVE_OPT_RETRIEVE_RAW = 1 << 0,
};

enum xnvme_store_opts {
	XNVME_KVS_STORE_OPT_DONT_STORE_IF_KEY_NOT_EXISTS = 1 << 0,
	XNVME_KVS_STORE_OPT_DONT_STORE_IF_KEY_EXISTS     = 1 << 1,
	XNVME_KVS_STORE_OPT_COMPRESS                     = 1 << 2,
};

/**
 * Submit, and optionally wait for completion of, a KV Retrieve
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param key pointer to a KV Key buffer
 * @param key_len KV Key size in bytes
 * @param vbuf pointer to Host Buffer
 * @param vbuf_nbytes Host Buffer size in bytes
 * @param opt Retrieve options, see ::xnvme_retrieve_opts
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_kvs_retrieve(struct xnvme_cmd_ctx *ctx, uint32_t nsid, const void *key, uint8_t key_len,
		   const void *vbuf, uint32_t vbuf_nbytes, uint8_t opt);

/**
 * Submit, and optionally wait for completion of, a KV Store
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param key pointer to a KV Key buffer
 * @param key_len KV Key size in bytes
 * @param vbuf pointer to data payload
 * @param vbuf_nbytes data payload size in bytes
 * @param opt Store-options, see ::xnvme_store_opts
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_kvs_store(struct xnvme_cmd_ctx *ctx, uint32_t nsid, const void *key, uint8_t key_len,
		const void *vbuf, uint32_t vbuf_nbytes, uint8_t opt);

/**
 * Submit, and optionally wait for completion of, a KV Delete
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param key pointer to a KV Key buffer
 * @param key_len KV Key size in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_kvs_delete(struct xnvme_cmd_ctx *ctx, uint32_t nsid, const void *key, uint8_t key_len);

/**
 * Submit, and optionally wait for completion of, a KV Exist
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param key pointer to a KV Key buffer
 * @param key_len KV Key size in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_kvs_exist(struct xnvme_cmd_ctx *ctx, uint32_t nsid, const void *key, uint8_t key_len);

/**
 * Submit, and optionally wait for completion of, a KV List
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param key pointer to a KV Key buffer
 * @param key_len KV Key size in bytes
 * @param vbuf pointer to Host Buffer
 * @param vbuf_nbytes Host Buffer size in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_kvs_list(struct xnvme_cmd_ctx *ctx, uint32_t nsid, const void *key, uint8_t key_len,
	       const void *vbuf, uint32_t vbuf_nbytes);
