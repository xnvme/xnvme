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
qublk_io_threads_start(struct qublk_dev *devs, uint32_t ndevs, struct qublk_thread **threads,
		       uint32_t *nthreads);
void
qublk_io_threads_join(struct qublk_thread *threads, uint32_t nthreads);

#endif
