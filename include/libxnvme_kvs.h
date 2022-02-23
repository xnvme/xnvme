/**
 * The User space library for Key-Value Namespaces based on xNVMe, the
 * Cross-platform libraries and tools for NVMe devices
 *
 * Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * @headerfile libxnvme_kv.h
 */
#ifndef __LIBXNVME_KV_H
#define __LIBXNVME_KV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libxnvme.h>

#define XNVME_KVS_RETRIEVE_OPT_RETRIEVE_RAW 1 << 0

// Only update existing
#define XNVME_KVS_STORE_OPT_DONT_STORE_IF_KEY_NOT_EXISTS 1 << 0

// Only add new
#define XNVME_KVS_STORE_OPT_DONT_STORE_IF_KEY_EXISTS 1 << 1
#define XNVME_KVS_STORE_OPT_COMPRESS                 1 << 2

/**
 * Submit, and optionally wait for completion of, a KV Retrieve
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param key pointer to a KV Key buffer
 * @param key_len KV Key size in bytes
 * @param dbuf pointer to Host Buffer
 * @param dbuf_nbytes Host Buffer size in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_kvs_retrieve(struct xnvme_cmd_ctx *ctx, uint32_t nsid, const void *key, uint8_t key_len,
		   uint8_t opt, const void *dbuf, size_t dbuf_nbytes);

/**
 * Submit, and optionally wait for completion of, a KV Store
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param key pointer to a KV Key buffer
 * @param key_len KV Key size in bytes
 * @param dbuf pointer to data payload
 * @param dbuf_nbytes data payload size in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_kvs_store(struct xnvme_cmd_ctx *ctx, uint32_t nsid, const void *key, uint8_t key_len,
		uint8_t opt, const void *dbuf, size_t dbuf_nbytes);

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
 * @param dbuf pointer to Host Buffer
 * @param dbuf_nbytes Host Buffer size in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_kvs_list(struct xnvme_cmd_ctx *ctx, uint32_t nsid, const void *key, uint8_t key_len,
	       const void *dbuf, size_t dbuf_nbytes);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_KV_H */
