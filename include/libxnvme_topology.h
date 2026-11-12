/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_topology.h
 */

/**
 * An NVMe subsystem, the device handle for it and those for its controllers
 *
 * @note No function of the API takes or returns it
 */
struct xnvme_subsystem {
	struct xnvme_dev *dev; ///< Device handle of the subsystem

	struct xnvme_dev *controllers; ///< Device handles of the controllers of the subsystem
};

/**
 * An NVMe controller, the device handle for it and those for its namespaces
 *
 * @note No function of the API takes or returns it
 */
struct xnvme_controller {
	struct xnvme_dev *dev;        ///< Device handle of the controller
	struct xnvme_dev *namespaces; ///< Device handles of the namespaces of the controller
};

/**
 * An NVMe namespace, the device handle for it
 *
 * @note No function of the API takes or returns it
 */
struct xnvme_namespace {
	struct xnvme_dev *dev; ///< Device handle of the namespace
};

/**
 * Reset the NVM subsystem of the controller behind 'dev'
 *
 * The Linux NVMe driver does it with NVME_IOCTL_SUBSYS_RESET, SPDK with
 * spdk_nvme_ctrlr_reset_subsystem(); other backends return -ENOSYS.
 *
 * @param dev Device handle obtained with xnvme_dev_open_uri()
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned, or the negated
 * status code of the completion.
 */
int
xnvme_subsystem_reset(struct xnvme_dev *dev);

/**
 * Reset the controller behind 'dev'
 *
 * The Linux NVMe driver does it with NVME_IOCTL_RESET, SPDK with spdk_nvme_ctrlr_reset(); other
 * backends return -ENOSYS.
 *
 * @param dev Device handle obtained with xnvme_dev_open_uri()
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned, or the negated
 * status code of the completion.
 */
int
xnvme_controller_reset(struct xnvme_dev *dev);

/**
 * Rescan the namespaces of the controller behind 'dev'
 *
 * The Linux NVMe driver does it with NVME_IOCTL_RESCAN; the upcie backend does nothing and
 * returns 0; other backends return -ENOSYS.
 *
 * @param dev Device handle obtained with xnvme_dev_open_uri()
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned, or the negated
 * status code of the completion.
 */
int
xnvme_namespace_rescan(struct xnvme_dev *dev);

/**
 * Read NVMe PCIe controller reigsters
 *
 * @param dev Device handle obtained with xnvme_dev_open_uri()
 * @param bar NVMe PCIe BAR0 registers buffer to save read values
 * @return On success, 0 is return. On error, a non-zero value is returned.
 */
int
xnvme_controller_get_registers(struct xnvme_dev *dev, struct xnvme_spec_ctrlr_bar *bar);
