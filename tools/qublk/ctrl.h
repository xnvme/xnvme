// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef QUBLK_CTRL_H
#define QUBLK_CTRL_H

#include "qublk.h"

int
qublk_ctrl_open(struct qublk_dev *dev);
void
qublk_ctrl_close(struct qublk_dev *dev);

int
qublk_ctrl_get_features(struct qublk_dev *dev, uint64_t *features);
int
qublk_ctrl_add_dev(struct qublk_dev *dev);
int
qublk_ctrl_set_params(struct qublk_dev *dev);
int
qublk_ctrl_start_dev(struct qublk_dev *dev);
int
qublk_ctrl_stop_dev(struct qublk_dev *dev);
int
qublk_ctrl_del_dev(struct qublk_dev *dev);

#endif
