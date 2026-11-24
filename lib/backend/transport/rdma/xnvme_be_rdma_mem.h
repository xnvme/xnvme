// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_RDMA_MEM_H
#define __INTERNAL_XNVME_BE_RDMA_MEM_H

#include <xnvme_transport_mem.h>

int
xnvme_be_rdma_mem_init(struct xnvme_dev *dev);

int
xnvme_be_rdma_mem_term(struct xnvme_dev *dev);

#endif // __INTERNAL_XNVME_BE_RDMA_MEM_H
