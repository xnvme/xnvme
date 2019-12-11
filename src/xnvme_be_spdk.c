// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#define XNVME_BE_SPDK_BID 0x59DA
#define XNVME_BE_SPDK_NAME "SPDK"

int
xnvme_be_spdk_ident_from_uri(const char *uri, struct xnvme_ident *ident)
{
	uint32_t domain, bus, slot, fn, nsid;
	char prefix[XNVME_IDENT_URI_LEN] = { 0 };
	int matches;

	if (strlen(uri) > XNVME_IDENT_URI_LEN) {
		errno = EINVAL;
		return -1;
	}
	if (strlen(uri) < 7) {
		errno = EINVAL;
		return -1;
	}

	matches = sscanf(uri, "%7[pciespdk:/]%4x:%2x:%2x.%1x/%x", prefix,
			 &domain, &bus, &slot, &fn, &nsid);
	if (!((strncmp(prefix, "pci://", 6) == 0) || \
	      (strncmp(prefix, "pcie://", 7) == 0) || \
	      (strncmp(prefix, "spdk://", 7) == 0))) {
		errno = EINVAL;
		return -1;
	}

	strncpy(ident->uri, uri, XNVME_IDENT_URI_LEN);
	ident->bid = XNVME_BE_SPDK_BID;

	switch (matches) {
	case 6:
		ident->type = XNVME_IDENT_NS;
		ident->nsid = nsid;
		ident->nst = XNVME_SPEC_NSTYPE_NOCHECK;
		snprintf(ident->be_uri, XNVME_IDENT_URI_LEN,
			 "traddr:%04x:%02x:%02x:%1x", domain, bus, slot, fn);
		return 0;

	case 5:
		ident->type = XNVME_IDENT_CTRLR;
		snprintf(ident->be_uri, XNVME_IDENT_URI_LEN,
			 "traddr:%04x:%02x:%02x:%1x", domain, bus, slot, fn);
		return 0;
	}

	errno = EINVAL;
	return -1;
}

#ifndef XNVME_BE_SPDK_ENABLED
struct xnvme_be xnvme_be_spdk = {
	.bid = XNVME_BE_SPDK_BID,
	.name = XNVME_BE_SPDK_NAME,

	.ident_from_uri = xnvme_be_spdk_ident_from_uri,

	.enumerate = xnvme_be_nosys_enumerate,

	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,

	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,

	.async_init = xnvme_be_nosys_async_init,
	.async_term = xnvme_be_nosys_async_term,
	.async_poke = xnvme_be_nosys_async_poke,
	.async_wait = xnvme_be_nosys_async_wait,

	.cmd_pass = xnvme_be_nosys_cmd_pass,
	.cmd_pass_admin = xnvme_be_nosys_cmd_pass_admin,
};
#else
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

static int _do_spdk_env_init = 1;

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
	case 0:
		XNVME_STDOUT_FD = dup(STDOUT_FILENO);

		fflush(stdout);
		XNVME_STDOUT_FNULL = fopen("/dev/null", "w");
		dup2(fileno(XNVME_STDOUT_FNULL), STDOUT_FILENO);
		break;
	case 1:
		dup2(XNVME_STDOUT_FD, STDOUT_FILENO);
		close(XNVME_STDOUT_FD);
		break;
	}
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
		errno = EIO;
		return -1;
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
probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
	 struct spdk_nvme_ctrlr_opts *opts)
{
	struct xnvme_be_spdk *state = cb_ctx;

	if (spdk_nvme_transport_id_compare(&state->trid, trid)) {
		XNVME_DEBUG("trid->traddr: %s != state->trid.traddr: %s",
			    trid->traddr, state->trid.traddr);
		return false;
	}

	/* Disable CMB sqs / cqs for now due to shared PMR / CMB */
	opts->use_cmb_sqs = false;

	/* Enable all I/O command-sets */
	opts->command_set = 0x6;

	return !state->attached;
}

/**
 * Sets up the state{ns, nsid, ctrlr, attached} given via the cb_ctx
 * using the first available name-space.
 */
static void
attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *XNVME_UNUSED(trid),
	  struct spdk_nvme_ctrlr *ctrlr,
	  const struct spdk_nvme_ctrlr_opts *XNVME_UNUSED(opts))
{
	struct xnvme_be_spdk *state = cb_ctx;
	struct spdk_nvme_ns *ns = NULL;

	ns = spdk_nvme_ctrlr_get_ns(ctrlr, state->ident.nsid);
	if (ns == NULL) {
		XNVME_DEBUG("FAILED: ctrlr_get_ns(0x%x)", state->ident.nsid);
		return;
	}
	if (!spdk_nvme_ns_is_active(ns)) {
		XNVME_DEBUG("FAILED: nsid: 0x%x, !active", state->ident.nsid);
		return;
	}

	state->ns = ns;
	state->ctrlr = ctrlr;
	state->attached = 1;
}

void
xnvme_be_spdk_state_term(struct xnvme_be_spdk *state)
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

	free(state);
}

void
xnvme_be_spdk_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	xnvme_be_spdk_state_term((struct xnvme_be_spdk *)dev->be);
	dev->be = NULL;
}

/**
 * Enumerates NVMe devices as seen by SPDK and grabs the first matching 'ident'
 * - Allocates 'xnvme_be_spdk' and assigns function interface
 * - Attaches to a single controller matching 'ident'
 * - Associates first available namespace
 * - Copies namespace data
 * - Creates an IO qpair for SYNC commands and allocates associated lock
 */
struct xnvme_be_spdk *
xnvme_be_spdk_state_init(const struct xnvme_ident *ident,
			 int XNVME_UNUSED(opts))
{
	const struct spdk_nvme_ctrlr_data *ctrlr_data;
	const struct spdk_nvme_ns_data *ns_data;
	struct xnvme_be_spdk *state;
	int err;

	state = malloc(sizeof(*state));
	if (!state) {
		XNVME_DEBUG("FAILED: malloc(spdk_be_state)");
		return NULL;
	}
	memset(state, 0, sizeof(*state));

	state->be = xnvme_be_spdk;
	state->ident = *ident;

	state->opts.name = "libxnvme";
	state->opts.shm_id = 0;
	state->opts.master_core = 0;

	if (_do_spdk_env_init) {
		// SPDK and DPDK are very chatty, this makes them less so
		//spdk_log_set_print_level(SPDK_LOG_ERROR);
		//rte_log_set_global_level(RTE_LOG_EMERG);

		xnvme_stdout_choker(0);

		/*
		 * SPDK relies on an abstraction around the local environment
		 * named env that handles memory allocation and PCI device
		 * operations.  This library must be initialized first.
		 */
		spdk_env_opts_init(&(state->opts));
		err = spdk_env_init(&(state->opts));

		xnvme_stdout_choker(1);

		if (err) {
			XNVME_DEBUG("FAILED: spdk_env_init");
			xnvme_be_spdk_state_term(state);
			return NULL;
		}

		_do_spdk_env_init = 0;
	}

	/*
	 * Parse 'be_uri' into transport_id so we can use it to compare to the
	 * probed controller
	 */
	state->trid.trtype = SPDK_NVME_TRANSPORT_PCIE;

	err = spdk_nvme_transport_id_parse(&state->trid, ident->be_uri);
	if (err) {
		XNVME_DEBUG("FAILED: *_id_parse be_uri(%s), err: %d",
			    ident->be_uri, err);
		errno = -err;
		xnvme_be_spdk_state_term(state);
		return NULL;
	}

	/*
	 * Start the SPDK NVMe enumeration process.
	 *
	 * probe_cb will be called for each NVMe controller found, giving our
	 * application a choice on whether to attach to each controller.
	 *
	 * attach_cb will then be called for each controller after the SPDK NVMe
	 * driver has completed initializing the controller we chose to attach.
	 */
	for (int i = 0; !state->attached; ++i) {
		if (XNVME_BE_SPDK_MAX_PROBE_ATTEMPTS == i) {
			XNVME_DEBUG("FAILED: max attempts exceeded");
			xnvme_be_spdk_state_term(state);
			return NULL;
		}

		err = spdk_nvme_probe(&state->trid, state, probe_cb, attach_cb,
				      NULL);
		if ((err) || (!state->attached)) {
			XNVME_DEBUG("FAILED: spdk_nvme_probe, a:%d, e:%d, i:%d",
				    state->attached, err, i);
		}
	}

	// Copy controller information

	ctrlr_data = spdk_nvme_ctrlr_get_data(state->ctrlr);
	if (ctrlr_data == NULL) {
		XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_get_data");
		xnvme_be_spdk_state_term(state);
		return NULL;
	}
	state->ctrlr_data = *ctrlr_data;

	// Copy namespace information
	ns_data = spdk_nvme_ns_get_data(state->ns);
	if (ns_data == NULL) {
		XNVME_DEBUG("FAILED: spdk_nvme_ns_get_data");
		xnvme_be_spdk_state_term(state);
		return NULL;
	}
	state->ns_data = *ns_data;

	// Setup NVMe IO qpair for SYNC commands
	state->qpair = spdk_nvme_ctrlr_alloc_io_qpair(state->ctrlr, NULL, 0);
	if (!state->qpair) {
		XNVME_DEBUG("FAILED: allocating qpair");
		xnvme_be_spdk_state_term(state);
		return NULL;
	}

	// Setup IO qpair lock for SYNC commands
	err = pthread_mutex_init(&state->qpair_lock, NULL);
	if (err) {
		XNVME_DEBUG("FAILED: pthread_mutex_init(): '%s'",
			    strerror(err));
		xnvme_be_spdk_state_term(state);
		return NULL;
	}

	return state;
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
	copts->use_cmb_sqs = false;

	/* Enable all I/O command-sets */
	copts->command_set = 0x6;

	return 1;
}

/**
 * Checks whether the given 'nsid' is of 'nst'
 *
 * returns When given 'nsid' is of 'nst', then 0 is returned.
 */
static inline
int
__ns_is_nst(struct spdk_nvme_ctrlr *ctrlr, uint32_t nsid,
	    enum xnvme_spec_nstype nst)
{
	struct xnvme_spec_idfy_ns *idfy = NULL;
	struct xnvme_spec_cmd cmd = { 0 };
	struct xnvme_req req = { 0 };

	idfy = spdk_dma_malloc(sizeof(*idfy), 0x1000, NULL);
	if (!idfy) {
		XNVME_DEBUG("FAILED: spdk_dma_malloc()");
		return -1;
	}

	cmd.common.opcode = XNVME_SPEC_OPC_IDFY;
	cmd.common.nsid = nsid;
	cmd.idfy.cns = XNVME_SPEC_IDFY_NS;
	cmd.idfy.nst = nst;
	//cmd.idfy.cntid = cntid;
	//cmd.idfy.nvmsetid = nvmsetid;
	//cmd.idfy.uuid = uuid;

	if (cmd_admin_submit(ctrlr, &cmd, idfy, sizeof(*idfy), 0x0, 0, 0x0,
			     cmd_sync_cb, &req)) {
		XNVME_DEBUG("FAILED: cmd_admin_submit");
		spdk_free(idfy);
		return -1;
	}
	while (!req.async.cb_arg) {	// Wait for completion-indicator
		spdk_nvme_ctrlr_process_admin_completions(ctrlr);
	}
	req.async.cb_arg = NULL;

	if (xnvme_req_cpl_status(&req)) {
		XNVME_DEBUG("FAILED: xnvme_req_cpl_status()");
		errno = EIO;
		return -1;
	}

	spdk_free(idfy);

	return 0;
}

static void
enumerate_attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
		    struct spdk_nvme_ctrlr *ctrlr,
		    const struct spdk_nvme_ctrlr_opts *XNVME_UNUSED(copts))
{
	struct xnvme_enumeration *list = cb_ctx;
	const int num_ns = spdk_nvme_ctrlr_get_num_ns(ctrlr);

	// TODO: add the controller to the enumeration

	for (int nsid = 1; nsid <= num_ns; nsid++) {
		struct spdk_nvme_ns *ns = NULL;
		const struct spdk_nvme_ctrlr_data *ctrlr_data;
		const struct spdk_nvme_ns_data *ns_data;
		struct xnvme_ident entry = { 0 };
		int err;

		ctrlr_data = spdk_nvme_ctrlr_get_data(ctrlr);
		if (ctrlr_data == NULL) {
			XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_get_data");
			continue;
		}

		ns = spdk_nvme_ctrlr_get_ns(ctrlr, nsid);
		if (ns == NULL) {
			XNVME_DEBUG("FAILED: skipping invalid nsid: %d", nsid);
			continue;
		}
		if (!spdk_nvme_ns_is_active(ns)) {
			XNVME_DEBUG("FAILED: skipping inactive nsid: %d", nsid);
			continue;
		}
		ns_data = spdk_nvme_ns_get_data(ns);
		if (ns_data == NULL) {
			XNVME_DEBUG("FAILED: spdk_nvme_ns_get_data");
			continue;
		}

		entry.type = XNVME_IDENT_NS;
		snprintf(entry.uri, sizeof(entry.uri),
			 "pci://%s/%d", trid->traddr, nsid);
		entry.bid = XNVME_BE_SPDK_BID;
		entry.nsid = nsid;

		err = __ns_is_nst(ctrlr, nsid, XNVME_SPEC_NSTYPE_LBLK);
		if (!err) {
			entry.nst = XNVME_SPEC_NSTYPE_LBLK;
			if (xnvme_enumeration_append(list, &entry)) {
				XNVME_DEBUG("FAILED: adding entry");
			}
			continue;
		}

		err = __ns_is_nst(ctrlr, nsid, XNVME_SPEC_NSTYPE_ZONED);
		if (!err) {
			entry.nst = XNVME_SPEC_NSTYPE_ZONED;
			if (xnvme_enumeration_append(list, &entry)) {
				XNVME_DEBUG("FAILED: adding entry");
			}
			continue;
		}
	}

	if (spdk_nvme_detach(ctrlr)) {
		XNVME_DEBUG("FAILED: spdk_nvme_detach");
	}
}

int
xnvme_be_spdk_enumerate(struct xnvme_enumeration *list, int XNVME_UNUSED(opts))
{
	struct spdk_nvme_transport_id trid = {
		.trtype = SPDK_NVME_TRANSPORT_PCIE
	};
	int err;

	/*
	 * SPDK relies on an abstraction around the local environment
	 * named env that handles memory allocation and PCI device
	 * operations. This library must be initialized first.
	 */
	if (_do_spdk_env_init) {
		struct spdk_env_opts eopts;

		memset(&eopts, 0, sizeof(eopts));
		eopts.name = "libxnvme-enumerate";
		eopts.shm_id = 0;
		eopts.master_core = 0;

		xnvme_stdout_choker(0);		// Choke stdout ON

		// SPDK and DPDK are very chatty, this makes them less so
		//spdk_log_set_print_level(SPDK_LOG_ERROR);
		//rte_log_set_global_level(RTE_LOG_EMERG);

		// Initialize environment
		spdk_env_opts_init(&eopts);
		err = spdk_env_init(&eopts);

		xnvme_stdout_choker(1);		// Choke stdout OFF

		if (err) {
			XNVME_DEBUG("FAILED: spdk_env_init");
			return -1;
		}

		_do_spdk_env_init = 0;
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
	err = spdk_nvme_probe(&trid, list, enumerate_probe_cb,
			      enumerate_attach_cb,
			      NULL);
	if (err) {
		XNVME_DEBUG("FAILED: spdk_nvme_probe, err: %d", err);
	}

	return 0;
}

struct xnvme_dev *
xnvme_be_spdk_dev_open(const struct xnvme_ident *ident, int opts)
{
	struct xnvme_be_spdk *state = NULL;
	struct xnvme_dev *dev = NULL;

	dev = malloc(sizeof(*dev));
	if (!dev) {
		XNVME_DEBUG("FAILED: malloc(xnvme_dev)");
		return NULL;
	}
	memset(dev, 0, sizeof(*dev));
	dev->ident = *ident;

	state = xnvme_be_spdk_state_init(ident, opts);
	if (!state) {
		XNVME_DEBUG("FAILED: xnvme_be_spdk_state_init");
		free(dev);
		return NULL;
	}

	if (sizeof(dev->ns) != sizeof(state->ns_data)) {
		XNVME_DEBUG("FAILED: invalid internal representation");
		goto failed;
	}
	if (sizeof(dev->ctrlr) != sizeof(state->ctrlr_data)) {
		XNVME_DEBUG("FAILED: invalid internal representation");
		goto failed;
	}

	dev->be = (struct xnvme_be *)state;

	memcpy(&dev->ns, &state->ns_data, sizeof(dev->ns));
	memcpy(&dev->ctrlr, &state->ctrlr_data, sizeof(dev->ctrlr));

	xnvme_be_dev_derive_geometry(dev);

	return dev;

failed:
	XNVME_DEBUG("FAILED: xnvme_be_spdk_dev_open");
	xnvme_be_spdk_dev_close(dev);
	free(dev);
	return NULL;
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
struct xnvme_async_ctx *
xnvme_be_spdk_async_init(struct xnvme_dev *dev, uint32_t depth,
			 uint16_t XNVME_UNUSED(flags))
{
	struct xnvme_be_spdk *be = (struct xnvme_be_spdk *)(dev->be);
	struct spdk_nvme_io_qpair_opts qpair_opts = { 0 };
	struct xnvme_async_ctx *ctx = NULL;

	spdk_nvme_ctrlr_get_default_io_qpair_opts(be->ctrlr, &qpair_opts,
			sizeof(qpair_opts));

	if (depth) {
		qpair_opts.io_queue_size = depth;
		qpair_opts.io_queue_requests = depth * 2;
	}

	ctx = calloc(1, sizeof(*ctx));
	if (!ctx) {
		XNVME_DEBUG("FAILED: calloc, ctx: %p, errno: %s",
			    (void *)ctx, strerror(errno));
		// Propagate errno
		return NULL;
	}

	ctx->depth = qpair_opts.io_queue_size;

	ctx->be_ctx = spdk_nvme_ctrlr_alloc_io_qpair(be->ctrlr, &qpair_opts,
			sizeof(qpair_opts));
	if (!ctx->be_ctx) {
		XNVME_DEBUG("FAILED: alloc. qpair");
		free(ctx);
		errno = ENOMEM;
		return NULL;
	}

	return ctx;
}

int
xnvme_be_spdk_async_term(struct xnvme_dev *XNVME_UNUSED(dev),
			 struct xnvme_async_ctx *ctx)
{
	if (!ctx) {
		XNVME_DEBUG("FAILED: ctx: %p", (void *)ctx);
		errno = EINVAL;
		return -1;
	}

	if (!ctx->be_ctx) {
		XNVME_DEBUG("FAILED: be_ctx: %p", (void *)ctx->be_ctx);
		errno = EINVAL;
		return -1;
	}

	{
		struct spdk_nvme_qpair *qpair = ctx->be_ctx;
		int err = spdk_nvme_ctrlr_free_io_qpair(qpair);
		if (err) {
			XNVME_DEBUG("FAILED: free qpair: %p, errno: %s",
				    (void *)qpair, strerror(errno));
			errno = EIO;
			return -1;
		}

		free(ctx);
	}

	return 0;
}

int
xnvme_be_spdk_async_poke(struct xnvme_dev *XNVME_UNUSED(dev),
			 struct xnvme_async_ctx *ctx, uint32_t max)
{
	struct spdk_nvme_qpair *qpair = ctx->be_ctx;
	int32_t res;

	res = spdk_nvme_qpair_process_completions(qpair, max);
	if (res < 0) {
		XNVME_DEBUG("FAILED: processing completions: res: %d", res);
		return -1;
	}

	return res;
}

int
xnvme_be_spdk_async_wait(struct xnvme_dev *dev, struct xnvme_async_ctx *ctx)
{
	int acc = 0;

	while (ctx->outstanding) {
		int res;

		res = xnvme_be_spdk_async_poke(dev, ctx, 0);
		if (res < 0) {
			XNVME_DEBUG("FAILED: xnvme_be_spdk_async_poke");
			return -1;
		}

		acc += res;
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
	struct xnvme_be_spdk *state = (struct xnvme_be_spdk *)dev->be;
	struct xnvme_req req_local = { 0 };

	if (!req) {	// Ensure that a req is available
		req = &req_local;
	}
	// req.async.cb_arg is used as completion-indicator
	if (req->async.cb_arg) {
		XNVME_DEBUG("FAILED: sync.cmd may not provide async.cb_arg");
		errno = EINVAL;
		return -1;
	}

	if (cmd_admin_submit(state->ctrlr, cmd, dbuf, dbuf_nbytes, mbuf,
			     mbuf_nbytes, opts, cmd_sync_cb, req)) {
		XNVME_DEBUG("FAILED: cmd_admin_submit");
		errno = EIO;
		return -1;
	}

	while (!req->async.cb_arg) {	// Wait for completion-indicator
		spdk_nvme_ctrlr_process_admin_completions(state->ctrlr);
	}
	req->async.cb_arg = NULL;

	// check for errors
	if (xnvme_req_cpl_status(req)) {
		XNVME_DEBUG("FAILED: xnvme_req_cpl_status()");
		errno = EIO;
		return -1;
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
	struct xnvme_be_spdk *state = (struct xnvme_be_spdk *)dev->be;
	struct spdk_nvme_qpair *qpair = req->async.ctx->be_ctx;
	int err = 0;

	// TODO: do something with mbuf?

	// Early exit when queue is full
	if ((req->async.ctx->outstanding + 1) > req->async.ctx->depth) {
		XNVME_DEBUG("FAILED: queue is full");
		errno = EAGAIN;
		return -1;
	}

	req->async.ctx->outstanding += 1;
	err = submit_ioc(state->ctrlr, qpair, cmd, dbuf, dbuf_nbytes, mbuf,
			 cmd_async_cb, req);
	if (err) {
		req->async.ctx->outstanding -= 1;
		XNVME_DEBUG("FAILED: submission failed");
		errno = EIO;
		return -1;
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
	struct xnvme_be_spdk *state = (struct xnvme_be_spdk *)dev->be;
	struct spdk_nvme_qpair *qpair = state->qpair;
	pthread_mutex_t *qpair_lock = &state->qpair_lock;
	struct xnvme_req req_local = { 0 };

	int err = 0;

	if (!req) {			// Ensure that a req is available
		req = &req_local;
	}
	if (req->async.cb_arg) {	// It is used as completion-indicator
		XNVME_DEBUG("FAILED: sync.cmd may not provide async.cb_arg");
		errno = EINVAL;
		return -1;
	}

	pthread_mutex_lock(qpair_lock);
	err = submit_ioc(state->ctrlr, qpair, cmd, dbuf, dbuf_nbytes,
			 mbuf, cmd_sync_cb, req);
	pthread_mutex_unlock(qpair_lock);
	if (err) {
		XNVME_DEBUG("FAILED: submit_ioc(), err: %d", err);
		errno = EIO;
		return -1;
	}

	while (!req->async.cb_arg) {
		pthread_mutex_lock(qpair_lock);
		spdk_nvme_qpair_process_completions(qpair, 0);
		pthread_mutex_unlock(qpair_lock);
	}
	req->async.cb_arg = NULL;

	if (xnvme_req_cpl_status(req)) {
		XNVME_DEBUG("FAILED: xnvme_req_cpl_status()");
		errno = EIO;
		return -1;
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
		if ((!req) || (!req->async.ctx) || (!req->async.ctx->be_ctx)) {
			XNVME_DEBUG("FAILED: req: %p", (void *)req);
			errno = EINVAL;
			return -1;
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

struct xnvme_be xnvme_be_spdk = {
	.bid = XNVME_BE_SPDK_BID,
	.name = XNVME_BE_SPDK_NAME,

	.ident_from_uri = xnvme_be_spdk_ident_from_uri,

	.enumerate = xnvme_be_spdk_enumerate,

	.dev_open = xnvme_be_spdk_dev_open,
	.dev_close = xnvme_be_spdk_dev_close,

	.buf_alloc = xnvme_be_spdk_buf_alloc,
	.buf_realloc = xnvme_be_spdk_buf_realloc,
	.buf_free = xnvme_be_spdk_buf_free,
	.buf_vtophys = xnvme_be_spdk_buf_vtophys,

	.async_init = xnvme_be_spdk_async_init,
	.async_term = xnvme_be_spdk_async_term,
	.async_poke = xnvme_be_spdk_async_poke,
	.async_wait = xnvme_be_spdk_async_wait,

	.cmd_pass = xnvme_be_spdk_cmd_pass,
	.cmd_pass_admin = xnvme_be_spdk_cmd_pass_admin,
};

#endif
