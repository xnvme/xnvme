/*
 * fio xNVMe IO Engine
 *
 * IO engine using the asynchronous interface of the xNVMe C API.
 *
 * See: http://xnvme.io/
 *
 * -----------------------------------------------------------------------------
 *
 * Notes on the implementation and fio IO engines in general
 * =========================================================
 *
 * Built-in engine interface:
 *
 * - static void fio_init xnvme_fioe_register(void)
 * - static void fio_exit xnvme_fioe_unregister(void)
 * - static struct ioengine_ops ioengine
 * - Usage: '--ioengine=myengine'
 *
 * External engine interface:
 *
 * - struct ioengine_ops ioengine
 * - Usage: '--ioengine=external:/path/to/myengine.so'
 *
 * When writing an external engine you actually have two choices, you can:
 *
 * 1) Follow the "External engine interface" as described above
 * 2) Fake an internal engine
 *    - Implement the "Built-in engine interface"
 *    - Inject the engine via LD_PRELOAD=/path/to/myengine.so
 *    - NOTE: by injecting you are potentially overwriting more symbols than
 *      just those required by the "Built-in engine interface"
 *
 * It seems like the "cleanest" approach is to implement en engine following the
 * "External engine interface", however, there is some spurious behavior/race
 * causing a segfault when accessing `td->io_ops` in `_queue()`.
 *
 * However, for some reason, this segfault does not occur if `td->io_ops` is
 * touched during `_init()` which is why `_init()` echoes the value of
 * `td->io_ops`.
 */
#include <stdlib.h>
#include <assert.h>
#include <fio/fio.h>
#include <fio/optgroup.h>
#include <libxnvme.h>


struct xnvme_fioe_data {
	///< I/O completion queue
	struct io_u **iocq;

	///< # of iocq entries; incremented via getevents()/cb_pool()
	unsigned int completed;

	///< # of errors; incremented when observed on completion via getevents()/cb_pool()
	uint64_t ecount;

	uint64_t ssw;
	size_t sect_nbytes;
	struct xnvme_dev *dev;
	struct xnvme_async_ctx *ctx;
	struct xnvme_req_pool *reqs;
	const struct xnvme_geo *geo;

	struct fio_file *files[10];
	int nfiles;
};

struct xnvme_fioe_wrap {
	struct xnvme_fioe_data *xd;
	struct io_u *io_u;
};
struct xnvme_fioe_options {
	void *padding;
	unsigned int hipri;
	unsigned int sqpoll_thread;
};

static struct fio_option options[] = {
	{
		.name   = "hipri",
		.lname  = "High Priority",
		.type   = FIO_OPT_STR_SET,
		.off1   = offsetof(struct xnvme_fioe_options, hipri),
		.help   = "Use polled IO completions",
		.category = FIO_OPT_C_ENGINE,
		.group  = FIO_OPT_G_IOURING,
	},
	{
		.name   = "sqthread_poll",
		.lname  = "Kernel SQ thread polling",
		.type   = FIO_OPT_INT,
		.off1   = offsetof(struct xnvme_fioe_options, sqpoll_thread),
		.help   = "Offload submission/completion to kernel thread",
		.category = FIO_OPT_C_ENGINE,
		.group  = FIO_OPT_G_IOURING,
	},
	{
		.name   = NULL,
	},
};

static void
cb_pool(struct xnvme_req *req, void *cb_arg)
{
	struct io_u *io_u = cb_arg;
	struct xnvme_fioe_wrap *wrap = io_u->engine_data;
	struct xnvme_fioe_data *xd = wrap->xd;

	if (xnvme_req_cpl_status(req)) {
		xnvme_req_pr(req, XNVME_PR_DEF);
		xd->ecount += 1;
		io_u->error = EIO;
	}

	xd->iocq[xd->completed++] = io_u;
	SLIST_INSERT_HEAD(&req->pool->head, req, link);
}

static void
xnvme_fioe_cleanup(struct thread_data *td)
{
	struct xnvme_fioe_data *xd = td->io_ops_data;

	//log_info("xnvme_fioe: cleanup()\n");

	if (!xd) {
		return;
	}

	log_info("xnvem_fioe: cleanup(): xd->ecount: %zu\n", xd->ecount);

	if (xd->dev) {
		xnvme_async_term(xd->dev, xd->ctx);
	}

	xnvme_req_pool_free(xd->reqs);
	xnvme_dev_close(xd->dev);

	free(xd->iocq);
	free(xd);
}

static int
xnvme_fioe_setup(struct thread_data *td)
{
	struct xnvme_fioe_data *xd = NULL;

	xd = malloc(sizeof(*xd));
	if (!xd) {
		return 1;
	}
	memset(xd, 0, sizeof(*xd));

	xd->iocq = malloc(td->o.iodepth * sizeof(struct io_u *));
	memset(xd->iocq, 0, td->o.iodepth * sizeof(struct io_u *));

	td->io_ops_data = xd;

	return 0;
}

static int
xnvme_fioe_init(struct thread_data *td)
{
	struct xnvme_fioe_data *xd = NULL;
	struct xnvme_fioe_options *o = td->eo;
	int flags = 0;

	if (o->hipri) {
		flags |= XNVME_ASYNC_IOPOLL;
	}
	if (o->sqpoll_thread) {
		flags |= XNVME_ASYNC_SQPOLL;
	}

	if (!td->o.use_thread) {
		log_err("xnvme_fioe: init(): --thread=1 is required\n");
		return 1;
	}
	if (td->files_size != 1) {
		log_err("xnvme_fioe: init(): provide only one '--filename'\n");
		return 1;
	}
	if (xnvme_fioe_setup(td)) {
		log_err("xnvme_fioe: init(): xnvme_fioe_setup(): failed\n");
		return 1;
	}
	xd = td->io_ops_data;

	xd->dev = xnvme_dev_open(td->files[0]->file_name);
	if (!xd->dev) {
		log_err("xnvme_fioe: init(): {uri: '%s', err: '%s'}\n",
			td->files[0]->file_name, strerror(errno));
		return 1;
	}
	xd->geo = xnvme_dev_get_geo(xd->dev);
	xd->sect_nbytes = xd->geo->nbytes;
	xd->ssw = xnvme_dev_get_ssw(xd->dev);
	xd->ecount = 0;

	if (xnvme_async_init(xd->dev, &(xd->ctx), td->o.iodepth, flags)) {
		log_err("xnvme_fioe: init(): failed xnvme_async_init()\n");
		return 1;
	}
	if (xnvme_req_pool_alloc(&xd->reqs, td->o.iodepth + 1)) {
		log_err("xnvme_fioe: init(): xnvme_req_pool_alloc()\n");
		return 1;
	}
	if (xnvme_req_pool_init(xd->reqs, xd->ctx, cb_pool, xd)) {
		log_err("xnvme_fioe: init(): xnvme_req_pool_init()\n");
		return 1;
	}

	{
		int cur = xd->nfiles++;

		xd->files[cur] = td->files[cur];
		xd->files[cur]->filetype = FIO_TYPE_BLOCK;
		xd->files[cur]->real_file_size = xd->geo->tbytes;
		fio_file_set_size_known(xd->files[cur]);
	}

	log_info("xnvme_fioe: init(): td->io_ops: %p\n", td->io_ops);

	return 0;
}

static int
xnvme_fioe_iomem_alloc(struct thread_data *td, size_t total_mem)
{
	struct xnvme_fioe_data *xd = td->io_ops_data;

	if (xd->dev) {
		td->orig_buffer = xnvme_buf_alloc(xd->dev, total_mem, NULL);
		XNVME_DEBUG("xnvme_buf_alloced: %p", (void *)td->orig_buffer)
	} else {
		td->orig_buffer = malloc(total_mem);
		XNVME_DEBUG("malloc: %p", (void *)td->orig_buffer)
	}

	return td->orig_buffer == NULL;
}

static void
xnvme_fioe_iomem_free(struct thread_data *td)
{
	struct xnvme_fioe_data *xd = td->io_ops_data;

	if (!xd->dev) {
		XNVME_DEBUG("hmm: %p", (void *)xd->dev);
		free(td->orig_buffer);
		return;
	}

	xnvme_buf_free(xd->dev, td->orig_buffer);
}

static int
xnvme_fioe_io_u_init(struct thread_data *td, struct io_u *io_u)
{
	struct xnvme_fioe_data *xd = td->io_ops_data;
	struct xnvme_fioe_wrap *wrap;

	io_u->engine_data = NULL;

	wrap = calloc(1, sizeof(*wrap));
	if (wrap == NULL) {
		return 1;
	}

	wrap->io_u = io_u;
	wrap->xd = xd;

	io_u->engine_data = wrap;

	return 0;
}

static void
xnvme_fioe_io_u_free(struct thread_data *td, struct io_u *io_u)
{
	struct xnvme_fioe_wrap *wrap = io_u->engine_data;

	if (wrap) {
		assert(wrap->io_u == io_u);
		free(wrap);
		io_u->engine_data = NULL;
	}
}

static struct io_u *
xnvme_fioe_event(struct thread_data *td, int event)
{
	struct xnvme_fioe_data *xd = td->io_ops_data;

	assert(event >= 0);
	assert((unsigned)event < xd->completed);

	return xd->iocq[event];
}

static int
xnvme_fioe_getevents(struct thread_data *td, unsigned int min,
		     unsigned int max, const struct timespec *t)
{
	struct xnvme_fioe_data *xd = td->io_ops_data;
	int err;

	xd->completed = 0;

	if (t) {
		assert(false);
	}

	do {
		err = xnvme_async_poke(xd->dev, xd->ctx, max - xd->completed);
		if (err >= 0) {
			continue;
		}
		switch (err) {
		case -EBUSY:
		case -EAGAIN:
			usleep(1);
			break;

		default:
			XNVME_DEBUG("Oh my");
			assert(false);
			return 0;
		}
	} while (xd->completed < min);

	return xd->completed;
}

static enum fio_q_status
xnvme_fioe_queue(struct thread_data *td, struct io_u *io_u) {
	struct xnvme_fioe_data *xd = td->io_ops_data;
	struct xnvme_req *req = NULL;
	uint32_t nsid = xnvme_dev_get_nsid(xd->dev);
	uint64_t slba;
	uint16_t nlb;
	int err;

	fio_ro_check(td, io_u);

	slba = io_u->offset >> xd->ssw;
	nlb = (io_u->xfer_buflen / xd->sect_nbytes) - 1;

	if (td->io_ops->flags & FIO_SYNCIO)
	{
		log_err("xnvme_fioe: queue(): Got sync...");
		assert(false);
		return FIO_Q_COMPLETED;
	}

	req = SLIST_FIRST(&xd->reqs->head);
	SLIST_REMOVE_HEAD(&xd->reqs->head, link);

	req->async.cb_arg = io_u;

	XNVME_DEBUG("slba: 0x%lx, nlb: 0x%x, buf: 0x%p",
		    slba, nlb, io_u->xfer_buf);

	switch (io_u->ddir)
	{
	case DDIR_READ:
		err = xnvme_cmd_read(xd->dev, nsid, slba, nlb, io_u->xfer_buf,
				     NULL, XNVME_CMD_ASYNC, req);
		break;

	case DDIR_WRITE:
		err = xnvme_cmd_write(xd->dev, nsid, slba, nlb, io_u->xfer_buf,
				      NULL, XNVME_CMD_ASYNC, req);
		break;

	default:
		log_err("xnvme_fioe: queue(): ENOSYS: %u\n", io_u->ddir);
		err = -1;
		assert(false);
		break;
	}

	switch (err)
	{
	case 0:
		return FIO_Q_QUEUED;

	case -EBUSY:
	case -EAGAIN:
		SLIST_INSERT_HEAD(&req->pool->head, req, link);
		return FIO_Q_BUSY;

	default:
		log_err("xnvme_fioe: queue(): err: '%d'", err);

		SLIST_INSERT_HEAD(&req->pool->head, req, link);
		io_u->error = err;
		assert(false);
		return FIO_Q_COMPLETED;
	}
}

static int
xnvme_fioe_close(struct thread_data *td, struct fio_file *f)
{
	return 0;
}

static int
xnvme_fioe_open(struct thread_data *td, struct fio_file *f)
{
	/*
	log_info("xnvme_fioe: open():\n");
	log_info("  file_name: '%s'\n", f->file_name);
	log_info("  fileno: '%d\n", f->fileno);
	log_info("  io_size: '%zu'\n", f->io_size);
	log_info("  real_file_size: '%zu'\n", f->real_file_size);
	log_info("  file_offset: '%zu'\n", f->file_offset);
	*/

	return 0;
}

struct ioengine_ops ioengine = {
	.name		= "xnvme",
	.version	= FIO_IOOPS_VERSION,
	.options                = options,
	.option_struct_size     = sizeof(struct xnvme_fioe_options),
	.flags		= \
	FIO_DISKLESSIO | \
	FIO_NODISKUTIL | \
	FIO_NOEXTEND | \
	FIO_MEMALIGN | \
	FIO_RAWIO,

	.cleanup	= xnvme_fioe_cleanup,
	.init		= xnvme_fioe_init,

	.iomem_free	= xnvme_fioe_iomem_free,
	.iomem_alloc	= xnvme_fioe_iomem_alloc,

	.io_u_free	= xnvme_fioe_io_u_free,
	.io_u_init	= xnvme_fioe_io_u_init,

	.event		= xnvme_fioe_event,
	.getevents	= xnvme_fioe_getevents,
	.queue		= xnvme_fioe_queue,

	.close_file	= xnvme_fioe_close,
	.open_file	= xnvme_fioe_open,
};
