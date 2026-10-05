#ifndef _INTERNAL_XNVME_BE_NVMF_DISCOVERY_H
#define _INTERNAL_XNVME_BE_NVMF_DISCOVERY_H

int
xnvme_be_nvmf_get_discovery_log(struct xnvme_be_nvmf_ctrlr *ctrlr,
				struct xnvme_be_nvmf_qpair *admin_qpair,
				struct xnvme_spec_discovery_log_page *log_page);

#endif // _INTERNAL_XNVME_BE_NVMF_DISCOVERY_H
