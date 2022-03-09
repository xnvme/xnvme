// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_CMD_H
#define __INTERNAL_XNVME_CMD_H

/**
 * Enumeration of `xnvme_cmd` options
 *
 * @enum xnvme_cmd_opts
 */
enum xnvme_cmd_opts {
	XNVME_CMD_SYNC  = 0x1 << 0, ///< XNVME_CMD_SYNC: Synchronous command
	XNVME_CMD_ASYNC = 0x1 << 1, ///< XNVME_CMD_ASYNC: Asynchronous command

	XNVME_CMD_UPLD_SGLD = 0x1 << 2, ///< XNVME_CMD_UPLD_SGLD: User-managed SGL data
	XNVME_CMD_UPLD_SGLM = 0x1 << 3, ///< XNVME_CMD_UPLD_SGLM: User-managed SGL meta
};

#define XNVME_CMD_MASK_IOMD (XNVME_CMD_SYNC | XNVME_CMD_ASYNC)
#define XNVME_CMD_MASK_UPLD (XNVME_CMD_UPLD_SGLD | XNVME_CMD_UPLD_SGLM)
#define XNVME_CMD_MASK      (XNVME_CMD_MASK_IOMD | XNVME_CMD_MASK_UPLD)

#define XNVME_CMD_DEF_IOMD XNVME_CMD_SYNC
#define XNVME_CMD_DEF_UPLD (0x0)

#endif /* __INTERNAL_XNVME_CMD_H */
