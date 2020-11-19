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
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace Identifier
 * @param slba The LBA to start reading from
 * @param nlb The number of LBAs to read. NOTE: nlb is a zero-based value
 * @param dbuf Pointer to data-payload
 * @param mbuf Pointer to meta-payload
 * @param opts command options, see ::xnvme_cmd_opts
 * @param ctx Pointer to structure for NVMe completion and async. context
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_nvm_read(struct xnvme_dev *dev, uint32_t nsid, uint64_t slba, uint16_t nlb, void *dbuf,
	       void *mbuf, int opts, struct xnvme_cmd_ctx *ctx);

/**
 * Submit, and optionally wait for completion of, a NVMe Write
 *
 * @see xnvme_cmd_opts
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace Identifier
 * @param slba The LBA to start the write at
 * @param nlb Number of LBAs to be written. NOTE: nlb is a zero-based value
 * @param dbuf Pointer to buffer; Payload as indicated by 'opts'
 * @param mbuf Pointer to buffer; Payload as indicated by 'opts'
 * @param opts command options, see ::xnvme_cmd_opts
 * @param ctx Pointer to structure for NVMe completion and async. context
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_nvm_write(struct xnvme_dev *dev, uint32_t nsid, uint64_t slba,
		uint16_t nlb, const void *dbuf, const void *mbuf, int opts,
		struct xnvme_cmd_ctx *ctx);

/**
 * Submit a write uncorrected command
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace Identifier
 * @param slba The Starting Destination LBA
 * @param nlb Number Of Logical Blocks
 * @param opts command options, see ::xnvme_cmd_opts
 * @param ret Pointer to structure for NVMe completion and async. context
*/
int
xnvme_nvm_write_uncorrectable(struct xnvme_dev *dev, uint32_t nsid, uint64_t slba, uint16_t nlb,
			      int opts, struct xnvme_cmd_ctx *ret);

/**
 * Submit a write zeroes command
 *
 * @see xnvme_cmd_opts
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace Identifier
 * @param sdlba The Starting Destination LBA to start copying to
 * @param nlb Number Of Logical Blocks
 * @param opts command options, see ::xnvme_cmd_opts
 * @param ret Pointer to structure for NVMe completion and async. context
 *
 * @return On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate and `ret` filled with lower-level status codes* @param dev
 */
int
xnvme_nvm_write_zeroes(struct xnvme_dev *dev, uint32_t nsid, uint64_t sdlba, uint16_t nlb,
		       int opts,
		       struct xnvme_cmd_ctx *ret);

/**
 * Submit, and optionally wait for completion of a NVMe Simple-Copy-Command
 *
 * @see xnvme_cmd_opts
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace Identifier
 * @param sdlba The Starting Destination LBA to start copying to
 * @param ranges Pointer to ranges-buffer, see ::xnvme_spec_nvm_scopy_fmt_zero
 * @param nr Number of ranges in the given ranges-buffer, zero-based value
 * @param copy_fmt For of ranges
 * @param opts command options, see ::xnvme_cmd_opts
 * @param ret Pointer to structure for NVMe completion and async. context
 *
 * @return On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate and `ret` filled with lower-level status codes
 */
int
xnvme_nvm_scopy(struct xnvme_dev *dev, uint32_t nsid, uint64_t sdlba,
		struct xnvme_spec_nvm_scopy_fmt_zero *ranges, uint8_t nr,
		enum xnvme_nvm_scopy_fmt copy_fmt, int opts, struct xnvme_cmd_ctx *ret);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_NVM */
