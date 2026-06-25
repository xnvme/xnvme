// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_SPDK_ENABLED
#include <errno.h>

#include <sys/types.h>
#include <unistd.h>
#include <rte_log.h>
#include <spdk/log.h>
#include <spdk/nvme_spec.h>

#include <xnvme_be.h>
#include <xnvme_queue.h>
#include <xnvme_be_spdk.h>
#include <xnvme_be_registry.h>
#include <xnvme_dev.h>

#define XNVME_BE_SPDK_MAX_PROBE_ATTEMPTS 1
#define XNVME_BE_SPDK_AVLB_TRANSPORTS 3

static int g_xnvme_be_spdk_transport[] = {
#ifdef XNVME_BE_SPDK_TRANSPORT_PCIE_ENABLED
	SPDK_NVME_TRANSPORT_PCIE,
#endif
#ifdef XNVME_BE_SPDK_TRANSPORT_TCP_ENABLED
	SPDK_NVME_TRANSPORT_TCP,
#endif
#ifdef XNVME_BE_SPDK_TRANSPORT_RDMA_ENABLED
	SPDK_NVME_TRANSPORT_RDMA,
#endif
#ifdef XNVME_BE_SPDK_TRANSPORT_FC_ENABLED
	SPDK_NVME_TRANSPORT_FC,
#endif
};
static int g_xnvme_be_spdk_ntransport =
	sizeof g_xnvme_be_spdk_transport / sizeof *g_xnvme_be_spdk_transport;

static int g_xnvme_be_spdk_env_is_initialized = 0;

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
#ifdef XNVME_DEBUG_ENABLED
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
#ifdef XNVME_DEBUG_ENABLED
	return;
#endif

	xnvme_stdout_choker(0);
	xnvme_stderr_choker(0);
}

/**
 * Initialize an spdk_env_opts the way SPDK currently expects callers to.
 *
 * Starting with SPDK v26.05, spdk_env_opts_init() preserves the caller's
 * opts_size rather than setting it to sizeof(*opts); a zero value caps the
 * effective struct to the smallest historical boundary and silently drops
 * any newer fields. The size stamp is part of SPDK's forward-compat scheme:
 * callers declare how much of the struct they understand so the library
 * does not write past the end. Wrap the size-stamp + init pair so both
 * call sites state it once and consistently.
 */
static inline void
xnvme_spdk_env_opts_init(struct spdk_env_opts *opts)
{
	opts->opts_size = sizeof(*opts);
	spdk_env_opts_init(opts);
}

static inline int
_spdk_env_init(struct spdk_env_opts *opts)
{
	struct spdk_env_opts _spdk_opts = {0};
	int err;

	if (g_xnvme_be_spdk_env_is_initialized) {
		XNVME_DEBUG("INFO: already initialized");
		err = 0;
		goto exit;
	}

	xnvme_spdk_choker_on();

	if (!opts) {
		opts = &_spdk_opts;

		xnvme_spdk_env_opts_init(opts);
		opts->name = "xnvme";
		opts->shm_id = 0;
	}

	err = spdk_env_init(opts);
	if (err < 0) {
		XNVME_DEBUG("FAILED: spdk_env_init(), err: %d", err);
	}

	g_xnvme_be_spdk_env_is_initialized = err == 0;

	xnvme_spdk_choker_off();

exit:
	XNVME_DEBUG("INFO: spdk_env_is_initialized: %d", g_xnvme_be_spdk_env_is_initialized);

	return err;
}

const char *
_spdk_nvmf_adrfam_str(enum spdk_nvmf_adrfam adrfam)
{
	switch (adrfam) {
	case SPDK_NVMF_ADRFAM_IPV4:
		return "IPv4";
	case SPDK_NVMF_ADRFAM_IPV6:
		return "IPv6";
	case SPDK_NVMF_ADRFAM_IB:
	case SPDK_NVMF_ADRFAM_FC:
	case SPDK_NVMF_ADRFAM_INTRA_HOST:
	case SPDK_NVMF_ADRFAM_NOT_SPECIFIED:
		return "";
	}

	return "";
}

/**
 * Construct a SPDK transport identifier from a xNVMe identifier
 *
 * A trtype must be supplied as xNVMe does not distinguish between TCP, RDMA,
 * etc. it only has the 'fab' prefix for Fabrics transports.
 *
 * TODO:
 * - construct trid for FC
 */
static inline int
_xnvme_be_spdk_ident_to_trid(const struct xnvme_ident *ident, struct xnvme_opts *opts,
			     struct spdk_nvme_transport_id *trid, int trtype)
{
	char trid_str[1024] = {0}; // TODO: fix this size
	char addr[SPDK_NVMF_TRADDR_MAX_LEN + 1] = {0};
	int port = 0;
	int err;

	memset(trid, 0, sizeof(*trid));

	switch (trtype) {
	case SPDK_NVME_TRANSPORT_PCIE:
		if (strnlen(ident->uri, XNVME_IDENT_URI_LEN)) {
			sprintf(trid_str, "trtype:%s traddr:%s",
				spdk_nvme_transport_id_trtype_str(trtype), ident->uri);
		} else {
			sprintf(trid_str, "trtype:%s", spdk_nvme_transport_id_trtype_str(trtype));
		}

		break;

	case SPDK_NVME_TRANSPORT_TCP:
	case SPDK_NVME_TRANSPORT_RDMA:
		if (sscanf(ident->uri, "%[^:]:%d", addr, &port) != 2) {
			XNVME_DEBUG("FAILED: uri: %s, trtype: %s", ident->uri,
				    spdk_nvme_transport_id_trtype_str(trtype));
			return -EINVAL;
		}
		snprintf(trid_str, sizeof(trid_str),
			 "trtype:%s adrfam:%s traddr:%s trsvcid:%d subnqn:%s",
			 spdk_nvme_transport_id_trtype_str(trtype),
			 opts->adrfam ? opts->adrfam : "IPv4", addr, port,
			 opts->subnqn ? opts->subnqn : SPDK_NVMF_DISCOVERY_NQN);
		break;

	case SPDK_NVME_TRANSPORT_FC:
	case SPDK_NVME_TRANSPORT_CUSTOM:
		XNVME_DEBUG("FAILED: unsupported trtype: %s",
			    spdk_nvme_transport_id_trtype_str(trtype));
		return -ENOSYS;
	}

	err = spdk_nvme_transport_id_parse(trid, trid_str);
	if (err) {
		XNVME_DEBUG("FAILED: parsing trid_str: %s from uri: %s, err: %d", trid_str,
			    ident->uri, err);
		return err;
	}

	return 0;
}

int
_spdk_nvme_transport_id_pr(const struct spdk_nvme_transport_id *trid)
{
	int wrtn = 0;

	wrtn += printf("trid:\n");
	wrtn += printf("  trstring: '%s'\n", trid->trstring);
	wrtn += printf("  trtype: 0x%x\n", trid->trtype);
	wrtn += printf("  adrfam: 0x%x\n", trid->adrfam);
	wrtn += printf("  traddr: '%s'\n", trid->traddr);
	wrtn += printf("  trsvcid: '%s'\n", trid->trsvcid);
	wrtn += printf("  subnqn: '%s'\n", trid->subnqn);
	wrtn += printf("  priority: 0x%x\n", trid->priority);

	return wrtn;
}

int
_spdk_nvme_transport_id_compare_weak(const struct spdk_nvme_transport_id *trid1,
				     const struct spdk_nvme_transport_id *trid2)
{
	struct spdk_nvme_transport_id one = *trid1;
	struct spdk_nvme_transport_id other = *trid2;

	memset(one.subnqn, 0, sizeof one.subnqn);
	memset(other.subnqn, 0, sizeof one.subnqn);

	return spdk_nvme_transport_id_compare(&one, &other);
}

bool
_spdk_setup_controller_opts(struct xnvme_opts *opts, const struct spdk_nvme_transport_id *trid,
			    struct spdk_nvme_ctrlr_opts *ctrlr_opts)
{
#ifdef XNVME_DEBUG_ENABLED
	_spdk_nvme_transport_id_pr(trid);
#endif

	ctrlr_opts->command_set = opts->css.given ? opts->css.value : SPDK_NVME_CC_CSS_IOCS;

	switch (trid->trtype) {
	case SPDK_NVME_TRANSPORT_PCIE:
		ctrlr_opts->use_cmb_sqs = opts->use_cmb_sqs ? true : false;
		break;

	case SPDK_NVME_TRANSPORT_TCP:
	case SPDK_NVME_TRANSPORT_RDMA:
		ctrlr_opts->header_digest = 1;
		ctrlr_opts->data_digest = 1;
		ctrlr_opts->keep_alive_timeout_ms = opts->keep_alive_timeout_ms;
		if (opts->hostnqn) {
			strncpy(ctrlr_opts->hostnqn, opts->hostnqn, SPDK_NVMF_NQN_MAX_LEN);
		}
		break;

	case SPDK_NVME_TRANSPORT_VFIOUSER:
	case SPDK_NVME_TRANSPORT_FC:
	case SPDK_NVME_TRANSPORT_CUSTOM:
	case SPDK_NVME_TRANSPORT_CUSTOM_FABRICS:
		XNVME_DEBUG("FAILED: unsupported trtype: %d", trid->trtype);
		return false;
	}
	return true;
}

/**
 * Attach to the device matching the device provided by the user, and only when
 * it is not already attached, e.g. attach when:
 *
 * (!attached) && (requested == probed)
 *
 * Also, setup controller-options
 */
static bool
probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *probed,
	 struct spdk_nvme_ctrlr_opts *ctrlr_opts)
{
	struct xnvme_dev *dev = cb_ctx;
	struct xnvme_be_spdk_state *state = (void *)dev->be.state;
	struct spdk_nvme_transport_id req = {0};

	if (state->attached) {
		XNVME_DEBUG("SKIP: Already attached");
		return false;
	}
	if (_xnvme_be_spdk_ident_to_trid(&dev->ident, &dev->opts, &req, probed->trtype)) {
		XNVME_DEBUG("SKIP: !_xnvme_be_spdk_ident_to_trid()");
		return false;
	}
	if (_spdk_nvme_transport_id_compare_weak(probed, &req)) {
		XNVME_DEBUG("SKIP: mismatching trid prbed != ctx");

#ifdef XNVME_DEBUG_ENABLED
		_spdk_nvme_transport_id_pr(probed);
		_spdk_nvme_transport_id_pr(&req);
#endif

		return false;
	}

	return _spdk_setup_controller_opts(&dev->opts, probed, ctrlr_opts);
}

static void
#ifdef XNVME_DEBUG_ENABLED
timeout_cb_func(void *XNVME_UNUSED(cb_arg), struct spdk_nvme_ctrlr *ctrlr,
		struct spdk_nvme_qpair *qpair, uint16_t cid)
#else
timeout_cb_func(void *XNVME_UNUSED(cb_arg), struct spdk_nvme_ctrlr *ctrlr,
		struct spdk_nvme_qpair *XNVME_UNUSED(qpair), uint16_t XNVME_UNUSED(cid))
#endif
{
	XNVME_DEBUG("FAILED: timeout reached cid=%d qpair=%p\n", cid, qpair);
	spdk_nvme_ctrlr_fail(ctrlr);
}

/**
 * Records the attached controller in state so ctrlr_init can return it.
 *
 * When nsid is set, rejects controllers that do not have the requested namespace active.
 * This is needed for NVMe-oF auto-discovery, where SPDK may call attach_cb for both
 * the discovery controller and the actual NVM subsystem controllers; we must skip the
 * discovery controller (which has no data namespaces) and wait for the right one.
 */
static void
attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid, struct spdk_nvme_ctrlr *ctrlr,
	  const struct spdk_nvme_ctrlr_opts *XNVME_UNUSED(ctrlr_opts))
{
	struct xnvme_dev *dev = cb_ctx;
	struct xnvme_opts *opts = &dev->opts;
	struct xnvme_be_spdk_state *state = (void *)dev->be.state;

	if (state->attached) {
		XNVME_DEBUG("SKIP: Already attached");
		spdk_nvme_detach(ctrlr);
		return;
	}

	if (opts->nsid) {
		struct spdk_nvme_ns *ns = spdk_nvme_ctrlr_get_ns(ctrlr, opts->nsid);
		if (!ns || !spdk_nvme_ns_is_active(ns)) {
			XNVME_DEBUG("SKIP: nsid 0x%x not found or inactive on this controller",
				    opts->nsid);
			spdk_nvme_detach(ctrlr);
			return;
		}
	}

	state->ctrlr = ctrlr;
	state->attached = 1;
	opts->spdk_fabrics = trid->trtype > SPDK_NVME_TRANSPORT_PCIE;
}

static int
xnvme_be_spdk_ctrlr_term(void *ctrlr)
{
	int err = spdk_nvme_detach((struct spdk_nvme_ctrlr *)ctrlr);
	if (err) {
		XNVME_DEBUG("FAILED: spdk_nvme_detach()");
	}
	return err;
}

void
xnvme_be_spdk_dev_close(struct xnvme_dev *dev)
{
	struct xnvme_be_spdk_state *state = (void *)dev->be.state;
	spdk_nvme_qp_failure_reason reason;
	int err;

	if (!dev) {
		return;
	}

	if (!state->qpair) {
		return;
	}

	reason = spdk_nvme_qpair_get_failure_reason(state->qpair);
	if (reason) {
		XNVME_DEBUG("WARNING: qpair in failed state, reason: %d", reason);
	} else {
		spdk_nvme_ctrlr_free_io_qpair(state->qpair);
	}

	err = pthread_mutex_destroy(&state->qpair_lock);
	if (err) {
		printf("UNHANDLED: pthread_mutex_destroy(): '%s'\n", strerror(err));
	}
}

static void
identify_cb(void *ctx, const struct spdk_nvme_cpl *cpl)
{
	int *result = (int *)ctx;
	*result = cpl->status.sc;
	if (*result != 0) {
		XNVME_DEBUG("WARNING: bad identify, sct: %d, sc: %d", cpl->status.sct,
			    cpl->status.sc);
	}
}

static int
verify_ctrlr_ok(struct spdk_nvme_ctrlr *ctrlr)
{
	struct spdk_nvme_cmd cmd = {0};
	int cmd_result = -1, buf_size = 4096;
	void *buf;
	int err;

	err = spdk_nvme_ctrlr_is_failed(ctrlr);
	if (err) {
		XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_is_failed()");
		return -EBUSY;
	}

	buf = spdk_dma_malloc(buf_size, 0, NULL);
	if (!buf) {
		XNVME_DEBUG("FAILED: spdk_dma_malloc()");
		return -ENOMEM;
	}
	cmd.opc = SPDK_NVME_OPC_IDENTIFY;
	cmd.cdw10_bits.identify.cns = SPDK_NVME_IDENTIFY_CTRLR;
	err = spdk_nvme_ctrlr_cmd_admin_raw(ctrlr, &cmd, buf, buf_size, identify_cb, &cmd_result);
	if (err) {
		XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_cmd_admin_raw(), err: %d", err);
		goto exit;
	}
	while (cmd_result == -1) {
		err = spdk_nvme_ctrlr_process_admin_completions(ctrlr);
		if (err < 0) {
			XNVME_DEBUG("WARNING: spdk_nvme_ctrlr_process_admin_completions() failed, "
				    "err: %d",
				    err);
			goto exit;
		}
	}

	err = cmd_result;

exit:
	spdk_dma_free(buf);
	return err;
}

/**
 * Initialize a new SPDK controller for the device URI.
 *
 * Inits the SPDK environment, if needed, and probes for a device matching the URI.
 */
static void *
xnvme_be_spdk_ctrlr_init(struct xnvme_dev *dev)
{
	struct xnvme_be_spdk_state *state = (void *)dev->be.state;
	struct spdk_env_opts env_opts = {0};
	int err;

	xnvme_spdk_env_opts_init(&env_opts);

	// SPDK default for shm_id is -1, so don't overwrite unless given
	if (dev->opts.shm_id) {
		XNVME_DEBUG("INFO: multi-process setup");
		env_opts.shm_id = dev->opts.shm_id;
	}

	// SPDK default for core_mask is 0x1, so don't overwrite unless given
	if (dev->opts.core_mask) {
		env_opts.core_mask = dev->opts.core_mask;
	}

	// SPDK default for main_core is -1, so don't overwrite unless given
	if (dev->opts.main_core) {
		env_opts.main_core = dev->opts.main_core;
	}

	/* DPDK's IOVA-mode auto-detection cannot see through nested vfio-pci */
	/* pass-through to the inner guest's DMA mask; setups that hit "IOVA */
	/* exceeding limits of current DMA mask" need iova_mode='pa'. Honour */
	/* xnvme_opts.iova_mode first, fall back to XNVME_SPDK_IOVA_MODE so a */
	/* deployment can flip the mode without touching every caller. */
	{
		const char *iova_mode = dev->opts.iova_mode;
		if (!iova_mode) {
			iova_mode = getenv("XNVME_SPDK_IOVA_MODE");
		}
		if (iova_mode) {
			env_opts.iova_mode = iova_mode;
		}
	}

	err = _spdk_env_init(&env_opts);
	if (err) {
		XNVME_DEBUG("FAILED: _spdk_env_init(), err: %d", err);
		/* The most common cause is DPDK's IOVA-mode auto-detect picking */
		/* VA on a host where the effective DMA mask is too narrow */
		/* (nested vfio-pci pass-through, certain uio_pci_generic */
		/* setups). Point the user at the three xNVMe knobs that flip */
		/* the mode to 'pa', so they don't have to dig through SPDK's */
		/* DPDK output to find the hint. */
		if (!env_opts.iova_mode || strcmp(env_opts.iova_mode, "pa") != 0) {
			fprintf(stderr,
				"xnvme: SPDK environment init failed (err=%d).\n"
				"       If this is a nested vfio-pci pass-through "
				"or a narrow-DMA-mask setup (look for 'IOVA "
				"exceeding limits of current DMA mask' in the "
				"SPDK output), set iova_mode to 'pa'.\n"
				"       Three ways, pick whichever fits the call "
				"path:\n"
				"         opts: xnvme_opts.iova_mode = \"pa\" "
				"before xnvme_dev_open()\n"
				"         cli:  pass --iova-mode pa on the "
				"xnvme command line\n"
				"         env:  export "
				"XNVME_SPDK_IOVA_MODE=pa\n",
				err);
		}
		return NULL;
	}

	for (int i = 0; !state->attached; ++i) {
		if (XNVME_BE_SPDK_MAX_PROBE_ATTEMPTS == i) {
			XNVME_DEBUG("FAILED: max attempts exceeded");
			errno = ENXIO;
			return NULL;
		}
		for (int t = 0; t < g_xnvme_be_spdk_ntransport; ++t) {
			int trtype = g_xnvme_be_spdk_transport[t];
			struct spdk_nvme_transport_id trid = {0};

			XNVME_DEBUG("############################");
			XNVME_DEBUG("INFO: trtype: %s, # %d of %d",
				    spdk_nvme_transport_id_trtype_str(trtype), t + 1,
				    g_xnvme_be_spdk_ntransport);

			if (_xnvme_be_spdk_ident_to_trid(&dev->ident, &dev->opts, &trid, trtype)) {
				XNVME_DEBUG("SKIP/FAILED: ident_to_trid()");
				continue;
			}

			err = spdk_nvme_probe(&trid, dev, probe_cb, attach_cb, NULL);
			if (err || !state->attached) {
				XNVME_DEBUG("FAILED: probe a:%d, e:%d, i:%d", state->attached, err,
					    i);
			}
		}
	}

	XNVME_DEBUG("INFO: ctrlr_init() OK");
	return state->ctrlr;
}

int
xnvme_be_spdk_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_be_spdk_state *state = (void *)dev->be.state;
	int err;

	// ctrlr is at state->ctrlr, placed there by platform (cref reuse or ctrlr_init).
	// On cref reuse, state->attached is 0 (fresh state); verify health before setting up a new
	// qpair. On fresh ctrlr_init, attach_cb sets state->attached=1 and we skip verification.
	if (!state->attached) {
		err = verify_ctrlr_ok(state->ctrlr);
		if (err) {
			XNVME_DEBUG("FAILED: verify_ctrlr_ok(), err: %d", err);
			return -EBUSY;
		}
	}

	dev->ident.dtype =
		dev->opts.nsid ? XNVME_DEV_TYPE_NVME_NAMESPACE : XNVME_DEV_TYPE_NVME_CONTROLLER;
	dev->ident.nsid = dev->opts.nsid;

	if (dev->opts.nsid) {
		struct spdk_nvme_ns *ns = spdk_nvme_ctrlr_get_ns(state->ctrlr, dev->opts.nsid);
		if (!ns) {
			XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_get_ns(0x%x)", dev->opts.nsid);
			return -EBUSY;
		}
		if (!spdk_nvme_ns_is_active(ns)) {
			XNVME_DEBUG("FAILED: !spdk_nvme_ns_is_active(nsid:0x%x)", dev->opts.nsid);
			return -EBUSY;
		}
		state->ns = ns;
		dev->ident.csi = spdk_nvme_ns_get_csi(ns);
	}

	if (dev->opts.command_timeout > 0 && dev->opts.admin_timeout > 0) {
		spdk_nvme_ctrlr_register_timeout_callback(state->ctrlr, dev->opts.command_timeout,
							  dev->opts.admin_timeout, timeout_cb_func,
							  0);
	}

	err = pthread_mutex_init(&state->qpair_lock, NULL);
	if (err) {
		XNVME_DEBUG("FAILED: pthread_mutex_init(): '%s'", strerror(err));
		return -err;
	}

	state->qpair = spdk_nvme_ctrlr_alloc_io_qpair(state->ctrlr, NULL, 0);
	if (!state->qpair) {
		XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_alloc_io_qpair()");
		pthread_mutex_destroy(&state->qpair_lock);
		return -ENOMEM;
	}

	XNVME_DEBUG("INFO: dev_open() OK");
	return 0;
}
#endif

struct xnvme_be_dev g_xnvme_be_spdk_dev = {
#ifdef XNVME_BE_SPDK_ENABLED
	.dev_open = xnvme_be_spdk_dev_open,
	.dev_close = xnvme_be_spdk_dev_close,
	.id = "spdk",
	.ctrlr_init = xnvme_be_spdk_ctrlr_init,
	.ctrlr_term = xnvme_be_spdk_ctrlr_term,
#else
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
	.id = "nosys",
#endif
};
