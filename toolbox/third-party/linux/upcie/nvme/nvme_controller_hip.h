// SPDX-License-Identifier: BSD-3-Clause

/**
 * HIP NVMe Controller Extension
 * ==============================
 *
 * This header extends the functionality defined in the uPCIe NVMe Controller
 * header `upcie/nvme/nvme_controller.h` with functions for HIP compatible
 * NVMe controllers.
 *
 * @file nvme_controller_hip.h
 * @version 0.5.0
 */

#include <pthread.h>
#include <time.h>

static inline uint64_t
nvme_hip_now_ns(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

/**
 * NVMe doorbell bridge (host-side)
 * ===============================
 *
 * On GPUs that cannot master MMIO writes to a peer device's BAR (e.g. consumer
 * RDNA parts, where hipHostRegister and hsa_amd_memory_lock both refuse to map
 * the NVMe BAR0 into the GPU), the device kernel cannot ring the doorbell
 * directly. The bridge closes that gap without a kernel module: the kernel
 * rings a GPU-writable shadow cell (host-pinned, zero-copy) and a host poller
 * mirrors that cell to the real BAR0 doorbell. The GPU still owns submission
 * (it builds SQEs in VRAM and decides when to ring); only the 4-byte MMIO is
 * relayed.
 *
 * Mirroring is unconditional and idempotent: the poller repeatedly writes the
 * current shadow value to the doorbell. NVMe doorbells are level-triggered
 * (the controller consumes SQEs up to the written tail), so re-writing the
 * same value is a no-op and no intermediate value can be missed. Ordering is
 * preserved because the kernel's __threadfence_system() makes the SQEs visible
 * before it updates the shadow tail, so any tail the poller observes already
 * has its SQEs in place.
 */
struct nvme_hip_db_bridge {
	volatile uint32_t *cells;     ///< Host view of the shadow cells: [0]=SQ tail, [1]=CQ head
	volatile uint32_t *sqdb_mmio; ///< Real BAR0 SQ doorbell (host mapping)
	volatile uint32_t *cqdb_mmio; ///< Real BAR0 CQ doorbell (host mapping)
	void *sqdb_shadow_dev;        ///< GPU alias of cells[0], stored in qp->sqdb
	void *cqdb_shadow_dev;        ///< GPU alias of cells[1], stored in qp->cqdb
	void *sq_host;                ///< Host view of the SQ (fine-grained coherent), freed on term
	void *cq_real;                ///< Host CQ the controller DMAs to; the CPU sees these writes coherently
	void *cq_shadow;              ///< Host coherent CQ mirror the GPU reaps; the poller copies real->shadow
	size_t cq_bytes;              ///< CQ size in bytes, for the poller mirror copy
	volatile uint64_t dbg_iters;  ///< Diagnostic: poller loop iterations during the run
	volatile uint32_t dbg_max_sq; ///< Diagnostic: max SQ tail the poller observed in the shadow
	uint64_t dbg_t_start_ns;      ///< Diagnostic: poller start time
	uint64_t dbg_t_ring_ns;       ///< Diagnostic: first nonzero SQ doorbell ring
	uint64_t dbg_t_cqe_ns;        ///< Diagnostic: first completion seen in cq_real
	pthread_t thread;             ///< Poller thread
	volatile int stop;            ///< Set to request the poller to exit
	int running;                  ///< Non-zero while the poller thread is joined-pending
};

/**
 * Poller: relay the shadow doorbells to BAR0 and mirror the CQ to the GPU
 *
 * Two host-mediated relays, because this GPU cannot master MMIO to the NVMe BAR
 * nor promptly observe a peer's DMA writes: (1) mirror the GPU-written shadow
 * doorbell cells to the real BAR0 doorbells; (2) copy the controller-written CQ
 * (which the CPU sees coherently) into the GPU-coherent shadow CQ that the reap
 * reads. NVMe writes each CQE's phase word last, so a mirrored phase flip
 * implies the entry is complete.
 */
static void *
nvme_hip_db_bridge_poll(void *arg)
{
	struct nvme_hip_db_bridge *bridge = arg;
	uint32_t last_sq = 0xFFFFFFFFu;
	uint32_t last_cq = 0xFFFFFFFFu;

	bridge->dbg_t_start_ns = nvme_hip_now_ns();

	while (!bridge->stop) {
		uint32_t sq = bridge->cells[0];
		uint32_t cq = bridge->cells[1];

		// Ring a doorbell only when its value changes. Writing every iteration
		// floods the NVMe's PCIe link with millions of redundant MMIO writes that
		// contend with the controller's completion DMA and delay completions past
		// the reap deadline.
		if (sq != last_sq) {
			*bridge->sqdb_mmio = sq;
			last_sq = sq;
			if (sq > bridge->dbg_max_sq) {
				bridge->dbg_max_sq = sq;
			}
			if (sq && !bridge->dbg_t_ring_ns) {
				bridge->dbg_t_ring_ns = nvme_hip_now_ns();
			}
		}
		if (cq != last_cq) {
			*bridge->cqdb_mmio = cq;
			last_cq = cq;
		}
		bridge->dbg_iters++;
		if (bridge->cq_shadow) {
			memcpy(bridge->cq_shadow, bridge->cq_real, bridge->cq_bytes);
			if (!bridge->dbg_t_cqe_ns &&
			    ((volatile struct nvme_completion *)bridge->cq_real)[0].status) {
				bridge->dbg_t_cqe_ns = nvme_hip_now_ns();
			}
		}
	}

	/* Final flush so the last tail/head and any last completions are delivered. */
	*bridge->sqdb_mmio = bridge->cells[0];
	*bridge->cqdb_mmio = bridge->cells[1];
	if (bridge->cq_shadow) {
		memcpy(bridge->cq_shadow, bridge->cq_real, bridge->cq_bytes);
	}

	return NULL;
}

/**
 * Set up a doorbell bridge for one queue pair
 *
 * Allocates the zero-copy shadow cells and records the real BAR0 doorbell
 * addresses. On success, sqdb_shadow_dev/cqdb_shadow_dev hold the GPU aliases
 * to store in the device queue pair.
 *
 * @return 0 on success. Positive values are hipError_t errors.
 */
static inline int
nvme_hip_db_bridge_init(struct nvme_hip_db_bridge *bridge, uint32_t *sqdb_mmio,
			uint32_t *cqdb_mmio)
{
	void *host = NULL;
	void *dev = NULL;
	int err;

	/* Fine-grained coherent so the GPU's doorbell store is visible to the host
	 * poller mid-kernel (consumer Radeon also needs HSA_FORCE_FINE_GRAIN_PCIE=1).
	 */
	err = hipHostMalloc(&host, 2 * sizeof(uint32_t),
			    hipHostMallocMapped | hipHostMallocCoherent);
	if (err) {
		UPCIE_DEBUG("FAILED: hipHostMalloc(shadow); hipError_t(%d)", err);
		return err;
	}

	((volatile uint32_t *)host)[0] = 0;
	((volatile uint32_t *)host)[1] = 0;

	err = hipHostGetDevicePointer(&dev, host, 0);
	if (err) {
		UPCIE_DEBUG("FAILED: hipHostGetDevicePointer(shadow); hipError_t(%d)", err);
		hipHostFree(host);
		return err;
	}

	bridge->cells = (volatile uint32_t *)host;
	bridge->sqdb_mmio = (volatile uint32_t *)sqdb_mmio;
	bridge->cqdb_mmio = (volatile uint32_t *)cqdb_mmio;
	bridge->sqdb_shadow_dev = (uint8_t *)dev;
	bridge->cqdb_shadow_dev = (uint8_t *)dev + sizeof(uint32_t);
	bridge->cq_real = NULL;
	bridge->cq_shadow = NULL;
	bridge->cq_bytes = 0;
	bridge->dbg_iters = 0;
	bridge->dbg_max_sq = 0;
	bridge->dbg_t_start_ns = 0;
	bridge->dbg_t_ring_ns = 0;
	bridge->dbg_t_cqe_ns = 0;
	bridge->stop = 0;
	bridge->running = 0;

	return 0;
}

/**
 * Start the bridge poller; call before launching the submitting kernel
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int
nvme_hip_db_bridge_start(struct nvme_hip_db_bridge *bridge)
{
	int perr;

	bridge->stop = 0;
	// pthread_create returns the error code directly and does not set errno.
	perr = pthread_create(&bridge->thread, NULL, nvme_hip_db_bridge_poll, bridge);
	if (perr) {
		UPCIE_DEBUG("FAILED: pthread_create(bridge); err(%d)", perr);
		return -perr;
	}
	bridge->running = 1;

	return 0;
}

/**
 * Stop the bridge poller; call after the submitting kernel completes
 */
static inline void
nvme_hip_db_bridge_stop(struct nvme_hip_db_bridge *bridge)
{
	if (!bridge->running) {
		return;
	}
	bridge->stop = 1;
	pthread_join(bridge->thread, NULL);
	bridge->running = 0;
}

/**
 * Release the bridge's shadow cells
 */
static inline void
nvme_hip_db_bridge_term(struct nvme_hip_db_bridge *bridge)
{
	nvme_hip_db_bridge_stop(bridge);
	if (bridge->cells) {
		hipHostFree((void *)bridge->cells);
		bridge->cells = NULL;
	}
	if (bridge->sq_host) {
		hipHostFree(bridge->sq_host);
		bridge->sq_host = NULL;
	}
	if (bridge->cq_shadow) {
		hipHostFree(bridge->cq_shadow);
		bridge->cq_shadow = NULL;
	}
	if (bridge->cq_real) {
		hipHostFree(bridge->cq_real);
		bridge->cq_real = NULL;
	}
}


/**
 * Deletes the HIP submission-queue and HIP completion-queue
 *
 * @param ctrlr Pointer to a pre-allocated NVMe controller
 * @param qpair Pointer to a queue-pair (from nvme_controller_hip_create_io_qpair)
 * @param heap Pointer to HIP Heap
 * @param bridge Pointer to the doorbell bridge for this queue pair
 *
 */
static inline void
nvme_controller_hip_delete_io_qpair(struct nvme_controller *ctrlr,
                                     struct nvme_qpair_hip *qpair,
                                     struct hipmem_heap *heap,
                                     struct nvme_hip_db_bridge *bridge)
{
	struct nvme_qpair_hip _qpair = {0};
	int err;

	err = hipMemcpyDtoH(&_qpair, (hipDeviceptr_t)qpair, sizeof(_qpair));
	if (err) {
		UPCIE_DEBUG("FAILED: hipMemcpyDtoH(device QP -> host QP); hipError_t(%d)", err);
	}

	/* sqdb/cqdb are GPU aliases of the bridge shadow cells, not BAR MMIO;
	 * releasing the bridge stops the poller and frees the shadow.
	 */
	nvme_hip_db_bridge_term(bridge);

	{
		struct nvme_command cmd = {0};
		struct nvme_completion cpl = {0};

		cmd.opc = 0x0; ///< Delete I/O Submission Queue
		cmd.cdw10 = _qpair.qid;

		err = nvme_qpair_submit_sync(&ctrlr->aq, &cmd, ctrlr->timeout_ms, &cpl);
		if (err) {
			UPCIE_DEBUG("FAILED: nvme_qpair_submit_sync(Delete SQ); err(%d)", err);
		}
	}

	{
		struct nvme_command cmd = {0};
		struct nvme_completion cpl = {0};

		cmd.opc = 0x4; ///< Delete I/O completion Queue
		cmd.cdw10 = _qpair.qid;

		err = nvme_qpair_submit_sync(&ctrlr->aq, &cmd, ctrlr->timeout_ms, &cpl);
		if (err) {
			UPCIE_DEBUG("FAILED: nvme_qpair_submit_sync(Delete CQ); err(%d)", err);
		}
	}

	/* The SQ, real CQ, and shadow CQ are all bridge host allocations freed by
	 * bridge_term (called above); nothing to release from the hip heap here.
	 */
	(void)heap;
}

/**
 * Allocates a HIP submission-queue, a HIP completion-queue, and wraps them in
 * the nvme_qpair struct
 *
 * @param ctrlr Pointer to a pre-allocated NVMe controller
 * @param qpair Pointer to a pre-allocated queue-pair (using HIP)
 * @param depth The queue depth
 * @param heap Pointer to HIP Heap
 * @param bridge Pointer to a pre-allocated doorbell bridge for this queue pair
 *
 * @return 0 on success. Negative values indicate errno-style errors, positive values are hipError_t errors.
 */
static inline int
nvme_controller_hip_create_io_qpair(struct nvme_controller *ctrlr,
                                     struct nvme_qpair_hip *qpair, uint16_t depth,
                                     struct hipmem_heap *heap,
                                     struct nvme_hip_db_bridge *bridge)
{
	/* _qpair declared at function scope so sq/cq remain accessible when building
	 * the Create I/O CQ/SQ admin commands below. This code is inlined here
	 * (instead of nvme_qpair_hip.h) to avoid pulling in hipmem_heap.h into
	 * device-code compilation units.
	 */
	struct nvme_qpair_hip _qpair = {0};
	uint64_t sq_phys = 0, cq_phys = 0;
	uint16_t qid;
	int err;

	(void)heap; /* SQ/CQ are host allocations owned by the bridge, not the hip heap */

	err = nvme_qid_find_free(ctrlr->qids);
	if (err < 1) {
		return -ENOMEM;
	}
	qid = err;

	err = nvme_qid_alloc(ctrlr->qids, qid);
	if (err) {
		UPCIE_DEBUG("FAILED: nvme_qid_alloc(): err(%d)", err);
		return err;
	}

	{
		uint8_t *bar0 = ctrlr->func.bars[0].region;
		int dstrd = nvme_reg_cap_get_dstrd(nvme_mmio_cap_read(bar0));

		/* The GPU cannot master MMIO to the NVMe BAR on this class of
		 * device, so route the doorbells through the host-relayed bridge:
		 * sqdb/cqdb become GPU aliases of the shadow cells, and the poller
		 * (started around the submitting kernel) mirrors them to the real
		 * BAR0 doorbells at the per-queue offsets computed here.
		 */
		err = nvme_hip_db_bridge_init(
			bridge,
			(uint32_t *)(bar0 + 0x1000 + ((2 * qid) << (2 + dstrd))),
			(uint32_t *)(bar0 + 0x1000 + ((2 * qid + 1) << (2 + dstrd))));
		if (err) {
			UPCIE_DEBUG("FAILED: nvme_hip_db_bridge_init(); err(%d)", err);
			return err;
		}

		_qpair.sqdb = bridge->sqdb_shadow_dev;
		_qpair.cqdb = bridge->cqdb_shadow_dev;
		_qpair.qid = qid;
		_qpair.tail = 0;
		_qpair.head = 0;
		_qpair.depth = depth;
		_qpair.phase = 1;
		_qpair.timeout_ms = ctrlr->timeout_ms;

		{
			int clock_rate_khz = 0;
			int dev;

			hipGetDevice(&dev);
			hipDeviceGetAttribute(&clock_rate_khz, hipDeviceAttributeWallClockRate, dev);
			_qpair.clocks_per_ms = (uint64_t)clock_rate_khz;
		}

		/* Queue placement follows which coherence direction actually works on
		 * this GPU. The SQ (GPU writes, controller reads) lives in fine-grained
		 * coherent host memory: both sides hit host DRAM coherently. A VRAM SQE
		 * write is not visible to the controller's peer read mid-kernel (it would
		 * fetch a stale zeroed entry, i.e. a Flush of nsid 0 -> Invalid
		 * Namespace). The CQ (controller writes, GPU reads) cannot be read
		 * directly by the GPU: neither a peer DMA into host memory nor into VRAM is
		 * observed promptly by a GPU poll on this device. So the controller DMAs
		 * completions into a plain host CQ (cq_real, which the CPU sees
		 * coherently), and the bridge poller mirrors that into a GPU-coherent
		 * shadow CQ (cq_shadow) that the reap reads. Data buffers stay in VRAM
		 * (filled before launch, so not subject to mid-kernel coherence).
		 */
		{
			size_t sq_bytes = depth * sizeof(struct nvme_command);
			size_t cq_bytes = depth * sizeof(struct nvme_completion);
			void *sq_dev = NULL, *cq_dev = NULL;

			err = hipHostMalloc(&bridge->sq_host, sq_bytes,
					    hipHostMallocMapped | hipHostMallocCoherent);
			if (err) {
				UPCIE_DEBUG("FAILED: hipHostMalloc(sq); hipError_t(%d)", err);
				return err;
			}
			memset(bridge->sq_host, 0, sq_bytes);

			err = hipHostGetDevicePointer(&sq_dev, bridge->sq_host, 0);
			if (err) {
				UPCIE_DEBUG("FAILED: hipHostGetDevicePointer(sq); hipError_t(%d)", err);
				return err;
			}
			_qpair.sq = sq_dev;

			err = hostmem_pagemap_virt_to_phys(bridge->sq_host, &sq_phys);
			if (err) {
				UPCIE_DEBUG("FAILED: hostmem_pagemap_virt_to_phys(sq); err(%d)", err);
				return err;
			}

			/* Real CQ: pinned host memory the controller DMAs to. */
			err = hipHostMalloc(&bridge->cq_real, cq_bytes, hipHostMallocMapped);
			if (err) {
				UPCIE_DEBUG("FAILED: hipHostMalloc(cq_real); hipError_t(%d)", err);
				return err;
			}
			memset(bridge->cq_real, 0, cq_bytes);

			err = hostmem_pagemap_virt_to_phys(bridge->cq_real, &cq_phys);
			if (err) {
				UPCIE_DEBUG("FAILED: hostmem_pagemap_virt_to_phys(cq); err(%d)", err);
				return err;
			}

			/* Shadow CQ: fine-grained coherent, the poller copies real->shadow
			 * and the GPU reaps from here.
			 */
			err = hipHostMalloc(&bridge->cq_shadow, cq_bytes,
					    hipHostMallocMapped | hipHostMallocCoherent);
			if (err) {
				UPCIE_DEBUG("FAILED: hipHostMalloc(cq_shadow); hipError_t(%d)", err);
				return err;
			}
			memset(bridge->cq_shadow, 0, cq_bytes);
			bridge->cq_bytes = cq_bytes;

			err = hipHostGetDevicePointer(&cq_dev, bridge->cq_shadow, 0);
			if (err) {
				UPCIE_DEBUG("FAILED: hipHostGetDevicePointer(cq_shadow); hipError_t(%d)", err);
				return err;
			}
			_qpair.cq = cq_dev;
		}

		err = hipMemcpyHtoD((hipDeviceptr_t)qpair, &_qpair, sizeof(_qpair));
		if (err) {
			UPCIE_DEBUG("FAILED: hipMemcpyHtoD(host QP -> device QP); hipError_t(%d)", err);
			return err;
		}
	}

	{
		struct nvme_command cmd = {0};
		struct nvme_completion cpl = {0};

		cmd.opc = 0x5; ///< Create I/O Completion Queue
		cmd.prp1 = cq_phys;
		cmd.cdw10 = ((depth - 1) << 16) | qid;
		cmd.cdw11 = 0x1; ///< Physically contigous

		err = nvme_qpair_submit_sync(&ctrlr->aq, &cmd, ctrlr->timeout_ms, &cpl);
		if (err) {
			UPCIE_DEBUG("FAILED: nvme_qpair_submit_sync(); err(%d)", err);
			return err;
		}
	}

	{
		struct nvme_command cmd = {0};
		struct nvme_completion cpl = {0};

		cmd.opc = 0x1; ///< Create I/O Submission Queue
		cmd.prp1 = sq_phys;
		cmd.cdw10 = ((depth - 1) << 16) | qid;
		cmd.cdw11 = (qid << 16) | 0x1; ///< CQID and Physically contigous

		err = nvme_qpair_submit_sync(&ctrlr->aq, &cmd, ctrlr->timeout_ms, &cpl);
		if (err) {
			UPCIE_DEBUG("FAILED: nvme_qpair_submit_sync(); err(%d)", err);
			return err;
		}
	}

	return 0;
}