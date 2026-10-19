// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_MACOS_ENABLED
#include <errno.h>
#include <xnvme_be_macos_driverkit.h>
#include <mach/mach_error.h>

#include <xnvme_dev.h>
#include <xnvme_queue.h>

struct xnvme_queue_macos_driverkit {
	struct xnvme_queue_base base;
	RingQueue *sq_queue;
	RingQueue *cq_queue;

	uint id;

	uint8_t _rsvd[208];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_macos_driverkit) == XNVME_BE_QUEUE_STATE_NBYTES,
		    "Incorrect size")

int
xnvme_be_macos_driverkit_queue_init(struct xnvme_queue *_queue, int XNVME_UNUSED(opts))
{
	struct xnvme_queue_macos_driverkit *q = (struct xnvme_queue_macos_driverkit *)_queue;
	struct xnvme_be_macos_driverkit_state *state =
		(struct xnvme_be_macos_driverkit_state *)_queue->base.dev->be.state;
	mach_vm_size_t size = 0;
	mach_vm_address_t addr;
	kern_return_t res;
	int qpid;

	qpid = _xnvme_be_driverkit_create_ioqpair(state, q->base.capacity, 0);
	if (qpid < 0) {
		XNVME_DEBUG("FAILED: nvme_create_ioqpair(); err: %d", qpid);
		return qpid;
	}
	q->id = qpid;

	res = IOConnectMapMemory64(state->ctrlr->device_connection, 1 << 16 | qpid,
				   mach_task_self(), &addr, &size, kIOMapAnywhere);
	XNVME_DEBUG("INFO: IOConnectMapMemory64(); returned: %x, %s", res, mach_error_string(res));
	if (res != kIOReturnSuccess) {
		XNVME_DEBUG("FAILED: IOConnectMapMemory64(); "
			    "ret(0x%08x), '%s'",
			    res, mach_error_string(res));
		return -EIO;
	}
	q->sq_queue = (RingQueue *)addr;

	size = 0;
	res = IOConnectMapMemory64(state->ctrlr->device_connection, qpid, mach_task_self(), &addr,
				   &size, kIOMapAnywhere);
	XNVME_DEBUG("INFO: IOConnectMapMemory64(); returned: %x, %s", res, mach_error_string(res));
	if (res != kIOReturnSuccess) {
		XNVME_DEBUG("FAILED: IOConnectMapMemory64(); "
			    "ret(0x%08x), '%s'",
			    res, mach_error_string(res));
		return -EIO;
	}
	q->cq_queue = (RingQueue *)addr;

	return 0;
}

int
xnvme_be_macos_driverkit_queue_term(struct xnvme_queue *_queue)
{
	struct xnvme_queue_macos_driverkit *q = (struct xnvme_queue_macos_driverkit *)_queue;
	struct xnvme_be_macos_driverkit_state *state =
		(struct xnvme_be_macos_driverkit_state *)q->base.dev->be.state;
	int ret;

	ret = _xnvme_be_driverkit_delete_ioqpair(state, q->id);
	if (ret) {
		XNVME_DEBUG("FAILED: _xnvme_be_driverkit_delete_ioqpair()");
		return ret;
	}

	q->cq_queue = NULL;
	q->sq_queue = NULL;
	return ret;
}

int
xnvme_be_macos_driverkit_queue_poke(struct xnvme_queue *queue, uint32_t XNVME_UNUSED(max))
{
	struct xnvme_queue_macos_driverkit *q = (struct xnvme_queue_macos_driverkit *)queue;
	struct xnvme_be_macos_driverkit_state *state =
		(struct xnvme_be_macos_driverkit_state *)queue->base.dev->be.state;
	uint64_t qid[2] = {q->id, q->id}; // Submission queue, completion queue
	int reaped = 0;
	int queue_ret;
	NvmeSubmitCmd nvme_cmd;
	kern_return_t ret;
	struct xnvme_cmd_ctx *ctx;

	ret = IOConnectCallScalarMethod(state->ctrlr->device_connection, NVME_POKE, qid, 2, NULL,
					0);
	if (ret != kIOReturnSuccess) {
		XNVME_DEBUG("FAILED: IOConnectCallScalarMethod(); ret(0x%x), msg(%s)", ret,
			    mach_error_string(ret));
		return -EIO;
	}
	while ((queue_ret = queue_dequeue(q->cq_queue, &nvme_cmd)) == 0) {
		ctx = (struct xnvme_cmd_ctx *)nvme_cmd.backend_opaque;
		memcpy(&ctx->cpl, nvme_cmd.cpl, sizeof(struct xnvme_spec_cpl));
		ctx->async.cb(ctx, ctx->async.cb_arg);

		reaped += 1;
		queue->base.outstanding -= 1;
	}

	if (queue_ret == QUEUE_ERROR) {
		XNVME_DEBUG("FAILED: queue_ret == QUEUE_ERROR");
		return -EIO;
	}

	return reaped;
}

int
xnvme_be_macos_driverkit_async_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
				      void *mbuf, size_t mbuf_nbytes)
{
	struct xnvme_queue_macos_driverkit *q =
		(struct xnvme_queue_macos_driverkit *)ctx->async.queue;
	struct xnvme_be_macos_driverkit_state *state =
		(struct xnvme_be_macos_driverkit_state *)ctx->dev->be.state;
	NvmeSubmitCmd cmd = {0};
	int queue_ret;
	struct buffer *buf;
	uint64_t _buf_base_offset;

	cmd.queue_id = state->qid_sync;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_READ:
		break;

	case XNVME_SPEC_FS_OPC_READ:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
		break;

	case XNVME_SPEC_NVM_OPC_WRITE:
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
		break;
	}

	if (dbuf) {
		buf = buffer_find(state->buffers, (uint64_t)dbuf);
		if (!buf) {
			XNVME_DEBUG("FAILED: buffer_find()");
			return -EINVAL;
		}
		_buf_base_offset = ((uint64_t)dbuf) - buf->vaddr;
		cmd.dbuf_nbytes = dbuf_nbytes;
		cmd.dbuf_offset = _buf_base_offset;
		cmd.dbuf_token = buf->vaddr;
		XNVME_DEBUG("INFO: buf token: %llx: %llx, %llx", (uint64_t)buf->vaddr,
			    (uint64_t)dbuf, cmd.dbuf_offset);
	}
	if (mbuf) {
		buf = buffer_find(state->buffers, (uint64_t)mbuf);
		if (!buf) {
			XNVME_DEBUG("FAILED: buffer_find()");
			return -EINVAL;
		}
		_buf_base_offset = ((uint64_t)mbuf) - buf->vaddr;
		cmd.mbuf_nbytes = mbuf_nbytes;
		cmd.mbuf_offset = _buf_base_offset;
		cmd.mbuf_token = buf->vaddr;
		XNVME_DEBUG("INFO: buf token: %llx: %llx, %llx", (uint64_t)buf->vaddr,
			    (uint64_t)mbuf, cmd.mbuf_offset);
	}
	memcpy(cmd.cmd, &ctx->cmd, sizeof(struct xnvme_spec_cmd));
	cmd.backend_opaque = (uint64_t)ctx;

	queue_ret = queue_enqueue(q->sq_queue, &cmd);
	if (queue_ret == QUEUE_FULL) {
		return -EBUSY;
	} else if (queue_ret) {
		XNVME_DEBUG("FAILED: queue_enqueue()");
		return -EIO;
	}

	q->base.outstanding += 1;
	return 0;
}

#endif

struct xnvme_be_async g_xnvme_be_macos_driverkit_async = {
#ifdef XNVME_PLATFORM_MACOS_ENABLED
	.cmd_io = xnvme_be_macos_driverkit_async_cmd_io,
	.cmd_iov = xnvme_be_nosys_queue_cmd_iov,
	.poke = xnvme_be_macos_driverkit_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_macos_driverkit_queue_init,
	.term = xnvme_be_macos_driverkit_queue_term,
#else
	.cmd_io = xnvme_be_nosys_queue_cmd_io,
	.cmd_iov = xnvme_be_nosys_queue_cmd_iov,
	.poke = xnvme_be_nosys_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nosys_queue_init,
	.term = xnvme_be_nosys_queue_term,
#endif
	.get_completion_fd = xnvme_be_nosys_queue_get_completion_fd,
	.id = "driverkit",
};
