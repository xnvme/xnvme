// SPDX-FileCopyrightText: Simon A. F. Lund
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_DMAMEM_ENABLED
#include <dirent.h>
#include <limits.h>
#include <stdatomic.h>
#include <xnvme_dev.h>
#include <xnvme_be_dmamem.h>

static _Atomic int g_ctrlr_count;

int
xnvme_be_dmamem_resolve_vfio_cdev(const char *bdf, char *cdev_path, size_t cdev_path_len)
{
	char sysfs_path[PATH_MAX] = {0};
	DIR *dir;
	struct dirent *ent;
	int found = 0;

	snprintf(sysfs_path, sizeof(sysfs_path), "/sys/bus/pci/devices/%s/vfio-dev", bdf);
	dir = opendir(sysfs_path);
	if (!dir) {
		return -errno;
	}
	while ((ent = readdir(dir))) {
		if (strncmp(ent->d_name, "vfio", 4) != 0) {
			continue;
		}
		snprintf(cdev_path, cdev_path_len, "/dev/vfio/devices/%s", ent->d_name);
		found = 1;
		break;
	}
	closedir(dir);
	return found ? 0 : -ENOENT;
}

/**
 * Return non-zero if the target BDF has a vfio-cdev entry, i.e.
 * /sys/bus/pci/devices/<bdf>/vfio-dev/vfio* exists.
 */
static int
_bdf_has_vfio_cdev(const char *bdf)
{
	char sysfs_path[PATH_MAX] = {0};
	DIR *dir;
	struct dirent *ent;
	int found = 0;

	snprintf(sysfs_path, sizeof(sysfs_path), "/sys/bus/pci/devices/%s/vfio-dev", bdf);
	dir = opendir(sysfs_path);
	if (!dir) {
		return 0;
	}
	while ((ent = readdir(dir))) {
		if (strncmp(ent->d_name, "vfio", 4) == 0) {
			found = 1;
			break;
		}
	}
	closedir(dir);
	return found;
}

/**
 * Read the kernel driver bound to `bdf` and derive the dmamem mode.
 *
 * vfio-pci        -> VFIO_CDEV if /dev/iommu and a vfio-cdev entry both
 *                    exist, otherwise VFIO_TYPE1 (legacy vfio container).
 *                    Env override: XNVME_DMAMEM_VFIO_MODE = iommufd |
 *                    type1 | auto.
 * uio_pci_generic -> UIO_LUT (pci_bar_map + hostmem hugepage + LUT).
 * anything else   -> -ENOTSUP.
 */
static int
_detect_mode(const char *bdf, enum xnvme_be_dmamem_mode *mode_out)
{
	char link_path[PATH_MAX] = {0};
	char resolved[PATH_MAX] = {0};
	const char *driver;
	const char *override;
	ssize_t ret;

	snprintf(link_path, sizeof(link_path), "/sys/bus/pci/devices/%s/driver", bdf);
	ret = readlink(link_path, resolved, sizeof(resolved) - 1);
	if (ret < 0) {
		return -errno;
	}
	resolved[ret] = '\0';

	driver = strrchr(resolved, '/');
	driver = driver ? driver + 1 : resolved;

	if (!strcmp(driver, "uio_pci_generic")) {
		*mode_out = XNVME_BE_DMAMEM_MODE_UIO_LUT;
		return 0;
	}

	if (strcmp(driver, "vfio-pci")) {
		XNVME_DEBUG("FAILED: unsupported driver '%s' bound to '%s'", driver, bdf);
		return -ENOTSUP;
	}

	override = getenv("XNVME_DMAMEM_VFIO_MODE");
	if (override && !strcmp(override, "iommufd")) {
		*mode_out = XNVME_BE_DMAMEM_MODE_VFIO_CDEV;
		return 0;
	}
	if (override && !strcmp(override, "type1")) {
		*mode_out = XNVME_BE_DMAMEM_MODE_VFIO_TYPE1;
		return 0;
	}

	/* auto: prefer iommufd if usable, else type1 */
	if (access("/dev/iommu", R_OK | W_OK) == 0 && _bdf_has_vfio_cdev(bdf)) {
		*mode_out = XNVME_BE_DMAMEM_MODE_VFIO_CDEV;
	} else {
		*mode_out = XNVME_BE_DMAMEM_MODE_VFIO_TYPE1;
	}
	return 0;
}

/**
 * Release the process-wide RTE.
 *
 * Order matters: heap first (releases the freelist bookkeeping), then
 * dmamem (unmaps the DMA mapping in VFIO_CDEV mode; no-op in UIO_LUT),
 * then the mode-specific backing (memfd + IOAS + iommufd, or
 * hostmem_hugepage), then reset the mode.
 */
static void
_rte_term(void)
{
	if (!g_dmamem_rte.is_initialized) {
		return;
	}

	if (g_dmamem_rte.heap_alive) {
		dmamem_heap_term(&g_dmamem_rte.heap);
		g_dmamem_rte.heap_alive = 0;
	}
	if (g_dmamem_rte.dmem_alive) {
		dmamem_destroy(&g_dmamem_rte.dmem);
		g_dmamem_rte.dmem_alive = 0;
	}
	if (g_dmamem_rte.hp_alive) {
		hostmem_hugepage_free(&g_dmamem_rte.hp);
		g_dmamem_rte.hp_alive = 0;
	}
	if (g_dmamem_rte.type1_container_alive) {
		vfio_container_close(&g_dmamem_rte.type1_container);
		g_dmamem_rte.type1_container_alive = 0;
		g_dmamem_rte.type1_iommu_set = 0;
	}
	if (g_dmamem_rte.ioas_alive) {
		iommufd_destroy(&g_dmamem_rte.iommufd, g_dmamem_rte.ioas_id);
		g_dmamem_rte.ioas_alive = 0;
	}
	if (g_dmamem_rte.iommufd_alive) {
		iommufd_close(&g_dmamem_rte.iommufd);
		g_dmamem_rte.iommufd_alive = 0;
	}

	g_dmamem_rte.mode = XNVME_BE_DMAMEM_MODE_UNSET;
	g_dmamem_rte.is_initialized = 0;
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

	err = iommufd_open(&g_dmamem_rte.iommufd);
	if (err) {
		XNVME_DEBUG("FAILED: iommufd_open(); err(%d)", err);
		return err;
	}
	g_dmamem_rte.iommufd_alive = 1;

	err = iommufd_ioas_alloc(&g_dmamem_rte.iommufd, &g_dmamem_rte.ioas_id);
	if (err) {
		XNVME_DEBUG("FAILED: iommufd_ioas_alloc(); err(%d)", err);
		return err;
	}
	g_dmamem_rte.ioas_alive = 1;

	err = dmamem_from_memfd(&g_dmamem_rte.dmem, &g_dmamem_rte.iommufd, g_dmamem_rte.ioas_id,
				heap_size, hugepgsz);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_from_memfd(); err(%d)", err);
		return err;
	}
	g_dmamem_rte.dmem_alive = 1;

	err = dmamem_heap_init(&g_dmamem_rte.heap, &g_dmamem_rte.dmem, 4096);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_heap_init(); err(%d)", err);
		return err;
	}
	g_dmamem_rte.heap_alive = 1;

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

	err = hostmem_config_init(&g_dmamem_rte.hp_cfg);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_config_init(); err(%d)", err);
		return err;
	}
	heap_size =
		((heap_size + g_dmamem_rte.hp_cfg.hugepgsz - 1) / g_dmamem_rte.hp_cfg.hugepgsz) *
		g_dmamem_rte.hp_cfg.hugepgsz;

	err = hostmem_hugepage_alloc(heap_size, &g_dmamem_rte.hp, &g_dmamem_rte.hp_cfg);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_hugepage_alloc(); err(%d)", err);
		return err;
	}
	g_dmamem_rte.hp_alive = 1;

	err = dmamem_from_hostmem_lut(&g_dmamem_rte.dmem, &g_dmamem_rte.hp);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_from_hostmem_lut(); err(%d) "
			    "(missing CAP_SYS_ADMIN?)",
			    err);
		return err;
	}
	g_dmamem_rte.dmem_alive = 1;

	err = dmamem_heap_init(&g_dmamem_rte.heap, &g_dmamem_rte.dmem, 4096);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_heap_init(); err(%d)", err);
		return err;
	}
	g_dmamem_rte.heap_alive = 1;

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

	err = hostmem_config_init(&g_dmamem_rte.hp_cfg);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_config_init(); err(%d)", err);
		return err;
	}
	heap_size =
		((heap_size + g_dmamem_rte.hp_cfg.hugepgsz - 1) / g_dmamem_rte.hp_cfg.hugepgsz) *
		g_dmamem_rte.hp_cfg.hugepgsz;

	err = hostmem_hugepage_alloc(heap_size, &g_dmamem_rte.hp, &g_dmamem_rte.hp_cfg);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_hugepage_alloc(); err(%d)", err);
		return err;
	}
	g_dmamem_rte.hp_alive = 1;

	err = vfio_container_open(&g_dmamem_rte.type1_container);
	if (err) {
		XNVME_DEBUG("FAILED: vfio_container_open(); err(%d)", err);
		return err;
	}
	g_dmamem_rte.type1_container_alive = 1;

	return 0;
}

/**
 * Bring up the process-wide RTE in the given mode, or verify an already
 * initialized RTE matches.
 */
static int
_rte_init(enum xnvme_be_dmamem_mode mode, size_t heap_size)
{
	int err;

	if (g_dmamem_rte.is_initialized) {
		if (g_dmamem_rte.mode != mode) {
			XNVME_DEBUG("FAILED: existing dmamem RTE mode(%d) != requested(%d)",
				    g_dmamem_rte.mode, mode);
			return -EINVAL;
		}
		return 0;
	}

	if (!heap_size) {
		heap_size = XNVME_BE_DMAMEM_DEFAULT_HEAP_SIZE;
	}

	g_dmamem_rte.mode = mode;

	switch (mode) {
	case XNVME_BE_DMAMEM_MODE_VFIO_CDEV:
		err = _rte_init_vfio_cdev(heap_size);
		break;
	case XNVME_BE_DMAMEM_MODE_UIO_LUT:
		err = _rte_init_uio_lut(heap_size);
		break;
	case XNVME_BE_DMAMEM_MODE_VFIO_TYPE1:
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

	g_dmamem_rte.is_initialized = 1;
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
_type1_attach_and_maybe_map(struct xnvme_be_dmamem_ctrlr *ctrlr, const char *bdf)
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

	err = vfio_group_set_container(&ctrlr->type1_group, &g_dmamem_rte.type1_container);
	if (err < 0) {
		XNVME_DEBUG("FAILED: vfio_group_set_container(); errno(%d)", errno);
		err = -errno;
		goto fail_group;
	}
	ctrlr->type1_group_attached = 1;

	if (g_dmamem_rte.type1_iommu_set) {
		/* Container already set_iommu'd + mapped by an earlier controller. */
		return 0;
	}

	err = vfio_get_api_version(&g_dmamem_rte.type1_container, &api_version);
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

	if (!vfio_check_extension(&g_dmamem_rte.type1_container, VFIO_TYPE1_IOMMU)) {
		XNVME_DEBUG("FAILED: VFIO_TYPE1_IOMMU extension not supported");
		err = -ENOTSUP;
		goto fail_group;
	}

	err = vfio_set_iommu(&g_dmamem_rte.type1_container, VFIO_TYPE1_IOMMU);
	if (err < 0) {
		XNVME_DEBUG("FAILED: vfio_set_iommu(TYPE1); errno(%d)", errno);
		err = -errno;
		goto fail_group;
	}
	g_dmamem_rte.type1_iommu_set = 1;

	/* MAP_DMA the whole hugepage at a caller-chosen base_iova; the
	 * dmamem sits on that mapping as ARITHMETIC (base_iova + offset). */
	err = dmamem_from_hostmem_type1(&g_dmamem_rte.dmem, &g_dmamem_rte.type1_container,
					(uint64_t)0, &g_dmamem_rte.hp);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_from_hostmem_type1(); err(%d)", err);
		goto fail_group;
	}
	g_dmamem_rte.dmem_alive = 1;

	err = dmamem_heap_init(&g_dmamem_rte.heap, &g_dmamem_rte.dmem, 4096);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_heap_init(); err(%d)", err);
		dmamem_destroy(&g_dmamem_rte.dmem);
		g_dmamem_rte.dmem_alive = 0;
		goto fail_group;
	}
	g_dmamem_rte.heap_alive = 1;

	return 0;

fail_group:
	if (ctrlr->type1_group.fd >= 0) {
		vfio_group_close(&ctrlr->type1_group);
		ctrlr->type1_group_attached = 0;
	}
	return err;
}

/**
 * Bring up the shared controller state for one physical NVMe.
 *
 * Detects the kernel driver bound to the device, brings up (or verifies)
 * the RTE in the matching mode, opens the controller through one of the
 * three nvme_controller_open_dmamem_{vfio,uio,type1} entry points, and
 * creates one shared sync IO qpair for xnvme's synchronous submit path.
 * The async path allocates additional qpairs on demand via queue_init.
 * The IO qpair helpers are shared between modes.
 */
void *
xnvme_be_dmamem_ctrlr_init(struct xnvme_dev *dev)
{
	struct xnvme_be_dmamem_ctrlr *ctrlr;
	enum xnvme_be_dmamem_mode mode = XNVME_BE_DMAMEM_MODE_UNSET;
	char cdev_path[PATH_MAX] = {0};
	int err;

	err = _detect_mode(dev->ident.uri, &mode);
	if (err) {
		XNVME_DEBUG("FAILED: _detect_mode(%s); err(%d)", dev->ident.uri, err);
		errno = -err;
		return NULL;
	}

	err = _rte_init(mode, dev->opts.host_heap_size);
	if (err) {
		XNVME_DEBUG("FAILED: _rte_init(); err(%d)", err);
		errno = -err;
		return NULL;
	}

	if (XNVME_BE_DMAMEM_MODE_VFIO_CDEV == mode) {
		err = xnvme_be_dmamem_resolve_vfio_cdev(dev->ident.uri, cdev_path,
							sizeof(cdev_path));
		if (err) {
			XNVME_DEBUG("FAILED: resolve_vfio_cdev(%s); err(%d)", dev->ident.uri, err);
			errno = -err;
			return NULL;
		}
	}

	ctrlr = calloc(1, sizeof(*ctrlr));
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: calloc(ctrlr)");
		errno = ENOMEM;
		return NULL;
	}

	ctrlr->ctrl = calloc(1, sizeof(*ctrlr->ctrl));
	if (!ctrlr->ctrl) {
		XNVME_DEBUG("FAILED: calloc(ctrl)");
		free(ctrlr);
		errno = ENOMEM;
		return NULL;
	}

	ctrlr->type1_group.fd = -1;

	switch (mode) {
	case XNVME_BE_DMAMEM_MODE_VFIO_CDEV:
		err = nvme_controller_open_dmamem_vfio(ctrlr->ctrl, &ctrlr->ctx,
						       &g_dmamem_rte.iommufd, g_dmamem_rte.ioas_id,
						       &g_dmamem_rte.heap, cdev_path);
		break;
	case XNVME_BE_DMAMEM_MODE_UIO_LUT:
		err = nvme_controller_open_dmamem_uio(ctrlr->ctrl, &ctrlr->uio_ctx,
						      &g_dmamem_rte.heap, dev->ident.uri);
		break;
	case XNVME_BE_DMAMEM_MODE_VFIO_TYPE1:
		err = _type1_attach_and_maybe_map(ctrlr, dev->ident.uri);
		if (err) {
			break;
		}
		err = nvme_controller_open_dmamem_type1(
			ctrlr->ctrl, &ctrlr->type1_ctx, &g_dmamem_rte.type1_container,
			&ctrlr->type1_group, &g_dmamem_rte.heap, dev->ident.uri);
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
		XNVME_DEBUG("FAILED: nvme_controller_open_dmamem*(); err(%d)", err);
		free(ctrlr->ctrl);
		free(ctrlr);
		errno = -err;
		return NULL;
	}

	err = nvme_controller_create_io_qpair_dmamem(
		ctrlr->ctrl, &ctrlr->sync, 16, &g_dmamem_rte.heap, &ctrlr->sync_sq_offset,
		&ctrlr->sync_cq_offset, &ctrlr->sync_prp_offset);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_controller_create_io_qpair_dmamem(); err(%d)", err);
		switch (mode) {
		case XNVME_BE_DMAMEM_MODE_VFIO_CDEV:
			nvme_controller_close_dmamem_vfio(ctrlr->ctrl, &ctrlr->ctx,
							  &g_dmamem_rte.heap);
			break;
		case XNVME_BE_DMAMEM_MODE_UIO_LUT:
			nvme_controller_close_dmamem_uio(ctrlr->ctrl, &ctrlr->uio_ctx,
							 &g_dmamem_rte.heap);
			break;
		case XNVME_BE_DMAMEM_MODE_VFIO_TYPE1:
			nvme_controller_close_dmamem_type1(ctrlr->ctrl, &ctrlr->type1_ctx,
							   &g_dmamem_rte.heap);
			if (ctrlr->type1_group_attached) {
				vfio_group_close(&ctrlr->type1_group);
				ctrlr->type1_group_attached = 0;
			}
			break;
		default:
			break;
		}
		free(ctrlr->ctrl);
		free(ctrlr);
		errno = -err;
		return NULL;
	}

	g_ctrlr_count++;
	return ctrlr;
}

int
xnvme_be_dmamem_ctrlr_term(void *handle)
{
	struct xnvme_be_dmamem_ctrlr *ctrlr = handle;

	nvme_controller_delete_io_qpair_dmamem(ctrlr->ctrl, &ctrlr->sync, &g_dmamem_rte.heap,
					       ctrlr->sync_sq_offset, ctrlr->sync_cq_offset,
					       ctrlr->sync_prp_offset);

	switch (g_dmamem_rte.mode) {
	case XNVME_BE_DMAMEM_MODE_VFIO_CDEV:
		nvme_controller_close_dmamem_vfio(ctrlr->ctrl, &ctrlr->ctx, &g_dmamem_rte.heap);
		break;
	case XNVME_BE_DMAMEM_MODE_UIO_LUT:
		nvme_controller_close_dmamem_uio(ctrlr->ctrl, &ctrlr->uio_ctx, &g_dmamem_rte.heap);
		break;
	case XNVME_BE_DMAMEM_MODE_VFIO_TYPE1:
		nvme_controller_close_dmamem_type1(ctrlr->ctrl, &ctrlr->type1_ctx,
						   &g_dmamem_rte.heap);
		if (ctrlr->type1_group_attached) {
			vfio_group_close(&ctrlr->type1_group);
			ctrlr->type1_group_attached = 0;
		}
		break;
	default:
		break;
	}

	free(ctrlr->ctrl);
	free(ctrlr);

	if (--g_ctrlr_count == 0) {
		_rte_term();
	}

	return 0;
}

void
xnvme_be_dmamem_dev_close(struct xnvme_dev *XNVME_UNUSED(dev))
{
}

int
xnvme_be_dmamem_dev_open(struct xnvme_dev *dev)
{
	dev->ident.dtype =
		dev->opts.nsid ? XNVME_DEV_TYPE_NVME_NAMESPACE : XNVME_DEV_TYPE_NVME_CONTROLLER;
	dev->ident.csi = XNVME_SPEC_CSI_NVM;
	dev->ident.nsid = dev->opts.nsid;

	return 0;
}

#endif

struct xnvme_be_dev g_xnvme_be_dmamem_dev = {
#ifdef XNVME_BE_DMAMEM_ENABLED
	.dev_open = xnvme_be_dmamem_dev_open,
	.dev_close = xnvme_be_dmamem_dev_close,
	.id = "dmamem",
	.ctrlr_init = xnvme_be_dmamem_ctrlr_init,
	.ctrlr_term = xnvme_be_dmamem_ctrlr_term,
#else
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
	.id = "nosys",
	.ctrlr_init = NULL,
	.ctrlr_term = NULL,
#endif
};
