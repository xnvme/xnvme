// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_ENABLED
#include <limits.h>
#include <sys/file.h>
#include <sys/stat.h>
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

	if (g_upcie_rte.mproc) {
		xnvme_be_upcie_mproc_rte_term();
	}

	hostmem_heap_term(&g_upcie_rte.heap);

	g_upcie_rte.is_initialized = 0;
}

/**
 * Initialize the uPCIe runtime-environment
 *
 * The state is globally accessible via g_upcie_rte, this function initializes it, unless it is
 * already initialized, then it exits early.
 */
static int
_rte_init(struct xnvme_opts *opts)
{
	size_t heap_size = opts->host_heap_size;
	int err;

	if (g_upcie_rte.is_initialized) {
		return 0;
	}

	if (!heap_size) {
		heap_size = XNVME_BE_UPCIE_DEFAULT_HEAP_SIZE;
	}

	err = hostmem_config_init(&g_upcie_rte.config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_config_init(); err(%d)", err);
		return err;
	}

	if (opts->shm_id) {
		err = xnvme_be_upcie_mproc_rte_init(opts->shm_id);
		if (err) {
			XNVME_DEBUG("FAILED: xnvme_be_upcie_mproc_rte_init(); err(%d)", err);
			return err;
		}
	}

	heap_size = ((heap_size + g_upcie_rte.config.hugepgsz - 1) / g_upcie_rte.config.hugepgsz) *
		    g_upcie_rte.config.hugepgsz;

	err = hostmem_heap_init(&g_upcie_rte.heap, heap_size, &g_upcie_rte.config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_heap_init(); err(%d)", err);
		return err;
	}

	if (g_upcie_rte.mproc) {
		if (g_upcie_rte.mproc->is_primary) {
			/* Store hugepage path in shm for secondaries to use for import */
			struct xnvme_be_upcie_mproc_shm *shm = g_upcie_rte.mproc->shm;
			char *path = g_upcie_rte.heap.memory.path;

			snprintf(shm->hugepage_path, sizeof(shm->hugepage_path), "%s", path);

			shm->hugepage_base = (uint64_t)g_upcie_rte.heap.memory.virt;
			atomic_store_explicit(&shm->is_initialized, true, memory_order_release);
		} else {
			struct xnvme_be_upcie_mproc_shm *shm = g_upcie_rte.mproc->shm;

			// Wait 1 second for primary to copy hugepage info to shared memory
			for (int i = 0; i < 1000; i++) {
				if (atomic_load_explicit(&shm->is_initialized,
							 memory_order_acquire)) {
					break;
				}
				usleep(1000);
			}

			if (!atomic_load_explicit(&shm->is_initialized, memory_order_acquire)) {
				XNVME_DEBUG("FAILED: Timed out while waiting for primary process "
					    "hugepage "
					    "information");
				return -ENOENT;
			}

			err = xnvme_be_upcie_mproc_import_admin_hugepage();
			if (err) {
				XNVME_DEBUG(
					"FAILED: xnvme_be_upcie_mproc_import_admin_hugepage(); "
					"err(%d)",
					err);
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

static int
_initialize_ctrlr(struct xnvme_dev *dev, struct xnvme_be_upcie_ctrlr *ctrlr)
{
	char driver_name[sizeof(dev->ident.kernel_driver)] = {0};
	int err;

	err = xnvme_be_upcie_get_driver_name(dev->ident.uri, driver_name, sizeof(driver_name));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_get_driver_name(%s); err(%d)", dev->ident.uri,
			    err);
		return err;
	}
	snprintf(dev->ident.kernel_driver, sizeof(dev->ident.kernel_driver), "%s", driver_name);

	if (g_upcie_rte.mproc) {
		// ctrlr->ctrl stored in shared memory, so no calloc()
		err = xnvme_be_upcie_mproc_ctrlr_shm_init(dev, ctrlr, driver_name);
		if (err) {
			XNVME_DEBUG("FAILED: xnvme_be_upcie_mproc_ctrlr_shm_init(); err(%d)", err);
			return err;
		}
	} else {
		ctrlr->ctrl = calloc(1, sizeof(*ctrlr->ctrl));
		if (!ctrlr->ctrl) {
			XNVME_DEBUG("FAILED: calloc(ctrl)");
			return -ENOMEM;
		}
	}

	if (!strcmp(driver_name, "vfio-pci")) {
		ctrlr->backend = NVME_BACKEND_VFIO;
		err = nvme_controller_open_vfio(ctrlr->ctrl, &ctrlr->vfio, dev->ident.uri,
						&g_upcie_rte.heap);
	} else if (!strcmp(driver_name, "uio_pci_generic")) {
		ctrlr->backend = NVME_BACKEND_SYSFS;
		err = nvme_controller_open(ctrlr->ctrl, dev->ident.uri, &g_upcie_rte.heap);
	} else {
		XNVME_DEBUG("FAILED: unsupported driver '%s'", driver_name);
		err = -ENOTSUP;
	}
	if (err) {
		XNVME_DEBUG("FAILED: %s(%s)",
			    ctrlr->backend == NVME_BACKEND_VFIO ? "nvme_controller_open_vfio"
								: "nvme_controller_open",
			    dev->ident.uri);
		goto failed;
	}

	err = xnvme_be_upcie_ctrlr_mutex_lock(ctrlr);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_ctrlr_mutex_lock(); err(%d)", err);
		goto failed_close_ctrlr;
	}

	err = nvme_controller_create_io_qpair(ctrlr->ctrl, &ctrlr->sync, 16);

	xnvme_be_upcie_ctrlr_mutex_unlock(ctrlr);

	if (err) {
		XNVME_DEBUG("FAILED: nvme_controller_create_io_qpair(%d)", err);
		goto failed_close_ctrlr;
	}

	if (ctrlr->shm) {
		// Publish the fully-opened controller so secondaries may attach.
		atomic_store_explicit(&ctrlr->shm->is_initialized, true, memory_order_release);
	}

	return 0;

failed_close_ctrlr:
	if (ctrlr->backend == NVME_BACKEND_VFIO) {
		nvme_controller_close_vfio(ctrlr->ctrl, &ctrlr->vfio);
	} else {
		nvme_controller_close(ctrlr->ctrl);
	}

failed:
	if (g_upcie_rte.mproc) {
		xnvme_be_upcie_mproc_ctrlr_shm_term(ctrlr);
	} else {
		free(ctrlr->ctrl);
	}

	return err;
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
	bool is_owner; // Is this process the owner of the controller?
	int err;

	err = _rte_init(&dev->opts);
	if (err) {
		XNVME_DEBUG("FAILED: _rte_init()");
		errno = -err;
		return NULL;
	}

	is_owner = !g_upcie_rte.mproc || g_upcie_rte.mproc->is_primary;

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

	if (is_owner) {
		err = _initialize_ctrlr(dev, ctrlr);
	} else {
		err = xnvme_be_upcie_mproc_ctrlr_shm_attach(dev, ctrlr);
	}

	if (err) {
		XNVME_DEBUG("FAILED: %s(); err(%d)",
			    is_owner ? "_initialize_ctrlr"
				     : "xnvme_be_upcie_mproc_ctrlr_shm_attach",
			    err);
		errno = -err;
		goto failed;
	}

	g_ctrlr_count++;

	return ctrlr;

failed:
	if (ctrlr) {
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
	bool is_owner =
		!g_upcie_rte.mproc ||
		g_upcie_rte.mproc->is_primary; // Is this process the owner of the controller?

	if (g_upcie_rte.mproc) {
		if (g_upcie_rte.mproc->is_primary) {
			int attached = atomic_load(&ctrlr->shm->refcount);
			if (attached > 1) {
				XNVME_DEBUG("WARNING: terminating controller with %d secondary "
					    "process(es) still "
					    "attached",
					    attached - 1);
			}
		}

		xnvme_be_upcie_mproc_create_or_delete_io_qpair(ctrlr, &ctrlr->sync, 0, false);

		// Manually send qpair deletion commands to NVMe controller to not
		// free DMA memory that the primary process does not own. Is skipped
		// automatically if not in multi-process mode.
		xnvme_be_upcie_mproc_free_all_queues(ctrlr);
	} else {
		nvme_controller_delete_io_qpair(ctrlr->ctrl, &ctrlr->sync);
	}

	if (is_owner) {
		if (ctrlr->backend == NVME_BACKEND_VFIO) {
			nvme_controller_close_vfio(ctrlr->ctrl, &ctrlr->vfio);
		} else {
			nvme_controller_close(ctrlr->ctrl);
		}
	} else {
		nvme_request_pool_term_prps(ctrlr->ctrl->aq.rpool, &g_upcie_rte.heap);
		pci_func_close(&ctrlr->ctrl->func);
		free(ctrlr->ctrl->aq.rpool);
	}

	if (g_upcie_rte.mproc) {
		xnvme_be_upcie_mproc_ctrlr_shm_term(ctrlr);
	}

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
