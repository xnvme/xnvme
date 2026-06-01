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

#define XNVME_BE_UPCIE_NQUEUES_MAX   64
#define XNVME_BE_UPCIE_QSIZE_MAX     1024
#define XNVME_BE_UPCIE_MAX_HUGEPAGES 128

/**
 * Per-queue metadata stored in POSIX shm for secondary processes to reconstruct
 * local queue state without sending admin commands.
 */
struct xnvme_be_upcie_qinfo {
	uint32_t qid;
	uint16_t depth;
	uint16_t sq_tail;   ///< Saved SQ tail, restored on next claim
	uint64_t sq_offset; ///< Byte offset of SQ ring from hugepage base
	uint64_t cq_offset; ///< Byte offset of CQ ring from hugepage base
	uint16_t cq_head;   ///< Saved CQ head, restored on next claim
	uint8_t cq_phase;   ///< Saved CQ phase bit, restored on next claim
	uint8_t _pad[5];
};

/**
 * Shared multi-process rendezvous for the upcie backend.
 *
 * Stored in a POSIX shared memory object named /xnvme-upcie-{sanitized-bdf}.
 * The primary creates all I/O queue pairs upfront from its DMA heap (hugepage).
 * Secondary processes import the hugepage, reconstruct local queue state, and
 * claim queue pairs atomically via qidmap without submitting admin commands.
 *
 * The phys_lut array stores the physical address of each hugepage backing the
 * heap. Secondary processes use these to create IOMMU mappings for their data
 * buffers and to identify queue ring physical addresses.
 */
struct xnvme_be_upcie_mproc {
	_Atomic uint64_t qidmap;  ///< Bit N set means queue ID N is available
	_Atomic int32_t refcount; ///< Number of processes currently attached
	int nqueues;              ///< Number of pre-allocated I/O queue pairs
	char hugepage_path[256];  ///< Path to primary's hugepage file
	size_t hugepage_size;     ///< Total hugepage allocation size in bytes
	uint32_t nphys;           ///< Number of hugepages backing the heap
	uint32_t _pad;
	uint64_t phys_lut[XNVME_BE_UPCIE_MAX_HUGEPAGES]; ///< Physical address per hugepage
	struct xnvme_be_upcie_qinfo queues[XNVME_BE_UPCIE_NQUEUES_MAX]; ///< Index 0 unused

	///< Identify data cached by the primary so secondaries can derive geometry
	///< without issuing admin commands. id_ready is set to 1 atomically after all
	///< fields below have been written.
	_Atomic uint8_t id_ready;
	uint8_t csi; ///< Command Set Identifier determined by the primary
	uint8_t _pad_id[2];
	uint32_t nsid; ///< Namespace ID for id_ns / idcss_ns
	struct xnvme_spec_idfy_ctrlr id_ctrlr;
	struct xnvme_spec_idfy_ns id_ns;
	struct xnvme_spec_idfy_ctrlr idcss_ctrlr;
	struct xnvme_spec_idfy_ns idcss_ns;
};

struct xnvme_queue_upcie {
	struct xnvme_queue_base base;
	struct nvme_qpair qpair;
	uint8_t borrowed_ring; ///< Ring memory is borrowed from primary's hugepage
	uint8_t _rvds[167];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_upcie) == XNVME_BE_QUEUE_STATE_NBYTES,
		    "Incorrect size")

enum nvme_backend {
	NVME_BACKEND_SYSFS = 0,
	NVME_BACKEND_VFIO,
};

/**
 * Shared controller state, one per physical controller, managed by cref.
 *
 * When shm_fd >= 0 the process is primary and owns the POSIX shm.
 * When shm_fd == -1 the process is a secondary that has attached to shm.
 * When shm_fd == -2 the process is in single-process (auto) mode with no shm.
 */
struct xnvme_be_upcie_ctrlr {
	struct nvme_controller *ctrl;
	struct nvme_qpair sync; ///< Synchronous I/O queue pair
	struct vfio_ctx vfio;
	enum nvme_backend backend;
	struct xnvme_be_upcie_mproc *mproc;        ///< Shared rendezvous (in POSIX shm)
	int shm_fd;                                ///< >=0 primary, -1 secondary, -2 auto
	char shm_name[64];                         ///< POSIX shm name, set at init/attach time
	struct hostmem_hugepage imported_hugepage; ///< Secondary: imported primary hugepage
	struct nvme_qpair *preallocated;           ///< Primary: pre-allocated queue pairs
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
 */
struct xnvme_be_upcie_rte {
	struct hostmem_config config;
	struct hostmem_heap heap;
	int is_initialized;
};

extern struct xnvme_be_upcie_rte g_upcie_rte;

extern struct xnvme_be_mem g_xnvme_be_upcie_mem;
extern struct xnvme_be_admin g_xnvme_be_upcie_admin;
extern struct xnvme_be_sync g_xnvme_be_upcie_sync;
extern struct xnvme_be_async g_xnvme_be_upcie_async;
extern struct xnvme_be_dev g_xnvme_be_upcie_dev;

int
_upcie_claim_qpair(struct xnvme_be_upcie_ctrlr *ctrlr, void *ring_base, struct nvme_qpair *dest);

void
_upcie_release_qpair(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_qpair *qp);

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
