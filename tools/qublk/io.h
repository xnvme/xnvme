// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef QUBLK_IO_H
#define QUBLK_IO_H

#include "qublk.h"

int
qublk_io_init(struct qublk_dev *dev);
void
qublk_io_fini(struct qublk_dev *dev);

int
qublk_io_thread_start(struct qublk_dev *dev);
void
qublk_io_thread_join(struct qublk_dev *dev);

#endif
