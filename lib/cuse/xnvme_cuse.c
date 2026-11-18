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

#include <linux/nvme_ioctl.h>

#include <ufuse.h>

#include <xnvme_cmd.h>
#include <xnvme_dev.h>

// CUSE never raises max_pages above FUSE_DEFAULT_MAX_PAGES_PER_REQ (32), so the kernel limits each
// direction of an ioctl to 32 pages: 128 KiB with 4 KiB pages, 2 MiB with 64 KiB pages
#define XNVME_CUSE_MAX_WRITE (1u << 20)
#define XNVME_CUSE_HDR_NBYTES 4096 // the fuse/ioctl headers ahead of a request's data

// A FUSE_IOCTL_RETRY carries at most FUSE_IOCTL_MAX_IOV (256) iovecs, in and out together;
// the vectored retry lists the cmd, the metadata and each segment as both: (1 + 1 + 125) * 2
#define XNVME_CUSE_VEC_MAX_SEGS 125

// The 32-bit command is widened into the 64-bit one; they share their layout up to timeout_ms
XNVME_STATIC_ASSERT(offsetof(struct nvme_passthru_cmd, timeout_ms) ==
			    offsetof(struct nvme_passthru_cmd64, timeout_ms),
		    "nvme_passthru_cmd is not a prefix of nvme_passthru_cmd64")

// Some backends (e.g. uPCIe) keep per-process state their sync command path does not lock
static pthread_mutex_t g_cuse_dispatch_mutex = PTHREAD_MUTEX_INITIALIZER;

// Appends a range of the caller's address space to a retry list; an empty range is left out
static uint32_t
cuse_fiov_push(struct fuse_ioctl_iovec *fiov, uint32_t n, uint64_t base, uint64_t nbytes)
{
	if (nbytes) {
		fiov[n].base = base;
		fiov[n++].len = nbytes;
	}

	return n;
}

static struct xnvme_cmd_ctx
cuse_ctx_from_cmd(struct xnvme_dev *dev, const struct nvme_passthru_cmd64 *cmd)
{
	struct xnvme_cmd_ctx ctx = {0};

	xnvme_dev_get_cmd_ctx(dev, &ctx);

	xnvme_cmd_sqe_set_opc(&ctx.cmd, cmd->opcode);
	ctx.cmd.common.nsid = cmd->nsid;
	ctx.cmd.common.cdw2 = cmd->cdw2;
	ctx.cmd.common.cdw3 = cmd->cdw3;
	ctx.cmd.common.cdw10 = cmd->cdw10;
	ctx.cmd.common.cdw11 = cmd->cdw11;
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
	return ctx->cpl.status >> 1;
}

static int
cuse_passthru(const struct ufuse_req *req, struct xnvme_dev *dev, const struct fuse_ioctl_in *ioc,
	      const void *buf, size_t buf_nbytes)
{
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
		return ufuse_req_reply_err(req, ENOTTY);
	}

	if (buf_nbytes < cmd_nbytes) {
		struct fuse_ioctl_iovec iov = {ioc->arg, cmd_nbytes};

		return ufuse_req_reply_ioctl_retry(req, &iov, 1, &iov, 1);
	}
	memcpy(&cmd, buf, cmd_nbytes);

	if (buf_nbytes < cmd_nbytes + cmd.data_len + cmd.metadata_len) {
		struct fuse_ioctl_iovec iov[3];
		uint32_t n = 0;

		n = cuse_fiov_push(iov, n, ioc->arg, cmd_nbytes);
		n = cuse_fiov_push(iov, n, cmd.addr, cmd.data_len);
		n = cuse_fiov_push(iov, n, cmd.metadata, cmd.metadata_len);

		return ufuse_req_reply_ioctl_retry(req, iov, n, iov, n);
	}

	// Bounced: some backends submit only from their own DMA-registered heap
	if (cmd.data_len) {
		dbuf = xnvme_buf_alloc(dev, cmd.data_len);
		if (!dbuf) {
			XNVME_DEBUG("FAILED: xnvme_buf_alloc(data_len: %u)", cmd.data_len);
			rc = ufuse_req_reply_err(req, ENOMEM);
			goto exit;
		}
		memcpy(dbuf, (const char *)buf + cmd_nbytes, cmd.data_len);
	}
	if (cmd.metadata_len) {
		mbuf = xnvme_buf_alloc(dev, cmd.metadata_len);
		if (!mbuf) {
			XNVME_DEBUG("FAILED: xnvme_buf_alloc(metadata_len: %u)", cmd.metadata_len);
			rc = ufuse_req_reply_err(req, ENOMEM);
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
		rc = ufuse_req_reply_err(req, -err);
		goto exit;
	}

	reply_nbytes = cmd_nbytes + cmd.data_len + cmd.metadata_len;
	reply = malloc(reply_nbytes);
	if (!reply) {
		XNVME_DEBUG("FAILED: malloc(reply_nbytes: %zu)", reply_nbytes);
		rc = ufuse_req_reply_err(req, ENOMEM);
		goto exit;
	}
	memcpy(reply, &cmd, cmd_nbytes);
	// The 32-bit 'result' truncates to cdw0, as the kernel driver's does
	if (cmd_nbytes == sizeof(struct nvme_passthru_cmd)) {
		((struct nvme_passthru_cmd *)reply)->result = xnvme_cmd_cpl_get_dw0_1(&ctx.cpl);
	} else {
		((struct nvme_passthru_cmd64 *)reply)->result = xnvme_cmd_cpl_get_dw0_1(&ctx.cpl);
	}
	if (cmd.data_len) {
		memcpy(reply + cmd_nbytes, dbuf, cmd.data_len);
	}
	if (cmd.metadata_len) {
		memcpy(reply + cmd_nbytes + cmd.data_len, mbuf, cmd.metadata_len);
	}

	rc = ufuse_req_reply_ioctl(req, cuse_nvme_status(&ctx), reply, reply_nbytes);

exit:
	free(reply);
	xnvme_buf_free(dev, mbuf);
	xnvme_buf_free(dev, dbuf);

	return rc;
}

// The request is laid out [cmd][iovec array][metadata][data segments, back to back]
static int
cuse_passthru_vec(const struct ufuse_req *req, struct xnvme_dev *dev,
		  const struct fuse_ioctl_in *ioc, const void *buf, size_t buf_nbytes)
{
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
		return ufuse_req_reply_err(req, ENOTTY);
	}

	if (buf_nbytes < sizeof(*cmd)) {
		struct fuse_ioctl_iovec iov = {ioc->arg, sizeof(*cmd)};

		return ufuse_req_reply_ioctl_retry(req, &iov, 1, &iov, 1);
	}
	if (cmd->vec_cnt > XNVME_CUSE_VEC_MAX_SEGS) {
		XNVME_DEBUG("FAILED: vec_cnt(%u) exceeds the retry-list ceiling", cmd->vec_cnt);
		return ufuse_req_reply_err(req, EINVAL);
	}

	iovec_nbytes = (size_t)cmd->vec_cnt * sizeof(*uiov);
	meta_off = sizeof(*cmd) + iovec_nbytes;
	data_off = meta_off + cmd->metadata_len;

	if (buf_nbytes < data_off) {
		struct fuse_ioctl_iovec iov[3];
		uint32_t n = 0;

		n = cuse_fiov_push(iov, n, ioc->arg, sizeof(*cmd));
		n = cuse_fiov_push(iov, n, cmd->addr, iovec_nbytes);
		n = cuse_fiov_push(iov, n, cmd->metadata, cmd->metadata_len);

		return ufuse_req_reply_ioctl_retry(req, iov, n, iov, n);
	}

	uiov = (const void *)((const char *)buf + sizeof(*cmd));
	// Bounded before summing, so the sum cannot wrap and understate the data coming
	for (uint32_t i = 0; i < cmd->vec_cnt; i++) {
		if (uiov[i].iov_len > XNVME_CUSE_MAX_WRITE) {
			XNVME_DEBUG("FAILED: iov_len(%zu) exceeds XNVME_CUSE_MAX_WRITE",
				    uiov[i].iov_len);
			return ufuse_req_reply_err(req, EINVAL);
		}
		data_nbytes += uiov[i].iov_len;
	}

	if (buf_nbytes < data_off + data_nbytes) {
		struct fuse_ioctl_iovec iov[3 + XNVME_CUSE_VEC_MAX_SEGS];
		uint32_t n = 0;

		n = cuse_fiov_push(iov, n, ioc->arg, sizeof(*cmd));
		n = cuse_fiov_push(iov, n, cmd->addr, iovec_nbytes);
		n = cuse_fiov_push(iov, n, cmd->metadata, cmd->metadata_len);
		for (uint32_t i = 0; i < cmd->vec_cnt; i++) {
			n = cuse_fiov_push(iov, n, (uint64_t)(uintptr_t)uiov[i].iov_base,
					   uiov[i].iov_len);
		}

		return ufuse_req_reply_ioctl_retry(req, iov, n, iov, n);
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
			rc = ufuse_req_reply_err(req, ENOMEM);
			goto exit;
		}
		memcpy(seg, (const char *)buf + off, uiov[i].iov_len);
		dvec[dvec_cnt].iov_base = seg;
		dvec[dvec_cnt++].iov_len = uiov[i].iov_len;
		off += uiov[i].iov_len;
	}
	if (cmd->metadata_len) {
		mbuf = xnvme_buf_alloc(dev, cmd->metadata_len);
		if (!mbuf) {
			XNVME_DEBUG("FAILED: xnvme_buf_alloc(metadata_len: %u)",
				    cmd->metadata_len);
			rc = ufuse_req_reply_err(req, ENOMEM);
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
		rc = ufuse_req_reply_err(req, -err);
		goto exit;
	}

	reply = malloc(data_off + data_nbytes);
	if (!reply) {
		XNVME_DEBUG("FAILED: malloc(reply_nbytes: %zu)", data_off + data_nbytes);
		rc = ufuse_req_reply_err(req, ENOMEM);
		goto exit;
	}
	memcpy(reply, buf, meta_off);
	((struct nvme_passthru_cmd64 *)reply)->result = xnvme_cmd_cpl_get_dw0_1(&ctx.cpl);
	if (cmd->metadata_len) {
		memcpy(reply + meta_off, mbuf, cmd->metadata_len);
	}
	off = data_off;
	for (uint32_t i = 0; i < dvec_cnt; i++) {
		memcpy(reply + off, dvec[i].iov_base, dvec[i].iov_len);
		off += dvec[i].iov_len;
	}

	rc = ufuse_req_reply_ioctl(req, cuse_nvme_status(&ctx), reply, data_off + data_nbytes);

exit:
	free(reply);
	xnvme_buf_free(dev, mbuf);
	for (uint32_t i = 0; i < dvec_cnt; i++) {
		xnvme_buf_free(dev, dvec[i].iov_base);
	}

	return rc;
}

static int
cuse_dispatch_ioctl(const struct ufuse_req *req, struct xnvme_dev *dev,
		    const struct fuse_ioctl_in *ioc, const void *buf, size_t buf_nbytes)
{
	if (ioc->flags & FUSE_IOCTL_COMPAT) {
		XNVME_DEBUG("FAILED: FUSE_IOCTL_COMPAT is not supported");
		return ufuse_req_reply_err(req, ENOSYS);
	}

	switch (ioc->cmd) {
	case NVME_IOCTL_ID:
		// As the kernel driver: a controller's char device has no nsid of its own
		if (xnvme_dev_get_ident(dev)->dtype == XNVME_DEV_TYPE_NVME_CONTROLLER) {
			return ufuse_req_reply_err(req, ENOTTY);
		}
		return ufuse_req_reply_ioctl(req, (int)xnvme_dev_get_nsid(dev), NULL, 0);

	case NVME_IOCTL_ADMIN_CMD:
	case NVME_IOCTL_IO_CMD:
	case NVME_IOCTL_ADMIN64_CMD:
	case NVME_IOCTL_IO64_CMD:
		return cuse_passthru(req, dev, ioc, buf, buf_nbytes);

	case NVME_IOCTL_IO64_CMD_VEC:
		return cuse_passthru_vec(req, dev, ioc, buf, buf_nbytes);

	default:
		XNVME_DEBUG("FAILED: unsupported ioctl cmd(0x%x)", ioc->cmd);
		return ufuse_req_reply_err(req, ENOTTY);
	}
}

// What the callbacks of a session need, as the 'ctx' of ufuse_dispatch()
struct cuse_session {
	struct xnvme_dev *dev;
	int destroyed;
};

static void
cuse_op_open(const struct ufuse_req *req, uint64_t XNVME_UNUSED(ino),
	     const struct fuse_open_in *XNVME_UNUSED(in))
{
	struct fuse_open_out out = {0};

	ufuse_req_reply(req, &out, sizeof(out));
}

static void
cuse_op_release(const struct ufuse_req *req, uint64_t XNVME_UNUSED(ino),
		const struct fuse_release_in *XNVME_UNUSED(in))
{
	ufuse_req_reply(req, NULL, 0);
}

static void
cuse_op_ioctl(const struct ufuse_req *req, uint64_t XNVME_UNUSED(ino),
	      const struct fuse_ioctl_in *ioc, const void *buf, size_t buf_nbytes)
{
	struct cuse_session *session = req->ctx;

	pthread_mutex_lock(&g_cuse_dispatch_mutex);
	cuse_dispatch_ioctl(req, session->dev, ioc, buf, buf_nbytes);
	pthread_mutex_unlock(&g_cuse_dispatch_mutex);
}

static void
cuse_op_destroy(const struct ufuse_req *req, uint64_t XNVME_UNUSED(ino))
{
	struct cuse_session *session = req->ctx;

	session->destroyed = 1;
}

static const struct ufuse_ops g_cuse_ops = {
	.open = cuse_op_open,
	.release = cuse_op_release,
	.ioctl = cuse_op_ioctl,
	.destroy = cuse_op_destroy,
};

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
	struct cuse_session session = {.dev = ctx->dev};
	struct ufuse_session s;
	struct ufuse_req req;
	void *buf = NULL;
	size_t buf_nbytes;
	ssize_t n;
	int err;

	// A retried ioctl brings up to 32 pages of data, which exceeds XNVME_CUSE_MAX_WRITE with
	// pages above 32 KiB; a request larger than the buffer fails the caller's ioctl with EIO
	buf_nbytes = 32 * (size_t)sysconf(_SC_PAGESIZE);
	if (buf_nbytes < XNVME_CUSE_MAX_WRITE) {
		buf_nbytes = XNVME_CUSE_MAX_WRITE;
	}
	buf_nbytes += XNVME_CUSE_HDR_NBYTES;

	ufuse_session_init(&s);
	s.max_read = XNVME_CUSE_MAX_WRITE;
	s.max_write = XNVME_CUSE_MAX_WRITE;

	err = ufuse_cuse_open(&s);
	if (err) {
		XNVME_DEBUG("FAILED: ufuse_cuse_open(); err(%d); try 'modprobe cuse'", err);
		goto failed;
	}

	buf = malloc(buf_nbytes);
	if (!buf) {
		XNVME_DEBUG("FAILED: malloc(%zu)", buf_nbytes);
		err = -ENOMEM;
		goto failed;
	}

	n = ufuse_read(&s, buf, buf_nbytes, &req);
	if (n < 0) {
		err = (int)n;
		XNVME_DEBUG("FAILED: ufuse_read(CUSE_INIT); err(%d)", err);
		goto failed;
	}
	err = ufuse_cuse_init_reply(&s, &req, ctx->name, 0, 0, CUSE_UNRESTRICTED_IOCTL);
	if (err) {
		XNVME_DEBUG("FAILED: ufuse_cuse_init_reply(%s); err(%d)", ctx->name, err);
		goto failed;
	}

	// ppoll() readiness can go stale before read() runs; O_NONBLOCK turns
	// that into EAGAIN instead of a blocking read() on a request that never comes
	if (fcntl(s.fd, F_SETFL, O_NONBLOCK) < 0) {
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

	while (!cuse->stop && !session.destroyed) {
		struct pollfd pfd = {.fd = s.fd, .events = POLLIN};

		if (ppoll(&pfd, 1, NULL, &wait_mask) < 0) {
			continue; // EINTR: re-check cuse->stop above
		}

		n = ufuse_read(&s, buf, buf_nbytes, &req);
		if (n == -ENODEV) {
			// The char-device is gone, possibly rejected by the kernel
			// after our CUSE_INIT reply (e.g. a duplicate name)
			XNVME_DEBUG("FAILED: CUSE char-device is gone (errno(ENODEV))");
			break;
		}
		if (n < 0) {
			// EINTR: stop signal; EAGAIN: stale ppoll() readiness; ENOENT:
			// the kernel aborted an interrupted request
			continue;
		}

		// Answers ENOSYS for an opcode without a callback in 'g_cuse_ops', and EIO for
		// a request whose length is not what its opcode carries
		ufuse_dispatch(&s, &g_cuse_ops, &session, buf, (size_t)n);
	}

	free(buf);
	ufuse_cuse_close(&s);
	return NULL;

failed:
	free(buf);
	ufuse_cuse_close(&s);
	cuse->init_rc = err;
	sem_post(ctx->ready);
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
