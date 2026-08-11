// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_UPCIE_H
#define __INTERNAL_XNVME_BE_UPCIE_H
#include <pthread.h>

#include <xnvme_be.h>
#include <xnvme_queue.h>

#define _UPCIE_WITH_NVME
#include <upcie/upcie.h>

/**
 * Default heap size used for the host DMA heap (upcie) and the GPU device heap
 * (upcie-cuda) when xnvme_opts leaves the size unset (0)
 */
#define XNVME_BE_UPCIE_DEFAULT_HEAP_SIZE (1024ULL * 1024 * 1024)

struct xnvme_queue_upcie {
	struct xnvme_queue_base base;
	struct nvme_qpair qpair;
	size_t sq_offset;  ///< Heap offset of the qpair's SQ, for delete
	size_t cq_offset;  ///< Heap offset of the qpair's CQ, for delete
	size_t prp_offset; ///< Heap offset of the qpair's per-request PRP scratch, for delete
	uint8_t _rvds[144];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_upcie) == XNVME_BE_QUEUE_STATE_NBYTES,
		    "Incorrect size")

/**
 * How the target is attached, and with it how the DMA heap is wired up.
 *
 * Decided at ctrlr_init from the kernel driver bound to the BDF.
 */
enum xnvme_be_upcie_mode {
	XNVME_BE_UPCIE_MODE_UNSET = 0,
	XNVME_BE_UPCIE_MODE_VFIO_CDEV,  ///< vfio-cdev + iommufd + memfd
	XNVME_BE_UPCIE_MODE_UIO_LUT,    ///< uio_pci_generic + hugepages + LUT translator
	XNVME_BE_UPCIE_MODE_VFIO_TYPE1, ///< vfio-pci + legacy type1 container
};

/**
 * Shared controller state, one per physical controller, managed by cref.
 */
struct xnvme_be_upcie_ctrlr {
	struct nvme_controller *ctrl;
	struct nvme_dmamem_vfio_ctx ctx;        ///< vfio-cdev + iommufd state (VFIO_CDEV mode)
	struct nvme_dmamem_uio_ctx uio_ctx;     ///< pci_bar_map state    (UIO_LUT mode)
	struct nvme_dmamem_type1_ctx type1_ctx; ///< type1 device fd state (VFIO_TYPE1 mode)
	struct vfio_group type1_group;          ///< Owned per-controller in VFIO_TYPE1 mode
	int type1_group_attached;               ///< Whether type1_group is set_container'd
	struct nvme_qpair sync; ///< Shared submission/completion queue for synchronous IOs
	size_t sync_sq_offset;  ///< Heap offset of the sync qpair's SQ
	size_t sync_cq_offset;  ///< Heap offset of the sync qpair's CQ
	size_t sync_prp_offset; ///< Heap offset of the sync qpair's per-request PRP scratch
};

/**
 * Per-device state embedded in xnvme_dev.be.state.
 * The first field (ctrlr) is the cref handle written by the platform.
 */
struct xnvme_be_upcie_state {
	struct xnvme_be_upcie_ctrlr *ctrlr; ///< Shared controller (first field for platform)

	uint8_t _rvds[120];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_upcie_state) == XNVME_BE_STATE_NBYTES, "Incorrect size")

/**
 * State used across multiple instances of controllers/namespaces
 *
 * One dmamem_heap regardless of mode, so every controller allocates queues,
 * PRP scratch and data buffers from the same offset space. It sits on a
 * hostmem_hugepage whose borrowed phys_lut resolves addresses at submit time.
 */
struct xnvme_be_upcie_rte {
	enum xnvme_be_upcie_mode mode;
	struct iommufd iommufd; ///< VFIO_CDEV mode
	int iommufd_alive;      ///< Whether the iommufd handle is open
	int ioas_alive;         ///< Whether an IOAS is allocated on it
	struct hostmem_config config;
	struct hostmem_hugepage hp;
	struct dmamem dmem;
	struct dmamem_heap heap;
	struct vfio_container
		type1_container;   ///< VFIO_TYPE1 mode; shared, first ctrlr sets the iommu
	int type1_container_alive; ///< Whether the container is open
	int type1_iommu_set;       ///< Whether VFIO_SET_IOMMU has been applied to it
	int hp_alive;              ///< Whether hp holds an allocation
	int dmem_alive;            ///< Whether dmem wraps hp
	int heap_alive;            ///< Whether heap is initialized
	int is_initialized;
};

extern struct xnvme_be_upcie_rte g_upcie_rte;

extern struct xnvme_be_mem g_xnvme_be_upcie_mem;
extern struct xnvme_be_admin g_xnvme_be_upcie_admin;
extern struct xnvme_be_sync g_xnvme_be_upcie_sync;
extern struct xnvme_be_async g_xnvme_be_upcie_async;
extern struct xnvme_be_dev g_xnvme_be_upcie_dev;

int
xnvme_be_upcie_get_driver_name(const char *bdf, char *driver_name, size_t driver_name_len);

// Used by xnvme_be_upcie_cuda_dev.c
void
xnvme_be_upcie_dev_close(struct xnvme_dev *dev);
int
xnvme_be_upcie_dev_open(struct xnvme_dev *dev);
void *
xnvme_be_upcie_ctrlr_init(struct xnvme_dev *dev);
int
xnvme_be_upcie_ctrlr_term(void *handle);

// Used by xnvme_be_upcie_cuda_async.c
int
xnvme_be_upcie_queue_init(struct xnvme_queue *queue, int opts);
int
xnvme_be_upcie_queue_term(struct xnvme_queue *queue);
int
xnvme_be_upcie_queue_poke(struct xnvme_queue *queue, uint32_t max);
#endif /* __INTERNAL_XNVME_BE_UPCIE */
