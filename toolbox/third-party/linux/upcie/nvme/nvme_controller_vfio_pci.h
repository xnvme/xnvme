// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * NVMe controller bring-up helpers shared across vfio-pci backings
 * ================================================================
 *
 * From a caller-supplied vfio device fd, acquire BAR0 and drive the
 * NVMe controller enable sequence. Both nvme_controller_open_vfio
 * (legacy vfio type1 container) and nvme_controller_open_dmamem_vfio
 * (iommufd + dmamem) reach the same code once they have a device fd:
 * bus-master enable, BAR0 mmap, CC.EN=0/1 dance, CAP/CSTS handshake.
 * That invariant middle lives here. The parts that vary (how the fd
 * was obtained, how DMA is mapped, which heap backs the admin queue)
 * stay with each caller.
 *
 * @file nvme_controller_vfio_pci.h
 * @version 0.5.1
 */

/**
 * Enable PCI bus master on a vfio device via config-space write.
 *
 * The controller needs BM=1 to DMA into any host- or peer-mapped
 * memory, regardless of which DMA API installed the mapping.
 */
static inline int
nvme_vfio_pci_bus_master_enable(int device_fd)
{
	struct vfio_region_info config = {0};
	uint16_t cmd;
	ssize_t ret;
	int err;

	config.index = VFIO_PCI_CONFIG_REGION_INDEX;
	err = vfio_device_get_region_info(device_fd, &config);
	if (err < 0) {
		return -errno;
	}

	ret = pread(device_fd, &cmd, sizeof(cmd), config.offset + PCI_COMMAND);
	if (ret != (ssize_t)sizeof(cmd)) {
		return ret < 0 ? -errno : -EIO;
	}

	if (!(cmd & PCI_COMMAND_MASTER)) {
		cmd |= PCI_COMMAND_MASTER;
		ret = pwrite(device_fd, &cmd, sizeof(cmd), config.offset + PCI_COMMAND);
		if (ret != (ssize_t)sizeof(cmd)) {
			return ret < 0 ? -errno : -EIO;
		}
	}

	return 0;
}

/**
 * Acquire BAR0 for the given vfio device.
 *
 * Enables bus master, mmap's BAR0 through the vfio fd, and populates
 * ctrlr->func.bars[0] so the submit/reap primitives can find the
 * doorbell region.
 *
 * @param device_fd      Open vfio device fd (obtained from a group or
 *                       from an iommufd cdev bind).
 * @param ctrlr          Controller descriptor; func.bars[0] is filled in.
 * @param bar0_out       On success, the mmap'd BAR0 base.
 * @param bar0_size_out  On success, the mapping length.
 *
 * @return 0 on success, negative errno on error.
 */
static inline int
nvme_vfio_pci_acquire_bar0(int device_fd, struct nvme_controller *ctrlr, void **bar0_out,
			   size_t *bar0_size_out)
{
	struct vfio_region_info region = {0};
	void *bar0;
	int err;

	err = nvme_vfio_pci_bus_master_enable(device_fd);
	if (err) {
		UPCIE_DEBUG("FAILED: nvme_vfio_pci_bus_master_enable(); err(%d)", err);
		return err;
	}

	region.index = VFIO_PCI_BAR0_REGION_INDEX;
	err = vfio_device_get_region_info(device_fd, &region);
	if (err < 0) {
		UPCIE_DEBUG("FAILED: vfio_device_get_region_info(); errno(%d)", errno);
		return -errno;
	}

	bar0 = vfio_map_region(device_fd, region.size, region.offset);
	if (bar0 == MAP_FAILED) {
		UPCIE_DEBUG("FAILED: vfio_map_region(); errno(%d)", errno);
		return -errno;
	}

	ctrlr->func.bars[0].region = bar0;
	ctrlr->func.bars[0].size = region.size;
	ctrlr->func.bars[0].id = 0;
	ctrlr->func.bars[0].fd = device_fd;

	*bar0_out = bar0;
	*bar0_size_out = region.size;

	return 0;
}

/**
 * Read CAP, derive controller timeout, disable the controller, wait
 * for CSTS.RDY=0.
 *
 * The caller uses *cap_out when programming CC on enable.
 */
static inline int
nvme_controller_reset_via_bar0(struct nvme_controller *ctrlr, uint8_t *bar0, uint64_t *cap_out)
{
	uint64_t cap = nvme_mmio_cap_read(bar0);
	int err;

	ctrlr->timeout_ms = nvme_reg_cap_get_to(cap) * 500;

	nvme_mmio_cc_disable(bar0);

	err = nvme_mmio_csts_wait_until_not_ready(bar0, ctrlr->timeout_ms);
	if (err) {
		UPCIE_DEBUG("FAILED: nvme_mmio_csts_wait_until_not_ready(); err(%d)", err);
		return err;
	}

	*cap_out = cap;
	return 0;
}

/**
 * Program CC with the standard defaults and wait for CSTS.RDY=1.
 *
 * The AQA/ASQ/ACQ programming must have already happened (via
 * nvme_mmio_aq_setup) so the controller finds the admin queue on
 * enable.
 */
static inline int
nvme_controller_enable_via_bar0(struct nvme_controller *ctrlr, uint8_t *bar0, uint64_t cap)
{
	uint32_t css = (nvme_reg_cap_get_css(cap) & (1 << 6)) ? 0x6 : 0x0;
	uint32_t cc = 0;
	int err;

	cc = nvme_reg_cc_set_css(cc, css);
	cc = nvme_reg_cc_set_shn(cc, 0x0);
	cc = nvme_reg_cc_set_mps(cc, 0x0);
	cc = nvme_reg_cc_set_ams(cc, 0x0);
	cc = nvme_reg_cc_set_iosqes(cc, 6);
	cc = nvme_reg_cc_set_iocqes(cc, 4);
	cc = nvme_reg_cc_set_en(cc, 0x1);

	nvme_mmio_cc_write(bar0, cc);

	err = nvme_mmio_csts_wait_until_ready(bar0, ctrlr->timeout_ms);
	if (err) {
		UPCIE_DEBUG("FAILED: nvme_mmio_csts_wait_until_ready(); err(%d)", err);
		return err;
	}

	return 0;
}
