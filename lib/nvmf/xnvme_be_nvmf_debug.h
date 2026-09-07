#ifndef _INTERNAL_XNVME_BE_NVMF_DEBUG_H
#define _INTERNAL_XNVME_BE_NVMF_DEBUG_H

#include <libxnvme.h>

int
_xnvme_print_error_code(struct xnvme_spec_cpl *cpl);

static inline void
_hexdump_range(void *buf, size_t len)
{
	for (size_t i = 0; i < len; ++i) {
		if (i % 16 == 0) {
			printf("\n%08zx: ", i);
		}
		printf("%02x ", ((unsigned char *)buf)[i]);
	}
	printf("\n");
}

static inline void 
_print_nvme_completion(struct xnvme_spec_cpl *cpl)
{
	XNVME_DEBUG("INFO: NVMe Completion - cid: %u, sc: %u, sct: %u",
		    cpl->cid, cpl->status.sc, cpl->status.sct);
	if (cpl->status.sc || cpl->status.sct) {
		XNVME_DEBUG("INFO: NVMe Completion indicates an error");
		_xnvme_print_error_code(cpl);
	}
	
}

#endif // _INTERNAL_XNVME_BE_NVMF_DEBUG_H