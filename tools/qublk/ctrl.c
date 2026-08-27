// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "ctrl.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <liburing.h>

#define CTRL_DEV "/dev/ublk-control"

static int
ctrl_uring_cmd(int ctrl_fd, uint32_t cmd_op, const struct ublksrv_ctrl_cmd *cmd)
{
	struct io_uring ring;
	struct io_uring_params p = {
		.flags = IORING_SETUP_SQE128 | IORING_SETUP_CQE32,
	};
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;
	int rc, res;

	rc = io_uring_queue_init_params(2, &ring, &p);
	if (rc < 0) {
		fprintf(stderr, "io_uring_queue_init_params(ctrl): %s\n", strerror(-rc));
		return rc;
	}

	sqe = io_uring_get_sqe(&ring);
	if (!sqe) {
		io_uring_queue_exit(&ring);
		return -ENOMEM;
	}

	io_uring_prep_rw(IORING_OP_URING_CMD, sqe, ctrl_fd, NULL, 0, 0);
	sqe->cmd_op = cmd_op;
	memset(sqe->cmd, 0, 80);
	memcpy(sqe->cmd, cmd, sizeof(*cmd));

	rc = io_uring_submit_and_wait(&ring, 1);
	if (rc < 0) {
		fprintf(stderr, "io_uring_submit_and_wait(ctrl 0x%x): %s\n", cmd_op,
			strerror(-rc));
		io_uring_queue_exit(&ring);
		return rc;
	}

	rc = io_uring_peek_cqe(&ring, &cqe);
	if (rc < 0) {
		fprintf(stderr, "io_uring_peek_cqe(ctrl 0x%x): %s\n", cmd_op, strerror(-rc));
		io_uring_queue_exit(&ring);
		return rc;
	}

	res = cqe->res;
	io_uring_cqe_seen(&ring, cqe);
	io_uring_queue_exit(&ring);

	if (res < 0) {
		fprintf(stderr, "ublk ctrl cmd 0x%x failed: %s\n", cmd_op, strerror(-res));
		return res;
	}
	return 0;
}

int
qublk_ctrl_open(struct qublk_dev *dev)
{
	dev->ctrl_fd = open(CTRL_DEV, O_RDWR | O_CLOEXEC);
	if (dev->ctrl_fd < 0) {
		fprintf(stderr, "open(%s): %s\n", CTRL_DEV, strerror(errno));
		return -errno;
	}
	return 0;
}

void
qublk_ctrl_close(struct qublk_dev *dev)
{
	if (dev->ctrl_fd >= 0) {
		close(dev->ctrl_fd);
		dev->ctrl_fd = -1;
	}
}

int
qublk_ctrl_get_features(struct qublk_dev *dev, uint64_t *features)
{
	uint64_t buf[UBLK_FEATURES_LEN / sizeof(uint64_t)] = {0};
	struct ublksrv_ctrl_cmd cmd = {
		.dev_id = (uint32_t)-1,
		.queue_id = (uint16_t)-1,
		.addr = (uint64_t)(uintptr_t)buf,
		.len = sizeof(buf),
	};
	int rc;

	rc = ctrl_uring_cmd(dev->ctrl_fd, UBLK_U_CMD_GET_FEATURES, &cmd);
	if (rc < 0) {
		return rc;
	}
	*features = buf[0];
	return 0;
}

int
qublk_ctrl_add_dev(struct qublk_dev *dev)
{
	struct ublksrv_ctrl_dev_info info = {
		.nr_hw_queues = dev->nqueues,
		.queue_depth = dev->qdepth,
		.max_io_buf_bytes = dev->max_io_buf,
		.dev_id = (uint32_t)dev->dev_id,
		.flags = dev->flags,
	};
	struct ublksrv_ctrl_cmd cmd = {
		.dev_id = (uint32_t)dev->dev_id,
		.queue_id = (uint16_t)-1,
		.addr = (uint64_t)(uintptr_t)&info,
		.len = sizeof(info),
	};
	int rc;

	rc = ctrl_uring_cmd(dev->ctrl_fd, UBLK_U_CMD_ADD_DEV, &cmd);
	if (rc < 0) {
		return rc;
	}
	dev->dev_id = (int)info.dev_id;
	return 0;
}

int
qublk_ctrl_set_params(struct qublk_dev *dev)
{
	const struct xnvme_geo *geo = dev->geo;
	uint8_t lba_shift = dev->lba_shift;
	uint64_t dev_sectors_512 = geo->tbytes >> 9;
	struct ublk_params params = {
		.len = sizeof(params),
		.types = UBLK_PARAM_TYPE_BASIC,
		.basic =
			{
				.attrs = dev->has_vwc ? (UBLK_ATTR_VOLATILE_CACHE | UBLK_ATTR_FUA)
						      : 0,
				.logical_bs_shift = lba_shift,
				.physical_bs_shift = lba_shift,
				.io_opt_shift = lba_shift,
				.io_min_shift = lba_shift,
				.max_sectors = dev->max_io_buf >> 9,
				.dev_sectors = dev_sectors_512,
			},
	};
	struct ublksrv_ctrl_cmd cmd = {
		.dev_id = (uint32_t)dev->dev_id,
		.queue_id = (uint16_t)-1,
		.addr = (uint64_t)(uintptr_t)&params,
		.len = sizeof(params),
	};

	return ctrl_uring_cmd(dev->ctrl_fd, UBLK_U_CMD_SET_PARAMS, &cmd);
}

int
qublk_ctrl_start_dev(struct qublk_dev *dev)
{
	struct ublksrv_ctrl_cmd cmd = {
		.dev_id = (uint32_t)dev->dev_id,
		.queue_id = (uint16_t)-1,
		.data[0] = (uint64_t)getpid(),
	};
	return ctrl_uring_cmd(dev->ctrl_fd, UBLK_U_CMD_START_DEV, &cmd);
}

int
qublk_ctrl_stop_dev(struct qublk_dev *dev)
{
	struct ublksrv_ctrl_cmd cmd = {
		.dev_id = (uint32_t)dev->dev_id,
		.queue_id = (uint16_t)-1,
	};
	return ctrl_uring_cmd(dev->ctrl_fd, UBLK_U_CMD_STOP_DEV, &cmd);
}

int
qublk_ctrl_del_dev(struct qublk_dev *dev)
{
	struct ublksrv_ctrl_cmd cmd = {
		.dev_id = (uint32_t)dev->dev_id,
		.queue_id = (uint16_t)-1,
	};
	return ctrl_uring_cmd(dev->ctrl_fd, UBLK_U_CMD_DEL_DEV, &cmd);
}
