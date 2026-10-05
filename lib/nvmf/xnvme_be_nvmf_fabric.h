#ifndef _INTERNAL_XNVME_BE_NVMF_FABRIC_H
#define _INTERNAL_XNVME_BE_NVMF_FABRIC_H

#include <stdint.h>

#include <xnvme_be_nvmf_ctrlr.h>
#include <xnvme_be_nvmf_qpair.h>

/**
 * Sets CC.EN and waits on CSTS.RDY through Fabrics Property Get and Set on
 * @admin_qpair. Backs `xnvme_be_nvmf_ctrlr_enable()`.
 */
int
xnvme_be_nvmf_fabric_enable(struct xnvme_be_nvmf_ctrlr *ctrlr,
			    struct xnvme_be_nvmf_qpair *admin_qpair, bool is_discovery);

/**
 * Sends the Fabrics Connect command on @qpair, the core half of
 * `xnvme_be_nvmf_qpair_connect()`. The transport connect must already have
 * reached `CONNECTED`.
 */
int
xnvme_be_nvmf_fabric_connect(struct xnvme_be_nvmf_qpair *qpair);

static inline void
xnvme_be_nvmf_keyed_sgl_init(struct xnvme_spec_sgl_descriptor *sgl, void *buf, size_t len,
			     uint64_t key)
{
	assert(sgl != NULL);
	assert(buf != NULL);
	assert(len < (1UL << 24)); // Ensure length fits within 24 bits for the SGL descriptor
	assert(key < (1UL << 32)); // Ensure key fits within 64 bits for the SGL descriptor

	// Set the SGL descriptor type and subtype
	sgl->keyed.type    = XNVME_SPEC_SGL_DESCR_TYPE_KEYED_DATA_BLOCK;
	sgl->keyed.subtype = XNVME_SPEC_SGL_DESCR_SUBTYPE_ADDRESS;

	// set the SGL descriptor to point to the internal buffer
	sgl->addr      = (uintptr_t)buf;
	sgl->keyed.len = len;
	sgl->keyed.key = key;
}

#if 0  // Determine whether to expose property get/set functions
/**
 * Fabrics Property Get / Set on @admin_qpair. @property is one of the
 * `XNVME_SPEC_FABRIC_PROP_*` offsets.
 */
int
xnvme_be_nvmf_fabric_prop_get(struct xnvme_be_nvmf_ctrlr *ctrlr,
			      struct xnvme_be_nvmf_qpair *admin_qpair, uint32_t property,
			      uint64_t *value);

int
xnvme_be_nvmf_fabric_prop_set(struct xnvme_be_nvmf_ctrlr *ctrlr,
			      struct xnvme_be_nvmf_qpair *admin_qpair, uint32_t property,
			      uint64_t value);
#endif // Determine whether to expose property get/set functions

#endif /* _INTERNAL_XNVME_BE_NVMF_FABRIC_H */