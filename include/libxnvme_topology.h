/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @headerfile libxnvme_topology.h
 */

struct xnvme_subsystem {
	struct xnvme_dev *dev;

	struct xnvme_dev *controllers;
};

struct xnvme_controller {
	struct xnvme_dev *dev;
	struct xnvme_dev *namespaces;
};

struct xnvme_namespace {
	struct xnvme_dev *dev;
};

int
xnvme_subsystem_reset(struct xnvme_dev *dev);

int
xnvme_controller_reset(struct xnvme_dev *dev);

int
xnvme_namespace_rescan(struct xnvme_dev *dev);

/**
 * Read NVMe PCIe controller reigsters
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param bar NVMe PCIe BAR0 registers buffer to save read values
 * @return On success, 0 is return. On error, a non-zero value is returned.
 */
int
xnvme_controller_get_registers(struct xnvme_dev *dev, struct xnvme_spec_ctrlr_bar *bar);
