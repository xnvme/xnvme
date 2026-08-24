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

enum xnvme_nvmf_rdmacm_state {
	XNVME_NVMF_RDMACM_STATE_INVALID = 0,
	XNVME_NVMF_RDMACM_STATE_RESOLVE_ADDRESS,
	XNVME_NVMF_RDMACM_STATE_RESOLVE_ROUTE,
	XNVME_NVMF_RDMACM_STATE_CONNECTING,
	XNVME_NVMF_RDMACM_STATE_CONNECTED,
	XNVME_NVMF_RDMACM_STATE_DISCONNECTED,
	XNVME_NVMF_RDMACM_STATE_ERROR,
	XNVME_NVMF_RDMACM_STATE_MAX = XNVME_NVMF_RDMACM_STATE_ERROR,
};

enum xnvme_be_nvmf_wr_type {
	XNVME_BE_NVMF_WR_TYPE_INVALID = 0,
	XNVME_BE_NVMF_WR_TYPE_FC_REQUEST,
	XNVME_BE_NVMF_WR_TYPE_SEND,
	XNVME_BE_NVMF_WR_TYPE_RECV,
	XNVME_BE_NVMF_WR_TYPE_MAX = XNVME_BE_NVMF_WR_TYPE_RECV,
};

struct xnvme_be_nvmf_rdma_qpair {
	struct xnvme_be_nvmf_qpair base;
	enum xnvme_nvmf_rdmacm_state rdma_qp_state;
	struct rdma_cm_id *cm_id;
	struct ibv_mr *send_mr;
	struct ibv_mr *recv_mr;
	struct ibv_cq *send_cq;
	struct ibv_cq *recv_cq;
	void *send_buffer;
	void *recv_buffer;
	struct {
		void *buffer;
		size_t size;
		struct ibv_mr *mr;
	} internal; /* Internal buffer space for the RDMA QPair */
	struct ibv_qp_init_attr qp_init_attr;
};

struct xnvme_be_nvmf_rdma_ctrlr {
	struct xnvme_be_nvmf_ctrlr base;
	pthread_mutex_t lock;           ///< Controller lock for thread-safe operations
	struct rdma_addrinfo *res;      ///< Resolved address information array for the controller
	struct rdma_addrinfo *selected; ///< Selected address information for the controller
	struct rdma_event_channel *event_channel;
	struct ibv_pd *pd;
};

struct xnvme_be_nvmf_wr_id {
	union {
		struct {
			uint64_t index : 12;
			uint64_t type  : 4;
			uint64_t rsvd  : 48;
		};
		uint64_t raw;
	};
};

/* Functions defined in xnvme_be_nvmf_rdma_qpair.c, used by rdma_ctrlr.c */
int
_handle_rdmacm_event(struct rdma_cm_event *event);

/* Function defined in xnvme_be_nvmf_rdma_ctrlr.c, used by rdma.c */
int
xnvme_be_nvmf_create_rdma_controller(struct xnvme_be_nvmf_ctrlr **ctrlr);

int
xnvme_be_nvmf_create_rdma_qpair(struct xnvme_be_nvmf_ctrlr *ctrlr, struct xnvme_be_nvmf_qpair_attr *attr,
				struct xnvme_be_nvmf_qpair **qpair);

#endif /* _INTERNAL_XNVME_BE_NVMF_RDMA_H */