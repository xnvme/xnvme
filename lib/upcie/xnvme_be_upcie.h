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

struct xnvme_queue_upcie {
	struct xnvme_queue_base base;
	struct nvme_qpair qpair;
	uint8_t _rvds[168];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_upcie) == XNVME_BE_QUEUE_STATE_NBYTES,
		    "Incorrect size")

enum nvme_backend {
	NVME_BACKEND_SYSFS = 0,
	NVME_BACKEND_VFIO,
};

struct xnvme_be_upcie_ctrlr_shm {
	_Atomic int32_t refcount;    ///< Number of processes currently attached
	_Atomic bool is_initialized; ///< Set by primary once the controller is fully opened
	pthread_mutex_t aq_mutex;    ///< Process-shared mutex for admin queue access
	char driver_name[32];
	struct nvme_controller ctrl; ///< Embedded controller; pointer fields use primary's VA
};

/**
 * Shared controller state, one per physical controller, managed by cref.
 */
struct xnvme_be_upcie_ctrlr {
	struct nvme_controller *ctrl;
	struct nvme_qpair sync; ///< Shared submission/completion queue for synchronous IOs
	struct vfio_ctx vfio;
	enum nvme_backend backend;

	char lock_name[64];
	int lock_fd;

	char shm_name[64];
	int shm_fd;
	struct xnvme_be_upcie_ctrlr_shm *shm;
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
 * Shared information about hugepages
 */
struct xnvme_be_upcie_mproc_shm {
	char hugepage_path[256]; ///< Path to primary's hugepage file
	uint64_t hugepage_base;  ///< Primary's hugepage virtual base for secondary pointer fixup
	_Atomic int refcount;    ///< Number of processes currently attached
	_Atomic bool is_initialized;
};

/**
 * Multi-process state
 */
struct xnvme_be_upcie_mproc {
	bool is_primary; ///< If true, this process is owner of the shared memory

	char lock_name[64];
	int lock_fd;

	char shm_name[64];
	int shm_fd;
	struct xnvme_be_upcie_mproc_shm *shm;

	struct hostmem_hugepage *primary_hugepage; ///< Imported hugepage for admin queue
};

/**
 * State used across multiple instances of controllers/namespaces
 */
struct xnvme_be_upcie_rte {
	struct hostmem_config config;
	struct hostmem_heap heap;
	struct xnvme_be_upcie_mproc *mproc; ///< will be NULL if not in multi-process mode
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

// Used for multi-process mode
int
xnvme_be_upcie_mproc_rte_init(int shm_id);
void
xnvme_be_upcie_mproc_rte_term();
int
xnvme_be_upcie_mproc_import_admin_hugepage();

void
xnvme_be_upcie_ctrlr_mutex_lock(struct xnvme_be_upcie_ctrlr *ctrlr);
void
xnvme_be_upcie_ctrlr_mutex_unlock(struct xnvme_be_upcie_ctrlr *ctrlr);

int
xnvme_be_upcie_mproc_ctrlr_shm_init(struct xnvme_dev *dev, struct xnvme_be_upcie_ctrlr *ctrlr,
				    char *driver_name);
int
xnvme_be_upcie_mproc_ctrlr_shm_attach(struct xnvme_dev *dev, struct xnvme_be_upcie_ctrlr *ctrlr);
void
xnvme_be_upcie_mproc_ctrlr_shm_term(struct xnvme_be_upcie_ctrlr *ctrlr);

int
xnvme_be_upcie_mproc_create_or_delete_io_qpair(struct xnvme_be_upcie_ctrlr *ctrlr,
					       struct nvme_qpair *qpair, uint16_t depth,
					       bool create);

#endif /* __INTERNAL_XNVME_BE_UPCIE */
