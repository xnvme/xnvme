/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_kvs.h
 */

/**
 * Options of the KV Retrieve command, the bits of xnvme_cmd_prep_kv_retrieve_args.ro
 *
 * @enum xnvme_retrieve_opts
 */
enum xnvme_retrieve_opts {
	XNVME_KVS_RETRIEVE_OPT_RETRIEVE_RAW = 1 << 0, ///< Retrieve the value as stored, raw
};

/**
 * Options of the KV Store command, the bits of xnvme_cmd_prep_kv_store_args.so
 *
 * @enum xnvme_store_opts
 */
enum xnvme_store_opts {
	XNVME_KVS_STORE_OPT_DONT_STORE_IF_KEY_NOT_EXISTS = 1 << 0, ///< Store only if key exists
	XNVME_KVS_STORE_OPT_DONT_STORE_IF_KEY_EXISTS     = 1 << 1, ///< Store only if key is new
	XNVME_KVS_STORE_OPT_COMPRESS                     = 1 << 2, ///< Compress the value
};
