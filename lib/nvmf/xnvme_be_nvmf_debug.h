#ifndef _INTERNAL_XNVME_BE_NVMF_DEBUG_H
#define _INTERNAL_XNVME_BE_NVMF_DEBUG_H

#include <libxnvme.h>

int
_xnvme_print_error_code(struct xnvme_spec_cpl *cpl);

static inline void _print_nvme_completion(struct xnvme_spec_cpl *cpl)
{
	XNVME_DEBUG("INFO: NVMe Completion - cid: %u, sc: %u, sct: %u",
		    cpl->cid, cpl->status.sc, cpl->status.sct);
}

#endif // _INTERNAL_XNVME_BE_NVMF_DEBUG_H