// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_ENABLED
#include <stdatomic.h>
#include <xnvme_dev.h>
#include <xnvme_be_upcie.h>

static _Atomic int g_ctrlr_count;

struct xnvme_be_upcie_enumerate_context {
	struct xnvme_opts *opts;
	xnvme_enumerate_cb cb_func;
	void *cb_args;
};

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
_rte_init(void)
{
	int err;

	if (g_upcie_rte.is_initialized) {
		return 0;
	}

	err = hostmem_config_init(&g_upcie_rte.config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_config_init(); err(%d)", err);
		return err;
	}

	err = hostmem_heap_init(&g_upcie_rte.heap, 256 * 1024 * 1024, &g_upcie_rte.config);
	if (err) {
		XNVME_DEBUG("FAILED: hostmem_heap_init(); err(%d)", err);
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
 * Initialize the uPCIe controller.
 *
 * Initializes the runtime environment, allocates a shared xnvme_be_upcie_ctrlr,
 * opens the NVMe controller and creates a sync qpair. The returned handle is
 * stored in cref and written to dev->be.state[0] by the platform.
 */
static void *
xnvme_be_upcie_ctrlr_init(struct xnvme_dev *dev)
{
	struct xnvme_be_upcie_ctrlr *ctrlr;
	int err;

	err = _rte_init();
	if (err) {
		XNVME_DEBUG("FAILED: _rte_init()");
		errno = -err;
		return NULL;
	}

	err = _pci_enable_bus_master(dev->ident.uri);
	if (err) {
		XNVME_DEBUG("FAILED: _pci_enable_bus_master(%s)", dev->ident.uri);
		errno = -err;
		return NULL;
	}

	ctrlr = calloc(1, sizeof(*ctrlr));
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: calloc(ctrlr)");
		return NULL;
	}

	ctrlr->ctrl = calloc(1, sizeof(*ctrlr->ctrl));
	if (!ctrlr->ctrl) {
		XNVME_DEBUG("FAILED: calloc(ctrl)");
		free(ctrlr);
		return NULL;
	}

	err = nvme_controller_open(ctrlr->ctrl, dev->ident.uri, &g_upcie_rte.heap);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_controller_open(%s)", dev->ident.uri);
		errno = -err;
		free(ctrlr->ctrl);
		free(ctrlr);
		return NULL;
	}

	err = nvme_controller_create_io_qpair(ctrlr->ctrl, &ctrlr->sync, 16);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_controller_create_io_qpair(%d)", err);
		errno = -err;
		nvme_controller_close(ctrlr->ctrl);
		free(ctrlr->ctrl);
		free(ctrlr);
		return NULL;
	}

	g_ctrlr_count++;

	return ctrlr;
}

static int
xnvme_be_upcie_ctrlr_term(void *handle)
{
	struct xnvme_be_upcie_ctrlr *ctrlr = handle;

	nvme_qpair_term(&ctrlr->sync);
	nvme_controller_close(ctrlr->ctrl);
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

int
instantiate(struct pci_func *func, void *callback_arg)
{
	struct xnvme_be_upcie_enumerate_context *ectx = callback_arg;
	struct xnvme_opts opts = *ectx->opts;
	struct xnvme_dev *dev = NULL;

	if (!(((func->ident.classcode >> 8) & 0xFFFF) == 0x0108)) {
		return PCI_SCAN_ACTION_RELEASE_FUNC;
	}

	opts.be = "upcie";

	dev = xnvme_dev_open(func->bdf, &opts);
	if (!dev) {
		XNVME_DEBUG("xnvme_dev_open(); errno()")
		return PCI_SCAN_ACTION_RELEASE_FUNC;
	}

	if (ectx->cb_func(dev, ectx->cb_args)) {
		xnvme_dev_close(dev);
		return PCI_SCAN_ACTION_RELEASE_FUNC;
	}

	return PCI_SCAN_ACTION_CLAIM_FUNC;
}

int
xnvme_be_upcie_enumerate(const char *sys_uri, struct xnvme_opts *opts, xnvme_enumerate_cb cb_func,
			 void *cb_args)
{
	struct xnvme_be_upcie_enumerate_context ectx = {0};
	int err;

	if (sys_uri) {
		XNVME_DEBUG("FAILED: does not support 'sys_uri'");
		return -ENOSYS;
	}

	ectx.opts = opts;
	ectx.cb_func = cb_func;
	ectx.cb_args = cb_args;

	err = pci_scan(instantiate, &ectx);
	if (err) {
		perror("pci_scan()");
	}

	return 0;
}
#endif

struct xnvme_be_dev g_xnvme_be_upcie_dev = {
#ifdef XNVME_BE_UPCIE_ENABLED
	.enumerate = xnvme_be_upcie_enumerate,
	.dev_open = xnvme_be_upcie_dev_open,
	.dev_close = xnvme_be_upcie_dev_close,
	.id = "upcie",
	.ctrlr_init = xnvme_be_upcie_ctrlr_init,
	.ctrlr_term = xnvme_be_upcie_ctrlr_term,
#else
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
	.id = "nosys",
	.ctrlr_init = NULL,
	.ctrlr_term = NULL,
#endif
};
