#ifndef _INTERNAL_XNVME_BE_NVMF_CTRLR_H
#define _INTERNAL_XNVME_BE_NVMF_CTRLR_H

#include <stdint.h>

struct xnvme_be_nvmf_ctrlr_ops;

struct xnvme_be_nvmf_ctrlr {
	struct xnvme_be_nvmf_ctrlr_ops *ops;
	uint8_t ctrlr_id; ///< Controller ID for this device

	// candidates for 'flags'
	uint8_t attached;
	uint8_t discovery_ctrlr;
};

struct xnvme_be_nvmf_ctrlr_ops {
	int (*connect)(struct xnvme_be_nvmf_ctrlr *ctrlr, const char *uri);
	int (*disconnect)(struct xnvme_be_nvmf_ctrlr *ctrlr);
	int (*destroy)(struct xnvme_be_nvmf_ctrlr *ctrlr);
};

#endif /* _INTERNAL_XNVME_BE_NVMF_CTRLR_H */