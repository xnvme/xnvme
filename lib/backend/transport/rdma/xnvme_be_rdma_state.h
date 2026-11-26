#ifndef __INTERNAL_XNVME_BE_RDMA_STATE_H
#define __INTERNAL_XNVME_BE_RDMA_STATE_H

#include <xnvme_be.h>

struct xnvme_be_rdma_state {
	void *ctrlr; ///< Pointer to attached controller (must be first: platform
		     ///< stores ctrlr at state[0])
	void *subsys;
	void *ns;          ///< Pointer to associated namespace
	void *admin_qpair; ///< Admin queue pair
	void *sync_qpair;  ///< Synchronous IO queue pair
	void *util_rbtree; ///< Pointer to utility red-black tree structure
	void *mem_cache;   ///< Last registered buffer found by a key lookup
	uint8_t _fill[72];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_rdma_state) == XNVME_BE_STATE_NBYTES, "Incorrect size")
XNVME_BE_CTRLR_STATE_FIRST(xnvme_be_rdma_state);

#endif /* __INTERNAL_XNVME_BE_RDMA_STATE_H */
