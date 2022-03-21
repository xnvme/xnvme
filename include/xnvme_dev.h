// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_DEV_H
#define __INTERNAL_XNVME_DEV_H

#include <libxnvme.h>
#include <xnvme_be.h>

enum xnvme_dev_type {
	XNVME_DEV_TYPE_UNKNOWN,
	XNVME_DEV_TYPE_NVME_CONTROLLER,
	XNVME_DEV_TYPE_NVME_NAMESPACE,
	XNVME_DEV_TYPE_BLOCK_DEVICE,
	XNVME_DEV_TYPE_FS_FILE,
	XNVME_DEV_TYPE_RAMDISK,
};

struct xnvme_dev {
	struct xnvme_geo geo;     ///< Device geometry
	struct xnvme_be be;       ///< Backend interface
	struct xnvme_ident ident; ///< Device identifier

	uint8_t _pad[4];

	struct {
		struct xnvme_spec_idfy_ctrlr ctrlr; ///< NVMe id-ctrlr
		struct xnvme_spec_idfy_ns ns;       ///< NVMe id-ns
	} id;

	struct {
		struct xnvme_spec_idfy_ctrlr ctrlr; ///< NVMe id-ctrlr
		struct xnvme_spec_idfy_ns ns;       ///< NVMe id-ns
	} idcss;                                    ///< Command Set Specific

	struct xnvme_opts opts; ///< Options
};
// XNVME_STATIC_ASSERT(sizeof(struct xnvme_ident) == 768, "Incorrect size")

int
xnvme_dev_alloc(struct xnvme_dev **dev);

int
xnvme_dev_be_init(struct xnvme_dev *dev, struct xnvme_be *be, const char *uri);

#endif /* __INTERNAL_XNVME_DEV_H */
