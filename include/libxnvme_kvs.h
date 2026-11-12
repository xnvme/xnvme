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
 * Each value is one bit of the Retrieve Option (RO) field, bits 15:08 of Command Dword 11.
 *
 * @see Key Value Command Set 1.4, Figure 20
 * @enum xnvme_retrieve_opts
 */
enum xnvme_retrieve_opts {
	XNVME_KVS_RETRIEVE_OPT_RRD = 1 << 0, ///< Return Raw Data (RRD), the value as stored
};

/**
 * Options of the KV Store command, the bits of xnvme_cmd_prep_kv_store_args.so
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
