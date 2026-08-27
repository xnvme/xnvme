// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_UPCIE_H
#define __INTERNAL_XNVME_BE_UPCIE_H
#include <pthread.h>
#include <stdatomic.h>

#include <xnvme_be.h>
#include <xnvme_queue.h>

#define _UPCIE_WITH_NVME
#include <upcie/upcie.h>

/**
 * Default heap size used for the host DMA heap (upcie) and the GPU device heap
 * (upcie-cuda) when xnvme_opts leaves the size unset (0)
 */
#define XNVME_BE_UPCIE_DEFAULT_HEAP_SIZE (1024ULL * 1024 * 1024)

/**
 * Heap offsets of a queue-pair's allocations
 *
 * Recorded at create so delete can hand the same offsets back to the heap.
 */
struct xnvme_be_upcie_qpair_offsets {
	size_t sq;  ///< Heap offset of the SQ
	size_t cq;  ///< Heap offset of the CQ
	size_t prp; ///< Heap offset of the per-request PRP scratch
};

struct xnvme_queue_upcie {
	struct xnvme_queue_base base;
	struct nvme_qpair qpair;
	struct xnvme_be_upcie_qpair_offsets offsets;
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
 * Per-controller shared segment
 *
 * One per physical controller, created by the primary. Embeds the full
 * struct nvme_controller so secondaries can attach without re-initializing
 * the device. Pointer fields inside the embedded controller reference the
 * primary's virtual address space; secondaries fix them up on attach by
 * the constant offset between their imported hugepage base and the
 * primary's published base.
 */
struct xnvme_be_upcie_ctrlr_shm {
	_Atomic int32_t refcount;    ///< Number of processes currently attached
	_Atomic bool is_initialized; ///< Set by primary once the controller is fully opened
	pthread_mutex_t aq_mutex;    ///< Process-shared mutex for admin queue access
	char driver_name[32];
	struct xnvme_be_upcie_qpair_offsets sync_offsets; ///< Heap offsets of the sync qpair
	struct nvme_controller ctrl; ///< Embedded controller; pointer fields use primary's VA
};

/**
 * How one controller is attached, one member per mode
 *
 * Which member carries state follows xnvme_be_upcie_rte.mode; the rest stay
 * zeroed. Grouped so the controller struct says "attachment" once rather
 * than interleaving three modes' bookkeeping with everything else.
 */
struct xnvme_be_upcie_ctrlr_attach {
	struct nvme_dmamem_vfio_ctx vfio;   ///< VFIO_CDEV: vfio-cdev + iommufd state
	struct nvme_dmamem_uio_ctx uio;     ///< UIO_LUT: pci_bar_map state
	struct nvme_dmamem_type1_ctx type1; ///< VFIO_TYPE1: device fd state
	struct vfio_group type1_group;      ///< VFIO_TYPE1: owned per controller
	int type1_group_attached;           ///< Whether type1_group is set_container'd
};

/**
 * Multi-process bookkeeping for one controller
 *
 * All of it is inert outside multi-process mode, where `shm` is NULL.
 */
struct xnvme_be_upcie_ctrlr_mproc {
	char lock_name[128];                  ///< Per-BDF primary-election lock path
	int lock_fd;                          ///< Owned by primary while it holds the controller
	char shm_name[64];                    ///< POSIX shm name for the per-controller segment
	int shm_fd;                           ///< Owned by primary; -1 in secondaries
	struct xnvme_be_upcie_ctrlr_shm *shm; ///< Per-controller shm (NULL outside mproc)
	size_t aq_rpool_prp_offset;           ///< Heap offset of a secondary's admin-rpool PRPs
};

/**
 * Shared controller state, one per physical controller, managed by cref.
 */
struct xnvme_be_upcie_ctrlr {
	struct nvme_controller *ctrl;
	struct xnvme_be_upcie_ctrlr_attach attach;
	struct nvme_qpair sync; ///< Shared submission/completion queue for synchronous IOs
	struct xnvme_be_upcie_qpair_offsets sync_offsets; ///< Heap offsets of the sync qpair
	struct xnvme_be_upcie_ctrlr_mproc mproc;
};

/**
 * One controller's iommu-map-pa handle
 *
 * IOMMU_IOAS_MAP_FILE rejects the dma-bufs CUDA and HIP export, so device memory
 * reaches the controller's domain through the out-of-tree iommu-map-pa module
 * instead. See <upcie/dmamem_iommu_map_pa.h>.
 */
struct xnvme_be_upcie_gpu_map {
	struct dmamem_iommu_map_pa imp;
	char bdf[DMAMEM_IOMMU_MAP_PA_BDF_LEN]; ///< Device whose domain is mapped into
	int alive;                             ///< Whether imp is open
	int slice;                             ///< Window slice held; -1 when none
};

/**
 * One controller's view of the GPU heap
 *
 * The heap is process-wide; the IOVAs it is reached by are per controller, so
 * each needs a table of its own.
 */
struct xnvme_be_upcie_gpu_dmem {
	struct xnvme_be_upcie_gpu_map map; ///< Borrowed by dmem; outlives it
	struct dmamem dmem;
};

/**
 * Per-device state embedded in xnvme_dev.be.state.
 *
 * The first field (ctrlr) is the cref handle written by the platform. The
 * second says where data buffers for this device live: the host heap for
 * `upcie`, the GPU heap for `upcie-cuda` and `upcie-hip`, assigned by the
 * backend's dev_open. That is what lets the admin, sync and async paths be
 * shared across the three, since they differ only in which dmamem a payload
 * translates through.
 */
struct xnvme_be_upcie_state {
	struct xnvme_be_upcie_ctrlr *ctrlr;  ///< Shared controller (first field for platform)
	struct dmamem *dmem;                 ///< Where this device's data buffers live
	struct xnvme_be_upcie_gpu_dmem *gpu; ///< Owned; NULL unless an IOMMU enforces for a GPU

	uint8_t _rvds[104];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_upcie_state) == XNVME_BE_STATE_NBYTES, "Incorrect size")

/**
 * Per-runtime shared segment (one per shm_id)
 *
 * Created by the primary. Carries the primary's hugepage backing-file path
 * and virtual base so secondaries can import the same memory and reach the
 * admin queue by a constant VA offset. The refcount is advisory.
 */
struct xnvme_be_upcie_mproc_shm {
	char hugepage_path[256]; ///< Path to primary's hugepage file
	uint64_t hugepage_base;  ///< Primary's hugepage virtual base for secondary pointer fixup
	_Atomic int refcount;    ///< Number of processes currently attached
	_Atomic bool is_initialized;
};

/**
 * Per-process multi-process state
 *
 * Populated by xnvme_be_upcie_mproc_rte_init when opts->shm_id != 0.
 * is_primary is decided by an advisory flock keyed on shm_id.
 */
struct xnvme_be_upcie_mproc {
	bool is_primary; ///< If true, this process owns the shared state

	char lock_name[64];
	int lock_fd;

	char shm_name[64];
	int shm_fd;
	struct xnvme_be_upcie_mproc_shm *shm;

	struct hostmem_hugepage *primary_hugepage; ///< Imported hugepage in a secondary
};

/**
 * State used across multiple instances of controllers/namespaces
 *
 * One dmamem_heap regardless of mode, so every controller allocates queues,
 * PRP scratch and data buffers from the same offset space. It sits on a
 * hostmem_hugepage whose borrowed phys_lut resolves addresses at submit time.
 */
/** VFIO_CDEV mode: one iommufd handle with one IOAS, shared by all controllers */
struct xnvme_be_upcie_rte_cdev {
	struct iommufd iommufd;
	int iommufd_alive; ///< Whether the iommufd handle is open
	int ioas_alive;    ///< Whether an IOAS is allocated on it
};

/** VFIO_TYPE1 mode: one legacy container, the first controller sets the iommu */
struct xnvme_be_upcie_rte_type1 {
	struct vfio_container container;
	int container_alive; ///< Whether the container is open
	int iommu_set;       ///< Whether VFIO_SET_IOMMU has been applied to it
};

/**
 * The DMA memory every controller allocates from, whatever the mode
 *
 * The `_alive` flags say how far bring-up got, so teardown unwinds exactly
 * what was set up and in the reverse order.
 */
struct xnvme_be_upcie_rte_mem {
	struct hostmem_config config;
	struct hostmem_hugepage hp;
	struct dmamem dmem;
	struct dmamem_heap heap;
	int hp_alive;   ///< Whether hp holds an allocation
	int dmem_alive; ///< Whether dmem wraps hp
	int heap_alive; ///< Whether heap is initialized
};

struct xnvme_be_upcie_rte {
	enum xnvme_be_upcie_mode mode;
	struct xnvme_be_upcie_rte_cdev cdev;
	struct xnvme_be_upcie_rte_type1 type1;
	struct xnvme_be_upcie_rte_mem mem;
	struct xnvme_be_upcie_mproc *mproc; ///< NULL when not in multi-process mode
	int is_initialized;
};

extern struct xnvme_be_upcie_rte g_upcie_rte;

extern struct xnvme_be_mem g_xnvme_be_upcie_mem;
extern struct xnvme_be_admin g_xnvme_be_upcie_admin;
extern struct xnvme_be_sync g_xnvme_be_upcie_sync;
extern struct xnvme_be_async g_xnvme_be_upcie_async;
extern struct xnvme_be_dev g_xnvme_be_upcie_dev;

// Attachment: driver probing, mode selection and vfio wiring (xnvme_be_upcie_vfio.c)
int
xnvme_be_upcie_get_driver_name(const char *bdf, char *driver_name, size_t driver_name_len);

int
xnvme_be_upcie_resolve_vfio_cdev(const char *bdf, char *cdev_path, size_t cdev_path_len);

int
xnvme_be_upcie_mode_from_driver(const char *bdf, const char *driver_name,
				enum xnvme_be_upcie_mode *mode, char *cdev_path,
				size_t cdev_path_len);

int
xnvme_be_upcie_type1_attach(struct xnvme_be_upcie_ctrlr *ctrlr, const char *bdf);

// Runtime and device lifecycle (xnvme_be_upcie_dev.c)
/**
 * Address-space width the DMA-address table is sized for; 0 for the default.
 *
 * Read from XNVME_UPCIE_VA_BITS. See the definition for what it costs and when
 * lowering it is warranted.
 */
int
xnvme_be_upcie_va_bits(void);

/** Whether device memory needs mapping for an IOMMU; false under UIO_LUT */
int
xnvme_be_upcie_gpu_map_required(void);

/**
 * Open this controller's iommu-map-pa handle on a slice of the IOVA window
 *
 * A mapping reaches one IOMMU domain, so each controller needs a slice of its
 * own.
 *
 * Call it once the controller has attached: the IOAS reports the ranges it
 * enforces only when it knows the device's reserved regions. Close it before the
 * controller detaches, since that replaces the domain the mappings live in.
 *
 * @param map  Caller-allocated handle to fill
 * @param bdf  The controller whose domain to map into
 * @param span Bytes needed for the heap; the slice is twice this, leaving room
 *             for xnvme_mem_map(). XNVME_UPCIE_GPU_IOVA_SLICE overrides it.
 *
 * @return 0 on success, whether or not a handle was opened; negative errno on
 *         failure. -ENOSPC when the window is out of slices.
 */
int
xnvme_be_upcie_gpu_map_open(struct xnvme_be_upcie_gpu_map *map, const char *bdf, uint64_t span);

void
xnvme_be_upcie_gpu_map_close(struct xnvme_be_upcie_gpu_map *map);

void
xnvme_be_upcie_dev_close(struct xnvme_dev *dev);
int
xnvme_be_upcie_dev_open(struct xnvme_dev *dev);
void *
xnvme_be_upcie_ctrlr_init(struct xnvme_dev *dev);
int
xnvme_be_upcie_ctrlr_term(void *handle);

/**
 * The admin, sync and async paths, shared by `upcie`, `upcie-cuda` and `upcie-hip`
 *
 * They translate payloads through the per-device dmamem in
 * xnvme_be_upcie_state, so the same implementation serves host and device
 * memory. The GPU backends wire these into their own xnvme_be_{admin,sync,
 * async} in xnvme_be_upcie_{cuda,hip}.c.
 */
int
xnvme_be_upcie_dmamem_map(struct dmamem *dmem, void *vaddr, size_t nbytes, uint64_t *phys);

int
xnvme_be_upcie_dmamem_unmap(struct dmamem *dmem, void *vaddr);

int
xnvme_be_upcie_queue_init(struct xnvme_queue *queue, int opts);
int
xnvme_be_upcie_queue_term(struct xnvme_queue *queue);
int
xnvme_be_upcie_queue_poke(struct xnvme_queue *queue, uint32_t max);
int
xnvme_be_upcie_async_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			    size_t mbuf_nbytes);
int
xnvme_be_upcie_async_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			     size_t dvec_nbytes, void *mbuf, size_t mbuf_nbytes);
int
xnvme_be_upcie_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t mbuf_nbytes);
int
xnvme_be_upcie_sync_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			    size_t dvec_nbytes, void *mbuf, size_t mbuf_nbytes);
int
xnvme_be_upcie_sync_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			      void *mbuf, size_t mbuf_nbytes);
int
xnvme_be_upcie_sync_cmd_pseudo(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			       void *mbuf, size_t mbuf_nbytes);

// Multi-process runtime bring-up / teardown
int
xnvme_be_upcie_mproc_rte_init(int shm_id);
void
xnvme_be_upcie_mproc_rte_term(void);
int
xnvme_be_upcie_mproc_import_admin_hugepage(void);

// Admin-queue mutex; no-op when not in multi-process mode
int
xnvme_be_upcie_ctrlr_mutex_lock(struct xnvme_be_upcie_ctrlr *ctrlr);
void
xnvme_be_upcie_ctrlr_mutex_unlock(struct xnvme_be_upcie_ctrlr *ctrlr);

// Per-controller shared segment for the primary/secondary handshake
int
xnvme_be_upcie_mproc_ctrlr_shm_init(struct xnvme_dev *dev, struct xnvme_be_upcie_ctrlr *ctrlr,
				    const char *driver_name);
int
xnvme_be_upcie_mproc_ctrlr_shm_attach(struct xnvme_dev *dev, struct xnvme_be_upcie_ctrlr *ctrlr);
void
xnvme_be_upcie_mproc_ctrlr_shm_term(struct xnvme_be_upcie_ctrlr *ctrlr);
void
xnvme_be_upcie_mproc_free_all_queues(struct xnvme_be_upcie_ctrlr *ctrlr);

// qids-bitmap lock; used by GPU-initiated queue create/destroy
int
xnvme_be_upcie_mproc_qids_lock(struct xnvme_be_upcie_ctrlr *ctrlr);
void
xnvme_be_upcie_mproc_qids_unlock(struct xnvme_be_upcie_ctrlr *ctrlr);

// Mutex-guarded IO qpair create/delete on the dmamem heap
int
xnvme_be_upcie_mproc_create_io_qpair(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_qpair *qpair,
				     uint16_t depth, struct xnvme_be_upcie_qpair_offsets *offsets);
void
xnvme_be_upcie_mproc_delete_io_qpair(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_qpair *qpair,
				     const struct xnvme_be_upcie_qpair_offsets *offsets);

#endif /* __INTERNAL_XNVME_BE_UPCIE */
