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

static int
cuse_dispatch_ioctl(int fd, uint64_t unique, struct xnvme_dev *dev,
		    const struct fuse_ioctl_in *ioc, const void *XNVME_UNUSED(buf),
		    size_t XNVME_UNUSED(buf_nbytes))
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
