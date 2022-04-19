// Copyright (C) Rishabh Shukla <rishabh.sh@samsung.com>
// Copyright (C) Pranjal Dave <pranjal.58@partner.samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#ifndef __INTERNAL_XNVME_BE_WINDOWS_NVME_H
#define __INTERNAL_XNVME_BE_WINDOWS_NVME_H

#define _NTSCSI_USER_MODE_
#include <xnvme_dev.h>
#include <windows.h>
#include <ddk/scsi.h>
#include <ntddscsi.h>

#define SPT_CDB_LENGTH    32
#define SPT_SENSE_LENGTH  32
#define SPTWB_DATA_LENGTH 512

struct scsi_pass_through_direct_with_buffer {
	SCSI_PASS_THROUGH_DIRECT sptd;
	ULONG filler; ///< realign buffers to double word boundary
	UCHAR uc_sense_buf[SPT_SENSE_LENGTH];
};

/**
 * Pass a NVMe IO Command through to the device via the Windows NVMe Kernel driver
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param dbuf pointer to data-payload
 * @param dbuf_nbytes size of data-payload in bytes
 * @param mbuf pointer to meta-payload
 * @param mbuf_nbytes size of the meta-payload in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_be_windows_sync_nvme_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
				  void *mbuf, size_t mbuf_nbytes);

/**
 * Pass a NVMe Admin Command through to the device via the Windows NVMe Kernel driver
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param dbuf pointer to data-payload
 * @param dbuf_nbytes size of data-payload in bytes
 * @param mbuf pointer to meta-payload
 * @param mbuf_nbytes size of the meta-payload in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_be_windows_nvme_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
				void *mbuf, size_t mbuf_nbytes);

int
xnvme_be_windows_block_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf,
				 size_t XNVME_UNUSED(dbuf_nbytes), void *XNVME_UNUSED(mbuf),
				 size_t XNVME_UNUSED(mbuf_nbytes));

#endif /* __INTERNAL_XNVME_BE_WINDOWS_NVME_H */
