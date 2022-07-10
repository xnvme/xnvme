#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_SPDK_ENABLED
#include <errno.h>
#include <libxnvme.h>
#include <libxnvme_znd.h>

#include <sys/types.h>
#include <unistd.h>
#include <rte_log.h>
#include <spdk/log.h>
#include <spdk/nvme_spec.h>

#include <xnvme_be.h>
#include <xnvme_queue.h>
#include <xnvme_be_spdk.h>
#include <xnvme_dev.h>
#include <xnvme_sgl.h>
#include <libxnvme_spec.h>
#include <libxnvme_adm.h>

#define XNVME_BE_SPDK_MAX_PROBE_ATTEMPTS 1
#define XNVME_BE_SPDK_AVLB_TRANSPORTS 3
#define XNVME_BE_SPDK_CTRLRS_MAX 100

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

struct _ctrlr {
	struct spdk_nvme_ctrlr *ctrlr;
	struct spdk_nvme_transport_id trid;
	int16_t refcount;
};

static struct _ctrlr g_ctrlrs[XNVME_BE_SPDK_CTRLRS_MAX];

static struct _ctrlr *
lookup(const struct spdk_nvme_transport_id *trid)
{
	for (int i = 0; i < XNVME_BE_SPDK_CTRLRS_MAX; ++i) {
		if (!g_ctrlrs[i].ctrlr) {
			break;
		}

		if (spdk_nvme_transport_id_compare(trid, &g_ctrlrs[i].trid)) {
			continue;
		}

		return &g_ctrlrs[i];
	}

	return NULL;
}

static int
add_controller(struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_transport_id *trid)
{
	for (int i = 0; i < XNVME_BE_SPDK_CTRLRS_MAX; ++i) {
		if (g_ctrlrs[i].ctrlr) {
			continue;
		}

		g_ctrlrs[i].ctrlr = ctrlr;
		g_ctrlrs[i].trid = *trid;

		return 0;
	}

	return 1;
}

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
 * Initialize the SPDK environment (DPDK), this is provided as an internal helper to avoid
 * re-initialization upon each entry into the library from calls of xnvme_enumerate() and
 * xnvme_dev_open(). In addition to regular SPDK environment initialization, then the following
 * things happen here:
 *
 * - The driver-registration-hook is invoked when requested by compiler-macro, this ensures loading
 *   of the NVMe driver even in scenarious where --whole-archive have not been used for linking
 * - The output-chokers are called, this chokes all the output from SPDK/DPDK, unless xNVMe is
 *   built with DEBUG enabled.
 */
static inline int
_spdk_env_init(struct xnvme_opts *xopts)
{
	struct spdk_env_opts env_opts = {0};
	int err;

	if (g_xnvme_be_spdk_env_is_initialized) {
		XNVME_DEBUG("INFO: already initialized");
		err = 0;
		goto exit;
	}

#ifdef SPDK_REGHOOK_NVME_PCIE_DRIVER
	XNVME_DEBUG("INFO: SPDK NVMe PCIe Driver registration -- BEGIN");
	if (!spdk_nvme_transport_available_by_name("PCIe")) {
		XNVME_DEBUG("INFO: registering...");
		spdk_reghook_nvme_pcie_driver();
	} else {
		XNVME_DEBUG("INFO: skipping, already registered.");
	}
	XNVME_DEBUG("INFO: SPDK NVMe PCIe Driver registration -- END");
#endif

	xnvme_spdk_choker_on();

	spdk_env_opts_init(&env_opts);
	env_opts.name = "xnvme";
	env_opts.shm_id = 0;

	if (xopts && xopts->core_mask) {
		XNVME_DEBUG("INFO: multi-process setup");
		env_opts.shm_id = xopts->shm_id;
		env_opts.core_mask = xopts->core_mask;
		env_opts.main_core = xopts->main_core;
	}

	err = spdk_env_init(&env_opts);
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
		return "";
	}

	return "";
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

void
xnvme_be_spdk_state_term(struct xnvme_be_spdk_state *state)
{
	int err;

	if (!state) {
		return;
	}
	if (state->qpair) {
		spdk_nvme_ctrlr_free_io_qpair(state->qpair);
		err = pthread_mutex_destroy(&state->qpair_lock);
		if (err) {
			printf("UNHANDLED: pthread_mutex_destroy(): '%s'\n", strerror(err));
		}
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

struct xnvme_be_spdk_enumerate_ctx {
	struct xnvme_opts *opts;
	xnvme_enumerate_cb enumerate_cb;
	void *cb_args;
};

static void
attach_cb(void *XNVME_UNUSED(cb_ctx), const struct spdk_nvme_transport_id *trid,
	  struct spdk_nvme_ctrlr *ctrlr,
	  const struct spdk_nvme_ctrlr_opts *XNVME_UNUSED(ctrlr_opts))
{
	if (lookup(trid)) {
		XNVME_DEBUG("INFO: trid-id already in controller-list, skipping it")
		return;
	}

	add_controller(ctrlr, trid);
}

/**
 * Always attached to everything found on the given transport
 */
static bool
probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
	 struct spdk_nvme_ctrlr_opts *ctrlr_opts)
{
	struct xnvme_opts *opts = cb_ctx;

	// Setup controller options, general as well as trtype-specific
	ctrlr_opts->command_set = opts->css.given ? opts->css.value : SPDK_NVME_CC_CSS_IOCS;

	switch (trid->trtype) {
	case SPDK_NVME_TRANSPORT_PCIE:
		ctrlr_opts->use_cmb_sqs = opts->use_cmb_sqs ? true : false;
		break;

	case SPDK_NVME_TRANSPORT_TCP:
	case SPDK_NVME_TRANSPORT_RDMA:
		ctrlr_opts->header_digest = 1;
		ctrlr_opts->data_digest = 1;
		ctrlr_opts->keep_alive_timeout_ms = 0;
		break;

	case SPDK_NVME_TRANSPORT_FC:
	case SPDK_NVME_TRANSPORT_CUSTOM:
	case SPDK_NVME_TRANSPORT_VFIOUSER:
		XNVME_DEBUG("FAILED: unsupported trtype: %d", trid->trtype);
		return false;
	}

	return true;
}

static int
init_dev_using_cref(struct xnvme_dev *dev, struct xnvme_opts *opts, struct _ctrlr *cref,
		    struct spdk_nvme_ns *ns, int nsid)
{
	struct xnvme_be_spdk_state *state = (void *)dev->be.state;
	struct spdk_nvme_transport_id *trid = &cref->trid;
	struct xnvme_ident *ident = &dev->ident;
	int err;

	switch (trid->trtype) {
	case SPDK_NVME_TRANSPORT_PCIE:
		snprintf(ident->uri, sizeof(ident->uri), "%s", trid->traddr);
		snprintf(ident->subnqn, sizeof(ident->subnqn), "%s", trid->subnqn);
		break;

	case SPDK_NVME_TRANSPORT_TCP:
	case SPDK_NVME_TRANSPORT_RDMA:
		snprintf(ident->uri, sizeof(ident->uri), "%s:%s", trid->traddr, trid->trsvcid);
		snprintf(ident->subnqn, sizeof(ident->subnqn), "%s", trid->subnqn);
		break;

	case SPDK_NVME_TRANSPORT_FC:
	case SPDK_NVME_TRANSPORT_CUSTOM:
	case SPDK_NVME_TRANSPORT_VFIOUSER:
		XNVME_DEBUG("SKIP: ENOSYS trtype: %s",
			    spdk_nvme_transport_id_trtype_str(trid->trtype));
		return -ENOSYS;
	}

	ident->dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
	ident->nsid = nsid;
	ident->csi = spdk_nvme_ns_get_csi(ns);
	snprintf(ident->subnqn, sizeof(ident->subnqn), "%s", trid->subnqn);

	dev->be = xnvme_be_spdk;
	dev->be.mem = g_xnvme_be_spdk_mem;
	dev->be.admin = g_xnvme_be_spdk_admin;
	dev->be.dev = g_xnvme_be_spdk_dev;
	dev->be.sync = g_xnvme_be_spdk_sync;
	dev->be.dev = g_xnvme_be_spdk_dev;
	dev->be.async = g_xnvme_be_spdk_async;

	dev->opts = *opts;
	dev->opts.be = dev->be.attr.name;
	dev->opts.admin = dev->be.admin.id;
	dev->opts.sync = dev->be.sync.id;
	dev->opts.async = dev->be.async.id;
	dev->opts.mem = "anonymous";
	dev->opts.dev = "anonymous";

	state->ctrlr = cref->ctrlr;

	// Setup IO qpair lock for SYNC commands
	err = pthread_mutex_init(&state->qpair_lock, NULL);
	if (err) {
		XNVME_DEBUG("FAILED: pthread_mutex_init(): '%s'", strerror(err));
		return -err;
	}

	// Setup IO qpair for SYNC commands
	state->qpair = spdk_nvme_ctrlr_alloc_io_qpair(state->ctrlr, NULL, 0);
	if (!state->qpair) {
		XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_alloc_io_qpair()");
		return -ENOMEM;
	}

	err = xnvme_be_dev_idfy(dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_dev_idfy()");
		xnvme_be_spdk_state_term((void *)dev->be.state);
		return err;
	}
	err = xnvme_be_dev_derive_geometry(dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_dev_derive_geometry()");
		xnvme_be_spdk_state_term((void *)dev->be.state);
		return err;
	}

	return 0;
}

int
xnvme_be_spdk_enumerate(const char *sys_uri, struct xnvme_opts *opts, xnvme_enumerate_cb cb_func,
			void *cb_args)
{
	if (_spdk_env_init(opts)) {
		XNVME_DEBUG("FAILED: _spdk_env_init()");
		return -ESRCH;
	}

	/**
	 * Probe for controllers, when 'sys_uri' is given, probe only fabrics, otherwise, probe
	 * only PCIe
	 */
	XNVME_DEBUG("INFO: probing START");
	for (int t = 0; t < g_xnvme_be_spdk_ntransport; ++t) {
		int trtype = g_xnvme_be_spdk_transport[t];
		char addr[SPDK_NVMF_TRADDR_MAX_LEN + 1] = {0};
		struct spdk_nvme_transport_id trid = {0};
		char trid_str[1024] = {0};
		int port = 0;
		int err;

		XNVME_DEBUG("INFO: do probe of trtype: %s, # %d of %d ?",
			    spdk_nvme_transport_id_trtype_str(trtype), t + 1,
			    g_xnvme_be_spdk_ntransport);

		switch (trtype) {
		case SPDK_NVME_TRANSPORT_FC:
		case SPDK_NVME_TRANSPORT_CUSTOM:
		case SPDK_NVME_TRANSPORT_VFIOUSER:
			XNVME_DEBUG("INFO: no; skipping due to ENOSYS");
			continue;

		case SPDK_NVME_TRANSPORT_PCIE:
			if (sys_uri) {
				XNVME_DEBUG("INFO: no; skip since expecting Fabrics(%s)", sys_uri);
				continue;
			}
			snprintf(trid_str, sizeof(trid_str), "trtype:%s",
				 spdk_nvme_transport_id_trtype_str(trtype));
			break;

		case SPDK_NVME_TRANSPORT_TCP:
		case SPDK_NVME_TRANSPORT_RDMA:
			if (!sys_uri) {
				XNVME_DEBUG("INFO: no; since since expecting PCIe, !sys_uri");
				continue;
			}
			if (sscanf(sys_uri, "%[^:]:%d", addr, &port) != 2) {
				XNVME_DEBUG("INFO: no; skip due to invalid dev->ident.uri: %s",
					    sys_uri);
				continue;
			}
			snprintf(trid_str, sizeof(trid_str),
				 "trtype:%s subnqn:%s adrfam:%s traddr:%s trsvcid:%d",
				 spdk_nvme_transport_id_trtype_str(trtype),
				 SPDK_NVMF_DISCOVERY_NQN, opts->adrfam ? opts->adrfam : "IPv4",
				 addr, port);
			break;
		}

		err = spdk_nvme_transport_id_parse(&trid, trid_str);
		if (err) {
			XNVME_DEBUG("FAILED: spdk_nvme_transport_id_parse(%s), err: %d", trid_str,
				    err);
			continue;
		}

		XNVME_DEBUG("INFO: yes; constructed trid, probing ...");
		err = spdk_nvme_probe(&trid, opts, probe_cb, attach_cb, NULL);
		if (err) {
			XNVME_DEBUG("FAILED: spdk_nvme_probe(), err: %d", err);
		}
	}
	XNVME_DEBUG("INFO: probing DONE.");

	/**
	 * Search attached controllers, their namespaces and encapsulate them in xnvme_dev
	 */
	for (int i = 0; i < XNVME_BE_SPDK_CTRLRS_MAX; ++i) {
		struct _ctrlr *cref = &g_ctrlrs[i];
		int num_ns;
		int cerr;

		if (!cref->ctrlr) {
			break;
		}

		// Do this to ensure that any namespaces added or removed are visible when re-using
		// a controller which since they were probed
		cerr = spdk_nvme_ctrlr_process_admin_completions(cref->ctrlr);
		if (cerr < 0) {
			XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_process_admin_completions(), err: %d",
				    cerr);
			continue;
		}

		num_ns = spdk_nvme_ctrlr_get_num_ns(cref->ctrlr);
		for (int nsid = 1; nsid <= num_ns; ++nsid) {
			struct spdk_nvme_ns *ns;
			struct xnvme_dev *dev;
			int err;

			if (!spdk_nvme_ctrlr_get_data(cref->ctrlr)) {
				XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_get_data");
				continue;
			}
			ns = spdk_nvme_ctrlr_get_ns(cref->ctrlr, nsid);
			if (!ns) {
				continue;
			}
			if (!spdk_nvme_ns_is_active(ns)) {
				XNVME_DEBUG("INFO: skipping inactive nsid: %d", nsid);
				continue;
			}
			if (!spdk_nvme_ns_get_data(ns)) {
				XNVME_DEBUG("FAILED: spdk_nvme_ns_get_data()");
				continue;
			}

			// Namespace looks good, construct xNVMe identifier, and device instance!
			err = xnvme_dev_alloc(&dev);
			if (err) {
				XNVME_DEBUG("FAILED: xnvme_dev_alloc()");
				return -err;
			}

			err = init_dev_using_cref(dev, opts, cref, ns, nsid);
			if (err) {
				continue;
			}

			if (cb_func(dev, cb_args)) {
				xnvme_dev_close(dev);
			}
		}
	}

	return 0;
}

int
xnvme_be_spdk_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_opts *opts = &dev->opts;

	if (_spdk_env_init(opts)) {
		XNVME_DEBUG("FAILED: _spdk_env_init()");
		return -ESRCH;
	}

	/**
	 * Probe for controllers matching the given 'dev'
	 * Note: dev->uri, can be either PCIe or TCP|RDMA
	 */
	XNVME_DEBUG("INFO: probing START");
	for (int t = 0; t < g_xnvme_be_spdk_ntransport; ++t) {
		int trtype = g_xnvme_be_spdk_transport[t];
		char addr[SPDK_NVMF_TRADDR_MAX_LEN + 1] = {0};
		struct spdk_nvme_transport_id trid = {0};
		char trid_str[1024] = {0};
		int port = 0;
		int err;

		XNVME_DEBUG("INFO: do probe of trtype: %s, # %d of %d ?",
			    spdk_nvme_transport_id_trtype_str(trtype), t + 1,
			    g_xnvme_be_spdk_ntransport);

		switch (trtype) {
		case SPDK_NVME_TRANSPORT_FC:
		case SPDK_NVME_TRANSPORT_CUSTOM:
		case SPDK_NVME_TRANSPORT_VFIOUSER:
			XNVME_DEBUG("INFO: no; skipping due to ENOSYS");
			continue;

		case SPDK_NVME_TRANSPORT_PCIE:
			snprintf(trid_str, sizeof(trid_str), "trtype:%s traddr:%s",
				 spdk_nvme_transport_id_trtype_str(trtype), dev->ident.uri);
			break;

		case SPDK_NVME_TRANSPORT_TCP:
		case SPDK_NVME_TRANSPORT_RDMA:
			if (sscanf(dev->ident.uri, "%[^:]:%d", addr, &port) != 2) {
				XNVME_DEBUG("INFO: no; skip due to invalid dev->ident.uri: %s",
					    dev->ident.uri);
				continue;
			}
			snprintf(trid_str, sizeof(trid_str),
				 "trtype:%s subnqn:%s adrfam:%s traddr:%s trsvcid:%d",
				 spdk_nvme_transport_id_trtype_str(trtype),
				 SPDK_NVMF_DISCOVERY_NQN, opts->adrfam ? opts->adrfam : "IPv4",
				 addr, port);
			break;
		}

		err = spdk_nvme_transport_id_parse(&trid, trid_str);
		if (err) {
			XNVME_DEBUG("FAILED: spdk_nvme_transport_id_parse(%s), err: %d", trid_str,
				    err);
			return err;
		}

		XNVME_DEBUG("INFO: yes; constructed trid, probing ...");
		err = spdk_nvme_probe(&trid, opts, probe_cb, attach_cb, NULL);
		if (err) {
			XNVME_DEBUG("FAILED: spdk_nvme_probe(), err: %d", err);
		}
	}
	XNVME_DEBUG("INFO: probing DONE");

	/**
	 * Search attached controllers for one matching the given 'dev', grab the namespace and
	 * initialize the dev
	 */
	for (int i = 0; i < XNVME_BE_SPDK_CTRLRS_MAX; ++i) {
		int nsid = dev->opts.nsid;
		struct _ctrlr *cref = &g_ctrlrs[i];
		struct spdk_nvme_ns *ns;
		int err;

		if (!cref->ctrlr) {
			break;
		}

		// Do this to ensure that any namespaces added or removed are visible when re-using
		// a controller which since they were probed
		err = spdk_nvme_ctrlr_process_admin_completions(cref->ctrlr);
		if (err < 0) {
			XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_process_admin_completions(), err: %d",
				    err);
			continue;
		}
		if (!spdk_nvme_ctrlr_get_data(cref->ctrlr)) {
			XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_get_data()");
			continue;
		}

		// Check whether the controller is a match
		switch (cref->trid.trtype) {
		case SPDK_NVME_TRANSPORT_FC:
		case SPDK_NVME_TRANSPORT_CUSTOM:
		case SPDK_NVME_TRANSPORT_VFIOUSER:
			XNVME_DEBUG("INFO: no; skipping due to ENOSYS");
			continue;

		case SPDK_NVME_TRANSPORT_PCIE:
			if (strcmp(cref->trid.traddr, dev->ident.uri)) {
				XNVME_DEBUG("INFO: no-match for PCIe %s, %s", cref->trid.traddr,
					    dev->ident.uri);
				continue;
			}
			break;

		case SPDK_NVME_TRANSPORT_TCP:
		case SPDK_NVME_TRANSPORT_RDMA: {
			char addr[SPDK_NVMF_TRADDR_MAX_LEN + 1] = {0};
			char port[SPDK_NVMF_TRSVCID_MAX_LEN + 1] = {0};

			if (sscanf(dev->ident.uri, "%[^:]:%s", addr, port) != 2) {
				XNVME_DEBUG("INFO: cannot parse host as addr,port");
				continue;
			}

			if (strcmp(cref->trid.traddr, addr) || strcmp(cref->trid.trsvcid, port) ||
			    ((!dev->opts.subnqn) || strcmp(cref->trid.subnqn, dev->opts.subnqn))) {
				XNVME_DEBUG("INFO: no-match for TCP|RDMA");
				continue;
			}
		} break;
		}

		// A matching controller is found, now get the namespace
		ns = spdk_nvme_ctrlr_get_ns(cref->ctrlr, nsid);
		if (!ns) {
			XNVME_DEBUG("FAILED: retrieving namespace(0x%x)", nsid);
			return -EINVAL;
			continue;
		}
		if (!spdk_nvme_ns_is_active(ns)) {
			XNVME_DEBUG("FAILED:: inactive namespace(0x%x)", nsid);
			continue;
		}
		if (!spdk_nvme_ns_get_data(ns)) {
			XNVME_DEBUG("FAILED: spdk_nvme_ns_get_data(0x%x)", nsid);
			continue;
		}

		// So far so good, initialize the 'dev' and return
		err = init_dev_using_cref(dev, &dev->opts, cref, ns, nsid);
		if (err) {
			XNVME_DEBUG("FAILED: init_dev_using_cref()");
			return err;
		}

		return 0;
	}

	return -EINVAL;
}
#endif

struct xnvme_be_dev g_xnvme_be_spdk_dev = {
#ifdef XNVME_BE_SPDK_ENABLED
	.enumerate = xnvme_be_spdk_enumerate,
	.dev_open = xnvme_be_spdk_dev_open,
	.dev_close = xnvme_be_spdk_dev_close,
#else
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
#endif
};
