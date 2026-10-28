// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_TRANSPORT_H
#define __INTERNAL_XNVME_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

/**
 * NVMe-oF transport descriptor and ops tables.
 *
 * Forward declarations only, so this header pulls in no ctrlr/qpair object
 * definitions. Each transport (RDMA, TCP, ...) provides one
 * `struct xnvme_transport` instance, cached by the core.
 *
 * The transport itself exposes only `ctrlr_probe`, to limit mutability. The
 * probe allocates a ctrlr and installs its `const`
 * `struct xnvme_transport_ctrlr_ops`. `ctrlr_qpair_alloc` allocates a qpair
 * and installs `struct xnvme_transport_qpair_ops` in it, so all other qpair
 * calls go through the qpair and never through the ctrlr.
 */

struct xnvme_transport_ctrlr;
struct xnvme_transport_qpair;

struct xnvme_transport_ctrlr_ops {
	void (*ctrlr_free)(struct xnvme_transport_ctrlr *ctrlr);
	int (*ctrlr_teardown)(struct xnvme_transport_ctrlr *ctrlr);
	int (*ctrlr_connect)(struct xnvme_transport_ctrlr *ctrlr, const char *uri);
	int (*ctrlr_disconnect)(struct xnvme_transport_ctrlr *ctrlr);
	int (*ctrlr_reg)(struct xnvme_transport_ctrlr *ctrlr, void *buf, size_t nbytes,
			 void **handle, uint32_t *key);
	int (*ctrlr_dereg)(struct xnvme_transport_ctrlr *ctrlr, void *handle);

	/** Allocate a qpair with its `ops` installed, all further calls go through the qpair */
	int (*ctrlr_qpair_alloc)(struct xnvme_transport_ctrlr *ctrlr,
				 struct xnvme_transport_qpair **qpair);
};

struct xnvme_transport_qpair_ops {
	/* control plane */
	void (*qpair_free)(struct xnvme_transport_qpair *qpair);
	int (*qpair_init)(struct xnvme_transport_qpair *qpair);
	int (*qpair_teardown)(struct xnvme_transport_qpair *qpair);
	int (*qpair_connect)(struct xnvme_transport_qpair *qpair);
	int (*qpair_disconnect)(struct xnvme_transport_qpair *qpair);

	/* data plane */
	int (*qpair_submit)(struct xnvme_transport_qpair *qpair, const void *capsule,
			    size_t nbytes, uint16_t cid);
	int (*qpair_poll)(struct xnvme_transport_qpair *qpair, uint32_t max);
};

struct xnvme_transport {
	const char *name;

	/** Allocate a ctrlr with `ops` installed */
	int (*ctrlr_probe)(struct xnvme_transport_ctrlr **ctrlr);
};
#endif /* __INTERNAL_XNVME_TRANSPORT_H */
