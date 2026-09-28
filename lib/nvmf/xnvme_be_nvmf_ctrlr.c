#include <stdatomic.h>
#include <errno.h>

#include <xnvme_dev.h>
#include <xnvme_be_nvmf.h>

#define _NVMF_DEBUG(fmt,...) NVMF_DEBUG(NVMF_DEBUG_CATEGORY_CORE_CTRLR, fmt, ##__VA_ARGS__)
#define _NVMF_INFO(fmt,...) NVMF_INFO(NVMF_DEBUG_CATEGORY_CORE_CTRLR, fmt, ##__VA_ARGS__)
#define _NVMF_WARN(fmt,...) NVMF_WARN(NVMF_DEBUG_CATEGORY_CORE_CTRLR, fmt, ##__VA_ARGS__)
#define _NVMF_ERROR(fmt,...) NVMF_ERROR(NVMF_DEBUG_CATEGORY_CORE_CTRLR, fmt, ##__VA_ARGS__)
#define _NVMF_TRACE(fmt,...) NVMF_TRACE(NVMF_DEBUG_CATEGORY_CORE_CTRLR, fmt, ##__VA_ARGS__)

/**
 * Maximum number of attempts to probe for a device matching the provided URI.
 *
 * This is used to retry probing for a device in case of transient errors or
 * delays in the NVMe-oF subsystem.
 */
#define XNVME_BE_NVMF_MAX_PROBE_ATTEMPTS 3

#define FOR_EACH_NVMF_TRANSPORT(transport)                                          \
	for (struct xnvme_be_nvmf_transport **transport =                           \
		     (struct xnvme_be_nvmf_transport **)g_xnvme_be_nvmf_transports; \
	     *transport != NULL; ++transport)

atomic_int g_xnvme_be_nvmf_ctrlr_id_counter = ATOMIC_VAR_INIT(0);
extern struct xnvme_be_nvmf_transport g_xnvme_be_nvmf_rdma_transport;

struct xnvme_be_nvmf_transport *g_xnvme_be_nvmf_transports[] = {&g_xnvme_be_nvmf_rdma_transport,
								NULL};

static inline int
xnvme_be_nvmf_ctrlr_connect(struct xnvme_be_nvmf_ctrlr *ctrlr, const char *uri)
{
	int err;
	if (!ctrlr || !uri)
		return -EINVAL;

	err = ctrlr->ops->connect(ctrlr, uri);
	if (err) {
		_NVMF_ERROR("FAILED: xnvme_be_nvmf_ctrlr_connect(), err: %d", err);
		ctrlr->ctrlr_state = XNVME_NVMF_CTRLR_STATE_ERROR;
		return err;
	}

	ctrlr->ctrlr_state = XNVME_NVMF_CTRLR_STATE_CONNECTED;
	ctrlr->attached = 1;

	return err;
}
                                
static inline int
xnvme_be_nvmf_ctrlr_disconnect(struct xnvme_be_nvmf_ctrlr *ctrlr)
{
	if (!ctrlr) {
		_NVMF_ERROR("FAILED: NULL ctrlr");
		return -EINVAL;
	}

	if (ctrlr->ops && ctrlr->ops->disconnect) {
		return ctrlr->ops->disconnect(ctrlr);
	}

	_NVMF_ERROR("FAILED: No disconnect operation defined for controller");
	return -ENOSYS;
}


static int
xnvme_be_nvmf_ctrlr_create(struct xnvme_be_nvmf_transport *transport,
	struct xnvme_be_nvmf_ctrlr_attr *attr,
	struct xnvme_be_nvmf_ctrlr **ctrlr)
{
    struct xnvme_be_nvmf_ctrlr *tmp = NULL;
    struct xnvme_be_nvmf_qpair_attr default_qpair_attr = {
        .qid = XNVME_BE_NVMF_ADMIN_QUEUE_ID,
        .qsize = 8,
        .capsule_size = NVME_CMD_CAPSULE_SIZE,
        .completion_size = NVME_CPL_CAPSULE_SIZE,
    };

    if (!transport || !attr || !ctrlr) {
        _NVMF_ERROR("FAILED: Invalid arguments");
        return -EINVAL;
    }

    if (!attr->dev) {
        _NVMF_ERROR("FAILED: Invalid device in controller attributes");
        return -EINVAL;
    }

	int err = transport->ops.create_ctrlr(&tmp);
	if (err) {
        _NVMF_ERROR("FAILED: transport->ops.create_ctrlr(), err: %d", err);
        return err;
    }

    pthread_mutex_init(&tmp->lock, NULL);
    tmp->ctrlr_id = attr->ctrlr_id;
    tmp->transport = transport;
	tmp->ctrlr_state = XNVME_NVMF_CTRLR_STATE_INIT;
	tmp->discovery_ctrlr = strlen(attr->dev->ident.subnqn) == 0 ? 1 : 0;
    tmp->last_allocated_queue_id = XNVME_BE_NVMF_IO_QUEUE_ID_START;
	tmp->attached = 0;
    
    _NVMF_INFO("INFO: ctrlr->discovery_ctrlr set to %d based on dev->ident.subnqn=\"%s\"",
	    tmp->discovery_ctrlr, attr->dev->ident.subnqn);

    err = xnvme_be_nvmf_qpair_create(tmp, attr->dev, &default_qpair_attr,
					      &tmp->admin_qpair);
	if (err) {
		_NVMF_ERROR("FAILED: xnvme_be_nvmf_qpair_create() for admin_qpair, err: %d",
			    err);
        free(tmp);
        return err;
    }

	*ctrlr = tmp;
	return err;
}

static inline int
xnvme_be_nvmf_ctrlr_destroy(struct xnvme_be_nvmf_ctrlr *ctrlr)
{
	if (ctrlr->ops && ctrlr->ops->destroy) {
		return ctrlr->ops->destroy(ctrlr);
	}

	free(ctrlr);

	return 0;
}

static inline int
xnvme_be_nvmf_transport_probe(struct xnvme_be_nvmf_transport *transport, 
	struct xnvme_be_nvmf_ctrlr_attr *attr, struct xnvme_be_nvmf_ctrlr **ctrlr)
{
	struct xnvme_be_nvmf_ctrlr *tmp_ctrlr;
	int err;

	err = xnvme_be_nvmf_ctrlr_create(transport, attr, &tmp_ctrlr);
	if (err) {
		_NVMF_ERROR("FAILED: transport->ops->create_ctrlr(), err: %d", err);
		return err;
	}

	err = xnvme_be_nvmf_ctrlr_connect(tmp_ctrlr, attr->dev->ident.uri);
	if (err) {
		_NVMF_ERROR("FAILED: xnvme_be_nvmf_ctrlr_connect(), err: %d", err);
		goto destroy_controller;
	}

	*ctrlr = tmp_ctrlr;

	return 0;

destroy_controller:
	xnvme_be_nvmf_ctrlr_destroy(tmp_ctrlr);
	return err;
}

static inline void 
_dump_ctrlr(struct xnvme_be_nvmf_ctrlr *ctrlr)
{
	_NVMF_DEBUG("INFO: ctrlr: \n" \
		"\tctrlr_id: %d\n" \
		"\tctrlr_state: %d" \
		"\tattached: %d\n" \
		"\tdiscovery_ctrlr: %d",
		    ctrlr->admin_qpair->cntlid, ctrlr->ctrlr_state,
		    ctrlr->attached, ctrlr->discovery_ctrlr);
}

/**
 * Initialize a new controller for the device URI.
 *
 * Inits the environment, if needed, and probes for a device matching the URI.
 */
void *
xnvme_be_nvmf_ctrlr_init(struct xnvme_dev *dev)
{
	struct xnvme_be_nvmf_state *state = (void *)dev->be.state;
	struct xnvme_be_nvmf_ctrlr *ctrlr = NULL;
	struct xnvme_be_nvmf_ctrlr_attr attr = {
		.dev = dev,
	};
	int ctrlr_id;
	int err;

	_NVMF_INFO("INFO: ctrlr_init() for NVMe-oF device: %s, ns=%u, discovery=%s",
		    dev->ident.uri, dev->ident.nsid, dev->ident.nsid == 0 ? "yes" : "no");

	if (state->ctrlr) {
		_NVMF_INFO("INFO: Controller already initialized, reusing existing controller");
		return state->ctrlr;
	}

	ctrlr_id = atomic_fetch_add(&g_xnvme_be_nvmf_ctrlr_id_counter, 1);
	// _NVMF_INFO("INFO: Assigned controller ID: %d", ctrlr_id);

	attr.ctrlr_id = ctrlr_id;

	// Probe the uri to check if the device is reachable and supports NVMe-oF.
	for (int i = 0; !ctrlr; ++i) {
		// If the maximum number of attempts is reached, return an error.
		if (XNVME_BE_NVMF_MAX_PROBE_ATTEMPTS == i) {
			_NVMF_ERROR("FAILED: max attempts exceeded");
			errno = ENXIO;
			goto free_ctrlr_id;
		}

		FOR_EACH_NVMF_TRANSPORT(transport)
		{
			_NVMF_INFO("INFO: Attempting to probe transport: %s", (*transport)->name);

			err = xnvme_be_nvmf_transport_probe(*transport, &attr, &ctrlr);
			if (!err) {
				_NVMF_INFO("INFO: Successfully connected to transport: %s",
					    dev->ident.uri);
				_NVMF_INFO("INFO: transport->probe() successful, device is "
					    "reachable and supports NVMe-oF");
				break;
			} else {
				_NVMF_INFO("INFO: transport->probe() failed for transport: %s, "
					    "err: %d",
					    (*transport)->name, err);
			}
		}
	}

	if (!ctrlr) {
		_NVMF_ERROR("FAILED: No transport could connect to the device: %s",
			    dev->ident.uri);
		errno = ENXIO;
		goto free_ctrlr_id;
	}

	_dump_ctrlr(ctrlr);

	_NVMF_INFO("INFO: ctrlr_init() OK");
	return ctrlr;

free_ctrlr_id:
	// attempt to free the controller ID if we failed to allocate a controller.
	// If this fails, then we don't know what ID should be free, and we should leave it alone.
	// TODO: Implement as a bitmask or find something more comprehensive to reallocate
	// controller IDs
	atomic_compare_exchange_strong(&g_xnvme_be_nvmf_ctrlr_id_counter, &ctrlr_id, ctrlr_id - 1);
	return NULL;
}

int
xnvme_be_nvmf_ctrlr_term(void *ctrlr)
{
	struct xnvme_be_nvmf_ctrlr *nvmf_ctrlr = (void *)ctrlr;
	int err;

	_NVMF_INFO("INFO: ctrlr_term() for NVMe-oF controller");

	if (nvmf_ctrlr) {
		if (nvmf_ctrlr->ctrlr_state == XNVME_NVMF_CTRLR_STATE_CONNECTED) {
			err = xnvme_be_nvmf_ctrlr_disconnect(nvmf_ctrlr);
			if (err) {
				_NVMF_ERROR("FAILED: xnvme_be_nvmf_ctrlr_disconnect(), err: %d",
					    err);
				return err;
			}
		}

		err = xnvme_be_nvmf_ctrlr_destroy(nvmf_ctrlr);
		if (err) {
			_NVMF_ERROR("FAILED: xnvme_be_nvmf_ctrlr_destroy(), err: %d", err);
			return err;
		}

		free(nvmf_ctrlr);
	}

	return 0;
}