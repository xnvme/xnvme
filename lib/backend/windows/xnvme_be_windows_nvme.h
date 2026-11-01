// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

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

#endif /* __INTERNAL_XNVME_BE_WINDOWS_NVME_H */
