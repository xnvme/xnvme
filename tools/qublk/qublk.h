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
#include <xnvme_cuse.h>

/**
 * The most entries an io_uring can have: the kernel's IORING_MAX_ENTRIES, which
 * is not exported to user space
 */
#define QUBLK_MAX_RING_ENTRIES 32768

// User copy (kernel 6.5) ABI for older headers; GET_FEATURES says whether the kernel has it
#ifndef UBLK_F_USER_COPY
#define UBLK_F_USER_COPY (1UL << 7)
#define UBLK_IO_BUF_BITS 25
#define UBLK_TAG_OFF     UBLK_IO_BUF_BITS
#define UBLK_TAG_BITS    16
#define UBLK_QID_OFF     (UBLK_TAG_OFF + UBLK_TAG_BITS)
#endif

struct qublk_dev;
struct qublk_queue;
struct qublk_thread;

struct qublk_io {
	uint16_t tag;
	void *buf;
	const struct ublksrv_io_desc *iod;
	struct qublk_queue *q;
	uint64_t zone_append_lba; ///< Sector a zone append landed on, else 0
};

struct qublk_queue {
	int q_id;
	uint32_t depth;
	uint32_t slot; ///< Index in 'thread->queues' and the fixed-file index of the ublkc fd
	struct ublksrv_io_desc *iod_arr;
	size_t iod_arr_bytes;
	struct xnvme_queue *xq;
	struct qublk_io *ios;
	struct qublk_dev *dev;
	struct qublk_thread *thread;
};

struct qublk_thread {
	int cpu; ///< -1 when unpinned
	struct io_uring ring;
	struct qublk_queue **queues;
	uint32_t nqueues;
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
	uint8_t user_copy; ///< UBLK_F_USER_COPY: data moves via pread()/pwrite() on ublkc_fd
	uint8_t added;
	uint8_t started;

	// Zoned serving (zoned.c); the geometry is in 512 B sectors and valid when 'zoned'
	uint8_t zoned;
	uint32_t zone_sectors;
	uint32_t nr_zones;
	uint32_t max_open;
	uint32_t max_active;
	uint32_t max_append_sectors;
	struct qublk_queue *queues;
	volatile sig_atomic_t stop;

	uint8_t cuse;                   ///< Set while /dev/ublkb<dev_id>-nvme is served
	struct xnvme_cuse cuse_session; ///< The CUSE session serving it
};

#endif
