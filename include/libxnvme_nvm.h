/*
 * The User space library for Non-Volatile Memory NVMe Namespaces based on xNVMe, the
 * Cross-platform libraries and tools for NVMe devices
 *
 * Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * @headerfile libxnvme_nvm.h
 */
#ifndef __LIBXNVME_NVM_H
#define __LIBXNVME_NVM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libxnvme.h>

/**
 * Submit, and optionally wait for completion of, a NVMe Read
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param slba The LBA to start reading from
 * @param nlb The number of LBAs to read. NOTE: nlb is a zero-based value
 * @param dbuf Pointer to data-payload
 * @param mbuf Pointer to meta-payload
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_nvm_read(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba, uint16_t nlb, void *dbuf,
	       void *mbuf);

/**
 * Submit, and optionally wait for completion of, a NVMe Write
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param slba The LBA to start the write at
 * @param nlb Number of LBAs to be written. NOTE: nlb is a zero-based value
 * @param dbuf Pointer to buffer; Payload as indicated by 'ctx->opts'
 * @param mbuf Pointer to buffer; Payload as indicated by 'ctx->opts'
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_nvm_write(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba, uint16_t nlb,
		const void *dbuf, const void *mbuf);

/**
 * Submit a write uncorrected command
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param slba The Starting Destination LBA
 * @param nlb Number Of Logical Blocks
 */
int
xnvme_nvm_write_uncorrectable(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba,
			      uint16_t nlb);

/**
 * Submit a write zeroes command
 *
 * @see xnvme_cmd_opts
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param sdlba The Starting Destination LBA to start copying to
 * @param nlb Number Of Logical Blocks
 *
 * @return On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate and param ctx.cpl filled with lower-level status codes
 */
int
xnvme_nvm_write_zeroes(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t sdlba, uint16_t nlb);

/**
 * Submit, and optionally wait for completion of, a NVMe Write
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param opcode Opcode for the NVMe command
 * @param nsid Namespace Identifier
 * @param slba The LBA to start the write at
 * @param nlb Number of LBAs to be written. NOTE: nlb is a zero-based value
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
void
xnvme_prep_nvm(struct xnvme_cmd_ctx *ctx, uint8_t opcode, uint32_t nsid, uint64_t slba,
	       uint16_t nlb);

/**
 * Submit, and optionally wait for completion of a NVMe Simple-Copy-Command
 *
 * @see xnvme_cmd_opts
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param sdlba The Starting Destination LBA to start copying to
 * @param ranges Pointer to ranges-buffer, see ::xnvme_spec_nvm_scopy_fmt_zero
 * @param nr Number of ranges in the given ranges-buffer, zero-based value
 * @param copy_fmt For of ranges
 *
 * @return On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate and `ctx.cpl` filled with lower-level status codes
 */
int
xnvme_nvm_scopy(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t sdlba,
		struct xnvme_spec_nvm_scopy_fmt_zero *ranges, uint8_t nr,
		enum xnvme_nvm_scopy_fmt copy_fmt);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_NVM */
