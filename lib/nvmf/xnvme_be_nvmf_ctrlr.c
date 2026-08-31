#include <errno.h>
#include <xnvme_dev.h>
#include <xnvme_be_nvmf.h>

int
xnvme_be_nvmf_ctrlr_create(struct xnvme_be_nvmf_transport *transport,
	struct xnvme_be_nvmf_ctrlr_attr *attr,
	struct xnvme_be_nvmf_ctrlr **ctrlr)
{
    struct xnvme_be_nvmf_ctrlr *tmp = NULL;
    struct xnvme_be_nvmf_qpair_attr admin_attr = {
        .qid = XNVME_BE_NVMF_ADMIN_QUEUE_ID,
        .qsize = 8,
        .capsule_size = NVME_CMD_CAPSULE_SIZE,
        .completion_size = NVME_CPL_CAPSULE_SIZE,
    };

    if (!transport || !attr || !ctrlr) {
        return -EINVAL;
    }

    if (!attr->dev) {
        return -EINVAL;
    }

	int err = transport->ops.create_ctrlr(&tmp);
	if (err) {
        return err;
    }

    pthread_mutex_init(&tmp->lock, NULL);
    tmp->ctrlr_id = attr->ctrlr_id;
    tmp->dev = attr->dev;
    tmp->transport = transport;
	tmp->ctrlr_state = XNVME_NVMF_CTRLR_STATE_INIT;
	tmp->discovery_ctrlr = attr->dev->opts.nsid == 0 ? 1 : 0;
    tmp->last_allocated_queue_id = XNVME_BE_NVMF_IO_QUEUE_ID_START;
	tmp->attached = 0;

    XNVME_DEBUG("INFO: ctrlr->discovery_ctrlr set to %d based on dev->opts.nsid=%u",
	    tmp->discovery_ctrlr, attr->dev->opts.nsid);

    err = xnvme_be_nvmf_qpair_create(tmp, &admin_attr,
					      &tmp->admin_qpair);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_nvmf_qpair_create() for admin_qpair, err: %d",
			    err);
        free(tmp);
        return err;
    }

	*ctrlr = tmp;
	return err;
}