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

	if (g_upcie_rte.connection.alive) {
		/* Nothing below this is ours to release: the memory belongs to
		 * whoever is serving, and closing the socket is what tells it
		 * to take back what this process still holds. */
		xnvme_be_upcie_cplane_disconnect();
		g_upcie_rte.is_initialized = 0;
		return;
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
 * initialized RTE matches. A non-zero opts->homi_id names a runtime another
 * process may already be serving; this connects to it where that is so, and
 * builds its own where it is not. Which attachment mode the device is under
 * does not enter into it: what crosses is descriptors, and a descriptor is a
 * descriptor whichever way the controller is reached.
 */
static int
_rte_init(enum xnvme_be_upcie_mode mode, struct xnvme_opts *opts, const char *bdf)
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

	if (!heap_size) {
		heap_size = XNVME_BE_UPCIE_DEFAULT_HEAP_SIZE;
	}

	g_upcie_rte.mode = mode;

	if (opts->homi_id) {
		/* Somebody may already own this identifier. Connecting to them
		 * is cheaper than allocating a runtime and then discovering
		 * they exist, and it is what makes the socket the way in. */
		err = xnvme_be_upcie_cplane_init_connection(opts->homi_id, bdf, NULL);
		if (!err) {
			g_upcie_rte.is_initialized = 1;
			return 0;
		}
		if (err != -ENOENT) {
			XNVME_DEBUG("FAILED: xnvme_be_upcie_cplane_init_connection(); err(%d)",
				    err);
			return err;
		}
	}

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

	err = _rte_init(mode, &dev->opts, dev->ident.uri);
	if (err) {
		XNVME_DEBUG("FAILED: _rte_init(mode(%d))", mode);
		errno = -err;
		return NULL;
	}

	/* Only the server writes the PCI Command register; the server already flipped
	 * Bus Master Enable at open time and a client neither needs to nor typically
	 * may touch config space. */
	if (!g_upcie_rte.connection.alive) {
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

	/* Connected: the controller is open in another process, so this builds a
	 * description of it and asks that process for a queue to submit on. */
	if (g_upcie_rte.connection.alive) {
		ctrlr->ctrl = calloc(1, sizeof(*ctrlr->ctrl));
		if (!ctrlr->ctrl) {
			XNVME_DEBUG("FAILED: calloc(ctrl)");
			errno = ENOMEM;
			goto failed;
		}

		err = xnvme_be_upcie_cplane_init_connection(dev->opts.homi_id, dev->ident.uri,
							    ctrlr);
		if (err) {
			XNVME_DEBUG("FAILED: xnvme_be_upcie_cplane_init_connection(%s); err(%d)",
				    dev->ident.uri, err);
			errno = -err;
			goto failed;
		}

		err = xnvme_be_upcie_cplane_ctrlr_from_record(ctrlr);
		if (err) {
			XNVME_DEBUG("FAILED: xnvme_be_upcie_cplane_ctrlr_from_record(); err(%d)",
				    err);
			errno = -err;
			goto failed;
		}

		/* No I/O queue yet: one is dedicated to whoever holds it, so it
		 * is asked for on first use rather than at open. */

		g_ctrlr_count++;

		return ctrlr;
	}

	ctrlr->ctrl = calloc(1, sizeof(*ctrlr->ctrl));
	if (!ctrlr->ctrl) {
		XNVME_DEBUG("FAILED: calloc(ctrl)");
		errno = ENOMEM;
		goto failed;
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

	g_ctrlr_count++;

	return ctrlr;

failed:
	if (ctrlr) {
		free(ctrlr->ctrl);
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
	if (g_upcie_rte.connection.alive) {
		/* Hand the I/O queue back, if one was ever asked for, and let go
		 * of the description. The controller itself is the server's and
		 * is not closed here; the BAR mapping goes with the runtime,
		 * not with this. */
		xnvme_be_upcie_ctrlr_admin_prp_release(ctrlr);
		xnvme_be_upcie_cplane_free_qpair(ctrlr, &ctrlr->sync);
		if (ctrlr->bar0) {
			munmap(ctrlr->bar0, ctrlr->bar0_nbytes);
		}
		/* The socket is not closed here: it is the process's rather
		 * than this controller's, so it goes with the runtime, which
		 * _rte_term() takes down when the last controller closes. */
		free(ctrlr->ctrl);
		free(ctrlr);

		if (--g_ctrlr_count == 0) {
			_rte_term();
		}

		return 0;
	}

	nvme_controller_delete_io_qpair_dmamem(ctrlr->ctrl, &ctrlr->sync, &g_upcie_rte.mem.heap,
					       ctrlr->sync_offsets.sq, ctrlr->sync_offsets.cq,
					       ctrlr->sync_offsets.prp);
	_ctrlr_close(ctrlr);
	free(ctrlr->ctrl);
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
