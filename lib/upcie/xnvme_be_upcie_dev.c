// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_ENABLED
#include <limits.h>
#include <stdatomic.h>
#include <xnvme_dev.h>
#include <xnvme_be_upcie.h>

static _Atomic int g_ctrlr_count;

int
xnvme_be_upcie_get_driver_name(const char *bdf, char *driver_name, size_t driver_name_len)
{
	char path[PATH_MAX] = {0};
	char link[PATH_MAX] = {0};
	ssize_t nbytes;
	char *base;

	snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/driver", bdf);

	nbytes = readlink(path, link, sizeof(link) - 1);
	if (nbytes < 0) {
		return -errno;
	}

	base = strrchr(link, '/');
	if (!base || !base[1]) {
		return -EINVAL;
	}

	snprintf(driver_name, driver_name_len, "%s", base + 1);

	return 0;
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

	if (g_upcie_rte.heap_alive) {
		dmamem_heap_term(&g_upcie_rte.heap);
		g_upcie_rte.heap_alive = 0;
	}
	if (g_upcie_rte.dmem_alive) {
		dmamem_destroy(&g_upcie_rte.dmem);
		g_upcie_rte.dmem_alive = 0;
	}
	if (g_upcie_rte.hp_alive) {
		hostmem_hugepage_free(&g_upcie_rte.hp);
		g_upcie_rte.hp_alive = 0;
	}
	if (g_upcie_rte.type1_container_alive) {
		vfio_container_close(&g_upcie_rte.type1_container);
		g_upcie_rte.type1_container_alive = 0;
		g_upcie_rte.type1_iommu_set = 0;
	}

	g_upcie_rte.mode = XNVME_BE_UPCIE_MODE_UNSET;
	g_upcie_rte.is_initialized = 0;
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

	err = hostmem_config_init(&g_upcie_rte.config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_config_init(); err(%d)", err);
		return err;
	}
	heap_size = ((heap_size + g_upcie_rte.config.hugepgsz - 1) / g_upcie_rte.config.hugepgsz) *
		    g_upcie_rte.config.hugepgsz;

	err = hostmem_hugepage_alloc(heap_size, &g_upcie_rte.hp, &g_upcie_rte.config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_hugepage_alloc(); err(%d)", err);
		return err;
	}
	g_upcie_rte.hp_alive = 1;

	err = dmamem_from_hostmem_lut(&g_upcie_rte.dmem, &g_upcie_rte.hp);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_from_hostmem_lut(); err(%d) "
			    "(missing CAP_SYS_ADMIN?)",
			    err);
		return err;
	}
	g_upcie_rte.dmem_alive = 1;

	err = dmamem_heap_init(&g_upcie_rte.heap, &g_upcie_rte.dmem, 4096);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_heap_init(); err(%d)", err);
		return err;
	}
	g_upcie_rte.heap_alive = 1;

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

	err = hostmem_config_init(&g_upcie_rte.config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_config_init(); err(%d)", err);
		return err;
	}
	heap_size = ((heap_size + g_upcie_rte.config.hugepgsz - 1) / g_upcie_rte.config.hugepgsz) *
		    g_upcie_rte.config.hugepgsz;

	err = hostmem_hugepage_alloc(heap_size, &g_upcie_rte.hp, &g_upcie_rte.config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_hugepage_alloc(); err(%d)", err);
		return err;
	}
	g_upcie_rte.hp_alive = 1;

	err = vfio_container_open(&g_upcie_rte.type1_container);
	if (err) {
		XNVME_DEBUG("FAILED: vfio_container_open(); err(%d)", err);
		return err;
	}
	g_upcie_rte.type1_container_alive = 1;

	return 0;
}

/**
 * Bring up the process-wide RTE in the given mode, or verify an already
 * initialized RTE matches.
 */
static int
_rte_init(enum xnvme_be_upcie_mode mode, size_t heap_size)
{
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

	switch (mode) {
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
 * Attach a controller's group to the RTE's type1 container, and on the
 * first controller also set_iommu(TYPE1) + MAP_DMA the hostmem hugepage
 * + init the shared dmamem_heap.
 *
 * Rolls back the group open (and any RTE-side state it turned on) on
 * failure. The container itself and the hostmem hugepage remain owned
 * by the RTE.
 */
static int
_type1_attach_and_maybe_map(struct xnvme_be_upcie_ctrlr *ctrlr, const char *bdf)
{
	int api_version = 0;
	int group_id = -1;
	int err;

	err = vfio_device_get_iommu_group_id(bdf, &group_id);
	if (err) {
		XNVME_DEBUG("FAILED: vfio_device_get_iommu_group_id(%s); err(%d)", bdf, err);
		return err;
	}

	err = vfio_group_open(group_id, &ctrlr->type1_group);
	if (err) {
		XNVME_DEBUG("FAILED: vfio_group_open(%d); err(%d)", group_id, err);
		return err;
	}

	err = vfio_group_get_status(&ctrlr->type1_group);
	if (err < 0) {
		XNVME_DEBUG("FAILED: vfio_group_get_status(); errno(%d)", errno);
		err = -errno;
		goto fail_group;
	}
	if (!(ctrlr->type1_group.status.flags & VFIO_GROUP_FLAGS_VIABLE)) {
		XNVME_DEBUG("FAILED: iommu group %d not viable", group_id);
		err = -EBUSY;
		goto fail_group;
	}

	err = vfio_group_set_container(&ctrlr->type1_group, &g_upcie_rte.type1_container);
	if (err < 0) {
		XNVME_DEBUG("FAILED: vfio_group_set_container(); errno(%d)", errno);
		err = -errno;
		goto fail_group;
	}
	ctrlr->type1_group_attached = 1;

	if (g_upcie_rte.type1_iommu_set) {
		/* Container already set_iommu'd + mapped by an earlier controller. */
		return 0;
	}

	err = vfio_get_api_version(&g_upcie_rte.type1_container, &api_version);
	if (err) {
		XNVME_DEBUG("FAILED: vfio_get_api_version(); err(%d)", err);
		goto fail_group;
	}
	if (api_version != VFIO_API_VERSION) {
		XNVME_DEBUG("FAILED: unexpected VFIO_API_VERSION(%d != %d)", api_version,
			    VFIO_API_VERSION);
		err = -EINVAL;
		goto fail_group;
	}

	if (!vfio_check_extension(&g_upcie_rte.type1_container, VFIO_TYPE1_IOMMU)) {
		XNVME_DEBUG("FAILED: VFIO_TYPE1_IOMMU extension not supported");
		err = -ENOTSUP;
		goto fail_group;
	}

	err = vfio_set_iommu(&g_upcie_rte.type1_container, VFIO_TYPE1_IOMMU);
	if (err < 0) {
		XNVME_DEBUG("FAILED: vfio_set_iommu(TYPE1); errno(%d)", errno);
		err = -errno;
		goto fail_group;
	}
	g_upcie_rte.type1_iommu_set = 1;

	/* MAP_DMA the whole hugepage at a caller-chosen base_iova; the
	 * dmamem sits on that mapping as ARITHMETIC (base_iova + offset). */
	err = dmamem_from_hostmem_type1(&g_upcie_rte.dmem, &g_upcie_rte.type1_container,
					(uint64_t)0, &g_upcie_rte.hp);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_from_hostmem_type1(); err(%d)", err);
		goto fail_group;
	}
	g_upcie_rte.dmem_alive = 1;

	err = dmamem_heap_init(&g_upcie_rte.heap, &g_upcie_rte.dmem, 4096);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_heap_init(); err(%d)", err);
		dmamem_destroy(&g_upcie_rte.dmem);
		g_upcie_rte.dmem_alive = 0;
		goto fail_group;
	}
	g_upcie_rte.heap_alive = 1;

	return 0;

fail_group:
	if (ctrlr->type1_group.fd >= 0) {
		vfio_group_close(&ctrlr->type1_group);
		ctrlr->type1_group_attached = 0;
	}
	return err;
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
	case XNVME_BE_UPCIE_MODE_UIO_LUT:
		nvme_controller_close_dmamem_uio(ctrlr->ctrl, &ctrlr->uio_ctx, &g_upcie_rte.heap);
		break;
	case XNVME_BE_UPCIE_MODE_VFIO_TYPE1:
		nvme_controller_close_dmamem_type1(ctrlr->ctrl, &ctrlr->type1_ctx,
						   &g_upcie_rte.heap);
		if (ctrlr->type1_group_attached) {
			vfio_group_close(&ctrlr->type1_group);
			ctrlr->type1_group_attached = 0;
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
	struct xnvme_be_upcie_ctrlr *ctrlr;
	char driver_name[sizeof(dev->ident.kernel_driver)] = {0};
	enum xnvme_be_upcie_mode mode = XNVME_BE_UPCIE_MODE_UNSET;
	int err;

	err = xnvme_be_upcie_get_driver_name(dev->ident.uri, driver_name, sizeof(driver_name));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_get_driver_name(%s); err(%d)", dev->ident.uri,
			    err);
		errno = -err;
		return NULL;
	}
	snprintf(dev->ident.kernel_driver, sizeof(dev->ident.kernel_driver), "%s", driver_name);

	if (!strcmp(driver_name, "vfio-pci")) {
		mode = XNVME_BE_UPCIE_MODE_VFIO_TYPE1;
	} else if (!strcmp(driver_name, "uio_pci_generic")) {
		mode = XNVME_BE_UPCIE_MODE_UIO_LUT;
	} else {
		XNVME_DEBUG("FAILED: unsupported driver '%s'", driver_name);
		errno = ENOTSUP;
		return NULL;
	}

	err = _rte_init(mode, dev->opts.host_heap_size);
	if (err) {
		XNVME_DEBUG("FAILED: _rte_init(mode(%d))", mode);
		errno = -err;
		return NULL;
	}

	err = _pci_enable_bus_master(dev->ident.uri);
	if (err) {
		XNVME_DEBUG("FAILED: _pci_enable_bus_master(%s)", dev->ident.uri);
		errno = -err;
		goto failed;
	}

	ctrlr = calloc(1, sizeof(*ctrlr));
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: calloc(ctrlr)");
		errno = ENOMEM;
		goto failed;
	}

	ctrlr->ctrl = calloc(1, sizeof(*ctrlr->ctrl));
	if (!ctrlr->ctrl) {
		XNVME_DEBUG("FAILED: calloc(ctrl)");
		errno = ENOMEM;
		goto failed;
	}

	ctrlr->type1_group.fd = -1;

	switch (g_upcie_rte.mode) {
	case XNVME_BE_UPCIE_MODE_UIO_LUT:
		err = nvme_controller_open_dmamem_uio(ctrlr->ctrl, &ctrlr->uio_ctx,
						      &g_upcie_rte.heap, dev->ident.uri);
		break;
	case XNVME_BE_UPCIE_MODE_VFIO_TYPE1:
		err = _type1_attach_and_maybe_map(ctrlr, dev->ident.uri);
		if (err) {
			break;
		}
		err = nvme_controller_open_dmamem_type1(
			ctrlr->ctrl, &ctrlr->type1_ctx, &g_upcie_rte.type1_container,
			&ctrlr->type1_group, &g_upcie_rte.heap, dev->ident.uri);
		if (err && ctrlr->type1_group_attached) {
			vfio_group_close(&ctrlr->type1_group);
			ctrlr->type1_group_attached = 0;
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
		ctrlr->ctrl, &ctrlr->sync, 16, &g_upcie_rte.heap, &ctrlr->sync_sq_offset,
		&ctrlr->sync_cq_offset, &ctrlr->sync_prp_offset);
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

	nvme_controller_delete_io_qpair_dmamem(ctrlr->ctrl, &ctrlr->sync, &g_upcie_rte.heap,
					       ctrlr->sync_sq_offset, ctrlr->sync_cq_offset,
					       ctrlr->sync_prp_offset);

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
	dev->ident.dtype =
		dev->opts.nsid ? XNVME_DEV_TYPE_NVME_NAMESPACE : XNVME_DEV_TYPE_NVME_CONTROLLER;
	dev->ident.csi = XNVME_SPEC_CSI_NVM;
	dev->ident.nsid = dev->opts.nsid;

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
