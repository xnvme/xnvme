// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>

#include <xnvme_cuse.h>

#ifdef XNVME_CUSE_ENABLED

#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <unistd.h>

#include <linux/fuse.h>
#include <linux/nvme_ioctl.h>

#include <xnvme_dev.h>

// CUSE never raises max_pages above FUSE_DEFAULT_MAX_PAGES_PER_REQ, so the kernel rejects an
// ioctl above 128KiB before this is consulted; it only bounds our own buffer
#define XNVME_CUSE_MAX_WRITE (1u << 20)
#define XNVME_CUSE_BUFSIZE (XNVME_CUSE_MAX_WRITE + 4096) // + the fuse/ioctl headers

// A FUSE_IOCTL_RETRY carries at most FUSE_IOCTL_MAX_IOV (256) iovecs, in and out together;
// the vectored retry lists the cmd, the metadata and each segment as both: (1 + 1 + 125) * 2
#define XNVME_CUSE_VEC_MAX_SEGS 125

// The 32-bit command is widened into the 64-bit one; they share their layout up to timeout_ms
XNVME_STATIC_ASSERT(offsetof(struct nvme_passthru_cmd, timeout_ms) ==
			    offsetof(struct nvme_passthru_cmd64, timeout_ms),
		    "nvme_passthru_cmd is not a prefix of nvme_passthru_cmd64")

// Some backends (e.g. uPCIe) keep per-process state their sync command path does not lock
static pthread_mutex_t g_cuse_dispatch_mutex = PTHREAD_MUTEX_INITIALIZER;

static uint32_t
cuse_iov_push(struct iovec *iov, uint32_t n, const void *base, size_t nbytes)
{
	if (nbytes) {
		iov[n].iov_base = (void *)base;
		iov[n++].iov_len = nbytes;
	}

	return n;
}

static int
cuse_reply(int fd, uint64_t unique, int32_t error, const void *meta, size_t meta_nbytes,
	   const void *payload, size_t payload_nbytes)
{
	struct fuse_out_header out = {
		.len = sizeof(out) + meta_nbytes + payload_nbytes,
		.error = error,
		.unique = unique,
	};
	struct iovec iov[3];
	uint32_t n = 0;

	n = cuse_iov_push(iov, n, &out, sizeof(out));
	n = cuse_iov_push(iov, n, meta, meta_nbytes);
	n = cuse_iov_push(iov, n, payload, payload_nbytes);

	return writev(fd, iov, (int)n) < 0 ? -errno : 0;
}

static int
cuse_reply_err(int fd, uint64_t unique, int err)
{
	return cuse_reply(fd, unique, -err, NULL, 0, NULL, 0);
}

static int
cuse_reply_ioctl(int fd, uint64_t unique, int32_t result, const void *buf, size_t buf_nbytes)
{
	struct fuse_ioctl_out out = {.result = result};

	return cuse_reply(fd, unique, 0, &out, sizeof(out), buf, buf_nbytes);
}

// The kernel re-sends the FUSE_IOCTL with 'in_iov' fetched, back to back, at the front of its
// buffer; the final reply's payload is scattered back over 'out_iov'
static int
cuse_reply_ioctl_retry(int fd, uint64_t unique, const struct iovec *in_iov, uint32_t in_count,
		       const struct iovec *out_iov, uint32_t out_count)
{
	struct fuse_ioctl_out out = {
		.flags = FUSE_IOCTL_RETRY,
		.in_iovs = in_count,
		.out_iovs = out_count,
	};
	struct fuse_ioctl_iovec fiov[in_count + out_count];

	for (uint32_t i = 0; i < in_count; i++) {
		fiov[i].base = (uint64_t)(uintptr_t)in_iov[i].iov_base;
		fiov[i].len = in_iov[i].iov_len;
	}
	for (uint32_t i = 0; i < out_count; i++) {
		fiov[in_count + i].base = (uint64_t)(uintptr_t)out_iov[i].iov_base;
		fiov[in_count + i].len = out_iov[i].iov_len;
	}

	return cuse_reply(fd, unique, 0, &out, sizeof(out), fiov,
			  sizeof(fiov[0]) * (in_count + out_count));
}

static struct xnvme_cmd_ctx
cuse_ctx_from_cmd(struct xnvme_dev *dev, const struct nvme_passthru_cmd64 *cmd)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);

	ctx.cmd.common.opcode = cmd->opcode;
	ctx.cmd.common.nsid = cmd->nsid;
	ctx.cmd.common.cdw02 = cmd->cdw2;
	ctx.cmd.common.cdw03 = cmd->cdw3;
	ctx.cmd.common.ndt =
		cmd->cdw10; // cdw10 and cdw11 are 'ndt' and 'ndm' in the common layout
	ctx.cmd.common.ndm = cmd->cdw11;
	ctx.cmd.common.cdw12 = cmd->cdw12;
	ctx.cmd.common.cdw13 = cmd->cdw13;
	ctx.cmd.common.cdw14 = cmd->cdw14;
	ctx.cmd.common.cdw15 = cmd->cdw15;

	return ctx;
}

// As nvme_submit_user_cmd(): the status field without its phase tag
static int
cuse_nvme_status(const struct xnvme_cmd_ctx *ctx)
{
	return ctx->cpl.status.val >> 1;
}

static int
cuse_passthru(int fd, uint64_t unique, struct xnvme_dev *dev, const struct fuse_ioctl_in *ioc,
	      const void *buf, size_t buf_nbytes)
{
	void *arg = (void *)(uintptr_t)ioc->arg;
	int admin = ioc->cmd == NVME_IOCTL_ADMIN_CMD || ioc->cmd == NVME_IOCTL_ADMIN64_CMD;
	size_t cmd_nbytes = (ioc->cmd == NVME_IOCTL_ADMIN_CMD || ioc->cmd == NVME_IOCTL_IO_CMD)
				    ? sizeof(struct nvme_passthru_cmd)
				    : sizeof(struct nvme_passthru_cmd64);
	struct nvme_passthru_cmd64 cmd = {0};
	struct xnvme_cmd_ctx ctx;
	void *dbuf = NULL, *mbuf = NULL;
	char *reply = NULL;
	size_t reply_nbytes;
	int err, rc;

	// As the kernel driver: a controller has no namespace of its own to send I/O to
	if (!admin && xnvme_dev_get_ident(dev)->dtype == XNVME_DEV_TYPE_NVME_CONTROLLER) {
		return cuse_reply_err(fd, unique, ENOTTY);
	}

	if (buf_nbytes < cmd_nbytes) {
		struct iovec iov = {arg, cmd_nbytes};

		return cuse_reply_ioctl_retry(fd, unique, &iov, 1, &iov, 1);
	}
	memcpy(&cmd, buf, cmd_nbytes);

	if (buf_nbytes < cmd_nbytes + cmd.data_len + cmd.metadata_len) {
		struct iovec iov[3];
		uint32_t n = 0;

		n = cuse_iov_push(iov, n, arg, cmd_nbytes);
		n = cuse_iov_push(iov, n, (void *)(uintptr_t)cmd.addr, cmd.data_len);
		n = cuse_iov_push(iov, n, (void *)(uintptr_t)cmd.metadata, cmd.metadata_len);

		return cuse_reply_ioctl_retry(fd, unique, iov, n, iov, n);
	}

	// Bounced: some backends submit only from their own DMA-registered heap
	if (cmd.data_len) {
		dbuf = xnvme_buf_alloc(dev, cmd.data_len);
		if (!dbuf) {
			XNVME_DEBUG("FAILED: xnvme_buf_alloc(data_len: %u)", cmd.data_len);
			rc = cuse_reply_err(fd, unique, ENOMEM);
			goto exit;
		}
		memcpy(dbuf, (const char *)buf + cmd_nbytes, cmd.data_len);
	}
	if (cmd.metadata_len) {
		mbuf = xnvme_buf_alloc(dev, cmd.metadata_len);
		if (!mbuf) {
			XNVME_DEBUG("FAILED: xnvme_buf_alloc(metadata_len: %u)", cmd.metadata_len);
			rc = cuse_reply_err(fd, unique, ENOMEM);
			goto exit;
		}
		memcpy(mbuf, (const char *)buf + cmd_nbytes + cmd.data_len, cmd.metadata_len);
	}

	ctx = cuse_ctx_from_cmd(dev, &cmd);
	err = admin ? xnvme_cmd_pass_admin(&ctx, dbuf, cmd.data_len, mbuf, cmd.metadata_len)
		    : xnvme_cmd_pass(&ctx, dbuf, cmd.data_len, mbuf, cmd.metadata_len);
	// A completion carries its status even when 'err' is set; only without one is the errno
	// all there is to relay
	if (err && !xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("FAILED: xnvme_cmd_pass%s(); err(%d)", admin ? "_admin" : "", err);
		rc = cuse_reply_err(fd, unique, -err);
		goto exit;
	}

	reply_nbytes = cmd_nbytes + cmd.data_len + cmd.metadata_len;
	reply = malloc(reply_nbytes);
	if (!reply) {
		XNVME_DEBUG("FAILED: malloc(reply_nbytes: %zu)", reply_nbytes);
		rc = cuse_reply_err(fd, unique, ENOMEM);
		goto exit;
	}
	memcpy(reply, &cmd, cmd_nbytes);
	// The 32-bit 'result' truncates to cdw0, as the kernel driver's does
	if (cmd_nbytes == sizeof(struct nvme_passthru_cmd)) {
		((struct nvme_passthru_cmd *)reply)->result = ctx.cpl.result;
	} else {
		((struct nvme_passthru_cmd64 *)reply)->result = ctx.cpl.result;
	}
	if (cmd.data_len) {
		memcpy(reply + cmd_nbytes, dbuf, cmd.data_len);
	}
	if (cmd.metadata_len) {
		memcpy(reply + cmd_nbytes + cmd.data_len, mbuf, cmd.metadata_len);
	}

	rc = cuse_reply_ioctl(fd, unique, cuse_nvme_status(&ctx), reply, reply_nbytes);

exit:
	free(reply);
	xnvme_buf_free(dev, mbuf);
	xnvme_buf_free(dev, dbuf);

	return rc;
}

// The request is laid out [cmd][iovec array][metadata][data segments, back to back]
static int
cuse_passthru_vec(int fd, uint64_t unique, struct xnvme_dev *dev, const struct fuse_ioctl_in *ioc,
		  const void *buf, size_t buf_nbytes)
{
	void *arg = (void *)(uintptr_t)ioc->arg;
	const struct nvme_passthru_cmd64 *cmd = buf;
	const struct iovec *uiov;
	size_t iovec_nbytes, meta_off, data_off, data_nbytes = 0, off;
	struct iovec dvec[XNVME_CUSE_VEC_MAX_SEGS];
	uint32_t dvec_cnt = 0;
	struct xnvme_cmd_ctx ctx;
	void *mbuf = NULL;
	char *reply = NULL;
	int err, rc;

	if (xnvme_dev_get_ident(dev)->dtype == XNVME_DEV_TYPE_NVME_CONTROLLER) {
		return cuse_reply_err(fd, unique, ENOTTY);
	}

	if (buf_nbytes < sizeof(*cmd)) {
		struct iovec iov = {arg, sizeof(*cmd)};

		return cuse_reply_ioctl_retry(fd, unique, &iov, 1, &iov, 1);
	}
	if (cmd->vec_cnt > XNVME_CUSE_VEC_MAX_SEGS) {
		XNVME_DEBUG("FAILED: vec_cnt(%u) exceeds the retry-list ceiling", cmd->vec_cnt);
		return cuse_reply_err(fd, unique, EINVAL);
	}

	iovec_nbytes = (size_t)cmd->vec_cnt * sizeof(*uiov);
	meta_off = sizeof(*cmd) + iovec_nbytes;
	data_off = meta_off + cmd->metadata_len;

	if (buf_nbytes < data_off) {
		struct iovec iov[3];
		uint32_t n = 0;

		n = cuse_iov_push(iov, n, arg, sizeof(*cmd));
		n = cuse_iov_push(iov, n, (void *)(uintptr_t)cmd->addr, iovec_nbytes);
		n = cuse_iov_push(iov, n, (void *)(uintptr_t)cmd->metadata, cmd->metadata_len);

		return cuse_reply_ioctl_retry(fd, unique, iov, n, iov, n);
	}

	uiov = (const void *)((const char *)buf + sizeof(*cmd));
	// Bounded before summing, so the sum cannot wrap and understate the data coming
	for (uint32_t i = 0; i < cmd->vec_cnt; i++) {
		if (uiov[i].iov_len > XNVME_CUSE_MAX_WRITE) {
			XNVME_DEBUG("FAILED: iov_len(%zu) exceeds XNVME_CUSE_MAX_WRITE",
				    uiov[i].iov_len);
			return cuse_reply_err(fd, unique, EINVAL);
		}
		data_nbytes += uiov[i].iov_len;
	}

	if (buf_nbytes < data_off + data_nbytes) {
		struct iovec iov[3 + XNVME_CUSE_VEC_MAX_SEGS];
		uint32_t n = 0;

		n = cuse_iov_push(iov, n, arg, sizeof(*cmd));
		n = cuse_iov_push(iov, n, (void *)(uintptr_t)cmd->addr, iovec_nbytes);
		n = cuse_iov_push(iov, n, (void *)(uintptr_t)cmd->metadata, cmd->metadata_len);
		for (uint32_t i = 0; i < cmd->vec_cnt; i++) {
			n = cuse_iov_push(iov, n, uiov[i].iov_base, uiov[i].iov_len);
		}

		return cuse_reply_ioctl_retry(fd, unique, iov, n, iov, n);
	}

	// Bounced segment by segment: some backends submit only from their own DMA-registered heap
	off = data_off;
	for (uint32_t i = 0; i < cmd->vec_cnt; i++) {
		void *seg;

		if (!uiov[i].iov_len) {
			continue;
		}
		seg = xnvme_buf_alloc(dev, uiov[i].iov_len);
		if (!seg) {
			XNVME_DEBUG("FAILED: xnvme_buf_alloc(iov_len: %zu)", uiov[i].iov_len);
			rc = cuse_reply_err(fd, unique, ENOMEM);
			goto exit;
		}
		memcpy(seg, (const char *)buf + off, uiov[i].iov_len);
		dvec_cnt = cuse_iov_push(dvec, dvec_cnt, seg, uiov[i].iov_len);
		off += uiov[i].iov_len;
	}
	if (cmd->metadata_len) {
		mbuf = xnvme_buf_alloc(dev, cmd->metadata_len);
		if (!mbuf) {
			XNVME_DEBUG("FAILED: xnvme_buf_alloc(metadata_len: %u)",
				    cmd->metadata_len);
			rc = cuse_reply_err(fd, unique, ENOMEM);
			goto exit;
		}
		memcpy(mbuf, (const char *)buf + meta_off, cmd->metadata_len);
	}

	ctx = cuse_ctx_from_cmd(dev, cmd);
	err = dvec_cnt ? xnvme_cmd_pass_iov(&ctx, dvec, dvec_cnt, data_nbytes, mbuf,
					    cmd->metadata_len)
		       : xnvme_cmd_pass(&ctx, NULL, 0, mbuf, cmd->metadata_len);
	if (err && !xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("FAILED: xnvme_cmd_pass%s(); err(%d)", dvec_cnt ? "_iov" : "", err);
		rc = cuse_reply_err(fd, unique, -err);
		goto exit;
	}

	reply = malloc(data_off + data_nbytes);
	if (!reply) {
		XNVME_DEBUG("FAILED: malloc(reply_nbytes: %zu)", data_off + data_nbytes);
		rc = cuse_reply_err(fd, unique, ENOMEM);
		goto exit;
	}
	memcpy(reply, buf, meta_off);
	((struct nvme_passthru_cmd64 *)reply)->result = ctx.cpl.result;
	if (cmd->metadata_len) {
		memcpy(reply + meta_off, mbuf, cmd->metadata_len);
	}
	off = data_off;
	for (uint32_t i = 0; i < dvec_cnt; i++) {
		memcpy(reply + off, dvec[i].iov_base, dvec[i].iov_len);
		off += dvec[i].iov_len;
	}

	rc = cuse_reply_ioctl(fd, unique, cuse_nvme_status(&ctx), reply, data_off + data_nbytes);

exit:
	free(reply);
	xnvme_buf_free(dev, mbuf);
	for (uint32_t i = 0; i < dvec_cnt; i++) {
		xnvme_buf_free(dev, dvec[i].iov_base);
	}

	return rc;
}

static int
cuse_dispatch_ioctl(int fd, uint64_t unique, struct xnvme_dev *dev,
		    const struct fuse_ioctl_in *ioc, const void *buf, size_t buf_nbytes)
{
	if (ioc->flags & FUSE_IOCTL_COMPAT) {
		XNVME_DEBUG("FAILED: FUSE_IOCTL_COMPAT is not supported");
		return cuse_reply_err(fd, unique, ENOSYS);
	}

	switch (ioc->cmd) {
	case NVME_IOCTL_ID:
		// As the kernel driver: a controller's char device has no nsid of its own
		if (xnvme_dev_get_ident(dev)->dtype == XNVME_DEV_TYPE_NVME_CONTROLLER) {
			return cuse_reply_err(fd, unique, ENOTTY);
		}
		return cuse_reply_ioctl(fd, unique, (int)xnvme_dev_get_nsid(dev), NULL, 0);

	case NVME_IOCTL_ADMIN_CMD:
	case NVME_IOCTL_IO_CMD:
	case NVME_IOCTL_ADMIN64_CMD:
	case NVME_IOCTL_IO64_CMD:
		return cuse_passthru(fd, unique, dev, ioc, buf, buf_nbytes);

	case NVME_IOCTL_IO64_CMD_VEC:
		return cuse_passthru_vec(fd, unique, dev, ioc, buf, buf_nbytes);

	default:
		XNVME_DEBUG("FAILED: unsupported ioctl cmd(0x%x)", ioc->cmd);
		return cuse_reply_err(fd, unique, ENOTTY);
	}
}

static int
cuse_init(int fd, const char *name, void *buf, size_t buf_nbytes)
{
	const struct fuse_in_header *in;
	const struct cuse_init_in *init_in;
	struct cuse_init_out init_out = {0};
	char dev_info[64];
	int dev_info_ret;
	size_t dev_info_len;
	ssize_t n;

	n = read(fd, buf, buf_nbytes);
	if (n < 0) {
		XNVME_DEBUG("FAILED: read(CUSE_INIT); errno(%d)", errno);
		return -errno;
	}
	if ((size_t)n < sizeof(*in) + sizeof(*init_in)) {
		XNVME_DEBUG("FAILED: short CUSE_INIT read; n(%zd)", n);
		return -EIO;
	}

	in = buf;
	if (in->opcode != CUSE_INIT) {
		XNVME_DEBUG("FAILED: first request opcode(%u) is not CUSE_INIT", in->opcode);
		return -EPROTO;
	}
	init_in = (const void *)((const char *)buf + sizeof(*in));

	dev_info_ret = snprintf(dev_info, sizeof(dev_info), "DEVNAME=%s", name);
	if (dev_info_ret < 0 || (size_t)dev_info_ret >= sizeof(dev_info)) {
		XNVME_DEBUG("FAILED: device name '%s' too long for CUSE_INIT reply", name);
		return -ENAMETOOLONG;
	}
	dev_info_len = (size_t)dev_info_ret + 1; // cuse_pack_info() includes each string's NUL

	init_out.major = FUSE_KERNEL_VERSION;
	init_out.minor = FUSE_KERNEL_MINOR_VERSION;
	init_out.flags = CUSE_UNRESTRICTED_IOCTL;
	init_out.max_read = XNVME_CUSE_MAX_WRITE;
	init_out.max_write = XNVME_CUSE_MAX_WRITE;

	if (init_in->major < 7) {
		XNVME_DEBUG("FAILED: kernel CUSE_INIT major(%u) < 7", init_in->major);
		cuse_reply_err(fd, in->unique, EPROTO);
		return -EPROTO;
	}

	return cuse_reply(fd, in->unique, 0, &init_out, sizeof(init_out), dev_info, dev_info_len);
}

struct cuse_session_ctx {
	struct xnvme_cuse *cuse;
	struct xnvme_dev *dev;
	const char *name;
	sem_t *ready;
};

static void *
cuse_session_main(void *arg)
{
	struct cuse_session_ctx *ctx = arg;
	struct xnvme_cuse *cuse = ctx->cuse;
	// 'ctx' is on xnvme_cuse_start()'s stack, invalid once sem_post() below runs
	struct xnvme_dev *dev = ctx->dev;
	void *buf;
	int fd, err;

	fd = open("/dev/cuse", O_RDWR);
	if (fd < 0) {
		err = -errno;
		XNVME_DEBUG("FAILED: open(/dev/cuse); err(%d); try 'modprobe cuse'", err);
		cuse->init_rc = err;
		sem_post(ctx->ready);
		return NULL;
	}

	buf = malloc(XNVME_CUSE_BUFSIZE);
	if (!buf) {
		XNVME_DEBUG("FAILED: malloc(XNVME_CUSE_BUFSIZE: %u)", XNVME_CUSE_BUFSIZE);
		close(fd);
		cuse->init_rc = -ENOMEM;
		sem_post(ctx->ready);
		return NULL;
	}

	err = cuse_init(fd, ctx->name, buf, XNVME_CUSE_BUFSIZE);
	if (err) {
		XNVME_DEBUG("FAILED: cuse_init(%s); err(%d)", ctx->name, err);
		free(buf);
		close(fd);
		cuse->init_rc = err;
		sem_post(ctx->ready);
		return NULL;
	}

	// ppoll() readiness can go stale before read() runs; O_NONBLOCK turns
	// that into EAGAIN instead of a blocking read() on a request that never comes
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
		XNVME_DEBUG("FAILED: fcntl(O_NONBLOCK); errno(%d)", errno);
	}

	cuse->init_rc = 0;
	sem_post(ctx->ready);

	// SIGUSR1 (xnvme_cuse_stop()'s wakeup) is unblocked only around ppoll(),
	// atomically with the wait, so a stop signal can never be missed
	sigset_t wait_mask;
	{
		sigset_t usr1_only;

		sigemptyset(&usr1_only);
		sigaddset(&usr1_only, SIGUSR1);
		pthread_sigmask(SIG_BLOCK, &usr1_only, &wait_mask);
		sigdelset(&wait_mask, SIGUSR1);
	}

	while (!cuse->stop) {
		const struct fuse_in_header *in;
		struct pollfd pfd = {.fd = fd, .events = POLLIN};
		ssize_t n;

		if (ppoll(&pfd, 1, NULL, &wait_mask) < 0) {
			continue; // EINTR: re-check cuse->stop above
		}

		n = read(fd, buf, XNVME_CUSE_BUFSIZE);
		if (n < 0) {
			// EINTR: stop signal; EAGAIN: stale ppoll() readiness; ENOENT:
			// the kernel aborted an interrupted request
			if (errno == EINTR || errno == EAGAIN || errno == ENOENT) {
				continue;
			}
			if (errno == ENODEV) {
				// The char-device is gone, possibly rejected by the kernel
				// after our CUSE_INIT reply (e.g. a duplicate name)
				XNVME_DEBUG("FAILED: CUSE char-device is gone (errno(ENODEV))");
				break;
			}
			continue;
		}
		if ((size_t)n < sizeof(*in)) {
			continue;
		}

		in = buf;
		switch (in->opcode) {
		case FUSE_OPEN: {
			struct fuse_open_out out = {0};

			cuse_reply(fd, in->unique, 0, &out, sizeof(out), NULL, 0);
			break;
		}
		case FUSE_RELEASE:
			cuse_reply_err(fd, in->unique, 0);
			break;
		case FUSE_IOCTL: {
			const struct fuse_ioctl_in *ioc =
				(const void *)((const char *)buf + sizeof(*in));
			const void *ioc_buf =
				ioc->in_size ? (const char *)ioc + sizeof(*ioc) : NULL;

			if ((size_t)n < sizeof(*in) + sizeof(*ioc) + ioc->in_size) {
				// The kernel never actually sends a short request; answer
				// rather than leave the caller's ioctl() blocked forever
				XNVME_DEBUG("FAILED: short FUSE_IOCTL; n(%zd) in_size(%u)", n,
					    ioc->in_size);
				cuse_reply_err(fd, in->unique, EIO);
				break;
			}
			pthread_mutex_lock(&g_cuse_dispatch_mutex);
			cuse_dispatch_ioctl(fd, in->unique, dev, ioc, ioc_buf, ioc->in_size);
			pthread_mutex_unlock(&g_cuse_dispatch_mutex);
			break;
		}
		case FUSE_DESTROY:
			cuse_reply_err(fd, in->unique, 0);
			goto done;
		default:
			XNVME_DEBUG("FAILED: unsupported FUSE opcode(%u)", in->opcode);
			cuse_reply_err(fd, in->unique, ENOSYS);
		}
	}

done:
	free(buf);
	close(fd);
	return NULL;
}

int
xnvme_cuse_start(struct xnvme_cuse *cuse, struct xnvme_dev *dev, const char *name)
{
	struct cuse_session_ctx ctx;
	sem_t ready;
	int err;

	// A kernel-managed NVMe device already has its own ioctl() interface,
	// and a device that is not NVMe has none to mimic
	if (!(dev->be.attr.caps &
	      (XNVME_BE_CAP_NVME_PCIE | XNVME_BE_CAP_NVME_TCP | XNVME_BE_CAP_NVME_RDMA))) {
		return -ENOTSUP;
	}

	if (sem_init(&ready, 0, 0) < 0) {
		XNVME_DEBUG("FAILED: sem_init(); errno(%d)", errno);
		return -errno;
	}

	ctx.cuse = cuse;
	ctx.dev = dev;
	ctx.name = name;
	ctx.ready = &ready;

	err = pthread_create(&cuse->tid, NULL, cuse_session_main, &ctx);
	if (err) {
		XNVME_DEBUG("FAILED: pthread_create(%s); err(%d)", name, err);
		cuse->tid = 0;
		sem_destroy(&ready);
		return -err;
	}

	// A stray signal here must not be mistaken for the session's own post
	while (sem_wait(&ready) < 0 && errno == EINTR) {
	}
	sem_destroy(&ready);

	return cuse->init_rc;
}

void
xnvme_cuse_stop(struct xnvme_cuse *cuse)
{
	// The process needs a no-op SIGUSR1 handler installed, or its default
	// action (terminate) fires instead of waking the session thread's ppoll()
	if (cuse->tid) {
		cuse->stop = 1;
		pthread_kill(cuse->tid, SIGUSR1);
		pthread_join(cuse->tid, NULL);
		cuse->tid = 0;
	}
}

#else

int
xnvme_cuse_start(struct xnvme_cuse *XNVME_UNUSED(cuse), struct xnvme_dev *XNVME_UNUSED(dev),
		 const char *XNVME_UNUSED(name))
{
	return -ENOSYS;
}

void
xnvme_cuse_stop(struct xnvme_cuse *XNVME_UNUSED(cuse))
{
}

#endif
