// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_TRANSPORT_CTRLR_H
#define __INTERNAL_XNVME_TRANSPORT_CTRLR_H

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

#include <xnvme_transport.h>

struct xnvme_transport_ctrlr_ops;
struct xnvme_transport_qpair_attr;

enum xnvme_transport_ctrlr_state {
	XNVME_TRANSPORT_CTRLR_STATE_INVALID = 0,
	XNVME_TRANSPORT_CTRLR_STATE_INIT,         // allocated
	XNVME_TRANSPORT_CTRLR_STATE_INITIALIZING, // initializing
	XNVME_TRANSPORT_CTRLR_STATE_CONNECTING,
	XNVME_TRANSPORT_CTRLR_STATE_CONNECTED,
	XNVME_TRANSPORT_CTRLR_STATE_DISCONNECTED,
	XNVME_TRANSPORT_CTRLR_STATE_ERROR,
	XNVME_TRANSPORT_CTRLR_STATE_MAX = XNVME_TRANSPORT_CTRLR_STATE_ERROR,
};

struct xnvme_transport_ctrlr_attr {
	uint8_t ctrlr_id;      ///< Controller ID for this NVMe-oF controller
	struct xnvme_dev *dev; ///< Pointer to the underlying xNVMe device
};

struct xnvme_transport_ctrlr {
	const struct xnvme_transport_ctrlr_ops *ops;
	enum xnvme_transport_ctrlr_state ctrlr_state; ///< Connection state of the controller
	struct xnvme_transport_qpair *admin_qpair;
	int last_allocated_queue_id;
	int last_assigned_discovery_id;
	SLIST_HEAD(, xnvme_transport_subsys)
	subsystems; ///< List of subsystems associated with this controller

	// candidates for 'flags'
	uint8_t attached;
};

/**
 * Glue-facing entry points, wired as `xnvme_be_dev.ctrlr_init` / `ctrlr_term`.
 */
void *
xnvme_transport_dev_ctrlr_init(struct xnvme_dev *dev, struct xnvme_transport *transport);

int
xnvme_transport_dev_ctrlr_term(void *ctrlr);

/**
 * Core ctrlr lifecycle.
 *
 * `ctrlr_create` composes the transport `ctrlr_probe` with the rest of the
 * controller setup (admin qpair). `ctrlr_destroy` is its
 * counterpart, composing `ctrlr_teardown` and `ctrlr_free`. `ctrlr_connect`
 * performs the transport-level connect followed by the admin qpair connect;
 * `ctrlr_disconnect` reverses it.
 */
int
xnvme_transport_ctrlr_create(struct xnvme_transport *transport,
			     struct xnvme_transport_ctrlr_attr *attr,
			     struct xnvme_transport_ctrlr **ctrlr);

int
xnvme_transport_ctrlr_destroy(struct xnvme_transport_ctrlr *ctrlr);

int
xnvme_transport_ctrlr_connect(struct xnvme_transport_ctrlr *ctrlr, const char *uri);

int
xnvme_transport_ctrlr_disconnect(struct xnvme_transport_ctrlr *ctrlr);

/**
 * Probes every registered transport for @dev->ident.uri, retrying up to
 * `XNVME_TRANSPORT_MAX_PROBE_ATTEMPTS` rounds. On success `*ctrlr` is a
 * connected controller. Returns 0 or a negative errno.
 */
int
xnvme_transport_ctrlr_probe(struct xnvme_dev *dev, struct xnvme_transport *transport,
			    struct xnvme_transport_ctrlr **ctrlr);

/**
 * Sets CC.EN and waits on CSTS.RDY, through Fabrics Property Get and Set on
 * the admin qpair.
 */
int
xnvme_transport_ctrlr_enable(struct xnvme_transport_ctrlr *ctrlr);

/**
 * Registers/deregisters @buf with the ctrlr's transport (PD-backed for
 * RDMA), forwarding to `ops->ctrlr_reg` / `ops->ctrlr_dereg`.
 */
int
xnvme_transport_ctrlr_reg(struct xnvme_transport_ctrlr *ctrlr, void *buf, size_t nbytes,
			  void **handle, uint32_t *key);

int
xnvme_transport_ctrlr_dereg(struct xnvme_transport_ctrlr *ctrlr, void *handle);

struct xnvme_transport_subsys *
xnvme_transport_ctrlr_find_first_subsys(struct xnvme_transport_ctrlr *ctrlr, const char *subnqn);

struct xnvme_transport_subsys *
xnvme_transport_ctrlr_find_first_subsys_nsidx(struct xnvme_transport_ctrlr *ctrlr, int nsidx);
#endif /* __INTERNAL_XNVME_TRANSPORT_CTRLR_H */