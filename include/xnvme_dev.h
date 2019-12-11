// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_DEV_H
#define __INTERNAL_XNVME_DEV_H

#include <libxnvme.h>

struct xnvme_dev {
	struct xnvme_geo geo;		///< Device geometry
	struct xnvme_ident ident;	///< Device identifier

	struct xnvme_spec_idfy_ctrlr ctrlr;	///< NVMe identify controller content
	struct xnvme_spec_idfy_ns ns;	///< NVMe identify namespace content

	uint64_t ssw;			///< Bit-width for LBA fmt conversion
	int cmd_opts;			///< Default options for CMD execution

	struct xnvme_be *be;		///< Backend interface
};

#endif /* __INTERNAL_XNVME_DEV_H */
