// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#define XNVME_BE_SPDK_NAME "spdk"

#ifdef XNVME_BE_SPDK_ENABLED
#include <sys/types.h>
#include <unistd.h>
#include <rte_log.h>
#include <spdk/log.h>

#include <xnvme_async.h>
#include <xnvme_be.h>
#include <xnvme_be_spdk.h>
#include <xnvme_dev.h>
#include <xnvme_sgl.h>
#include <libxnvme_spec.h>

#define XNVME_BE_SPDK_MAX_PROBE_ATTEMPTS 1

static int g_xnvme_be_spdk_env_is_initialized = 0;

static void
cmd_sync_cb(void *cb_arg, const struct spdk_nvme_cpl *cpl);

static inline int
cmd_admin_submit(struct spdk_nvme_ctrlr *ctrlr, struct xnvme_spec_cmd *cmd,
		 void *dbuf, uint32_t dbuf_nbytes, void *mbuf,
		 uint32_t mbuf_nbytes, int opts, spdk_nvme_cmd_cb cb_fn,
		 void *cb_arg);

/**
 * Choke stdout, this is used to choke the output from spdk_env_init
 */
static void
xnvme_stdout_choker(int action)
{
	static int XNVME_STDOUT_FD = 0;
	static FILE *XNVME_STDOUT_FNULL = NULL;

	switch (action) {
	case 1:
		XNVME_STDOUT_FD = dup(STDOUT_FILENO);

		fflush(stdout);
		XNVME_STDOUT_FNULL = fopen("/dev/null", "w");
		dup2(fileno(XNVME_STDOUT_FNULL), STDOUT_FILENO);
		break;
	case 0:
		dup2(XNVME_STDOUT_FD, STDOUT_FILENO);
		close(XNVME_STDOUT_FD);
		break;
	}
}

static void
xnvme_stderr_choker(int action)
{
	static int XNVME_STDERR_FD = 0;
	static FILE *XNVME_STDERR_FNULL = NULL;

	switch (action) {
	case 1:
		XNVME_STDERR_FD = dup(STDERR_FILENO);

		fflush(stderr);
		XNVME_STDERR_FNULL = fopen("/dev/null", "w");
		dup2(fileno(XNVME_STDERR_FNULL), STDERR_FILENO);
		break;
	case 0:
		dup2(XNVME_STDERR_FD, STDERR_FILENO);
		close(XNVME_STDERR_FD);
		break;
	}
}

static void
xnvme_spdk_choker_on(void)
{
#ifdef XNVME_BE_DEBUG_ENABLED
	spdk_log_set_print_level(SPDK_LOG_DEBUG);
	rte_log_set_global_level(RTE_LOG_DEBUG);
	return;
#endif

	xnvme_stdout_choker(1);
	xnvme_stderr_choker(1);

	spdk_log_set_print_level(SPDK_LOG_DISABLED);
	rte_log_set_global_level(RTE_LOG_EMERG);
}

static void
xnvme_spdk_choker_off(void)
{
#ifdef XNVME_BE_DEBUG_ENABLED
	return;
#endif

	xnvme_stdout_choker(0);
	xnvme_stderr_choker(0);
}

static inline int
_spdk_env_init(struct spdk_env_opts *opts)
{
	struct spdk_env_opts _spdk_opts = { 0 };
	int err;

	if (g_xnvme_be_spdk_env_is_initialized) {
		XNVME_DEBUG("INFO: spdk_env already initialized; skipping...");
		return 0;
	}

	xnvme_spdk_choker_on();

	if (!opts) {
		opts = &_spdk_opts;

		spdk_env_opts_init(opts);
		opts->name = "xnvme";
		opts->shm_id = 0;
	}

	err = spdk_env_init(opts);
	if (err < 0) {
		XNVME_DEBUG("FAILED: spdk_env_init(), err: %d", err);
	}

	g_xnvme_be_spdk_env_is_initialized = err == 0;

	xnvme_spdk_choker_off();

	return err;
}

void *
xnvme_be_spdk_buf_alloc(const struct xnvme_dev *dev, size_t nbytes,
			uint64_t *phys)
{
	const size_t alignment = dev->geo.nbytes;

	return spdk_dma_malloc(nbytes, alignment, phys);
}

void *
xnvme_be_spdk_buf_realloc(const struct xnvme_dev *dev, void *buf, size_t nbytes,
			  uint64_t *phys)
{
	const size_t alignment = dev->geo.nbytes;

	return spdk_dma_realloc(buf, nbytes, alignment, phys);
}

void
xnvme_be_spdk_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	spdk_dma_free(buf);
}

int
xnvme_be_spdk_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf,
			  uint64_t *phys)
{
	*phys = spdk_vtophys(buf, NULL);
	if (SPDK_VTOPHYS_ERROR == *phys) {
		return -EIO;
	}
	return 0;
}

static inline int
submit_ioc(struct spdk_nvme_ctrlr *ctrlr, struct spdk_nvme_qpair *qpair,
	   struct xnvme_spec_cmd *cmd, void *dbuf, uint32_t dbuf_nbytes,
	   void *mbuf, spdk_nvme_cmd_cb cb_fn, void *cb_arg)
{
#ifdef XNVME_TRACE_ENABLED
	XNVME_DEBUG("Dumping IO command");
	xnvme_spec_cmd_pr(cmd, XNVME_PR_DEF);
#endif

	return spdk_nvme_ctrlr_cmd_io_raw_with_md(ctrlr, qpair,
			(struct spdk_nvme_cmd *)cmd,
			dbuf, dbuf_nbytes, mbuf,
			cb_fn, cb_arg);
}

/**
 * Attach to the device matching the traddr and only if we have not yet attached
 */
static bool
probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid_probed,
	 struct spdk_nvme_ctrlr_opts *opts)
{
	struct xnvme_dev *dev = cb_ctx;
	struct xnvme_be_spdk_state *state = (void *)dev->be.state;
	struct spdk_nvme_transport_id trid_given = {
		.trtype = SPDK_NVME_TRANSPORT_PCIE
	};
	int err;

	if (state->attached) {
		XNVME_DEBUG("Already attached");
		return false;
	}

	{
		char trid[XNVME_IDENT_URI_LEN] = { 0 };

		sprintf(trid, "traddr:%s", dev->ident.trgt);

		/*
		 * Parse 'trgt' into trid so we can use it to compare to the
		 * probed controller
		 */
		err = spdk_nvme_transport_id_parse(&trid_given, trid);
		if (err) {
			XNVME_DEBUG("FAILED: parsing trid from trgt(%s), err: %d",
				    dev->ident.trgt, err);
			return false;
		}
	}

	if (spdk_nvme_transport_id_compare(trid_probed, &trid_given)) {
		XNVME_DEBUG("INFO: skipping trid_given->traddr: %s != "
			    "trid_probed->traddr: %s",
			    trid_given.traddr, trid_probed->traddr);
		return false;
	}

	/* Disable CMB sqs / cqs for now due to shared PMR / CMB */
	opts->use_cmb_sqs = false;

	/* Enable all I/O command-sets */
	// TODO: KNOWN ISSUE
	//opts->command_set = 0x6;

	return true;
}

/**
 * Sets up the state{ns, ctrlr, attached} given via the cb_ctx
 * using the first available name-space.
 */
static void
attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *XNVME_UNUSED(trid),
	  struct spdk_nvme_ctrlr *ctrlr,
	  const struct spdk_nvme_ctrlr_opts *XNVME_UNUSED(opts))
{
	struct xnvme_dev *dev = cb_ctx;
	struct xnvme_be_spdk_state *state = (void *)dev->be.state;
	struct spdk_nvme_ns *ns = NULL;

	ns = spdk_nvme_ctrlr_get_ns(ctrlr, dev->nsid);
	if (!ns) {
		XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_get_ns(0x%x)", dev->nsid);
		return;
	}
	if (!spdk_nvme_ns_is_active(ns)) {
		XNVME_DEBUG("FAILED: !spdk_nvme_ns_is_active(nsid:0x%x)",
			    dev->nsid);
		return;
	}

	state->ns = ns;
	state->ctrlr = ctrlr;
	state->attached = 1;
}

void
xnvme_be_spdk_state_term(struct xnvme_be_spdk_state *state)
{
	if (!state) {
		return;
	}
	if (state->qpair) {
		int err;

		spdk_nvme_ctrlr_free_io_qpair(state->qpair);
		err = pthread_mutex_destroy(&state->qpair_lock);
		if (err) {
			printf("UNHANDLED: pthread_mutex_destroy(): '%s'\n",
			       strerror(err));
		}
	}
	if (state->ctrlr) {
		spdk_nvme_detach(state->ctrlr);
	}
}

void
xnvme_be_spdk_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	xnvme_be_spdk_state_term((void *)dev->be.state);
	memset(&dev->be, 0, sizeof(dev->be));
}

/**
 * Always attach such that `enumerate_attach_cb` can grab namespaces
 *
 * And disable CMB sqs / cqs for now due to shared PMR / CMB
 */
static bool
enumerate_probe_cb(void *XNVME_UNUSED(cb_ctx),
		   const struct spdk_nvme_transport_id *XNVME_UNUSED(trid),
		   struct spdk_nvme_ctrlr_opts *copts)
{
	XNVME_DEBUG("INFO: hello hello!? probne!?");

	copts->use_cmb_sqs = false;

	/* Enable all I/O command-sets */
	// TODO: KNOWN ISSUE
	//copts->command_set = 0x6;

	return 1;
}

static void
enumerate_attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
		    struct spdk_nvme_ctrlr *ctrlr,
		    const struct spdk_nvme_ctrlr_opts *XNVME_UNUSED(copts))
{
	struct xnvme_enumeration *list = cb_ctx;
	const int num_ns = spdk_nvme_ctrlr_get_num_ns(ctrlr);

	XNVME_DEBUG("hello hello");

	for (int nsid = 1; nsid <= num_ns; ++nsid) {
		struct spdk_nvme_ns *ns;

		if (!spdk_nvme_ctrlr_get_data(ctrlr)) {
			XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_get_data");
			continue;
		}

		ns = spdk_nvme_ctrlr_get_ns(ctrlr, nsid);
		if (!ns) {
			XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_get_ns(nsid:%d)",
				    nsid);
			continue;
		}
		if (!spdk_nvme_ns_is_active(ns)) {
			XNVME_DEBUG("FAILED: skipping inactive nsid: %d", nsid);
			continue;
		}
		if (!spdk_nvme_ns_get_data(ns)) {
			XNVME_DEBUG("FAILED: spdk_nvme_ns_get_data()");
			continue;
		}

		// Namespace looks good, add it to the enumeration
		{
			char uri[XNVME_IDENT_URI_LEN] = { 0 };
			struct xnvme_ident ident = { 0 };

			snprintf(uri, sizeof(uri), "pci:%s?nsid=%d",
				 trid->traddr, nsid);
			xnvme_ident_from_uri(uri, &ident);

			if (xnvme_enumeration_append(list, &ident)) {
				XNVME_DEBUG("FAILED: adding ident");
			}
		}
	}

	if (spdk_nvme_detach(ctrlr)) {
		XNVME_DEBUG("FAILED: spdk_nvme_detach");
	}
}

//
// TODO: enumerate fabrics
//
int
xnvme_be_spdk_enumerate(struct xnvme_enumeration *list, int XNVME_UNUSED(opts))
{
	int err;

	XNVME_DEBUG("INFO: hello hello!?");

	if (_spdk_env_init(NULL)) {
		XNVME_DEBUG("FAILED: _spdk_env_init()");
		return -1;
	}

	/*
	 * Start the SPDK NVMe enumeration process
	 *
	 * enumerate_probe_cb will be called for each NVMe controller found,
	 * giving our application a choice on whether to attach to each
	 * controller
	 *
	 * enumerate_attach_cb will then be called for each controller after the
	 * SPDK NVMe driver has completed initializing the controller we chose
	 * to attach
	 */
	err = spdk_nvme_probe(NULL, list, enumerate_probe_cb,
			      enumerate_attach_cb,
			      NULL);
	if (err) {
		XNVME_DEBUG("FAILED: spdk_nvme_probe, err: %d", err);
	}

	return 0;
}

/**
 * Determine the following:
 *
 * - Determine device-type	(setup: dev->dtype)
 *
 * - Identify controller	(setup: dev->ctrlr)
 *
 * - Identify namespace		(setup: dev->ns)
 * - Identify namespace-id	(setup: dev->nsid)
 *
 * TODO: fixup for XNVME_DEV_TYPE_NVME_CONTROLLER
 */
int
xnvme_be_spdk_dev_idfy(struct xnvme_dev *dev)
{
	struct xnvme_be_spdk_state *state = (void *)dev->be.state;
	const struct spdk_nvme_ctrlr_data *ctrlr_data;
	const struct spdk_nvme_ns_data *ns_data;

	dev->dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;

	ctrlr_data = spdk_nvme_ctrlr_get_data(state->ctrlr);
	if (!ctrlr_data) {
		XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_get_data()");
		return -EINVAL;
	}
	memcpy(&dev->ctrlr, ctrlr_data, sizeof(dev->ctrlr));

	ns_data = spdk_nvme_ns_get_data(state->ns);
	if (!ns_data) {
		XNVME_DEBUG("FAILED: spdk_nvme_ns_get_data()");
		return -EINVAL;
	}
	memcpy(&dev->ns, ns_data, sizeof(dev->ns));

	return 0;
}

/**
 * - Intialize SPDK environment
 * - Attach to controller matching dev->ident
 * - create sync-io-qpair
 * - create lock protecting sync-io-qpair
 */
int
xnvme_be_spdk_state_init(struct xnvme_dev *dev)
{
	struct xnvme_be_spdk_state *state = (void *)dev->be.state;
	struct spdk_nvme_transport_id trid = {
		.trtype = SPDK_NVME_TRANSPORT_PCIE
	};
	int err;

	err = _spdk_env_init(NULL);
	if (err) {
		XNVME_DEBUG("FAILED: _spdk_env_init(), err: %d", err);
		return err;
	}

	/*
	 * Start the SPDK NVMe enumeration process.
	 *
	 * probe_cb will be called for each NVMe controller found,
	 * giving our application a choice on whether to attach to each
	 * controller.
	 *
	 * attach_cb will then be called for each controller after the
	 * SPDK NVMe driver has completed initializing the controller we
	 * chose to attach.
	 */
	for (int i = 0; !state->attached; ++i) {

		if (XNVME_BE_SPDK_MAX_PROBE_ATTEMPTS == i) {
			XNVME_DEBUG("FAILED: max attempts exceeded");
			return -ENXIO;
		}

		err = spdk_nvme_probe(&trid, dev, probe_cb, attach_cb, NULL);
		if ((err) || (!state->attached)) {
			XNVME_DEBUG("FAILED: probe a:%d, e:%d, i:%d",
				    state->attached, err, i);
		}
	}

	// Setup IO qpair lock for SYNC commands
	err = pthread_mutex_init(&state->qpair_lock, NULL);
	if (err) {
		XNVME_DEBUG("FAILED: pthread_mutex_init(): '%s'",
			    strerror(err));
		return -err;
	}

	// Setup IO qpair for SYNC commands
	state->qpair = spdk_nvme_ctrlr_alloc_io_qpair(state->ctrlr, NULL, 0);
	if (!state->qpair) {
		XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_alloc_io_qpair()");
		return -ENOMEM;
	}

	return 0;
}

int
xnvme_be_spdk_dev_from_ident(const struct xnvme_ident *ident,
			     struct xnvme_dev **dev)
{
	int err;

	err = xnvme_dev_alloc(dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_dev_alloc()");
		return err;
	}
	(*dev)->ident = *ident;
	(*dev)->be = xnvme_be_spdk;

	{
		uint32_t nsid;

		if (!xnvme_ident_opt_to_val(&(*dev)->ident, "nsid", &nsid)) {
			XNVME_DEBUG("xnvme_ident_opt_to_val(opt:nsid)");
			return -EINVAL;
		}

		(*dev)->nsid = nsid;
	}

	err = xnvme_be_spdk_state_init(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_spdk_state_init()");
		free(*dev);
		return err;
	}
	err = xnvme_be_spdk_dev_idfy(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_spdk_dev_idfy()");
		xnvme_be_spdk_state_term((void *)(*dev)->be.state);
		free(*dev);
		return err;
	}
	err = xnvme_be_dev_derive_geometry(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_dev_derive_geometry()");
		xnvme_be_spdk_state_term((void *)(*dev)->be.state);
		free(*dev);
		return err;
	}

	return err;
}

/**
 * Context for asynchronous command submission and completion
 *
 * The context is not thread-safe and the intent is that the user must
 * initialize the opaque #xnvme_async_ctx via xnvme_async_init() pr. thread,
 * which is then delegated to the backend, in this case XNVME_BE_SPDK, which
 * then initialized a struct containing what it needs for a submission /
 * completion path, in the case of XNVME_BE_SPDK, then a qpair is needed and
 * thus allocated and de-allocated by:
 *
 * The XNVME_BE_SPDK specific context is a SPDK qpair and it is carried inside:
 *
 * xnvme_async_ctx->be_ctx
 */
int
xnvme_be_spdk_async_init(struct xnvme_dev *dev, struct xnvme_async_ctx **ctx,
			 uint16_t depth, int XNVME_UNUSED(flags))
{
	struct xnvme_be_spdk_state *state = (void *)dev->be.state;
	struct xnvme_async_ctx_spdk *sctx = NULL;

	(*ctx) = calloc(1, sizeof(**ctx));
	if (!(*ctx)) {
		XNVME_DEBUG("FAILED: calloc, ctx: %p, errno: %s",
			    (void *)(*ctx), strerror(errno));
		return -errno;
	}

	(*ctx)->depth = depth;
	sctx = (void *)(*ctx);

	sctx->qpair = spdk_nvme_ctrlr_alloc_io_qpair(state->ctrlr, NULL, 0);
	if (!sctx->qpair) {
		XNVME_DEBUG("FAILED: alloc. qpair");
		free((*ctx));
		return -ENOMEM;
	}

	return 0;
}

int
xnvme_be_spdk_async_term(struct xnvme_async_ctx *ctx)
{
	struct xnvme_async_ctx_spdk *sctx = (void *)ctx;
	int err;

	if (!ctx) {
		XNVME_DEBUG("FAILED: ctx: %p", (void *)ctx);
		return -EINVAL;
	}
	if (!sctx->qpair) {
		XNVME_DEBUG("FAILED: !sctx->qpair");
		return -EINVAL;
	}

	err = spdk_nvme_ctrlr_free_io_qpair(sctx->qpair);
	if (err) {
		XNVME_DEBUG("FAILED: free qpair: %p, errno: %s",
			    (void *)sctx->qpair, strerror(errno));
		return err;
	}

	free(ctx);

	return err;
}

int
xnvme_be_spdk_async_poke(struct xnvme_async_ctx *ctx, uint32_t max)
{
	struct xnvme_async_ctx_spdk *sctx = (void *)ctx;
	int err;

	if (!sctx->outstanding) {
		return 0;
	}

	err = spdk_nvme_qpair_process_completions(sctx->qpair, max);
	if (err < 0) {
		XNVME_DEBUG("FAILED: spdk_nvme_qpair_process_completion(), "
			    "err: %d", err);
	}

	return err;
}

int
xnvme_be_spdk_async_wait(struct xnvme_async_ctx *ctx)
{
	int acc = 0;

	while (ctx->outstanding) {
		int err;

		err = xnvme_be_spdk_async_poke(ctx, 0);
		if (err < 0) {
			XNVME_DEBUG("FAILED: xnvme_be_spdk_async_poke");
			return err;
		}

		acc += err;
	}

	return acc;
}

static void
cmd_sync_cb(void *cb_arg, const struct spdk_nvme_cpl *cpl)
{
	struct xnvme_req *req = cb_arg;

	req->cpl = *(const struct xnvme_spec_cpl *)cpl;
	req->async.cb_arg = (void *)cb_arg;	// Assign completion-indicator
}

// TODO: consider whether 'opts' should be used for anything here...
static inline int
cmd_admin_submit(struct spdk_nvme_ctrlr *ctrlr, struct xnvme_spec_cmd *cmd,
		 void *dbuf, uint32_t dbuf_nbytes, void *mbuf,
		 uint32_t mbuf_nbytes, int XNVME_UNUSED(opts),
		 spdk_nvme_cmd_cb cb_fn, void *cb_arg)
{

	cmd->common.mptr = (uint64_t)mbuf ? (uint64_t)mbuf : cmd->common.mptr;

	if (mbuf_nbytes && mbuf) {
		cmd->common.ndm = mbuf_nbytes / 4;
	}

#ifdef XNVME_TRACE_ENABLED
	XNVME_DEBUG("Dumping ADMIN command");
	xnvme_spec_cmd_pr(cmd, 0x0);
#endif

	return spdk_nvme_ctrlr_cmd_admin_raw(ctrlr, (struct spdk_nvme_cmd *)cmd,
					     dbuf, dbuf_nbytes, cb_fn, cb_arg);
}

static inline int
cmd_admin(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd, void *dbuf,
	  size_t dbuf_nbytes, void *mbuf, size_t mbuf_nbytes, int opts,
	  struct xnvme_req *req)
{
	struct xnvme_be_spdk_state *state = (void *)dev->be.state;
	struct xnvme_req req_local = { 0 };

	if (!req) {	// Ensure that a req is available
		req = &req_local;
	}
	// req.async.cb_arg is used as completion-indicator
	if (req->async.cb_arg) {
		XNVME_DEBUG("FAILED: sync.cmd may not provide async.cb_arg");
		return -EINVAL;
	}

	if (cmd_admin_submit(state->ctrlr, cmd, dbuf, dbuf_nbytes, mbuf,
			     mbuf_nbytes, opts, cmd_sync_cb, req)) {
		XNVME_DEBUG("FAILED: cmd_admin_submit");
		return -EIO;
	}

	while (!req->async.cb_arg) {	// Wait for completion-indicator
		spdk_nvme_ctrlr_process_admin_completions(state->ctrlr);
	}
	req->async.cb_arg = NULL;

	// check for errors
	if (xnvme_req_cpl_status(req)) {
		XNVME_DEBUG("FAILED: xnvme_req_cpl_status()");
		return -EIO;
	}

	return 0;
}

static void
cmd_async_cb(void *cb_arg, const struct spdk_nvme_cpl *cpl)
{
	struct xnvme_req *req = cb_arg;

	req->async.ctx->outstanding -= 1;
	req->cpl = *(const struct xnvme_spec_cpl *)cpl;
	req->async.cb(req, req->async.cb_arg);
}

// TODO: consider whether 'mbuf_nbytes' is needed here
// TODO: consider whether 'opts' is needed here
static inline int
cmd_async_pass(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd, void *dbuf,
	       size_t dbuf_nbytes, void *mbuf, size_t XNVME_UNUSED(mbuf_nbytes),
	       int XNVME_UNUSED(opts), struct xnvme_req *req)
{
	struct xnvme_be_spdk_state *state = (void *)dev->be.state;
	struct xnvme_async_ctx_spdk *sctx = (void *)req->async.ctx;
	int err;

	// TODO: do something with mbuf?

	// Early exit when queue is full
	if (sctx->outstanding == sctx->depth) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}

	sctx->outstanding += 1;
	err = submit_ioc(state->ctrlr, sctx->qpair, cmd, dbuf, dbuf_nbytes, mbuf,
			 cmd_async_cb, req);
	if (err) {
		sctx->outstanding -= 1;
		XNVME_DEBUG("FAILED: submission failed");
		return err;
	}

	return 0;
}

// TODO: consider whether 'mbuf_nbytes' is needed here
// TODO: consider whether 'opts' is needed here
static inline int
cmd_sync_pass(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd, void *dbuf,
	      size_t dbuf_nbytes, void *mbuf, size_t XNVME_UNUSED(mbuf_nbytes),
	      int XNVME_UNUSED(opts), struct xnvme_req *req)
{
	struct xnvme_be_spdk_state *state = (void *)dev->be.state;
	struct spdk_nvme_qpair *qpair = state->qpair;
	pthread_mutex_t *qpair_lock = &state->qpair_lock;
	struct xnvme_req req_local = { 0 };

	int err = 0;

	if (!req) {			// Ensure that a req is available
		req = &req_local;
	}
	if (req->async.cb_arg) {	// It is used as completion-indicator
		XNVME_DEBUG("FAILED: sync.cmd may not provide async.cb_arg");
		return -EINVAL;
	}

	pthread_mutex_lock(qpair_lock);
	err = submit_ioc(state->ctrlr, qpair, cmd, dbuf, dbuf_nbytes,
			 mbuf, cmd_sync_cb, req);
	pthread_mutex_unlock(qpair_lock);
	if (err) {
		XNVME_DEBUG("FAILED: submit_ioc(), err: %d", err);
		return err;
	}

	while (!req->async.cb_arg) {
		pthread_mutex_lock(qpair_lock);
		spdk_nvme_qpair_process_completions(qpair, 0);
		pthread_mutex_unlock(qpair_lock);
	}
	req->async.cb_arg = NULL;

	if (xnvme_req_cpl_status(req)) {
		XNVME_DEBUG("FAILED: xnvme_req_cpl_status()");
		return -EIO;
	}

	return 0;
}

int
xnvme_be_spdk_cmd_pass(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
		       void *dbuf, size_t dbuf_nbytes, void *mbuf,
		       size_t mbuf_nbytes, int opts, struct xnvme_req *req)
{
	const int cmd_opts = opts & XNVME_CMD_MASK;

	switch (cmd_opts & XNVME_CMD_MASK_IOMD) {
	case XNVME_CMD_ASYNC:
		if ((!req) || (!req->async.ctx)) {
			XNVME_DEBUG("FAILED: req: %p", (void *)req);
			return -EINVAL;
		}
		return cmd_async_pass(dev, cmd, dbuf, dbuf_nbytes, mbuf,
				      mbuf_nbytes, cmd_opts, req);

	case XNVME_CMD_SYNC:
	default:
		return cmd_sync_pass(dev, cmd, dbuf, dbuf_nbytes, mbuf,
				     mbuf_nbytes, cmd_opts, req);
	}
}

int
xnvme_be_spdk_cmd_pass_admin(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
			     void *dbuf, size_t dbuf_nbytes, void *mbuf,
			     size_t mbuf_nbytes, int opts,
			     struct xnvme_req *req)
{
	const int cmd_opts = opts & XNVME_CMD_MASK;

	return cmd_admin(dev, cmd, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes,
			 cmd_opts, req);
}
#endif

static const char *g_schemes[] = {
	"pci",
	XNVME_BE_SPDK_NAME,
	"pcie",
};

struct xnvme_be xnvme_be_spdk = {
#ifdef XNVME_BE_SPDK_ENABLED
	.func = {
		.cmd_pass = xnvme_be_spdk_cmd_pass,
		.cmd_pass_admin = xnvme_be_spdk_cmd_pass_admin,

		.async_init = xnvme_be_spdk_async_init,
		.async_term = xnvme_be_spdk_async_term,
		.async_poke = xnvme_be_spdk_async_poke,
		.async_wait = xnvme_be_spdk_async_wait,

		.buf_alloc = xnvme_be_spdk_buf_alloc,
		.buf_vtophys = xnvme_be_spdk_buf_vtophys,
		.buf_realloc = xnvme_be_spdk_buf_realloc,
		.buf_free = xnvme_be_spdk_buf_free,

		.enumerate = xnvme_be_spdk_enumerate,

		.dev_from_ident = xnvme_be_spdk_dev_from_ident,
		.dev_close = xnvme_be_spdk_dev_close,
	},
#else
	.func = XNVME_BE_NOSYS_FUNC,
#endif
	.attr = {
		.name = XNVME_BE_SPDK_NAME,
#ifdef XNVME_BE_SPDK_ENABLED
		.enabled = 1,
#else
		.enabled = 0,
#endif
		.schemes = g_schemes,
		.nschemes = sizeof g_schemes / sizeof(*g_schemes),
	},
	.state = { 0 },
};
