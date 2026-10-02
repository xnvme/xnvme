// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE // ppoll()
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

/**
 * Payload ceiling per ioctl, on our side. It is not the real ceiling: CUSE
 * never raises max_pages above the kernel's FUSE_DEFAULT_MAX_PAGES_PER_REQ
 * (32 pages, 128KiB), so fuse_do_ioctl() rejects a round whose 'in_size' or
 * 'out_size' asks for more than that before this value is ever consulted.
 * Kept at 1MiB anyway: it only bounds our own buffer, and a smaller value
 * would not lift the kernel's actual limit.
 */
#define XNVME_CUSE_MAX_WRITE (1u << 20)
#define XNVME_CUSE_BUFSIZE \
	(XNVME_CUSE_MAX_WRITE + 4096) ///< + slack for the fuse/ioctl headers ahead of the payload

/**
 * NVME_IOCTL_IO64_CMD_VEC's retry sends the cmd struct, an optional metadata
 * segment and 'vec_cnt' data segments as both the in and out iovec list; the
 * kernel rejects a FUSE_IOCTL_RETRY whose in_iovs + out_iovs exceeds
 * FUSE_IOCTL_MAX_IOV (256), so this is the largest 'vec_cnt' that list can
 * ever carry (1 + 1 + 125, doubled, is already at the ceiling)
 */
#define XNVME_CUSE_VEC_MAX_SEGS 125

/**
 * Serializes dispatch across sessions: some backends (e.g. uPCIe) keep
 * per-process state their sync command path does not lock internally
 */
static pthread_mutex_t g_cuse_dispatch_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Send a fuse_out_header-framed reply: the header, then 'meta' (a fixed
 * reply struct such as fuse_open_out or fuse_ioctl_out, or NULL), then
 * 'payload' (the variable trailing bytes, or NULL)
 */
static int
cuse_reply(int fd, uint64_t unique, int32_t error, const void *meta, size_t meta_nbytes,
	   const void *payload, size_t payload_nbytes)
{
	struct fuse_out_header out;
	struct iovec iov[3];
	int n = 0;

	out.error = error;
	out.unique = unique;
	out.len = sizeof(out) + meta_nbytes + payload_nbytes;

	iov[n].iov_base = &out;
	iov[n++].iov_len = sizeof(out);
	if (meta_nbytes) {
		iov[n].iov_base = (void *)meta;
		iov[n++].iov_len = meta_nbytes;
	}
	if (payload_nbytes) {
		iov[n].iov_base = (void *)payload;
		iov[n++].iov_len = payload_nbytes;
	}

	return writev(fd, iov, n) < 0 ? -errno : 0;
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

/**
 * Reply FUSE_IOCTL_RETRY: the kernel re-invokes FUSE_IOCTL with the given
 * 'in_iov' regions fetched and concatenated at the front of in_buf; the
 * given 'out_iov' regions are where the eventual non-retry reply's payload
 * is scattered back to
 */
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

/**
 * Build an xnvme_cmd_ctx for the given raw opcode/nsid/cdw2-3/cdw10-15,
 * matching how xnvme padc/pioc's sub_pass() builds one from --cdwXX
 */
static struct xnvme_cmd_ctx
cuse_ctx_from_cdws(struct xnvme_dev *xdev, uint8_t opcode, uint32_t nsid, uint32_t cdw2,
		   uint32_t cdw3, uint32_t cdw10, uint32_t cdw11, uint32_t cdw12, uint32_t cdw13,
		   uint32_t cdw14, uint32_t cdw15)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(xdev);

	ctx.cmd.common.opcode = opcode;
	ctx.cmd.common.nsid = nsid;
	ctx.cmd.common.cdw02 = cdw2;
	ctx.cmd.common.cdw03 = cdw3;
	ctx.cmd.common.ndt = cdw10;
	ctx.cmd.common.ndm = cdw11;
	ctx.cmd.common.cdw12 = cdw12;
	ctx.cmd.common.cdw13 = cdw13;
	ctx.cmd.common.cdw14 = cdw14;
	ctx.cmd.common.cdw15 = cdw15;

	return ctx;
}

/**
 * Pack a completion's status the way the kernel NVMe driver's own ioctl()
 * does: the raw 16-bit status field with the phase tag (bit 0) dropped,
 * carrying SC, SCT, CRD, M and DNR (see nvme_submit_user_cmd() in the
 * kernel)
 */
static int
cuse_nvme_status(const struct xnvme_cmd_ctx *ctx)
{
	return ctx->cpl.status.val >> 1;
}

/**
 * Execute a 32-bit nvme_passthru_cmd, allocate and fill '*reply' with the
 * [struct][data][metadata] payload the ioctl's caller expects back
 *
 * @return the packed NVMe status (see cuse_nvme_status()) whenever the
 * command completed, even completed-with-error; negative errno when it
 * never completed at all
 */
static int
cuse_run_passthru(struct xnvme_dev *xdev, int admin, const struct nvme_passthru_cmd *ucmd,
		  void *dbuf, void *mbuf, void **reply, size_t *reply_nbytes)
{
	struct xnvme_cmd_ctx ctx = cuse_ctx_from_cdws(
		xdev, ucmd->opcode, ucmd->nsid, ucmd->cdw2, ucmd->cdw3, ucmd->cdw10, ucmd->cdw11,
		ucmd->cdw12, ucmd->cdw13, ucmd->cdw14, ucmd->cdw15);
	struct nvme_passthru_cmd *reply_cmd;
	int err;

	err = admin ? xnvme_cmd_pass_admin(&ctx, dbuf, ucmd->data_len, mbuf, ucmd->metadata_len)
		    : xnvme_cmd_pass(&ctx, dbuf, ucmd->data_len, mbuf, ucmd->metadata_len);
	// A real completion status is packed into cpl.status even when 'err' is
	// non-zero; only an untouched cpl.status means there is none to relay
	if (err && !xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("FAILED: xnvme_cmd_pass%s(); err(%d)", admin ? "_admin" : "", err);
		return err;
	}

	*reply_nbytes = sizeof(*reply_cmd) + ucmd->data_len + ucmd->metadata_len;
	*reply = malloc(*reply_nbytes);
	if (!*reply) {
		XNVME_DEBUG("FAILED: malloc(reply_nbytes: %zu)", *reply_nbytes);
		return -ENOMEM;
	}

	reply_cmd = *reply;
	*reply_cmd = *ucmd;
	reply_cmd->result = ctx.cpl.result; // 32-bit result truncates to cdw0, as the kernel does
	if (ucmd->data_len) {
		memcpy((char *)*reply + sizeof(*reply_cmd), dbuf, ucmd->data_len);
	}
	if (ucmd->metadata_len) {
		memcpy((char *)*reply + sizeof(*reply_cmd) + ucmd->data_len, mbuf,
		       ucmd->metadata_len);
	}

	return cuse_nvme_status(&ctx);
}

/**
 * Execute a 64-bit nvme_passthru_cmd64, allocate and fill '*reply' with the
 * [struct][data][metadata] payload the ioctl's caller expects back,
 * carrying the command's full 64-bit result value
 */
static int
cuse_run_passthru64(struct xnvme_dev *xdev, int admin, const struct nvme_passthru_cmd64 *ucmd,
		    void *dbuf, void *mbuf, void **reply, size_t *reply_nbytes)
{
	struct xnvme_cmd_ctx ctx = cuse_ctx_from_cdws(
		xdev, ucmd->opcode, ucmd->nsid, ucmd->cdw2, ucmd->cdw3, ucmd->cdw10, ucmd->cdw11,
		ucmd->cdw12, ucmd->cdw13, ucmd->cdw14, ucmd->cdw15);
	struct nvme_passthru_cmd64 *reply_cmd;
	int err;

	err = admin ? xnvme_cmd_pass_admin(&ctx, dbuf, ucmd->data_len, mbuf, ucmd->metadata_len)
		    : xnvme_cmd_pass(&ctx, dbuf, ucmd->data_len, mbuf, ucmd->metadata_len);
	if (err && !xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("FAILED: xnvme_cmd_pass%s(); err(%d)", admin ? "_admin" : "", err);
		return err;
	}

	*reply_nbytes = sizeof(*reply_cmd) + ucmd->data_len + ucmd->metadata_len;
	*reply = malloc(*reply_nbytes);
	if (!*reply) {
		XNVME_DEBUG("FAILED: malloc(reply_nbytes: %zu)", *reply_nbytes);
		return -ENOMEM;
	}

	reply_cmd = *reply;
	*reply_cmd = *ucmd;
	reply_cmd->result = ctx.cpl.result;
	if (ucmd->data_len) {
		memcpy((char *)*reply + sizeof(*reply_cmd), dbuf, ucmd->data_len);
	}
	if (ucmd->metadata_len) {
		memcpy((char *)*reply + sizeof(*reply_cmd) + ucmd->data_len, mbuf,
		       ucmd->metadata_len);
	}

	return cuse_nvme_status(&ctx);
}

struct cuse_passthru64_vec_layout {
	size_t iovec_off, iovec_nbytes;
	size_t meta_off, meta_nbytes;
	size_t data_off, data_nbytes;
};

/**
 * Compute the [struct][iovec array][metadata][data...] layout of an
 * NVME_IOCTL_IO64_CMD_VEC request; 'uiov' must already point at the
 * fetched iovec array
 */
static struct cuse_passthru64_vec_layout
cuse_passthru64_vec_layout(const struct nvme_passthru_cmd64 *ucmd, const struct iovec *uiov)
{
	struct cuse_passthru64_vec_layout l = {0};

	l.iovec_off = sizeof(*ucmd);
	l.iovec_nbytes = (size_t)ucmd->vec_cnt * sizeof(struct iovec);
	l.meta_off = l.iovec_off + l.iovec_nbytes;
	l.meta_nbytes = ucmd->metadata_len;
	l.data_off = l.meta_off + l.meta_nbytes;

	for (uint32_t i = 0; i < ucmd->vec_cnt; i++) {
		l.data_nbytes += uiov[i].iov_len;
	}

	return l;
}

/**
 * Execute a vectored 64-bit nvme_passthru_cmd64, gathering 'uiov's segments
 * (as laid out by 'l' in 'in_buf') into one contiguous xnvme_buf_alloc()'d
 * data buffer and the fetched metadata into its own, then issuing it via
 * the plain xnvme_cmd_pass() rather than xnvme_cmd_pass_iov(): a backend's
 * vectored path forces SGL unconditionally, which not every controller
 * supports, while the gathered data is already one contiguous run that
 * needs no SGL at all. The reply mirrors the request's layout, carrying
 * back whatever the command actually left in each buffer.
 */
static int
cuse_run_passthru64_vec(struct xnvme_dev *xdev, const struct nvme_passthru_cmd64 *ucmd,
			const struct iovec *uiov, const struct cuse_passthru64_vec_layout *l,
			const void *in_buf, void **reply, size_t *reply_nbytes)
{
	struct xnvme_cmd_ctx ctx = cuse_ctx_from_cdws(
		xdev, ucmd->opcode, ucmd->nsid, ucmd->cdw2, ucmd->cdw3, ucmd->cdw10, ucmd->cdw11,
		ucmd->cdw12, ucmd->cdw13, ucmd->cdw14, ucmd->cdw15);
	struct nvme_passthru_cmd64 *reply_cmd;
	void *dbuf = NULL, *mbuf = NULL;
	char *reply_buf;
	size_t off = 0;
	int err;

	if (l->data_nbytes) {
		dbuf = xnvme_buf_alloc(xdev, l->data_nbytes);
		if (!dbuf) {
			XNVME_DEBUG("FAILED: xnvme_buf_alloc(data_nbytes: %zu)", l->data_nbytes);
			return -ENOMEM;
		}

		// Gathered into one xnvme_buf_alloc()'d run, not kept as separate
		// segments: a backend's vectored path forces SGL even where every
		// segment landed contiguous, and not every controller supports one
		for (uint32_t i = 0; i < ucmd->vec_cnt; i++) {
			memcpy((char *)dbuf + off, (const char *)in_buf + l->data_off + off,
			       uiov[i].iov_len);
			off += uiov[i].iov_len;
		}
	}

	if (l->meta_nbytes) {
		mbuf = xnvme_buf_alloc(xdev, l->meta_nbytes);
		if (!mbuf) {
			XNVME_DEBUG("FAILED: xnvme_buf_alloc(meta_nbytes: %zu)", l->meta_nbytes);
			xnvme_buf_free(xdev, dbuf);
			return -ENOMEM;
		}
		memcpy(mbuf, (const char *)in_buf + l->meta_off, l->meta_nbytes);
	}

	err = xnvme_cmd_pass(&ctx, dbuf, l->data_nbytes, mbuf, l->meta_nbytes);
	if (err && !xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("FAILED: xnvme_cmd_pass(); err(%d)", err);
		xnvme_buf_free(xdev, dbuf);
		xnvme_buf_free(xdev, mbuf);
		return err;
	}

	*reply_nbytes = l->data_off + l->data_nbytes;
	*reply = malloc(*reply_nbytes);
	if (!*reply) {
		XNVME_DEBUG("FAILED: malloc(reply_nbytes: %zu)", *reply_nbytes);
		xnvme_buf_free(xdev, dbuf);
		xnvme_buf_free(xdev, mbuf);
		return -ENOMEM;
	}
	reply_buf = *reply;

	reply_cmd = (void *)reply_buf;
	*reply_cmd = *ucmd;
	reply_cmd->result = ctx.cpl.result;

	memcpy(reply_buf + l->iovec_off, uiov, l->iovec_nbytes);
	if (l->meta_nbytes) {
		memcpy(reply_buf + l->meta_off, mbuf, l->meta_nbytes);
	}
	memcpy(reply_buf + l->data_off, dbuf, l->data_nbytes);

	xnvme_buf_free(xdev, dbuf);
	xnvme_buf_free(xdev, mbuf);

	return cuse_nvme_status(&ctx);
}

/**
 * Answer one FUSE_IOCTL request; NVME_IOCTL_ID, NVME_IOCTL_ADMIN_CMD,
 * NVME_IOCTL_IO_CMD, NVME_IOCTL_ADMIN64_CMD, NVME_IOCTL_IO64_CMD and
 * NVME_IOCTL_IO64_CMD_VEC are answered, everything else declines with
 * ENOTTY
 */
static int
cuse_dispatch_ioctl(int fd, uint64_t unique, struct xnvme_dev *xdev,
		    const struct fuse_ioctl_in *ioc, const void *in_buf, size_t in_bufsz)
{
	void *arg = (void *)(uintptr_t)ioc->arg;
	const size_t cmd_nbytes = sizeof(struct nvme_passthru_cmd);
	const size_t cmd_nbytes64 = sizeof(struct nvme_passthru_cmd64);

	if (ioc->flags & FUSE_IOCTL_COMPAT) {
		XNVME_DEBUG("FAILED: FUSE_IOCTL_COMPAT is not supported");
		return cuse_reply_err(fd, unique, ENOSYS);
	}

	switch (ioc->cmd) {
	case NVME_IOCTL_ID:
		// The kernel driver declines this on a controller's char device
		// (no nsid of its own); match it for the same case here
		if (xnvme_dev_get_ident(xdev)->dtype == XNVME_DEV_TYPE_NVME_CONTROLLER) {
			return cuse_reply_err(fd, unique, ENOTTY);
		}
		return cuse_reply_ioctl(fd, unique, (int)xnvme_dev_get_nsid(xdev), NULL, 0);

	case NVME_IOCTL_ADMIN_CMD:
	case NVME_IOCTL_IO_CMD: {
		const struct nvme_passthru_cmd *ucmd;
		void *dbuf = NULL, *mbuf = NULL, *reply = NULL;
		size_t reply_nbytes = 0;
		int result, rc;

		// A controller has no namespace of its own; match the kernel
		// driver's own NVME_IOCTL_ID precedent above
		if (ioc->cmd == NVME_IOCTL_IO_CMD &&
		    xnvme_dev_get_ident(xdev)->dtype == XNVME_DEV_TYPE_NVME_CONTROLLER) {
			return cuse_reply_err(fd, unique, ENOTTY);
		}

		if (in_bufsz < cmd_nbytes) {
			struct iovec iov = {arg, cmd_nbytes};

			return cuse_reply_ioctl_retry(fd, unique, &iov, 1, &iov, 1);
		}

		ucmd = in_buf;
		if (in_bufsz < cmd_nbytes + ucmd->data_len + ucmd->metadata_len) {
			struct iovec iov[3];
			uint32_t n = 0;

			iov[n].iov_base = arg;
			iov[n++].iov_len = cmd_nbytes;
			if (ucmd->data_len) {
				iov[n].iov_base = (void *)(uintptr_t)ucmd->addr;
				iov[n++].iov_len = ucmd->data_len;
			}
			if (ucmd->metadata_len) {
				iov[n].iov_base = (void *)(uintptr_t)ucmd->metadata;
				iov[n++].iov_len = ucmd->metadata_len;
			}

			return cuse_reply_ioctl_retry(fd, unique, iov, n, iov, n);
		}

		// Bounce through xNVMe's own buffer: some backends submit only
		// from their own DMA-registered heap, never arbitrary process memory
		if (ucmd->data_len) {
			dbuf = xnvme_buf_alloc(xdev, ucmd->data_len);
			if (!dbuf) {
				XNVME_DEBUG("FAILED: xnvme_buf_alloc(data_len: %u)",
					    ucmd->data_len);
				return cuse_reply_err(fd, unique, ENOMEM);
			}
			memcpy(dbuf, (const char *)in_buf + cmd_nbytes, ucmd->data_len);
		}
		if (ucmd->metadata_len) {
			mbuf = xnvme_buf_alloc(xdev, ucmd->metadata_len);
			if (!mbuf) {
				XNVME_DEBUG("FAILED: xnvme_buf_alloc(metadata_len: %u)",
					    ucmd->metadata_len);
				xnvme_buf_free(xdev, dbuf);
				return cuse_reply_err(fd, unique, ENOMEM);
			}
			memcpy(mbuf, (const char *)in_buf + cmd_nbytes + ucmd->data_len,
			       ucmd->metadata_len);
		}

		result = cuse_run_passthru(xdev, ioc->cmd == NVME_IOCTL_ADMIN_CMD, ucmd, dbuf,
					   mbuf, &reply, &reply_nbytes);
		xnvme_buf_free(xdev, dbuf);
		xnvme_buf_free(xdev, mbuf);

		rc = result < 0 ? cuse_reply_err(fd, unique, -result)
				: cuse_reply_ioctl(fd, unique, result, reply, reply_nbytes);
		free(reply);
		return rc;
	}

	case NVME_IOCTL_ADMIN64_CMD:
	case NVME_IOCTL_IO64_CMD: {
		const struct nvme_passthru_cmd64 *ucmd;
		void *dbuf = NULL, *mbuf = NULL, *reply = NULL;
		size_t reply_nbytes = 0;
		int result, rc;

		if (ioc->cmd == NVME_IOCTL_IO64_CMD &&
		    xnvme_dev_get_ident(xdev)->dtype == XNVME_DEV_TYPE_NVME_CONTROLLER) {
			return cuse_reply_err(fd, unique, ENOTTY);
		}

		if (in_bufsz < cmd_nbytes64) {
			struct iovec iov = {arg, cmd_nbytes64};

			return cuse_reply_ioctl_retry(fd, unique, &iov, 1, &iov, 1);
		}

		ucmd = in_buf;
		if (in_bufsz < cmd_nbytes64 + ucmd->data_len + ucmd->metadata_len) {
			struct iovec iov[3];
			uint32_t n = 0;

			iov[n].iov_base = arg;
			iov[n++].iov_len = cmd_nbytes64;
			if (ucmd->data_len) {
				iov[n].iov_base = (void *)(uintptr_t)ucmd->addr;
				iov[n++].iov_len = ucmd->data_len;
			}
			if (ucmd->metadata_len) {
				iov[n].iov_base = (void *)(uintptr_t)ucmd->metadata;
				iov[n++].iov_len = ucmd->metadata_len;
			}

			return cuse_reply_ioctl_retry(fd, unique, iov, n, iov, n);
		}

		if (ucmd->data_len) {
			dbuf = xnvme_buf_alloc(xdev, ucmd->data_len);
			if (!dbuf) {
				XNVME_DEBUG("FAILED: xnvme_buf_alloc(data_len: %u)",
					    ucmd->data_len);
				return cuse_reply_err(fd, unique, ENOMEM);
			}
			memcpy(dbuf, (const char *)in_buf + cmd_nbytes64, ucmd->data_len);
		}
		if (ucmd->metadata_len) {
			mbuf = xnvme_buf_alloc(xdev, ucmd->metadata_len);
			if (!mbuf) {
				XNVME_DEBUG("FAILED: xnvme_buf_alloc(metadata_len: %u)",
					    ucmd->metadata_len);
				xnvme_buf_free(xdev, dbuf);
				return cuse_reply_err(fd, unique, ENOMEM);
			}
			memcpy(mbuf, (const char *)in_buf + cmd_nbytes64 + ucmd->data_len,
			       ucmd->metadata_len);
		}

		result = cuse_run_passthru64(xdev, ioc->cmd == NVME_IOCTL_ADMIN64_CMD, ucmd, dbuf,
					     mbuf, &reply, &reply_nbytes);
		xnvme_buf_free(xdev, dbuf);
		xnvme_buf_free(xdev, mbuf);

		rc = result < 0 ? cuse_reply_err(fd, unique, -result)
				: cuse_reply_ioctl(fd, unique, result, reply, reply_nbytes);
		free(reply);
		return rc;
	}

	case NVME_IOCTL_IO64_CMD_VEC: {
		const struct nvme_passthru_cmd64 *ucmd;
		const struct iovec *uiov;
		struct cuse_passthru64_vec_layout l;
		void *reply = NULL;
		size_t reply_nbytes = 0;
		int result, rc;

		if (xnvme_dev_get_ident(xdev)->dtype == XNVME_DEV_TYPE_NVME_CONTROLLER) {
			return cuse_reply_err(fd, unique, ENOTTY);
		}

		if (in_bufsz < cmd_nbytes64) {
			struct iovec iov = {arg, cmd_nbytes64};

			return cuse_reply_ioctl_retry(fd, unique, &iov, 1, &iov, 1);
		}

		ucmd = in_buf;
		if (ucmd->vec_cnt > XNVME_CUSE_VEC_MAX_SEGS) {
			XNVME_DEBUG("FAILED: vec_cnt(%u) exceeds the retry-list ceiling",
				    ucmd->vec_cnt);
			return cuse_reply_err(fd, unique, EINVAL);
		}

		if (in_bufsz < cmd_nbytes64 + (size_t)ucmd->vec_cnt * sizeof(struct iovec) +
				       ucmd->metadata_len) {
			struct iovec iov[3];
			uint32_t n = 0;

			iov[n].iov_base = arg;
			iov[n++].iov_len = cmd_nbytes64;
			iov[n].iov_base = (void *)(uintptr_t)ucmd->addr;
			iov[n++].iov_len = (size_t)ucmd->vec_cnt * sizeof(struct iovec);
			if (ucmd->metadata_len) {
				iov[n].iov_base = (void *)(uintptr_t)ucmd->metadata;
				iov[n++].iov_len = ucmd->metadata_len;
			}

			return cuse_reply_ioctl_retry(fd, unique, iov, n, iov, n);
		}

		uiov = (const void *)((const char *)in_buf + cmd_nbytes64);
		// Bound each segment before summing: an unbounded sum can wrap to a
		// small value, understating how much data is actually coming
		for (uint32_t i = 0; i < ucmd->vec_cnt; i++) {
			if (uiov[i].iov_len > XNVME_CUSE_MAX_WRITE) {
				XNVME_DEBUG("FAILED: iov_len(%zu) exceeds XNVME_CUSE_MAX_WRITE",
					    uiov[i].iov_len);
				return cuse_reply_err(fd, unique, EINVAL);
			}
		}
		l = cuse_passthru64_vec_layout(ucmd, uiov);

		if (in_bufsz < l.data_off + l.data_nbytes) {
			struct iovec iov[2 + ucmd->vec_cnt + (ucmd->metadata_len ? 1 : 0)];
			uint32_t n = 0;

			iov[n].iov_base = arg;
			iov[n++].iov_len = cmd_nbytes64;
			iov[n].iov_base = (void *)(uintptr_t)ucmd->addr;
			iov[n++].iov_len = l.iovec_nbytes;
			if (ucmd->metadata_len) {
				iov[n].iov_base = (void *)(uintptr_t)ucmd->metadata;
				iov[n++].iov_len = ucmd->metadata_len;
			}
			for (uint32_t i = 0; i < ucmd->vec_cnt; i++) {
				iov[n].iov_base = uiov[i].iov_base;
				iov[n++].iov_len = uiov[i].iov_len;
			}

			return cuse_reply_ioctl_retry(fd, unique, iov, n, iov, n);
		}

		result = cuse_run_passthru64_vec(xdev, ucmd, uiov, &l, in_buf, &reply,
						 &reply_nbytes);

		rc = result < 0 ? cuse_reply_err(fd, unique, -result)
				: cuse_reply_ioctl(fd, unique, result, reply, reply_nbytes);
		free(reply);
		return rc;
	}

	default:
		XNVME_DEBUG("FAILED: unsupported ioctl cmd(0x%x)", ioc->cmd);
		return cuse_reply_err(fd, unique, ENOTTY);
	}
}

/**
 * Handle the CUSE_INIT handshake: read it, reply with the device name so
 * the kernel creates /dev/<name>, matching cuse_pack_info()'s wire format
 * (each dev_info_argv string, NUL included, back to back; there is only
 * ever one string here)
 *
 * @return 0 on success, negative errno otherwise
 */
static int
cuse_init(int fd, const char *name, void *buf, size_t bufsize)
{
	const struct fuse_in_header *in;
	const struct cuse_init_in *init_in;
	struct cuse_init_out out = {0};
	char dev_info[64];
	int dev_info_ret;
	size_t dev_info_len;
	ssize_t n;

	n = read(fd, buf, bufsize);
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

	out.major = FUSE_KERNEL_VERSION;
	out.minor = FUSE_KERNEL_MINOR_VERSION;
	out.flags = CUSE_UNRESTRICTED_IOCTL;
	out.max_read = XNVME_CUSE_MAX_WRITE;
	out.max_write = XNVME_CUSE_MAX_WRITE;

	if (init_in->major < 7) {
		XNVME_DEBUG("FAILED: kernel CUSE_INIT major(%u) < 7", init_in->major);
		cuse_reply_err(fd, in->unique, EPROTO);
		return -EPROTO;
	}

	return cuse_reply(fd, in->unique, 0, &out, sizeof(out), dev_info, dev_info_len);
}

struct cuse_session_ctx {
	struct xnvme_cuse *cuse;
	struct xnvme_dev *xdev;
	const char *name;
	sem_t *ready;
};

static void *
cuse_session_main(void *arg)
{
	struct cuse_session_ctx *sctx = arg;
	struct xnvme_cuse *cuse = sctx->cuse;
	// 'sctx' is on xnvme_cuse_start()'s stack, invalid once sem_post() below runs
	struct xnvme_dev *xdev = sctx->xdev;
	void *buf;
	int fd, err;

	fd = open("/dev/cuse", O_RDWR);
	if (fd < 0) {
		err = -errno;
		XNVME_DEBUG("FAILED: open(/dev/cuse); err(%d); try 'modprobe cuse'", err);
		cuse->init_rc = err;
		sem_post(sctx->ready);
		return NULL;
	}

	buf = malloc(XNVME_CUSE_BUFSIZE);
	if (!buf) {
		XNVME_DEBUG("FAILED: malloc(XNVME_CUSE_BUFSIZE: %u)", XNVME_CUSE_BUFSIZE);
		close(fd);
		cuse->init_rc = -ENOMEM;
		sem_post(sctx->ready);
		return NULL;
	}

	err = cuse_init(fd, sctx->name, buf, XNVME_CUSE_BUFSIZE);
	if (err) {
		XNVME_DEBUG("FAILED: cuse_init(%s); err(%d)", sctx->name, err);
		free(buf);
		close(fd);
		cuse->init_rc = err;
		sem_post(sctx->ready);
		return NULL;
	}

	// ppoll() readiness can go stale before read() runs; O_NONBLOCK turns
	// that into EAGAIN instead of a blocking read() on a request that never comes
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
		XNVME_DEBUG("FAILED: fcntl(O_NONBLOCK); errno(%d)", errno);
	}

	cuse->init_rc = 0;
	sem_post(sctx->ready);

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
			cuse_dispatch_ioctl(fd, in->unique, xdev, ioc, ioc_buf, ioc->in_size);
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
xnvme_cuse_start(struct xnvme_cuse *cuse, struct xnvme_dev *xdev, const char *name)
{
	struct cuse_session_ctx sctx;
	sem_t ready;
	int err;

	if (sem_init(&ready, 0, 0) < 0) {
		XNVME_DEBUG("FAILED: sem_init(); errno(%d)", errno);
		return -errno;
	}

	sctx.cuse = cuse;
	sctx.xdev = xdev;
	sctx.name = name;
	sctx.ready = &ready;

	err = pthread_create(&cuse->tid, NULL, cuse_session_main, &sctx);
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
xnvme_cuse_start(struct xnvme_cuse *XNVME_UNUSED(cuse), struct xnvme_dev *XNVME_UNUSED(xdev),
		 const char *XNVME_UNUSED(name))
{
	return -ENOSYS;
}

void
xnvme_cuse_stop(struct xnvme_cuse *XNVME_UNUSED(cuse))
{
}

#endif
