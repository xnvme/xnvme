#ifndef _INTERNAL_XNVME_BE_NVMF_FABRIC_H
#define _INTERNAL_XNVME_BE_NVMF_FABRIC_H

int
xnvme_be_nvmf_initialize_remote_ctrlr(struct xnvme_be_nvmf_ctrlr *ctrlr);

int 
xnvme_be_nvmf_send_fabric_connect_command(struct xnvme_be_nvmf_qpair *qpair);

#endif /* _INTERNAL_XNVME_BE_NVMF_FABRIC_H */