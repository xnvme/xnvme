// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_ENABLED
#include <limits.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <xnvme_dev.h>
#include <xnvme_be_upcie.h>

static _Atomic int g_ctrlr_count;

/**
 * Address-space width the DMA-address table is sized for
 *
 * One entry per granule over `1 << va_bits`, so the default of 0, meaning
 * uPCIe's 47 bits, reserves 512 MiB with 2 MiB hugepages. That is virtual and
 * demand-paged, but `ulimit -v` refuses it and `vm.overcommit_memory=2` charges
 * it regardless of MAP_NORESERVE. Lowering it narrows what can be registered.
 */
int
xnvme_be_upcie_va_bits(void)
{
	const char *env = getenv("XNVME_UPCIE_VA_BITS");

	return env ? atoi(env) : 0;
}

/**
 * Default IOVA window, one slice per controller
 *
 * The base clears the IOVAs iommufd hands out from the low end and the reserved
 * regions near 4 GiB, and leaves the window inside a single usable range on a
 * 39-bit aperture, the narrowest in common use.
 */
#define XNVME_BE_UPCIE_GPU_IOVA_BASE (256ULL << 30)
#define XNVME_BE_UPCIE_GPU_IOVA_SIZE (64ULL << 30)
#define XNVME_BE_UPCIE_GPU_MAX_SLICES 64

static struct {
	struct dmamem_iommu_map_pa owner; ///< Holds the window claim; maps nothing itself
	int owner_alive;
	uint64_t base;
	uint64_t span; ///< Width of one slice
	int nslices;
	int used[XNVME_BE_UPCIE_GPU_MAX_SLICES];
	int nused;
} g_gpu_win;

static uint64_t
_env_u64(const char *name, uint64_t fallback)
{
	const char *env = getenv(name);
	char *end = NULL;
	uint64_t val;

	if (!env || !*env) {
		return fallback;
	}

	val = strtoull(env, &end, 0);
	if (end && *end) {
		XNVME_DEBUG("FAILED: %s('%s') is not a number; using the default", name, env);
		return fallback;
	}

	return val;
}

int
xnvme_be_upcie_gpu_map_required(void)
{
	return g_upcie_rte.mode != XNVME_BE_UPCIE_MODE_UIO_LUT;
}

/** Claim the window and fix its slice width; a no-op once claimed */
static int
_gpu_window_claim(const char *bdf, uint64_t span)
{
	uint64_t size, slice;
	int err;

	if (g_gpu_win.nslices) {
		if (span > g_gpu_win.span) {
			XNVME_DEBUG("FAILED: span(0x%" PRIx64 ") exceeds slice(0x%" PRIx64 ")",
				    span, g_gpu_win.span);
			return -ENOSPC;
		}

		return 0;
	}

	g_gpu_win.base = _env_u64("XNVME_UPCIE_GPU_IOVA_BASE", XNVME_BE_UPCIE_GPU_IOVA_BASE);
	size = _env_u64("XNVME_UPCIE_GPU_IOVA_SIZE", XNVME_BE_UPCIE_GPU_IOVA_SIZE);
	slice = _env_u64("XNVME_UPCIE_GPU_IOVA_SLICE", span * 2);

	if (!g_gpu_win.base || !span || (slice < span) || (slice > size)) {
		XNVME_DEBUG("FAILED: window 0x%" PRIx64 "+0x%" PRIx64 " cannot hold a 0x%" PRIx64
			    " slice; check XNVME_UPCIE_GPU_IOVA_{BASE,SIZE,SLICE}",
			    g_gpu_win.base, size, slice);
		return -EINVAL;
	}

	g_gpu_win.span = slice;
	g_gpu_win.nslices = (size / slice) > XNVME_BE_UPCIE_GPU_MAX_SLICES
				    ? XNVME_BE_UPCIE_GPU_MAX_SLICES
				    : (int)(size / slice);

	/* type1 needs no reservation: the window clears the iova-0 hugepage. */
	if (g_upcie_rte.mode != XNVME_BE_UPCIE_MODE_VFIO_CDEV) {
		return 0;
	}

	err = dmamem_iommu_map_pa_open(&g_gpu_win.owner, bdf, g_gpu_win.base, size);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_iommu_map_pa_open(%s); err(%d); module loaded?", bdf,
			    err);
		g_gpu_win.nslices = 0;
		return err;
	}

	err = dmamem_iommu_map_pa_reserve_window(&g_gpu_win.owner, &g_upcie_rte.cdev.iommufd);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_iommu_map_pa_reserve_window(0x%" PRIx64 "+0x%" PRIx64
			    "); err(%d)",
			    g_gpu_win.base, size, err);
		dmamem_iommu_map_pa_close(&g_gpu_win.owner);
		g_gpu_win.nslices = 0;
		return err;
	}
	g_gpu_win.owner_alive = 1;

	return 0;
}

/** Drop the claim once the last slice is handed back */
static void
_gpu_window_release(void)
{
	if (g_gpu_win.nused) {
		return;
	}

	if (g_gpu_win.owner_alive) {
		dmamem_iommu_map_pa_close(&g_gpu_win.owner);
	}

	/* The allowed ranges stay as they are; the IOAS goes with the runtime. */
	memset(&g_gpu_win, 0, sizeof(g_gpu_win));
}

int
xnvme_be_upcie_gpu_map_open(struct xnvme_be_upcie_gpu_map *map, const char *bdf, uint64_t span)
{
	int slice, err;

	map->slice = -1;

	if (!xnvme_be_upcie_gpu_map_required()) {
		return 0;
	}

	if (!UPCIE_HAVE_IOMMU_MAP_PA) {
		XNVME_DEBUG("FAILED: built without the iommu-map-pa UAPI");
		return -ENOTSUP;
	}

	err = _gpu_window_claim(bdf, span);
	if (err) {
		return err;
	}

	for (slice = 0; slice < g_gpu_win.nslices; ++slice) {
		if (!g_gpu_win.used[slice]) {
			break;
		}
	}
	if (slice == g_gpu_win.nslices) {
		XNVME_DEBUG("FAILED: all %d window slices in use; raise "
			    "XNVME_UPCIE_GPU_IOVA_SIZE",
			    g_gpu_win.nslices);
		_gpu_window_release();
		return -ENOSPC;
	}

	err = dmamem_iommu_map_pa_open(
		&map->imp, bdf, g_gpu_win.base + (uint64_t)slice * g_gpu_win.span, g_gpu_win.span);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_iommu_map_pa_open(%s); err(%d); module loaded?", bdf,
			    err);
		_gpu_window_release();
		return err;
	}

	g_gpu_win.used[slice] = 1;
	g_gpu_win.nused += 1;

	map->alive = 1;
	map->slice = slice;
	snprintf(map->bdf, sizeof(map->bdf), "%s", bdf);

	return 0;
}

void
xnvme_be_upcie_gpu_map_close(struct xnvme_be_upcie_gpu_map *map)
{
	if (!map->alive) {
		return;
	}

	dmamem_iommu_map_pa_close(&map->imp);

	if (map->slice >= 0) {
		g_gpu_win.used[map->slice] = 0;
		g_gpu_win.nused -= 1;
	}
	map->slice = -1;
	map->alive = 0;
	map->bdf[0] = '\0';

	_gpu_window_release();
}

/**
 * Terminate the uPCIe runtime-environment
 *
 * The state is globally accessible via g_upcie_rte, this function terminates it, unless it is
 * not initialized, then it exits early.
 */
static void
_rte_term(void)
{
	if (!g_upcie_rte.is_initialized) {
		return;
	}

	if (g_upcie_rte.mproc) {
		xnvme_be_upcie_mproc_rte_term();
	}

	if (g_upcie_rte.mem.heap_alive) {
		dmamem_heap_term(&g_upcie_rte.mem.heap);
		g_upcie_rte.mem.heap_alive = 0;
	}
	if (g_upcie_rte.mem.dmem_alive) {
		dmamem_destroy(&g_upcie_rte.mem.dmem);
		g_upcie_rte.mem.dmem_alive = 0;
	}
	if (g_upcie_rte.mem.hp_alive) {
		hostmem_hugepage_free(&g_upcie_rte.mem.hp);
		g_upcie_rte.mem.hp_alive = 0;
	}
	if (g_upcie_rte.type1.container_alive) {
		vfio_container_close(&g_upcie_rte.type1.container);
		g_upcie_rte.type1.container_alive = 0;
		g_upcie_rte.type1.iommu_set = 0;
	}
	if (g_upcie_rte.cdev.ioas_alive) {
		iommufd_destroy(&g_upcie_rte.cdev.iommufd, g_upcie_rte.cdev.iommufd.ioas_id);
		g_upcie_rte.cdev.ioas_alive = 0;
	}
	if (g_upcie_rte.cdev.iommufd_alive) {
		iommufd_close(&g_upcie_rte.cdev.iommufd);
		g_upcie_rte.cdev.iommufd_alive = 0;
	}

	g_upcie_rte.mode = XNVME_BE_UPCIE_MODE_UNSET;
	g_upcie_rte.is_initialized = 0;
}

/**
 * Bring up the RTE in VFIO_CDEV mode.
 *
 * /dev/iommu + one IOAS + a hugepage-backed memfd imported via
 * IOMMU_IOAS_MAP_FILE. All controllers opened afterwards attach into
 * this same IOAS so their PRPs translate arithmetically off the shared
 * base_iova.
 */
static int
_rte_init_vfio_cdev(size_t heap_size)
{
	size_t hugepgsz = 2ULL * 1024 * 1024;
	int err;

	/* dmamem_from_memfd requires size to be a multiple of hugepgsz;
	 * round up so callers (like xnvmeperf) that hand us a page-off
	 * size don't fail at -EINVAL. */
	heap_size = ((heap_size + hugepgsz - 1) / hugepgsz) * hugepgsz;

	err = iommufd_open(&g_upcie_rte.cdev.iommufd);
	if (err) {
		XNVME_DEBUG("FAILED: iommufd_open(); err(%d)", err);
		return err;
	}
	g_upcie_rte.cdev.iommufd_alive = 1;

	err = iommufd_ioas_alloc(&g_upcie_rte.cdev.iommufd);
	if (err) {
		XNVME_DEBUG("FAILED: iommufd_ioas_alloc(); err(%d)", err);
		return err;
	}
	g_upcie_rte.cdev.ioas_alive = 1;

	err = dmamem_from_memfd(&g_upcie_rte.mem.dmem, &g_upcie_rte.cdev.iommufd, heap_size,
				hugepgsz);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_from_memfd(); err(%d)", err);
		return err;
	}
	g_upcie_rte.mem.dmem_alive = 1;

	err = dmamem_heap_init(&g_upcie_rte.mem.heap, &g_upcie_rte.mem.dmem, 4096);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_heap_init(); err(%d)", err);
		return err;
	}
	g_upcie_rte.mem.heap_alive = 1;

	return 0;
}

/**
 * Bring up the RTE in UIO_LUT mode.
 *
 * Allocate a hostmem_hugepage (with phys_lut populated via pagemap) and
 * wrap it via dmamem_from_hostmem_lut so translation returns physical
 * addresses. Requires CAP_SYS_ADMIN so the pagemap read at
 * hostmem_hugepage_alloc time succeeds; without it dmamem_from_hostmem_lut
 * fails with EINVAL and the RTE bring-up aborts.
 */
static int
_rte_init_uio_lut(size_t heap_size)
{
	int err;

	err = hostmem_config_init(&g_upcie_rte.mem.config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_config_init(); err(%d)", err);
		return err;
	}
	heap_size = ((heap_size + g_upcie_rte.mem.config.hugepgsz - 1) /
		     g_upcie_rte.mem.config.hugepgsz) *
		    g_upcie_rte.mem.config.hugepgsz;

	err = hostmem_hugepage_alloc(heap_size, &g_upcie_rte.mem.hp, &g_upcie_rte.mem.config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_hugepage_alloc(); err(%d)", err);
		return err;
	}
	g_upcie_rte.mem.hp_alive = 1;

	err = dmamem_from_hostmem_registry(&g_upcie_rte.mem.dmem, &g_upcie_rte.mem.hp,
					   xnvme_be_upcie_va_bits());
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_from_hostmem_registry(); err(%d) "
			    "(missing CAP_SYS_ADMIN?)",
			    err);
		return err;
	}
	g_upcie_rte.mem.dmem_alive = 1;

	err = dmamem_heap_init(&g_upcie_rte.mem.heap, &g_upcie_rte.mem.dmem, 4096);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_heap_init(); err(%d)", err);
		return err;
	}
	g_upcie_rte.mem.heap_alive = 1;

	return 0;
}

/**
 * Bring up the RTE in VFIO_TYPE1 mode.
 *
 * Opens a vfio type1 container and allocates the hostmem_hugepage that
 * backs the shared dmamem_heap. The container is not yet iommu-set nor
 * mapped; that happens on the first ctrlr_init when a group first gets
 * attached (VFIO_SET_IOMMU requires a group attached to the container
 * before it can be set, so the mapping has to wait). Subsequent
 * ctrlr_init calls skip the iommu-set + MAP_DMA and just attach their
 * group + acquire BAR0.
 */
static int
_rte_init_vfio_type1(size_t heap_size)
{
	int err;

	err = hostmem_config_init(&g_upcie_rte.mem.config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_config_init(); err(%d)", err);
		return err;
	}
	heap_size = ((heap_size + g_upcie_rte.mem.config.hugepgsz - 1) /
		     g_upcie_rte.mem.config.hugepgsz) *
		    g_upcie_rte.mem.config.hugepgsz;

	err = hostmem_hugepage_alloc(heap_size, &g_upcie_rte.mem.hp, &g_upcie_rte.mem.config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_hugepage_alloc(); err(%d)", err);
		return err;
	}
	g_upcie_rte.mem.hp_alive = 1;

	err = vfio_container_open(&g_upcie_rte.type1.container);
	if (err) {
		XNVME_DEBUG("FAILED: vfio_container_open(); err(%d)", err);
		return err;
	}
	g_upcie_rte.type1.container_alive = 1;

	return 0;
}

/**
 * Bring up the process-wide RTE in the given mode, or verify an already
 * initialized RTE matches. When opts->shm_id is non-zero, additionally
 * enable multi-process mode; only UIO_LUT supports it because the primary
 * publishes its hugepage for secondaries to import, which the memfd and
 * type1-container paths cannot do.
 */
static int
_rte_init(enum xnvme_be_upcie_mode mode, struct xnvme_opts *opts)
{
	size_t heap_size = opts->host_heap_size;
	int err;

	if (g_upcie_rte.is_initialized) {
		if (g_upcie_rte.mode != mode) {
			XNVME_DEBUG("FAILED: existing upcie RTE mode(%d) != requested(%d)",
				    g_upcie_rte.mode, mode);
			return -EINVAL;
		}
		return 0;
	}

	if (opts->shm_id && mode != XNVME_BE_UPCIE_MODE_UIO_LUT) {
		XNVME_DEBUG("FAILED: shm_id requires UIO_LUT (uio_pci_generic); mode(%d)", mode);
		return -ENOTSUP;
	}

	if (!heap_size) {
		heap_size = XNVME_BE_UPCIE_DEFAULT_HEAP_SIZE;
	}

	g_upcie_rte.mode = mode;

	switch (mode) {
	case XNVME_BE_UPCIE_MODE_VFIO_CDEV:
		err = _rte_init_vfio_cdev(heap_size);
		break;
	case XNVME_BE_UPCIE_MODE_UIO_LUT:
		err = _rte_init_uio_lut(heap_size);
		break;
	case XNVME_BE_UPCIE_MODE_VFIO_TYPE1:
		err = _rte_init_vfio_type1(heap_size);
		break;
	default:
		err = -EINVAL;
		break;
	}

	if (err) {
		_rte_term();
		return err;
	}

	if (opts->shm_id) {
		err = xnvme_be_upcie_mproc_rte_init(opts->shm_id);
		if (err) {
			XNVME_DEBUG("FAILED: xnvme_be_upcie_mproc_rte_init(); err(%d)", err);
			_rte_term();
			return err;
		}

		if (g_upcie_rte.mproc->is_primary) {
			struct xnvme_be_upcie_mproc_shm *shm = g_upcie_rte.mproc->shm;

			snprintf(shm->hugepage_path, sizeof(shm->hugepage_path), "%s",
				 g_upcie_rte.mem.hp.path);
			shm->hugepage_base = (uint64_t)g_upcie_rte.mem.hp.virt;
			atomic_store_explicit(&shm->is_initialized, true, memory_order_release);
		} else {
			struct xnvme_be_upcie_mproc_shm *shm = g_upcie_rte.mproc->shm;

			for (int i = 0; i < 1000; i++) {
				if (atomic_load_explicit(&shm->is_initialized,
							 memory_order_acquire)) {
					break;
				}
				usleep(1000);
			}
			if (!atomic_load_explicit(&shm->is_initialized, memory_order_acquire)) {
				XNVME_DEBUG("FAILED: timed out waiting for primary hp publish");
				_rte_term();
				return -ENOENT;
			}
			err = xnvme_be_upcie_mproc_import_admin_hugepage();
			if (err) {
				XNVME_DEBUG("FAILED: mproc_import_admin_hugepage(); err(%d)", err);
				_rte_term();
				return err;
			}
		}
	}

	g_upcie_rte.is_initialized = 1;
	return 0;
}

/**
 * Enable PCI Bus Master for the given BDF
 *
 * Reads the PCI Command register via sysfs config space and sets the Bus Master Enable bit,
 * unless it is already set, then it exits early.
 *
 * @param bdf PCI Bus-Device-Function string, e.g. "0000:01:00.0"
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
static int
_pci_enable_bus_master(const char *bdf)
{
	char path[256] = {0};
	uint16_t cmd = 0;
	int fd;
	ssize_t ret;

	snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/config", bdf);

	fd = open(path, O_RDWR);
	if (fd < 0) {
		XNVME_DEBUG("FAILED: open(%s); errno(%d)", path, errno);
		return -errno;
	}

	ret = pread(fd, &cmd, sizeof(cmd), 0x04);
	if (ret != sizeof(cmd)) {
		XNVME_DEBUG("FAILED: pread(PCI_COMMAND); ret(%zd)", ret);
		close(fd);
		return -EIO;
	}

	if (cmd & 0x4) {
		close(fd);
		return 0;
	}

	cmd |= 0x4;

	ret = pwrite(fd, &cmd, sizeof(cmd), 0x04);
	if (ret != sizeof(cmd)) {
		XNVME_DEBUG("FAILED: pwrite(PCI_COMMAND); ret(%zd)", ret);
		close(fd);
		return -EIO;
	}

	close(fd);
	return 0;
}

/**
 * Close a controller the way it was opened, and detach its type1 group.
 *
 * Shared by the create-qpair failure path and by ctrlr_term, so the two
 * cannot drift apart.
 */
static void
_ctrlr_close(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	switch (g_upcie_rte.mode) {
	case XNVME_BE_UPCIE_MODE_VFIO_CDEV:
		nvme_controller_close_dmamem_vfio(ctrlr->ctrl, &ctrlr->attach.vfio,
						  &g_upcie_rte.mem.heap);
		break;
	case XNVME_BE_UPCIE_MODE_UIO_LUT:
		nvme_controller_close_dmamem_uio(ctrlr->ctrl, &ctrlr->attach.uio,
						 &g_upcie_rte.mem.heap);
		break;
	case XNVME_BE_UPCIE_MODE_VFIO_TYPE1:
		nvme_controller_close_dmamem_type1(ctrlr->ctrl, &ctrlr->attach.type1,
						   &g_upcie_rte.mem.heap);
		if (ctrlr->attach.type1_group_attached) {
			vfio_group_close(&ctrlr->attach.type1_group);
			ctrlr->attach.type1_group_attached = 0;
		}
		break;
	default:
		break;
	}
}

/**
 * Initialize the uPCIe controller.
 *
 * Initializes the runtime environment, allocates a shared xnvme_be_upcie_ctrlr,
 * opens the NVMe controller and creates a sync qpair. The returned handle is
 * stored in cref and written to dev->be.state[0] by the platform.
 */
void *
xnvme_be_upcie_ctrlr_init(struct xnvme_dev *dev)
{
	struct xnvme_be_upcie_ctrlr *ctrlr = NULL;
	char driver_name[sizeof(dev->ident.kernel_driver)] = {0};
	enum xnvme_be_upcie_mode mode = XNVME_BE_UPCIE_MODE_UNSET;
	char cdev_path[PATH_MAX] = {0};
	int err;

	err = xnvme_be_upcie_get_driver_name(dev->ident.uri, driver_name, sizeof(driver_name));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_get_driver_name(%s); err(%d)", dev->ident.uri,
			    err);
		errno = -err;
		return NULL;
	}
	snprintf(dev->ident.kernel_driver, sizeof(dev->ident.kernel_driver), "%s", driver_name);

	err = xnvme_be_upcie_mode_from_driver(dev->ident.uri, driver_name, &mode, cdev_path,
					      sizeof(cdev_path));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_mode_from_driver(%s); err(%d)", driver_name,
			    err);
		errno = -err;
		return NULL;
	}

	err = _rte_init(mode, &dev->opts);
	if (err) {
		XNVME_DEBUG("FAILED: _rte_init(mode(%d))", mode);
		errno = -err;
		return NULL;
	}

	/* Only the owner writes the PCI Command register; the primary already flipped
	 * Bus Master Enable at open time and a secondary neither needs to nor typically
	 * may touch config space. */
	if (!g_upcie_rte.mproc || g_upcie_rte.mproc->is_primary) {
		err = _pci_enable_bus_master(dev->ident.uri);
		if (err) {
			XNVME_DEBUG("FAILED: _pci_enable_bus_master(%s)", dev->ident.uri);
			errno = -err;
			goto failed;
		}
	}

	ctrlr = calloc(1, sizeof(*ctrlr));
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: calloc(ctrlr)");
		errno = ENOMEM;
		goto failed;
	}

	ctrlr->attach.type1_group.fd = -1;
	ctrlr->mproc.shm_fd = -1;
	ctrlr->mproc.lock_fd = -1;

	/* mproc secondary: skip open-and-initialize; attach to primary's controller via shm. */
	if (g_upcie_rte.mproc && !g_upcie_rte.mproc->is_primary) {
		err = xnvme_be_upcie_mproc_ctrlr_shm_attach(dev, ctrlr);
		if (err) {
			XNVME_DEBUG("FAILED: mproc_ctrlr_shm_attach(); err(%d)", err);
			errno = -err;
			goto failed;
		}
		g_ctrlr_count++;
		return ctrlr;
	}

	/* Primary path (or non-mproc): open the controller and create the sync qpair.
	 * For the mproc primary, allocate the per-controller shm first and use its
	 * embedded nvme_controller as the target so the primary's runtime state is
	 * directly visible to secondaries. */
	if (g_upcie_rte.mproc) {
		err = xnvme_be_upcie_mproc_ctrlr_shm_init(dev, ctrlr, driver_name);
		if (err) {
			XNVME_DEBUG("FAILED: mproc_ctrlr_shm_init(); err(%d)", err);
			errno = -err;
			goto failed;
		}
		/* ctrlr->ctrl now points into shm->ctrl */
	} else {
		ctrlr->ctrl = calloc(1, sizeof(*ctrlr->ctrl));
		if (!ctrlr->ctrl) {
			XNVME_DEBUG("FAILED: calloc(ctrl)");
			errno = ENOMEM;
			goto failed;
		}
	}

	switch (g_upcie_rte.mode) {
	case XNVME_BE_UPCIE_MODE_VFIO_CDEV:
		err = nvme_controller_open_dmamem_vfio(ctrlr->ctrl, &ctrlr->attach.vfio,
						       &g_upcie_rte.cdev.iommufd,
						       &g_upcie_rte.mem.heap, cdev_path);
		break;
	case XNVME_BE_UPCIE_MODE_UIO_LUT:
		err = nvme_controller_open_dmamem_uio(ctrlr->ctrl, &ctrlr->attach.uio,
						      &g_upcie_rte.mem.heap, dev->ident.uri);
		break;
	case XNVME_BE_UPCIE_MODE_VFIO_TYPE1:
		err = xnvme_be_upcie_type1_attach(ctrlr, dev->ident.uri);
		if (err) {
			break;
		}
		err = nvme_controller_open_dmamem_type1(
			ctrlr->ctrl, &ctrlr->attach.type1, &g_upcie_rte.type1.container,
			&ctrlr->attach.type1_group, &g_upcie_rte.mem.heap, dev->ident.uri);
		if (err && ctrlr->attach.type1_group_attached) {
			vfio_group_close(&ctrlr->attach.type1_group);
			ctrlr->attach.type1_group_attached = 0;
		}
		break;
	default:
		err = -EINVAL;
		break;
	}
	if (err) {
		XNVME_DEBUG("FAILED: nvme_controller_open_dmamem*(%s); err(%d)", dev->ident.uri,
			    err);
		errno = -err;
		goto failed;
	}

	err = nvme_controller_create_io_qpair_dmamem(
		ctrlr->ctrl, &ctrlr->sync, 16, &g_upcie_rte.mem.heap, &ctrlr->sync_offsets.sq,
		&ctrlr->sync_offsets.cq, &ctrlr->sync_offsets.prp);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_controller_create_io_qpair_dmamem(%d)", err);
		errno = -err;
		_ctrlr_close(ctrlr);
		goto failed;
	}

	/* Publish the fully-opened controller so mproc secondaries may attach. */
	if (ctrlr->mproc.shm) {
		atomic_store_explicit(&ctrlr->mproc.shm->is_initialized, true,
				      memory_order_release);
	}

	g_ctrlr_count++;

	return ctrlr;

failed:
	if (ctrlr) {
		if (ctrlr->mproc.shm) {
			xnvme_be_upcie_mproc_ctrlr_shm_term(ctrlr);
		} else {
			free(ctrlr->ctrl);
		}
		free(ctrlr);
	}

	if (g_ctrlr_count == 0) {
		_rte_term();
	}

	return NULL;
}

int
xnvme_be_upcie_ctrlr_term(void *handle)
{
	struct xnvme_be_upcie_ctrlr *ctrlr = handle;
	int is_secondary = g_upcie_rte.mproc && !g_upcie_rte.mproc->is_primary;

	if (is_secondary) {
		xnvme_be_upcie_mproc_delete_io_qpair(ctrlr, &ctrlr->sync, &ctrlr->sync_offsets);
		/* Do not close the controller: the primary owns it and closing here would
		 * tear down the shared admin queue. Just release the local BAR mapping
		 * (pci_func_close unmaps all bound BARs) and the local ctrl copy. */
		pci_func_close(&ctrlr->ctrl->func);
		xnvme_be_upcie_mproc_ctrlr_shm_term(ctrlr);
		free(ctrlr->ctrl);
	} else if (ctrlr->mproc.shm) {
		/* Primary in mproc: reap secondaries' still-allocated queues via the admin
		 * queue before we tear the shared segment down. */
		xnvme_be_upcie_mproc_delete_io_qpair(ctrlr, &ctrlr->sync, &ctrlr->sync_offsets);
		xnvme_be_upcie_mproc_free_all_queues(ctrlr);
		_ctrlr_close(ctrlr);
		xnvme_be_upcie_mproc_ctrlr_shm_term(ctrlr);
	} else {
		nvme_controller_delete_io_qpair_dmamem(
			ctrlr->ctrl, &ctrlr->sync, &g_upcie_rte.mem.heap, ctrlr->sync_offsets.sq,
			ctrlr->sync_offsets.cq, ctrlr->sync_offsets.prp);
		_ctrlr_close(ctrlr);
		free(ctrlr->ctrl);
	}
	free(ctrlr);

	if (--g_ctrlr_count == 0) {
		_rte_term();
	}

	return 0;
}

void
xnvme_be_upcie_dev_close(struct xnvme_dev *XNVME_UNUSED(dev))
{
}

int
xnvme_be_upcie_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;

	dev->ident.dtype =
		dev->opts.nsid ? XNVME_DEV_TYPE_NVME_NAMESPACE : XNVME_DEV_TYPE_NVME_CONTROLLER;
	dev->ident.csi = XNVME_SPEC_CSI_NVM;
	dev->ident.nsid = dev->opts.nsid;

	/* Data buffers come off the host heap; the GPU backends override this with
	 * their device heap once their runtime is up. */
	state->dmem = &g_upcie_rte.mem.dmem;

	return 0;
}

#endif

struct xnvme_be_dev g_xnvme_be_upcie_dev = {
#ifdef XNVME_BE_UPCIE_ENABLED
	.dev_open = xnvme_be_upcie_dev_open,
	.dev_close = xnvme_be_upcie_dev_close,
	.id = "upcie",
	.ctrlr_init = xnvme_be_upcie_ctrlr_init,
	.ctrlr_term = xnvme_be_upcie_ctrlr_term,
#else
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
	.id = "nosys",
	.ctrlr_init = NULL,
	.ctrlr_term = NULL,
#endif
};
