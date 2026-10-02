#ifndef _INTERNAL_XNVME_BE_NVMF_QPAIR_H
#define _INTERNAL_XNVME_BE_NVMF_QPAIR_H

#include <stddef.h>
#include <xnvme_be_nvmf_ctrlr.h>
#include <xnvme_be_nvmf_req.h>

struct xnvme_be_nvmf_qpair;
struct xnvme_be_nvmf_transport_ops;
struct xnvme_spec_cpl;

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

struct xnvme_be_nvmf_qpair_attr {
	uint16_t qid;
	uint16_t qsize;
	uint32_t capsule_size;
	uint32_t completion_size;
};

struct xnvme_be_nvmf_qpair {
	const struct xnvme_be_nvmf_transport_ops *ops;
	enum xnvme_nvmf_qpair_state state;
	struct xnvme_be_nvmf_qpair_attr attr;
	uint16_t cntlid; /* assigned by the controller in the Fabric Connect response */
	struct xnvme_be_nvmf_req_pool *req_pool;
	struct xnvme_be_nvmf_ctrlr *ctrlr;
	struct xnvme_dev *dev; ///< Pointer to the underlying xNVMe device
};

/**
 * Core ctrlr/qpair lifecycle and the hot-path submit/poll wrappers.
 *
 * connect / disconnect / destroy
 *   Manage the lifetime of the underlying transport connection. On connect the
 *   transport must pre-post receive buffers before returning. These are owned
 *   by the transport; the protocol layer never calls post_recv for them.
 *
 * qpair_submit
 *   Post one capsule (capsule, nbytes) onto the send queue, with @cid going
 *   into the transport work request identifier. The caller may reuse the
 *   buffer as soon as `qpair_submit` returns. The transport does not
 *   interpret the bytes.
 *
 * qpair_poll
 *   Drive the transport's completion machinery. For every received response
 *   capsule it calls `xnvme_be_nvmf_qpair_complete()`, the only up-call from
 *   the transport into the core. Returns the number of
 *   completions processed, or a negative errno on error.
 */

int
xnvme_be_nvmf_qpair_create(struct xnvme_be_nvmf_ctrlr *ctrlr, struct xnvme_dev *dev,
			   struct xnvme_be_nvmf_qpair_attr *attr,
			   struct xnvme_be_nvmf_qpair **qpair);

int
xnvme_be_nvmf_qpair_connect(struct xnvme_be_nvmf_qpair *qpair);
int
xnvme_be_nvmf_qpair_disconnect(struct xnvme_be_nvmf_qpair *qpair);
int
xnvme_be_nvmf_qpair_destroy(struct xnvme_be_nvmf_qpair *qpair);

/**
 * The only up-call from the transport into the core.
 * Looks up the request by `cpl->cid` in `qpair->req_pool`, copies the
 * completion into the request's `xnvme_cmd_ctx.cpl`, and marks the request
 * done. Returns 0, or a negative errno for an unknown or inactive CID.
 */
int
xnvme_be_nvmf_qpair_complete(struct xnvme_be_nvmf_qpair *qpair, const struct xnvme_spec_cpl *cpl);

static inline int
xnvme_be_nvmf_cmd_iov(struct xnvme_be_nvmf_qpair *qpair, struct xnvme_cmd_ctx *ctx,
		      struct iovec *dvec, size_t dvec_cnt, size_t dvec_nbytes, struct iovec *mvec,
		      size_t mvec_cnt, size_t mvec_nbyte)
{
	return qpair->ops->cmd_iov(qpair, ctx, dvec, dvec_cnt, dvec_nbytes, mvec, mvec_cnt,
				   mvec_nbyte);
}

static inline int
xnvme_be_nvmf_cmd_io(struct xnvme_be_nvmf_qpair *qpair, struct xnvme_cmd_ctx *ctx, void *dbuf,
		     size_t dbuf_nbytes, void *mbuf, size_t mbuf_nbytes)
{
	return qpair->ops->cmd_io(qpair, ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes);
}

/**
 * Posts one command capsule (@capsule, @nbytes) with @cid, the hot-path
 * data-plane entry. @cid goes into the transport work request identifier.
 */
static inline int
xnvme_be_nvmf_qpair_submit(struct xnvme_be_nvmf_qpair *qpair, const void *capsule, size_t nbytes,
			   uint16_t cid)
{
	return qpair->ops->qpair_submit(qpair, capsule, nbytes, cid);
}

/**
 * Processes up to @max completions, the hot-path data-plane entry. Returns
 * the number of completions processed, or a negative errno.
 */
static inline int
xnvme_be_nvmf_qpair_poll(struct xnvme_be_nvmf_qpair *qpair, uint32_t max)
{
	return qpair->ops->qpair_poll(qpair, max);
}

static inline bool
xnvme_be_nvmf_qpair_has_error(struct xnvme_be_nvmf_qpair *qpair)
{
	return qpair->state == XNVME_NVMF_QPAIR_STATE_ERROR;
}

static inline void
xnvme_be_nvmf_wait_for_completion(struct xnvme_be_nvmf_qpair *qpair, struct xnvme_be_nvmf_req *req)
{
	while (!xnvme_be_nvmf_req_is_complete(req)) {
		if (xnvme_be_nvmf_req_has_error(req))
			break;

		if (xnvme_be_nvmf_qpair_has_error(qpair))
			break;

		xnvme_be_nvmf_qpair_poll(qpair, 1);
	}
}
#endif /* _INTERNAL_XNVME_BE_NVMF_QPAIR_H */