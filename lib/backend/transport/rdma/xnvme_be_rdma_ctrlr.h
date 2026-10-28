// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_RDMA_CTRLR_H
#define __INTERNAL_XNVME_BE_RDMA_CTRLR_H

#include <rdma/rdma_cma.h>

#include <xnvme_transport.h>
#include <xnvme_transport_ctrlr.h>

#define TO_RDMA_CTRLR(ctrlr) container_of((ctrlr), struct xnvme_be_rdma_ctrlr, base)

struct xnvme_be_rdma_ctrlr {
	struct xnvme_transport_ctrlr base;
	struct rdma_addrinfo *res;      ///< Resolved address information array for the controller
	struct rdma_addrinfo *selected; ///< Selected address information for the controller
	struct ibv_pd *pd; ///< Allocated once, on the admin qpair's transport connect, and reused
			   ///< by every later qpair
};

// Defined in xnvme_be_rdma_ctrlr.c
extern struct xnvme_transport g_xnvme_be_rdma_transport;

#endif /* __INTERNAL_XNVME_BE_RDMA_CTRLR_H */
