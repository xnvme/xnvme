// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef QUBLK_ZONED_H
#define QUBLK_ZONED_H

#include "qublk.h"

/**
 * Serving a ZNS namespace as a zoned ublk device. All of it lives in zoned.c;
 * a qublk built without zoned ublk support refuses zoned namespaces there.
 */

int
qublk_zoned_init(struct qublk_dev *dev, int passes_nvme);

int
qublk_zoned_check_features(const struct qublk_dev *dev, uint64_t feat);

void
qublk_zoned_add_failed(const struct qublk_dev *dev, int rc);

void
qublk_zoned_print_info(const struct qublk_dev *dev);

void
qublk_zoned_set_params(const struct qublk_dev *dev, struct ublk_params *params);

size_t
qublk_zoned_buf_slack(const struct qublk_dev *dev);

int
qublk_zoned_op_is_append(uint8_t op);

int
qublk_zoned_dispatch(struct qublk_queue *q, struct qublk_io *io, struct xnvme_cmd_ctx *ctx);

int
qublk_zoned_complete(struct qublk_queue *q, struct qublk_io *io, struct xnvme_cmd_ctx *ctx);

#endif
