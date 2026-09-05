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
 * Offset of the doorbell registers in BAR0
 */
#define XNVME_BE_UPCIE_DOORBELL_OFFSET 0x1000

/**
 * Controllers one process may drive a GPU against at once
 */
#define XNVME_BE_UPCIE_GPU_CTRLRS_MAX 16

/**
 * Distinct client regions one server may have installed at once
 */
#define XNVME_BE_UPCIE_CPLANE_REGIONS_MAX 32

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
	uint64_t served_gone_ns; ///< When the server was found gone; 0 while it answers
	void *cq_gpu;            ///< XNVME_QUEUE_P2P_CQ_MIRROR: the CQ the controller writes
	uint32_t pokes_idle;     ///< Pokes since the last completion
	int cqmirror_slot;       ///< The warp mirroring cq_gpu; see xnvme_be_upcie_cuda_cqmirror.h
	uint8_t _rvds[128];
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
 * Shared controller state, one per physical controller, managed by cref.
 */
struct xnvme_be_upcie_ctrlr {
	struct nvme_controller *ctrl;
	struct xnvme_be_upcie_ctrlr_attach attach;
	struct nvme_qpair sync; ///< Shared submission/completion queue for synchronous IOs
	struct xnvme_be_upcie_qpair_offsets sync_offsets; ///< Heap offsets of the sync qpair
	struct nvme_request admin_prp; ///< PRP scratch for admin payloads, this controller's own

	int index;                        ///< The server's index for it, which requests name
	void *bar0;                       ///< This process's mapping of this BAR0
	uint64_t bar0_nbytes;             ///< How much of it
	struct nvme_controller_base base; ///< What the server opened the controller with
	char bdf[16];                     ///< The controller, as the server names it
	int attached;                     ///< Whether base and bdf came from the server
};

/**
 * The IOVA range memory the IOAS will not map itself is installed in
 *
 * IOMMU_IOAS_MAP_FILE rejects the dma-bufs CUDA and HIP export, so such memory
 * reaches a controller's domain through the out-of-tree iommu-map-pa module
 * instead; see <upcie/dmamem_iommu_map_pa.h>. One range per process, holding
 * one set of addresses, so every controller sharing a heap reaches it at the
 * same IOVAs: opened for the first controller and attached to by the rest. A
 * second range would reserve the same addresses in the same IOAS and be
 * refused, so `imp` is shared rather than owned.
 */
struct xnvme_be_upcie_iova_range {
	struct dmamem_iommu_map_pa *imp; ///< The process's range; NULL where the mode needs none
	int alive;                       ///< Whether imp is open
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
	struct xnvme_be_upcie_ctrlr *ctrlr;     ///< Shared controller (first field for platform)
	struct dmamem *dmem;                    ///< Where this device's data buffers live
	struct xnvme_be_upcie_iova_range range; ///< This device's hold on it; unopened if unneeded

	uint8_t _rvds[96];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_upcie_state) == XNVME_BE_STATE_NBYTES, "Incorrect size")

/**
 * What a server hands a client for it to attach to a controller
 *
 * The descriptors stay open in the server; a client receives copies.
 * `desc_offset` is into the heap `heap_fd` names.
 */
struct xnvme_be_upcie_cplane_share {
	int heap_fd;                      ///< The DMA heap, for the client to map
	int bar0_fd;                      ///< BAR0, since a client rings its own doorbell
	uint64_t bar0_nbytes;             ///< How much of BAR0 to map
	uint64_t heap_nbytes;             ///< How much of the heap to map
	struct nvme_controller_base base; ///< What a client builds its controller around
	uint64_t desc_offset;             ///< Where the heap's description sits
	char uri[32];                     ///< The identifier clients were given
	int live;                         ///< Whether the offsets above hold allocations
};

/**
 * Fill `out` with what a client of `dev` needs, placing the heap's description in the heap
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_share_get(struct xnvme_dev *dev, struct xnvme_be_upcie_cplane_share *out);

/**
 * Release what xnvme_be_upcie_cplane_share_get() took; safe on an all-zero `share`
 */
void
xnvme_be_upcie_cplane_share_put(struct xnvme_be_upcie_cplane_share *share);

/**
 * What the server holds for one registration of a client's memory
 *
 * The description, at `desc_offset`, is the client's to read; the rest is what
 * the server gives back when the client unregisters or goes away.
 */
struct xnvme_be_upcie_cplane_registration {
	uint64_t desc_offset; ///< The description, as a heap offset
	struct dmabuf dmabuf; ///< What dmabuf_import_attach() gave back
	int attached;         ///< Whether dmabuf holds an attachment
	int map_fd;           ///< iommu-map-pa handle; -1 where the mode needs none
	uint64_t map_handle;  ///< What iommu_map_pa_add() gave back
	int region;           ///< The installed region shared with; -1 when there is none
};

/**
 * Describe the client memory `dmabuf_fd` names in the addresses the controller `bdf` consumes
 *
 * Physical addresses where the IOMMU is out of the way, an IOVA where it is not.
 *
 * @param page_size The granule the client's runtime hands out
 * @param out Pre-allocated registration to fill
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_serve_register_mem(int dmabuf_fd, uint64_t nbytes, uint32_t page_size,
					 const char *bdf,
					 struct xnvme_be_upcie_cplane_registration *out);

/**
 * Release a registration and its description; safe on an all-zero `reg`
 */
void
xnvme_be_upcie_cplane_serve_unregister_mem(struct xnvme_be_upcie_cplane_registration *reg);

/**
 * Run a client's admin command on the admin queue of `dev`
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_serve_admin_cmd(struct xnvme_dev *dev, void *cmd, void *cpl);

/**
 * Allocate `nbytes` from the heap on a client's behalf, reporting its heap offset
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_serve_buf_alloc(size_t nbytes, uint64_t *offset);

/**
 * Free a heap allocation made by xnvme_be_upcie_cplane_serve_buf_alloc()
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_serve_buf_free(uint64_t offset);

/**
 * Write the socket path clients of `cplane_id` connect to into `path`
 */
void
xnvme_be_upcie_cplane_socket_path(uint32_t cplane_id, char *path, size_t nbytes);

/**
 * Connect to the server at `path`
 *
 * @return A connected socket the caller closes, -ENOENT when nobody serves
 * `path`, or another negative errno
 */
int
xnvme_be_upcie_cplane_connect(const char *path);

/**
 * Check that the process at the other end of `sock` runs as this user
 *
 * The socket is the whole boundary: a server submits what a client sends, and a
 * client maps what a server hands over, so either side refuses a peer of
 * another user.
 *
 * @param sock A connected socket
 * @param root_ok Non-zero to accept a peer running as root as well
 *
 * @return 0 when the peer may be trusted, -EPERM when it runs as another user,
 * or another negative errno
 */
int
xnvme_be_upcie_cplane_peer_check(int sock, int root_ok);

/**
 * Send `msg` on `sock` and replace it with the reply
 *
 * @param fds Pre-allocated array of NVME_CPLANE_FDS_MAX, or NULL
 * @param nfds Set to how many descriptors arrived
 *
 * @return 0 on success, the server's result when it refused, negative errno on error
 */
int
xnvme_be_upcie_cplane_ask(int sock, struct nvme_cplane_msg *msg, int *fds, uint32_t *nfds);

/**
 * Like xnvme_be_upcie_cplane_ask(), about `ctrlr`, on the runtime's socket
 *
 * Serialised, since the threads of a process share the one socket.
 */
int
xnvme_be_upcie_cplane_ask_ctrlr(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_cplane_msg *msg,
				int *fds, uint32_t *nfds);

/**
 * Ask the server on `sock` about the controller at `index`; the reply lands in `msg`
 *
 * @return 0 on success, the server's result when it refused (-ERANGE past the
 * last controller), negative errno on error
 */
int
xnvme_be_upcie_cplane_ask_status(int sock, uint32_t index, struct nvme_cplane_msg *msg);

/**
 * Connect to the runtime of `cplane_id` and map the heap and the BAR0 of `bdf` into `ctrlr`
 *
 * @param bdf The controller wanted, or NULL to map the heap alone
 * @param ctrlr Filled when `bdf` is given
 *
 * @return 0 on success, -ENOENT when nobody serves `cplane_id`, -ENODEV when the
 * server does not hold `bdf`, another negative errno on error
 */
int
xnvme_be_upcie_cplane_init_connection(uint32_t cplane_id, const char *bdf,
				      struct xnvme_be_upcie_ctrlr *ctrlr);

/**
 * Build `ctrlr->ctrl` from the base and BAR0 mapping xnvme_be_upcie_cplane_init_connection() set
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_ctrlr_from_base(struct xnvme_be_upcie_ctrlr *ctrlr);

/**
 * Unmap the heap and close the runtime's socket, which releases what this process held
 */
void
xnvme_be_upcie_cplane_disconnect(void);

/**
 * Like xnvme_be_upcie_cplane_ask_ctrlr(), handing the server `fd` with the request
 */
int
xnvme_be_upcie_cplane_ask_ctrlr_fd(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_cplane_msg *msg,
				   int fd);

/**
 * Have the server describe memory this process allocated, for the controller `ctrlr`
 *
 * The server leaves the description in the heap, in the addresses the controller
 * consumes, which only the server can know.
 *
 * @param dmabuf_fd The memory, as a dma-buf
 * @param page_size The granule this process's runtime hands out
 * @param desc_out Set to the description, in this process's mapping of the heap
 * @param offset_out Set to the heap offset that names the registration, or NULL
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_register_client_mem(struct xnvme_be_upcie_ctrlr *ctrlr, int dmabuf_fd,
					  uint64_t nbytes, uint32_t page_size,
					  const struct hostmem_shared_desc **desc_out,
					  uint64_t *offset_out);

/**
 * Give back the registration named by `offset`
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_unregister_client_mem(struct xnvme_be_upcie_ctrlr *ctrlr, uint64_t offset);

/**
 * Have the controller of `dev` create an I/O queue pair at device addresses the caller owns
 *
 * The caller vouches for the addresses; the identifier and the admin queue are
 * this process's.
 *
 * @param qid Set to the identifier allocated
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_ctrlr_qpair_create_at(struct xnvme_dev *dev, uint64_t sq_addr, uint64_t cq_addr,
				     uint16_t depth, uint32_t *qid);

/**
 * Delete a queue pair from xnvme_be_upcie_ctrlr_qpair_create_at(), leaving its memory alone
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_ctrlr_qpair_delete_at(struct xnvme_dev *dev, uint32_t qid);

/**
 * Have the server create an I/O queue pair on `ctrlr` in memory this process registered
 *
 * @param desc_offset The registration, as xnvme_be_upcie_cplane_register_client_mem() named it
 * @param sq_offset Submission queue, from the start of the registered memory
 * @param cq_offset Completion queue, from the start of the registered memory
 * @param qid Set to the identifier allocated
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_alloc_qpair_at(struct xnvme_be_upcie_ctrlr *ctrlr, uint64_t desc_offset,
				     uint64_t sq_offset, uint64_t cq_offset, uint16_t depth,
				     uint32_t *qid);

/**
 * Hand back a queue pair from xnvme_be_upcie_cplane_alloc_qpair_at()
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_free_qpair_at(struct xnvme_be_upcie_ctrlr *ctrlr, uint32_t qid);

/**
 * Have the server create an I/O queue pair of `depth` entries on `ctrlr`, and map it into `qpair`
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_alloc_qpair(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_qpair *qpair,
				  uint16_t depth);

/**
 * Hand back a queue pair from xnvme_be_upcie_cplane_alloc_qpair()
 */
void
xnvme_be_upcie_cplane_free_qpair(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_qpair *qpair);

/**
 * The xnvme_be_upcie_sync_cmd_admin() of a client: the server submits the command
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_sync_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes);

/**
 * Allocate `nbytes` of the server's heap through the socket of `ctrlr`
 *
 * @return The buffer on success, NULL with errno set on error
 */
void *
xnvme_be_upcie_cplane_buf_alloc(struct xnvme_be_upcie_ctrlr *ctrlr, size_t nbytes, uint64_t *phys);

/**
 * Free a buffer from xnvme_be_upcie_cplane_buf_alloc()
 */
void
xnvme_be_upcie_cplane_buf_free(struct xnvme_be_upcie_ctrlr *ctrlr, void *buf);

/**
 * What a client's poke that reaped nothing does
 *
 * A server shutting down deletes its clients' queues without completing what is in flight
 * on them. Once the socket says the server is gone and the command timeout has passed,
 * this completes those commands as aborted, so a caller draining the queue returns.
 *
 * @return How many commands it completed
 */
int
xnvme_be_upcie_cplane_queue_poke_idle(struct xnvme_queue_upcie *upcie_queue);

/**
 * The I/O queue for synchronous commands on `ctrlr`; a client asks for it on first use
 *
 * @return The queue on success, NULL with errno set on error
 */
struct nvme_qpair *
xnvme_be_upcie_ctrlr_ioq(struct xnvme_be_upcie_ctrlr *ctrlr);

/**
 * A client's PRP scratch for admin payloads on `ctrlr`, allocated on first use
 *
 * @return The request carrying the scratch, NULL with errno set on error
 */
struct nvme_request *
xnvme_be_upcie_ctrlr_admin_prp(struct xnvme_be_upcie_ctrlr *ctrlr);

/**
 * Free the scratch of xnvme_be_upcie_ctrlr_admin_prp(), when there is one
 */
void
xnvme_be_upcie_ctrlr_admin_prp_release(struct xnvme_be_upcie_ctrlr *ctrlr);

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

/**
 * This process's view of a runtime another process owns; zeroed in the owner
 *
 * `sock` is the one connection to the server, for every controller this process uses, since
 * each request names its controller. The server releases what this process holds when the
 * socket closes.
 */
struct xnvme_be_upcie_rte_connection {
	int alive;            ///< Whether this process is a client of a server
	void *heap_base;      ///< This process's mapping of the server's heap
	uint64_t heap_nbytes; ///< How much of it
	int sock;             ///< To the server; 0 when not connected
};

/**
 * State used across multiple instances of controllers/namespaces
 *
 * One dmamem_heap regardless of mode, so every controller allocates queues,
 * PRP scratch and data buffers from the same offset space. It sits on a
 * hostmem_hugepage whose borrowed phys_lut resolves addresses at submit time.
 */
struct xnvme_be_upcie_rte {
	enum xnvme_be_upcie_mode mode;
	struct xnvme_be_upcie_rte_cdev cdev;
	struct xnvme_be_upcie_rte_type1 type1;
	struct xnvme_be_upcie_rte_mem mem;
	struct xnvme_be_upcie_rte_connection connection; ///< Set when another process owns this
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

/** Whether a heap needs the IOVA range to reach a controller; false under UIO_LUT */
int
xnvme_be_upcie_iova_range_required(void);

/**
 * Claim the range for `bdf`, or take a hold on it and attach `bdf`
 *
 * Where the controller consumes physical addresses there is nothing to install
 * and nothing to claim, so this succeeds having done neither.
 *
 * Call it once the controller has attached: the IOAS reports the ranges it
 * enforces only when it knows the device's reserved regions.
 *
 * @param range Caller-allocated, cleared on failure
 * @param bdf   The controller that must reach the memory
 *
 * @return 0 on success, negative errno on failure
 */
int
xnvme_be_upcie_iova_range_open(struct xnvme_be_upcie_iova_range *range, const char *bdf);

/**
 * Have `bdf` reach the memory where the controllers before it do
 *
 * @param range An opened range, or one a mode left unopened
 * @param bdf   The controller to add
 *
 * @return 0 on success, negative errno on failure
 */
int
xnvme_be_upcie_iova_range_attach(struct xnvme_be_upcie_iova_range *range, const char *bdf);

/**
 * Drop the hold; the last one closes the range and every mapping installed in it
 *
 * Destroy the dmamem built on the range first, since the mappings go with it.
 */
void
xnvme_be_upcie_iova_range_close(struct xnvme_be_upcie_iova_range *range);

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

#endif /* __INTERNAL_XNVME_BE_UPCIE */
