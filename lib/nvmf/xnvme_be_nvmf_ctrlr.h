#ifndef _INTERNAL_XNVME_BE_NVMF_CTRLR_H
#define _INTERNAL_XNVME_BE_NVMF_CTRLR_H

/**
 * NVMe-oF controller creation and management functions
 * 
 * A NVMe-of Controller represents a remote NVMe uri/port combination as an endpoint for 
 * the NVMe-oF protocol. It manages the connection, state, and associated resources
 * for communicating with the remote NVMe device. This does not manage anything with subnqns. 
 * 
 * This includes creating and connecting admin and I/O queue pairs, managing controller state,
 * and handling discovery controllers.
 * 
 * @note This is a low-level backend implementation and should be used through the
 *       higher-level xNVMe API.
 */

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
	//struct xnvme_dev *dev; ///< Pointer to the underlying xNVMe device
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

void *
xnvme_be_nvmf_ctrlr_init(struct xnvme_dev *dev);

int
xnvme_be_nvmf_ctrlr_term(void *ctrlr);


static inline int
xnvme_be_nvmf_ctrlr_process_events(struct xnvme_be_nvmf_ctrlr *ctrlr, int timeout_ms)
{
	return ctrlr->ops->process_events(ctrlr, timeout_ms);
}

#endif /* _INTERNAL_XNVME_BE_NVMF_CTRLR_H */