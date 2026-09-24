// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef QUBLK_H
#define QUBLK_H

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdint.h>

#include <linux/ublk_cmd.h>
#include <liburing.h>
#include <libxnvme.h>

struct qublk_dev;
struct qublk_queue;
struct qublk_thread;

struct qublk_io {
	uint16_t tag;
	void *buf;
	const struct ublksrv_io_desc *iod;
	struct qublk_queue *q;
};

struct qublk_queue {
	int q_id;
	uint32_t depth;
	struct ublksrv_io_desc *iod_arr;
	size_t iod_arr_bytes;
	struct xnvme_queue *xq;
	struct qublk_io *ios;
	struct qublk_dev *dev;
	struct qublk_thread *thread;
};

struct qublk_thread {
	struct io_uring ring;
	struct qublk_queue *queue;
	sem_t *io_ready;
	pthread_t tid;
	int init_rc;
};

struct qublk_dev {
	const char *uri;
	int ctrl_fd;
	int ublkc_fd;
	int dev_id;
	uint32_t nqueues;
	uint32_t qdepth;
	uint32_t max_io_buf;
	uint64_t flags;
	struct xnvme_dev *xdev;
	const struct xnvme_geo *geo;
	uint8_t lba_shift;
	uint8_t has_vwc;
	uint8_t has_fua;
	uint8_t added;
	uint8_t started;
	struct qublk_queue *queues;
	volatile sig_atomic_t stop;
};

#endif
