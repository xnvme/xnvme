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

#include <stddef.h>
#include <stdint.h>
#include <sys/queue.h>

#include <xnvme_be_nvmf_transport.h>

struct xnvme_be_nvmf_transport_ops;
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
	uint8_t ctrlr_id;      ///< Controller ID for this NVMe-oF controller
	struct xnvme_dev *dev; ///< Pointer to the underlying xNVMe device
};

struct xnvme_be_nvmf_ctrlr {
	const struct xnvme_be_nvmf_transport_ops *ops;
	uint8_t ctrlr_id; ///< Controller ID for this device
	// struct xnvme_dev *dev; ///< Pointer to the underlying xNVMe device
	struct xnvme_be_nvmf_transport *transport; ///< Transport used by the NVMe-oF controller
	enum xnvme_nvmf_ctrlr_state ctrlr_state;   ///< Connection state of the controller
	struct xnvme_be_nvmf_qpair *admin_qpair;
	struct xnvme_be_nvmf_qpair *sync_qpair;
	int last_allocated_queue_id;
	int last_assigned_discovery_id;
	SLIST_HEAD(, xnvme_be_nvmf_subsys)
	subsystems; ///< List of subsystems associated with this controller

	// candidates for 'flags'
	uint8_t attached;
	uint8_t discovery_ctrlr;
};

/**
 * Glue-facing entry points, wired as `xnvme_be_dev.ctrlr_init` / `ctrlr_term`.
 *
 * Named with a `dev_` infix so they do not collide with the core-level
 * `xnvme_be_nvmf_ctrlr_init()` / `xnvme_be_nvmf_ctrlr_term()` lifecycle verbs.
 */
void *
xnvme_be_nvmf_dev_ctrlr_init(struct xnvme_dev *dev);

int
xnvme_be_nvmf_dev_ctrlr_term(void *ctrlr);

/**
 * Core ctrlr lifecycle.
 *
 * `ctrlr_create` composes the transport `ctrlr_alloc` and `ctrlr_init` with
 * the rest of the controller setup (admin qpair). `ctrlr_destroy` is its
 * counterpart, composing `ctrlr_teardown` and `ctrlr_free`. `ctrlr_connect`
 * performs the transport-level connect followed by the admin qpair connect;
 * `ctrlr_disconnect` reverses it.
 */
int
xnvme_be_nvmf_ctrlr_create(struct xnvme_be_nvmf_transport *transport,
			   struct xnvme_be_nvmf_ctrlr_attr *attr,
			   struct xnvme_be_nvmf_ctrlr **ctrlr);

int
xnvme_be_nvmf_ctrlr_destroy(struct xnvme_be_nvmf_ctrlr *ctrlr);

int
xnvme_be_nvmf_ctrlr_connect(struct xnvme_be_nvmf_ctrlr *ctrlr, const char *uri);

int
xnvme_be_nvmf_ctrlr_disconnect(struct xnvme_be_nvmf_ctrlr *ctrlr);

/**
 * Probes every registered transport for @dev->ident.uri, retrying up to
 * `XNVME_BE_NVMF_MAX_PROBE_ATTEMPTS` rounds. On success `*ctrlr` is a
 * connected controller. Returns 0 or a negative errno.
 */
int
xnvme_be_nvmf_ctrlr_probe(struct xnvme_dev *dev, struct xnvme_be_nvmf_ctrlr **ctrlr);

/**
 * Sets CC.EN and waits on CSTS.RDY / clears CC.EN, through Fabrics Property
 * Get and Set on the admin qpair.
 */
int
xnvme_be_nvmf_ctrlr_enable(struct xnvme_be_nvmf_ctrlr *ctrlr);

int
xnvme_be_nvmf_ctrlr_disable(struct xnvme_be_nvmf_ctrlr *ctrlr);

/**
 * Registers/deregisters @buf with the ctrlr's transport (PD-backed for
 * RDMA), forwarding to `ops->ctrlr_reg` / `ops->ctrlr_dereg`.
 */
int
xnvme_be_nvmf_ctrlr_reg(struct xnvme_be_nvmf_ctrlr *ctrlr, void *buf, size_t nbytes, void **handle,
			uint32_t *key);

int
xnvme_be_nvmf_ctrlr_dereg(struct xnvme_be_nvmf_ctrlr *ctrlr, void *handle);

struct xnvme_be_nvmf_subsys *
xnvme_be_nvmf_ctrlr_find_first_subsys(struct xnvme_be_nvmf_ctrlr *ctrlr, const char *subnqn);

struct xnvme_be_nvmf_subsys *
xnvme_be_nvmf_ctrlr_find_first_subsys_nsidx(struct xnvme_be_nvmf_ctrlr *ctrlr, int nsidx);
#endif /* _INTERNAL_XNVME_BE_NVMF_CTRLR_H */