/*
 * The User space library for Non-Volatile Memory NVMe Namespaces based on xNVMe, the
 * Cross-platform libraries and tools for NVMe devices
 *
 * Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
 * Copyright (C) Niclas Hedam <n.hedam@samsung.com, nhed@itu.dk>
 * SPDX-License-Identifier: Apache-2.0
 *
 * @headerfile libxnvme_compute.h
 */
#ifndef __LIBXNVME_COMPUTE_H
#define __LIBXNVME_COMPUTE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libxnvme.h>

/**
 * Load an compute ELF program
 *
 * @param buf Pointer to ELF program
 * @param buf_nbytes Size of the program in bytes
 * @param slot Program slot to load compute program into

 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_compute_load(struct xnvme_cmd_ctx *ctx, void *buf, size_t buf_nbytes, uint32_t slot);

/**
 * Unload an compute ELF program.
 *
 * @param slot Program slot to unload

 * @return On success, an xnvme_compute_prog struct of the program is returned.
 * On error, negative `errno` is returned.
 */
int
xnvme_compute_unload(struct xnvme_cmd_ctx *ctx, uint32_t slot);


/**
 * Execute an compute ELF program.
 *
 * @param slot Execute the program loaded into the specified slot

 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_compute_exec(struct xnvme_cmd_ctx *ctx, uint32_t slot);

/**
 * Push data to the data buffer
 *
 * @param buf Pointer to the data to be pushed
 * @param buf_nbytes Size of the data in bytes
 * @param slot Program slot to push data to

 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_compute_push(struct xnvme_cmd_ctx *ctx, void *buf, size_t buf_nbytes, uint32_t slot);

/**
 * Pull data from the data buffer
 *
 * @param buf Pointer to put the data in
 * @param buf_nbytes Size of the data in bytes
 * @param slot Program slot to pull data from

 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_compute_pull(struct xnvme_cmd_ctx *ctx, void *buf, size_t buf_nbytes, uint32_t slot);


#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_COMPUTE */
