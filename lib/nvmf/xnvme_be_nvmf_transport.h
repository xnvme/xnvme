#ifndef _INTERNAL_XNVME_BE_NVMF_TRANSPORT_H
#define _INTERNAL_XNVME_BE_NVMF_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

/**
 * NVMe-oF transport descriptor and ops table.
 *
 * Forward declarations only, so this header pulls in no ctrlr/qpair object
 * definitions. Each transport (RDMA, TCP, ...) provides one
 * `struct xnvme_be_nvmf_transport` instance, cached by the core.
 *
 * There is one `struct xnvme_be_nvmf_transport_ops` per transport, holding
 * every `ctrlr_*` and `qpair_*` entry. `ctrlr_alloc`
 * stores the ops pointer in the ctrlr, and `qpair_alloc` copies it into each
 * qpair, so both cache the same `const` pointer value.
 */

struct iovec;
struct xnvme_cmd_ctx;
struct xnvme_be_nvmf_ctrlr;
struct xnvme_be_nvmf_qpair;

struct xnvme_be_nvmf_transport_ops {
	/* control plane: ctrlr */
	int (*ctrlr_alloc)(struct xnvme_be_nvmf_ctrlr **ctrlr);
	void (*ctrlr_free)(struct xnvme_be_nvmf_ctrlr *ctrlr);
	int (*ctrlr_init)(struct xnvme_be_nvmf_ctrlr *ctrlr);
	int (*ctrlr_teardown)(struct xnvme_be_nvmf_ctrlr *ctrlr);
	int (*ctrlr_connect)(struct xnvme_be_nvmf_ctrlr *ctrlr, const char *uri);
	int (*ctrlr_disconnect)(struct xnvme_be_nvmf_ctrlr *ctrlr);
	int (*ctrlr_reg)(struct xnvme_be_nvmf_ctrlr *ctrlr, void *buf, size_t nbytes,
			 void **handle, uint32_t *key);
	int (*ctrlr_dereg)(struct xnvme_be_nvmf_ctrlr *ctrlr, void *handle);

	/* control plane: qpair */
	int (*qpair_alloc)(struct xnvme_be_nvmf_ctrlr *ctrlr, struct xnvme_be_nvmf_qpair **qpair);
	void (*qpair_free)(struct xnvme_be_nvmf_qpair *qpair);
	int (*qpair_init)(struct xnvme_be_nvmf_qpair *qpair);
	int (*qpair_teardown)(struct xnvme_be_nvmf_qpair *qpair);
	int (*qpair_connect)(struct xnvme_be_nvmf_qpair *qpair);
	int (*qpair_disconnect)(struct xnvme_be_nvmf_qpair *qpair);

	/* data plane */
	int (*qpair_submit)(struct xnvme_be_nvmf_qpair *qpair, const void *capsule, size_t nbytes,
			    uint16_t cid);
	int (*qpair_poll)(struct xnvme_be_nvmf_qpair *qpair, uint32_t max);
	int (*cmd_io)(struct xnvme_be_nvmf_qpair *qpair, struct xnvme_cmd_ctx *ctx, void *dbuf,
		      size_t dbuf_nbytes, void *mbuf, size_t mbuf_nbytes);
	int (*cmd_iov)(struct xnvme_be_nvmf_qpair *qpair, struct xnvme_cmd_ctx *ctx,
		       struct iovec *dvec, size_t dvec_cnt, size_t dvec_nbytes, struct iovec *mvec,
		       size_t mvec_cnt, size_t mvec_nbytes);
};

struct xnvme_be_nvmf_transport {
	const char *name;
	const struct xnvme_be_nvmf_transport_ops *ops;
};

#endif /* _INTERNAL_XNVME_BE_NVMF_TRANSPORT_H */
