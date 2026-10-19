// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_CRC_H
#define __INTERNAL_XNVME_CRC_H

uint64_t
xnvme_crc64_nvme(const void *buf, size_t len, uint64_t crc);

uint16_t
xnvme_crc16_t10dif(uint16_t init_crc, const void *buf, size_t len);

#endif /* __INTERNAL_XNVME_CRC_H */
