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
 *
 * CAVEAT -- Multi-device support
 *
 * Currently multiple devices is *not* supported. However, things are
 * progressing. The current caveats follow below.
 *
 * - 1) The implementation assumes that 'thread_data.o.nr_files' is available
 *   and that instances of 'fio_file.fileno' are valued [0,
 *   thread_data.o.nr_files -1].
 *   This is to pre-allocate file-wrapping-structures, xnvme_fioe_fwrap, at I/O
 *   engine initialization time and to reference file-wrapping with
 *   constant-time lookup
 *
 * - 2) iomem_{alloc/free} introduces a limitation with regards to multiple
 *   devices. Specifically, the devices opened must use backends which share
 *   memory allocators. E.g. using be:laio + be:liou is fine, using be:liou +
 *   be:spdk is not.
 *   This is due to the fio 'io_memem_*' helpers are not tied to devices, as
 *   such, it is required that all devices opened use compatible
 *   buffer-allocators. Currently, the implementation does dot check for this
 *   unsupported use-case, and will thus lead to a runtime error.
 *
 * - 3) The _open() and _close() functions do not implement the "real"
 *   device/file opening, this is done in _init() and torn down in _cleanup() as
 *   the io-engine needs device handles ready for iomem_{alloc/free}
 *
 * - 4) Submitting and reaping completions. This part is simply not implemented,
 *   and is thus the main limitation. Proper handling of multiple devices needs
 *   to be added to _event() and _getevents().
 *
 * CAVEAT -- Supporting NVMe devices formatted with extended-LBA
 *
 * To support extended-lba initial work has been done in xNVMe, however, further
 * work is probably need for this to trickle up from the fio I/O engine
 */
#include <stdlib.h>
#include <assert.h>
#include <fio.h>
#include <optgroup.h>
#include <libxnvme.h>

struct xnvme_fioe_fwrap {
	///< fio file representation
	struct fio_file *fio_file;

	///< xNVMe device handle
	struct xnvme_dev *dev;

	///< I/O completion queue
	struct io_u **iocq;

	///< # of iocq entries; incremented via getevents()/cb_pool()
	uint64_t completed;

	///< # of errors; incremented when observed on completion via getevents()/cb_pool()
	uint64_t ecount;

	struct xnvme_async_ctx *ctx;
	struct xnvme_req_pool *reqs;

	uint32_t ssw;

	uint32_t lba_nbytes;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_fioe_fwrap) == 64, "Incorrect size")

struct xnvme_fioe_data {
	uint64_t cur;
	uint64_t nopen;
	uint64_t nallocated;
	struct xnvme_fioe_fwrap files[];
};

struct xnvme_fioe_options {
	void *padding;
	unsigned int hipri;
	unsigned int sqpoll_thread;
	char *be;
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
		.name   = "be",
		.lname  = "xNVMe Backend",
		.type   = FIO_OPT_STR_STORE,
		.off1   = offsetof(struct xnvme_fioe_options, be),
		.help   = "Default backend when none is provided e.g. /dev/nvme0n1",
		.category = FIO_OPT_C_ENGINE,
		.group  = FIO_OPT_G_NBD,
	},
	{
		.name   = NULL,
	},
};

static void
cb_pool(struct xnvme_req *req, void *cb_arg)
{
	struct io_u *io_u = cb_arg;
	struct xnvme_fioe_data *xd = io_u->engine_data;
	struct xnvme_fioe_fwrap *fwrap = &xd->files[io_u->file->fileno];

	if (xnvme_req_cpl_status(req)) {
		xnvme_req_pr(req, XNVME_PR_DEF);
		fwrap->ecount += 1;
		io_u->error = EIO;
	}

	fwrap->iocq[fwrap->completed++] = io_u;
	SLIST_INSERT_HEAD(&req->pool->head, req, link);
}

#ifdef XNVME_DEBUG_ENABLED
static void
_fio_file_pr(struct fio_file *f)
{
	log_info("fio_file: { ");
	log_info("file_name: '%s', ", f->file_name);
	log_info("fileno: %d, ", f->fileno);
	log_info("io_size: %zu, ", f->io_size);
	log_info("real_file_size: %zu, ", f->real_file_size);
	log_info("file_offset: %zu", f->file_offset);
	log_info("}\n");
}
#endif

static int
_dev_close(struct thread_data *td, struct xnvme_fioe_fwrap *fwrap)
{
	if (fwrap->dev) {
		xnvme_async_term(fwrap->dev, fwrap->ctx);
	}
	free(fwrap->iocq);
	xnvme_req_pool_free(fwrap->reqs);
	xnvme_dev_close(fwrap->dev);

	memset(fwrap, 0, sizeof(*fwrap));

	return 0;
}

static void
xnvme_fioe_cleanup(struct thread_data *td)
{
	struct xnvme_fioe_data *xd = td->io_ops_data;

	for (uint64_t i = 0; i < xd->nallocated; ++i) {
		int err;

		err = _dev_close(td, &xd->files[i]);
		if (err) {
			XNVME_DEBUG("xnvme_fioe: cleanup(): Unexpected error");
		}
	}

	free(xd);
	td->io_ops_data = NULL;
}

static int
_dev_open(struct thread_data *td, struct fio_file *f)
{
	struct xnvme_fioe_options *o = td->eo;
	struct xnvme_fioe_data *xd = td->io_ops_data;
	struct xnvme_fioe_fwrap *fwrap;
	const struct xnvme_geo *geo;
	char dev_uri[XNVME_IDENT_URI_LEN] = { 0 };
	int fn_has_scheme;
	int flags = 0;

	XNVME_DEBUG("o->be: '%s'", o->be);

	if (o->be && (strlen(o->be) > XNVME_IDENT_SCHM_LEN)) {
		log_err("xnvme_fioe: invalid --be=%s\n", o->be);
		return 1;
	}
	if (o->hipri) {
		flags |= XNVME_ASYNC_IOPOLL;
	}
	if (o->sqpoll_thread) {
		flags |= XNVME_ASYNC_SQPOLL;
	}
	if (f->fileno > (int)xd->nallocated) {
		log_err("xnvme_fioe: _dev_open(); invalid assumption\n");
		return 1;
	}

	{
		char schm[XNVME_IDENT_SCHM_LEN];
		char sep[1];
		int matches;

		matches = sscanf(f->file_name, "%4[a-z]%1[:]", schm, sep) == 2;
		fn_has_scheme = matches == 2;
	}

	if (o->be && (!fn_has_scheme)) {
		snprintf(dev_uri, XNVME_IDENT_URI_LEN - 2, "%s:", o->be);
	}
	strncat(dev_uri, f->file_name, XNVME_IDENT_URI_LEN - strlen(dev_uri));

	XNVME_DEBUG("INFO: dev_uri: '%s'", dev_uri);

	fwrap = &xd->files[f->fileno];
	fwrap->dev = xnvme_dev_open(dev_uri);
	if (!fwrap->dev) {
		log_err("xnvme_fioe: init(): {uri: '%s', err: '%s'}\n",
			dev_uri, strerror(errno));
		return 1;
	}
	geo = xnvme_dev_get_geo(fwrap->dev);

	fwrap->iocq = malloc(td->o.iodepth * sizeof(struct io_u *));
	memset(fwrap->iocq, 0, td->o.iodepth * sizeof(struct io_u *));

	fwrap->completed = 0;
	fwrap->ecount = 0;

	if (xnvme_async_init(fwrap->dev, &(fwrap->ctx), td->o.iodepth, flags)) {
		log_err("xnvme_fioe: init(): failed xnvme_async_init()\n");
		return 1;
	}
	if (xnvme_req_pool_alloc(&fwrap->reqs, td->o.iodepth + 1)) {
		log_err("xnvme_fioe: init(): xnvme_req_pool_alloc()\n");
		return 1;
	}
	// NOTE: cb_args are assigned in _queue()
	if (xnvme_req_pool_init(fwrap->reqs, fwrap->ctx, cb_pool, NULL)) {
		log_err("xnvme_fioe: init(): xnvme_req_pool_init()\n");
		return 1;
	}

	fwrap->ssw = xnvme_dev_get_ssw(fwrap->dev);
	fwrap->lba_nbytes = geo->lba_nbytes;

	fwrap->fio_file = f;
	fwrap->fio_file->filetype = FIO_TYPE_BLOCK;
	fwrap->fio_file->real_file_size = geo->tbytes;
	fio_file_set_size_known(fwrap->fio_file);

	return 0;
}

static int
xnvme_fioe_init(struct thread_data *td)
{
	struct xnvme_fioe_data *xd = NULL;
	struct fio_file *f;
	unsigned int i;

	log_info("xnvme_fioe: init(): td->io_ops: %p\n", td->io_ops);

	if (!td->o.use_thread) {
		log_err("xnvme_fioe: init(): --thread=1 is required\n");
		return 1;
	}
	if (!td->io_ops) {
		log_err("xnvme_fioe: init(): !td->io_ops\n");
		log_err("xnvme_fioe: init(): Check fio version\n");
		log_err("xnvme_fioe: init(): I/O engine built against: '%s'\n",
			fio_version_string);
		return 1;
	}

	// Allocate and zero-fill xd
	xd = malloc(sizeof(*xd) + sizeof(*xd->files) * td->o.nr_files);
	if (!xd) {
		log_err("xnvme_fioe: init(): !malloc()\n");
		return 1;
	}
	memset(xd, 0, sizeof(*xd) + sizeof(*xd->files) * td->o.nr_files);
	td->io_ops_data = xd;

	for_each_file(td, f, i) {
		if (_dev_open(td, f)) {
			log_err("xnvme_fioe: init(): _dev_open(%s)\n",
				f->file_name);
			return 1;
		}

		++(xd->nallocated);
	}

	if (xd->nallocated != td->o.nr_files) {
		log_err("xnvme_fioe: init(): nallocated != td->o.nr_files\n");
		return 1;
	}

	return 0;
}

// NOTE: using the first device for buffer-allocators, see CAVEAT 2)
static int
xnvme_fioe_iomem_alloc(struct thread_data *td, size_t total_mem)
{
	struct xnvme_fioe_data *xd = td->io_ops_data;
	struct xnvme_fioe_fwrap *fwrap = &xd->files[0];

	if (!fwrap->dev) {
		log_err("xnvme_fioe: failed iomem_alloc(); no dev-handle\n");
		return 1;
	}

	td->orig_buffer = xnvme_buf_alloc(fwrap->dev, total_mem, NULL);

	return td->orig_buffer == NULL;
}

// NOTE: using the first device for buffer-allocators, see CAVEAT 2)
static void
xnvme_fioe_iomem_free(struct thread_data *td)
{
	struct xnvme_fioe_data *xd = td->io_ops_data;
	struct xnvme_fioe_fwrap *fwrap = &xd->files[0];

	if (!fwrap->dev) {
		log_err("xnvme_fioe: failed iomem_free(); no dev-handle\n");
		return;
	}

	xnvme_buf_free(fwrap->dev, td->orig_buffer);
}

static int
xnvme_fioe_io_u_init(struct thread_data *td, struct io_u *io_u)
{
	io_u->engine_data = td->io_ops_data;

	return 0;
}

static void
xnvme_fioe_io_u_free(struct thread_data *td, struct io_u *io_u)
{
	io_u->engine_data = NULL;
}

static struct io_u *
xnvme_fioe_event(struct thread_data *td, int event)
{
	struct xnvme_fioe_data *xd = td->io_ops_data;
	struct xnvme_fioe_fwrap *fwrap = &xd->files[xd->cur];

	assert(event >= 0);
	assert((unsigned)event < fwrap->completed);

	return fwrap->iocq[event];
}

static int
xnvme_fioe_getevents(struct thread_data *td, unsigned int min,
		     unsigned int max, const struct timespec *t)
{
	struct xnvme_fioe_data *xd = td->io_ops_data;
	struct xnvme_fioe_fwrap *fwrap;
	int err;

	fwrap = &xd->files[xd->cur];
	fwrap->completed = 0;

	if (t) {
		assert(false);
	}

	do {
		err = xnvme_async_poke(fwrap->dev, fwrap->ctx,
				       max - fwrap->completed);
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
	} while (fwrap->completed < min);

	return fwrap->completed;
}

static enum fio_q_status
xnvme_fioe_queue(struct thread_data *td, struct io_u *io_u)
{
	struct xnvme_fioe_data *xd = td->io_ops_data;
	struct xnvme_fioe_fwrap *fwrap;
	struct xnvme_req *req;
	uint32_t nsid;
	uint64_t slba;
	uint16_t nlb;
	int err;

	fio_ro_check(td, io_u);

	fwrap = &xd->files[io_u->file->fileno];
	nsid = xnvme_dev_get_nsid(fwrap->dev);

	slba = io_u->offset >> fwrap->ssw;
	nlb = (io_u->xfer_buflen / fwrap->lba_nbytes) - 1;

	if (td->io_ops->flags & FIO_SYNCIO) {
		log_err("xnvme_fioe: queue(): Got sync...\n");
		assert(false);
		return FIO_Q_COMPLETED;
	}

	req = SLIST_FIRST(&fwrap->reqs->head);
	SLIST_REMOVE_HEAD(&fwrap->reqs->head, link);

	req->async.cb_arg = io_u;

	switch (io_u->ddir) {
	case DDIR_READ:
		err = xnvme_cmd_read(fwrap->dev, nsid, slba, nlb,
				     io_u->xfer_buf, NULL, XNVME_CMD_ASYNC,
				     req);
		break;

	case DDIR_WRITE:
		err = xnvme_cmd_write(fwrap->dev, nsid, slba, nlb,
				      io_u->xfer_buf, NULL, XNVME_CMD_ASYNC,
				      req);
		break;

	default:
		log_err("xnvme_fioe: queue(): ENOSYS: %u\n", io_u->ddir);
		err = -1;
		assert(false);
		break;
	}

	switch (err) {
	case 0:
		return FIO_Q_QUEUED;

	case -EBUSY:
	case -EAGAIN:
		SLIST_INSERT_HEAD(&req->pool->head, req, link);
		return FIO_Q_BUSY;

	default:
		log_err("xnvme_fioe: queue(): err: '%d'\n", err);

		SLIST_INSERT_HEAD(&req->pool->head, req, link);
		io_u->error = abs(err);
		assert(false);
		return FIO_Q_COMPLETED;
	}
}

// See CAVEAT for explanation and _cleanup() + _dev_close() for implementation
static int
xnvme_fioe_close(struct thread_data *td, struct fio_file *f)
{
	struct xnvme_fioe_data *xd = td->io_ops_data;

	XNVME_DEBUG_FCALL(_fio_file_pr(f);)

	--(xd->nopen);

	return 0;
}

// See CAVEAT for explanation and _init() + _dev_open() for implementation
static int
xnvme_fioe_open(struct thread_data *td, struct fio_file *f)
{
	struct xnvme_fioe_data *xd = td->io_ops_data;

	XNVME_DEBUG_FCALL(_fio_file_pr(f);)

	// Prevent multiple devices / files
	if (xd->nopen) {
		log_err("xnvme_fioe: multiple devices are not supported\n");
		XNVME_DEBUG("xd->nopen > 0");
		return 1;
	}
	if (f->fileno) {
		log_err("xnvme_fioe: expected: f->fileno == 0\n");
		return 1;
	}

	if (f->fileno > (int)xd->nallocated) {
		XNVME_DEBUG("f->fileno > xd->nallocated; invalid assumption");
		return 1;
	}
	if (xd->files[f->fileno].fio_file != f) {
		XNVME_DEBUG("well... that is off..");
		return 1;
	}

	++(xd->nopen);

	return 0;
}

struct ioengine_ops ioengine = {
	.name			= "xnvme",
	.version		= FIO_IOOPS_VERSION,
	.options		= options,
	.option_struct_size	= sizeof(struct xnvme_fioe_options),
	.flags			= \
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
