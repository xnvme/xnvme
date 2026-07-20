// SPDX-FileCopyrightText: Simon A. F. Lund
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_DMAMEM_H
#define __INTERNAL_XNVME_BE_DMAMEM_H

#include <xnvme_be.h>
#include <xnvme_queue.h>

#define _UPCIE_WITH_NVME
#include <upcie/upcie.h>

/**
 * Default heap size used for the DMA heap when xnvme_opts leaves the
 * size unset (0). Sits on a hugepage-backed memfd and gets a single
 * contiguous IOVA window through iommufd, so allocations from this
 * heap translate arithmetically at submission.
 */
#define XNVME_BE_DMAMEM_DEFAULT_HEAP_SIZE (1024ULL * 1024 * 1024)

/**
 * Controller-open mode, decided by the kernel driver bound to the NVMe
 * device and (for vfio-pci) which vfio API is available.
 *
 * vfio-pci             -> either VFIO_CDEV (iommufd + memfd hugepage +
 *                         arithmetic translator) if /dev/iommu and a
 *                         vfio-cdev entry both exist, otherwise TYPE1
 *                         (legacy vfio container + hostmem hugepage +
 *                         arithmetic translator). The XNVME_DMAMEM_VFIO_MODE
 *                         env var (iommufd | type1 | auto) overrides the
 *                         default preference which is iommufd.
 * uio_pci_generic      -> UIO_LUT (pci_bar_map + hostmem hugepage +
 *                         LUT translator; requires iommu=pt or iommu=off).
 *
 * The RTE is process-wide so the first controller's mode pins the shape;
 * a later controller with a different mode is rejected.
 */
enum xnvme_be_dmamem_mode {
	XNVME_BE_DMAMEM_MODE_UNSET = 0,
	XNVME_BE_DMAMEM_MODE_VFIO_CDEV,
	XNVME_BE_DMAMEM_MODE_UIO_LUT,
	XNVME_BE_DMAMEM_MODE_VFIO_TYPE1,
};

struct xnvme_queue_dmamem {
	struct xnvme_queue_base base;
	struct nvme_qpair qpair;
	size_t sq_offset;  ///< Heap offset of the qpair's SQ, for delete
	size_t cq_offset;  ///< Heap offset of the qpair's CQ, for delete
	size_t prp_offset; ///< Heap offset of the qpair's per-request PRP scratch, for delete
	uint8_t _rvds[144];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_dmamem) == XNVME_BE_QUEUE_STATE_NBYTES,
		    "Incorrect size")

/**
 * Shared controller state, one per physical controller, managed by cref.
 *
 * ctx / uio_ctx are discriminated by the RTE's mode; only one is live at
 * a time.
 */
struct xnvme_be_dmamem_ctrlr {
	struct nvme_controller *ctrl;
	struct nvme_dmamem_vfio_ctx ctx;        ///< vfio-cdev + iommufd state (VFIO_CDEV mode)
	struct nvme_dmamem_uio_ctx uio_ctx;     ///< pci_bar_map state         (UIO_LUT mode)
	struct nvme_dmamem_type1_ctx type1_ctx; ///< type1 device fd state     (VFIO_TYPE1 mode)
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
struct xnvme_be_dmamem_state {
	struct xnvme_be_dmamem_ctrlr *ctrlr; ///< Shared controller (first field for platform)

	uint8_t _rvds[120];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_dmamem_state) == XNVME_BE_STATE_NBYTES,
		    "Incorrect size")

/**
 * State shared across every controller instance in the process.
 *
 * One dmamem_heap regardless of mode. In VFIO_CDEV mode it sits on a
 * hugepage-backed memfd wired into an iommufd IOAS; every controller
 * attaches into that same IOAS so PRPs translate arithmetically off a
 * shared base_iova. In UIO_LUT mode it sits on a hostmem_hugepage whose
 * borrowed phys_lut resolves PRPs to physical addresses at submit.
 */
struct xnvme_be_dmamem_rte {
	enum xnvme_be_dmamem_mode mode;
	struct iommufd iommufd;       ///< VFIO_CDEV mode
	uint32_t ioas_id;             ///< VFIO_CDEV mode
	struct hostmem_config hp_cfg; ///< UIO_LUT + VFIO_TYPE1 modes
	struct hostmem_hugepage hp;   ///< UIO_LUT + VFIO_TYPE1 modes
	struct vfio_container
		type1_container; ///< VFIO_TYPE1 mode; first ctrlr's iommu is set on it
	int type1_iommu_set;     ///< Whether the container has VFIO_SET_IOMMU applied
	struct dmamem dmem;
	struct dmamem_heap heap;
	int is_initialized;
	int iommufd_alive;
	int ioas_alive;
	int hp_alive;
	int type1_container_alive;
	int dmem_alive;
	int heap_alive;
};

extern struct xnvme_be_dmamem_rte g_dmamem_rte;

extern struct xnvme_be_mem g_xnvme_be_dmamem_mem;
extern struct xnvme_be_admin g_xnvme_be_dmamem_admin;
extern struct xnvme_be_sync g_xnvme_be_dmamem_sync;
extern struct xnvme_be_async g_xnvme_be_dmamem_async;
extern struct xnvme_be_dev g_xnvme_be_dmamem_dev;

int
xnvme_be_dmamem_resolve_vfio_cdev(const char *bdf, char *cdev_path, size_t cdev_path_len);

void
xnvme_be_dmamem_dev_close(struct xnvme_dev *dev);
int
xnvme_be_dmamem_dev_open(struct xnvme_dev *dev);
void *
xnvme_be_dmamem_ctrlr_init(struct xnvme_dev *dev);
int
xnvme_be_dmamem_ctrlr_term(void *handle);

int
xnvme_be_dmamem_queue_init(struct xnvme_queue *queue, int opts);
int
xnvme_be_dmamem_queue_term(struct xnvme_queue *queue);
int
xnvme_be_dmamem_queue_poke(struct xnvme_queue *queue, uint32_t max);

#endif /* __INTERNAL_XNVME_BE_DMAMEM_H */
