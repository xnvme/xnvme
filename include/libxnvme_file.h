/*
 * The User space library for Non-Volatile Memory NVMe Namespaces based on xNVMe, the
 * Cross-platform libraries and tools for NVMe devices
 *
 * Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * @headerfile libxnvme_nvm.h
 */
#ifndef __LIBXNVME_FILE_H
#define __LIBXNVME_FILE_H

#ifdef __cplusplus
extern "C" {
#endif
#include <sys/stat.h>
#include <libxnvme.h>

typedef struct xnvme_dev xnvme_file;

/**
 * Open the file identified by pathname for I/O operation
 *
 * @param pathname Path to the file
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
struct xnvme_file *
xnvme_file_open(const char *pathname, int flags);

/**
 * Close the file encapsulated by the given ::xnvme_file handle
 *
 * @param fh File-handle as obtained by with ::xnvme_file_open
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_file_close(struct xnvme_file *fh);

/**
 * Fills the given 'statbuf' with information about a file
 *
 * @param fh File-handle as obtained by with ::xnvme_file_open
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int xnvme_file_stat(struct xnvme_file *fh);

/**
 * Returns a synchronous command-context for the given file-handle
 *
 * For asynchronous I/O, see the ::xnvme_queue interface and use xnvme_cmd_ctx_from_queue
 */
struct xnvme_cmd_ctx *
xnvme_file_get_cmd_ctx(struct xnvme_file *fh);

/**
 * Perform a stateless read to the file or device encapsulated by 'ctx'
 *
 * Retrieve a synchronous context using ::xnvme_cmd_ctx_from_dev and an asynchronous context using
 * ::xnvme_cmd_ctx_from_queue
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param buf Pointer to data-payload
 * @param count The number of bytes to read
 * @param offset The offset, in bytes, to start reading from
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
ssize_t
xnvme_file_pread(struct xnvme_cmd_ctx *ctx, void *buf, size_t count, off_t offset);

/**
 * Perform a stateless write to the file or device encapsulated by 'ctx'
 *
 * Retrieve a synchronous context using ::xnvme_cmd_ctx_from_dev and an asynchronous context using
 * ::xnvme_cmd_ctx_from_queue
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param buf Pointer to data-payload
 * @param count The number of bytes to read
 * @param offset The offset, in bytes, to start reading from
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
ssize_t
xnvme_file_pwrite(struct xnvme_cmd_ctx *ctx, void *buf, size_t count, off_t offset);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_FILE */
