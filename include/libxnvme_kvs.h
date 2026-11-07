/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_kvs.h
 */

/**
 * Options of the KV Retrieve command, the bits of the 'opt' of xnvme_kvs_retrieve()
 *
 * Each value is one bit of the Retrieve Option (RO) field, bits 15:08 of Command Dword 11.
 *
 * @see Key Value Command Set 1.4, Figure 20
 * @enum xnvme_retrieve_opts
 */
enum xnvme_retrieve_opts {
	XNVME_KVS_RETRIEVE_OPT_RRD = 1 << 0, ///< Return Raw Data (RRD), the value as stored
};

/**
 * Options of the KV Store command, the bits of the 'opt' of xnvme_kvs_store()
 *
 * Each value is one bit of the Store Option (SO) field, bits 15:08 of Command Dword 11.
 *
 * @see Key Value Command Set 1.4, Figure 31
 * @enum xnvme_store_opts
 */
enum xnvme_store_opts {
	XNVME_KVS_STORE_OPT_SIKE  = 1 << 0, ///< Store If Key Exist (SIKE), only if key exists
	XNVME_KVS_STORE_OPT_SINKE = 1 << 1, ///< Store If No Key Exists (SINKE), only if key is new
	XNVME_KVS_STORE_OPT_NOCOMP = 1 << 2, ///< No Compression (NOCOMP), do not compress
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
