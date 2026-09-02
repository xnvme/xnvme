#ifndef _INTERNAL_XNVME_BE_NVMF_CTRLR_H
#define _INTERNAL_XNVME_BE_NVMF_CTRLR_H

#include <stdint.h>
#include <pthread.h>

struct xnvme_be_nvmf_ctrlr;

struct xnvme_be_nvmf_transport_ops {
	int (*create_ctrlr)(struct xnvme_be_nvmf_ctrlr **ctrlr);
};

struct xnvme_be_nvmf_transport {
	const char *name;
	struct xnvme_be_nvmf_transport_ops ops;
};

struct xnvme_be_nvmf_ctrlr_ops;
struct xnvme_be_nvmf_qpair_attr;

enum xnvme_nvmf_ctrlr_state {
	XNVME_NVMF_CTRLR_STATE_INVALID = 0,
	XNVME_NVMF_CTRLR_STATE_INIT,         // allocated
	XNVME_NVMF_CTRLR_STATE_INITIALIZING, // initializing
	XNVME_NVMF_CTRLR_STATE_CONNECTING,
	XNVME_NVMF_CTRLR_STATE_CONNECTED,
	XNVME_NVMF_CTRLR_STATE_DISCONNECTED,
	XNVME_NVMF_CTRLR_STATE_ERROR,
	XNVME_NVMF_CTRLR_STATE_MAX = XNVME_NVMF_CTRLR_STATE_ERROR,
};

struct xnvme_be_nvmf_ctrlr_attr {
	uint8_t ctrlr_id; ///< Controller ID for this NVMe-oF controller
	struct xnvme_dev *dev; ///< Pointer to the underlying xNVMe device	
};

struct xnvme_be_nvmf_ctrlr {
	struct xnvme_be_nvmf_ctrlr_ops *ops;
	pthread_mutex_t lock;
	uint8_t ctrlr_id;                     ///< Controller ID for this device
	struct xnvme_dev *dev; ///< Pointer to the underlying xNVMe device
	struct xnvme_be_nvmf_transport *transport; ///< Transport used by the NVMe-oF controller
	enum xnvme_nvmf_ctrlr_state ctrlr_state; ///< Connection state of the controller
	struct xnvme_be_nvmf_qpair *admin_qpair;
	struct xnvme_be_nvmf_qpair *sync_qpair;
	int last_allocated_queue_id;

	// candidates for 'flags'
	uint8_t attached;
	uint8_t discovery_ctrlr;
};

struct xnvme_be_nvmf_ctrlr_ops {
	int (*connect)(struct xnvme_be_nvmf_ctrlr *ctrlr, const char *uri);
	int (*disconnect)(struct xnvme_be_nvmf_ctrlr *ctrlr);
	int (*destroy)(struct xnvme_be_nvmf_ctrlr *ctrlr);

	int (*create_qpair)(struct xnvme_be_nvmf_ctrlr *ctrlr, struct xnvme_be_nvmf_qpair_attr *attr,
			    struct xnvme_be_nvmf_qpair **qpair);
	int (*process_events)(struct xnvme_be_nvmf_ctrlr *ctrlr, int timeout_ms);
};

int
xnvme_be_nvmf_ctrlr_create(struct xnvme_be_nvmf_transport *transport,
	struct xnvme_be_nvmf_ctrlr_attr *attr,
	struct xnvme_be_nvmf_ctrlr **ctrlr);

static inline int
xnvme_be_nvmf_ctrlr_disconnect(struct xnvme_be_nvmf_ctrlr *ctrlr)
{
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: NULL ctrlr");
		return -EINVAL;
	}

	if (ctrlr->ops && ctrlr->ops->disconnect) {
		return ctrlr->ops->disconnect(ctrlr);
	}

	XNVME_DEBUG("FAILED: No disconnect operation defined for controller");
	return -ENOSYS;
}

static inline int
xnvme_be_nvmf_ctrlr_destroy(struct xnvme_be_nvmf_ctrlr *ctrlr)
{
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: NULL ctrlr");
		return -EINVAL;
	}

	if (ctrlr->ops && ctrlr->ops->destroy) {
		return ctrlr->ops->destroy(ctrlr);
	}

	free(ctrlr);

	return 0;
}

static inline int
xnvme_be_nvmf_ctrlr_connect(struct xnvme_be_nvmf_ctrlr *ctrlr, const char *uri)
{
	int err;
	if (!ctrlr || !uri)
		return -EINVAL;

	err = ctrlr->ops->connect(ctrlr, uri);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_nvmf_ctrlr_connect(), err: %d", err);
		return err;
	}

	ctrlr->ctrlr_state = XNVME_NVMF_CTRLR_STATE_CONNECTED;
	ctrlr->attached = 1;

	return err;
}


static inline int
xnvme_be_nvmf_ctrlr_process_events(struct xnvme_be_nvmf_ctrlr *ctrlr, int timeout_ms)
{
	return ctrlr->ops->process_events(ctrlr, timeout_ms);
}

#endif /* _INTERNAL_XNVME_BE_NVMF_CTRLR_H */