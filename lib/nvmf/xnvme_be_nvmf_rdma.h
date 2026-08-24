#ifndef _INTERNAL_XNVME_BE_NVMF_RDMA_H
#define _INTERNAL_XNVME_BE_NVMF_RDMA_H

#include <rdma/rdma_cma.h>

#include <xnvme_be_nvmf.h>

#define TO_XNVME_NVMF_RDMA_QPAIR(qpair) \
	container_of((qpair), struct xnvme_be_nvmf_rdma_qpair, base)

void
xnvme_be_nvmf_rdma_on_capsule_recv(struct xnvme_be_nvmf_qpair *qpair, void *buf, size_t len);
void
xnvme_be_nvmf_rdma_on_send_cmpl(struct xnvme_be_nvmf_qpair *qpair, void *buf, int status);
void
xnvme_be_nvmf_rdma_on_state_change(struct xnvme_be_nvmf_qpair *qpair,
				   enum xnvme_nvmf_qpair_state state, void *ctx);

#define TO_XNVME_NVMF_RDMA_CTRLR(ctrlr) \
	container_of((ctrlr), struct xnvme_be_nvmf_rdma_ctrlr, base)

struct xnvme_be_nvmf_rdma_qpair {
	struct xnvme_be_nvmf_qpair base;
};

struct xnvme_be_nvmf_rdma_ctrlr {
	struct xnvme_be_nvmf_ctrlr base;
	pthread_mutex_t lock; ///< Controller lock for thread-safe operations
};

int
xnvme_be_nvmf_create_rdma_controller(struct xnvme_be_nvmf_ctrlr **ctrlr);

#endif /* _INTERNAL_XNVME_BE_NVMF_RDMA_H */