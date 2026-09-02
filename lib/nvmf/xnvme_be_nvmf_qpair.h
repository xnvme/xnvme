#ifndef _INTERNAL_XNVME_BE_NVMF_QPAIR_H
#define _INTERNAL_XNVME_BE_NVMF_QPAIR_H

#include <stddef.h>
#include <xnvme_be_nvmf_ctrlr.h>

struct xnvme_be_nvmf_qpair;
struct xnvme_be_nvmf_qpair_ops;

enum xnvme_nvmf_qpair_state {
	XNVME_NVMF_QPAIR_STATE_INVALID = 0,
	XNVME_NVMF_QPAIR_STATE_INIT,
	XNVME_NVMF_QPAIR_STATE_CONNECTING,
	XNVME_NVMF_QPAIR_STATE_CONNECTED, /* transport up; Fabric Connect sent, awaiting response
					   */
	XNVME_NVMF_QPAIR_STATE_READY, /* Fabric Connect Response received; queue ready for I/O */
	XNVME_NVMF_QPAIR_STATE_DISCONNECTED,
	XNVME_NVMF_QPAIR_STATE_ERROR,
	XNVME_NVMF_QPAIR_STATE_MAX = XNVME_NVMF_QPAIR_STATE_ERROR,
};

/**
 * on_capsule_recv  -- a capsule arrived; buf/len are valid until the callback returns.
 *                     Fires in transport delivery order, which is NOT guaranteed to match
 *                     command posting order; the protocol layer must match on CID.
 *
 * on_send_cmpl     -- the transport is done with buf; the caller may reclaim it.
 *                     buf echoes the pointer from send_capsule, serving as the identifier.
 *                     Fires in posting order for both RDMA (RC QP CQ ordering) and TCP
 *                     (sequential send path). Does NOT indicate the peer processed the
 *                     command; use on_capsule_recv and CID matching for that.
 *
 * on_state_change  -- the transport-level state transitioned to @state.
 */
typedef void (*xnvme_be_nvmf_capsule_recv_fn)(struct xnvme_be_nvmf_qpair *qpair, void *buf,
					      size_t len);
typedef void (*xnvme_be_nvmf_send_cmpl_fn)(struct xnvme_be_nvmf_qpair *qpair, void *buf,
					   int status);
typedef void (*xnvme_be_nvmf_state_change_fn)(struct xnvme_be_nvmf_qpair *qpair,
					      enum xnvme_nvmf_qpair_state state, void *ctx);

struct xnvme_be_nvmf_qpair_attr {
	uint16_t qid;
	uint16_t qsize;
	uint32_t capsule_size;
	uint32_t completion_size; 
};

struct xnvme_be_nvmf_qpair {
	struct xnvme_be_nvmf_qpair_ops *ops;
	enum xnvme_nvmf_qpair_state state;
	struct xnvme_be_nvmf_qpair_attr attr;
	uint16_t cntlid; /* assigned by the controller in the Fabric Connect response */
	struct xnvme_be_nvmf_req_pool *req_pool;
	struct xnvme_be_nvmf_ctrlr *ctrlr;

	xnvme_be_nvmf_capsule_recv_fn on_capsule_recv;
	xnvme_be_nvmf_send_cmpl_fn on_send_cmpl;
	xnvme_be_nvmf_state_change_fn on_state_change;
};

/**
 * Transport-level ops table. Each entry covers exactly one transport
 * primitive; nothing above the capsule boundary belongs here.
 *
 * connect / disconnect / destroy
 *   Manage the lifetime of the underlying transport connection. On connect the
 *   transport must pre-post receive buffers before returning. These are owned 
 *   by the transport; the protocol layer never calls post_recv for them. When 
 *   a reserve buffer is consumed the transport re-posts it before invoking 
 *   on_capsule_recv so the pool never drains. 
 *
 * send_capsule
 *   Place one capsule (buf, len) onto the send queue. The caller owns the
 *   buffer until on_send_cmpl fires. The transport does not interpret the
 *   bytes.
 *
 * post_recv
 *   Hand a receive buffer to the transport for the normal command-response
 *   path. The transport fills it and fires on_capsule_recv when a capsule
 *   arrives. The caller owns the buffer after the callback returns. Must not
 *   be used to manage the async reserve; that is transport-internal.
 *
 * process_completions
 *   Drive the transport's completion machinery. Fires the registered callbacks
 *   for every completed send and every received capsule. Returns the number
 *   of completions processed, or a negative errno on error.
 */
struct xnvme_be_nvmf_qpair_ops {
	int (*connect)(struct xnvme_be_nvmf_qpair *qpair);
	int (*disconnect)(struct xnvme_be_nvmf_qpair *qpair);
	int (*destroy)(struct xnvme_be_nvmf_qpair *qpair);

	int (*send_capsule)(struct xnvme_be_nvmf_qpair *qpair, void *buf, size_t len);
	int (*post_recv)(struct xnvme_be_nvmf_qpair *qpair, void *buf, size_t len);

	int (*process_completions)(struct xnvme_be_nvmf_qpair *qpair, int max_completions);
};

int
xnvme_be_nvmf_qpair_create(struct xnvme_be_nvmf_ctrlr *ctrlr, struct xnvme_be_nvmf_qpair_attr *attr,
			   struct xnvme_be_nvmf_qpair **qpair);

int
xnvme_be_nvmf_connect_qpair(struct xnvme_be_nvmf_qpair *qpair);
int
xnvme_be_nvmf_disconnect_qpair(struct xnvme_be_nvmf_qpair *qpair);
int
xnvme_be_nvmf_destroy_qpair(struct xnvme_be_nvmf_qpair *qpair);

static inline int
xnvme_be_nvmf_qpair_send_capsule(struct xnvme_be_nvmf_qpair *qpair, void *buf, size_t len)
{
	return qpair->ops->send_capsule(qpair, buf, len);
}

static inline int
xnvme_be_nvmf_qpair_post_recv(struct xnvme_be_nvmf_qpair *qpair, void *buf, size_t len)
{
	return qpair->ops->post_recv(qpair, buf, len);
}

static inline int
xnvme_be_nvmf_qpair_process_completions(struct xnvme_be_nvmf_qpair *qpair, int max_completions)
{
	return qpair->ops->process_completions(qpair, max_completions);
}

static inline void
xnvme_be_nvmf_wait_for_completion(struct xnvme_be_nvmf_qpair *qpair, struct xnvme_be_nvmf_req *req)
{
	while (req->cmpl_type != XNVME_BE_NVMF_REQ_CMPL_TYPE_RECV) {
		if (!req->status)
			break; 

		if (qpair->state == XNVME_NVMF_QPAIR_STATE_ERROR)
			break;
			
		xnvme_be_nvmf_qpair_process_completions(qpair, 1);
	}
}
#endif /* _INTERNAL_XNVME_BE_NVMF_QPAIR_H */