#ifndef _INTERNAL_XNVME_BE_NVMF_RDMA_H
#define _INTERNAL_XNVME_BE_NVMF_RDMA_H

#include <rdma/rdma_cma.h>

#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_ctrlr.h>
#include <xnvme_be_nvmf_qpair.h>
#include <xnvme_be_nvmf_transport.h>

#define TO_XNVME_NVMF_RDMA_QPAIR(qpair) \
	container_of((qpair), struct xnvme_be_nvmf_rdma_qpair, base)

#define TO_XNVME_NVMF_RDMA_CTRLR(ctrlr) \
	container_of((ctrlr), struct xnvme_be_nvmf_rdma_ctrlr, base)

#define XNVME_BE_NVMF_MAX_RDMACM_TIMEOUT_MS 2000

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
	struct rdma_event_channel *event_channel; ///< Per-qpair rdma_cm event channel
	struct rdma_cm_id *cm_id;
	struct ibv_mr *send_mr;
	struct ibv_mr *recv_mr;
	struct ibv_cq *send_cq;
	struct ibv_cq *recv_cq;
	void *send_buffer;
	void *recv_buffer;
	struct ibv_qp_init_attr qp_init_attr;
};

struct xnvme_be_nvmf_rdma_ctrlr {
	struct xnvme_be_nvmf_ctrlr base;
	struct rdma_addrinfo *res;      ///< Resolved address information array for the controller
	struct rdma_addrinfo *selected; ///< Selected address information for the controller
	struct ibv_pd *pd; ///< Allocated once, on the admin qpair's transport connect, and reused
			   ///< by every later qpair
};

struct xnvme_be_nvmf_wr_id {
	union {
		struct {
			uint64_t rsvd  : 48;
			uint64_t type  : 4;
			uint64_t index : 12;
		};
		uint64_t raw;
	};
};

/*
 * Bottom half (xnvme_be_nvmf_rdma_cm.c): drives the qpair's rdma_cm event
 * channel. Used by the top half's qpair_connect / qpair_disconnect
 * (xnvme_be_nvmf_rdma.c).
 */
int
_process_qpair_cm_events(struct xnvme_be_nvmf_qpair *qpair, int timeout_ms);

/*
 * Bottom half (xnvme_be_nvmf_rdma_verbs.c): drains one CQ's worth of
 * completions. Used by the top half's qpair_poll (xnvme_be_nvmf_rdma.c).
 */
int
_process_send_completions(struct xnvme_be_nvmf_qpair *qpair, int max_completions);
int
_process_recv_completions(struct xnvme_be_nvmf_qpair *qpair, int max_completions);

/*
 * Top half (xnvme_be_nvmf_rdma.c): the only up-call the bottom half is
 * allowed to reach, folding the length check, the qpair-state check, and the
 * call to xnvme_be_nvmf_qpair_complete() that used to live in the deleted
 * on_capsule_recv callback. Called from the bottom half's
 * xnvme_be_nvmf_rdma_verbs.c _handle_recv_cmpl().
 */
int
xnvme_be_nvmf_rdma_top_recv_complete(struct xnvme_be_nvmf_qpair *qpair, void *buf, size_t len);

#endif /* _INTERNAL_XNVME_BE_NVMF_RDMA_H */
