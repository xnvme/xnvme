// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libxnvme.h>

// This heap is the pool every client draws from, so it is sized for the I/O they
// do rather than for what HOMI does itself. It used to be 16MiB, which was right when
// a client brought its own memory: it now has none of its own, and asks for all of
// it here, so the old figure left clients unable to allocate a working buffer.
// Tunable with --host_heap_size for a machine with less to spare, or more to serve.
#define HOMI_HEAP_SIZE_PER_DEV (64ULL * 1024 * 1024)

// The GPU backends allocate a device heap for data buffers, which HOMI never allocates
// from; only the control structures it does need live on the host heap. Claiming the
// backend default would take a GiB of VRAM away from the clients. 2MiB is the dma-buf
// granularity that AMD requires, so it is the smallest heap both GPU backends accept.
#define HOMI_DEVICE_HEAP_SIZE (2ULL * 1024 * 1024)

#ifndef XNVME_PLATFORM_WINDOWS_ENABLED

static volatile sig_atomic_t stop = 0;

static void
handle_signal(int sig __attribute__((unused)))
{
	stop = 1;
}

static void
_xnvme_dev_close_all(struct xnvme_dev **devs, int count)
{
	for (int i = 0; i < count; i++) {
		xnvme_dev_close(devs[i]);
	}
	free(devs);
}

static int
_xnvme_dev_open_all(const char **uris, int count, struct xnvme_opts *opts, struct xnvme_dev ***out)
{
	struct xnvme_dev **devs;
	int opened = 0, err;

	devs = calloc(count, sizeof(*devs));
	if (!devs) {
		err = -errno;
		xnvme_cli_perr("Failed: calloc()", err);
		return err;
	}

	for (int i = 0; i < count; i++) {
		devs[i] = xnvme_dev_open(uris[i], opts);
		if (!devs[i]) {
			err = -errno;
			xnvme_cli_perr("Failed: xnvme_dev_open()", err);
			XNVME_DEBUG("Could not open uri(%s) at index(%d): err(%d)", uris[i], i,
				    err);
			goto failed;
		}
		opened++;
	}

	*out = devs;

	return 0;

failed:
	_xnvme_dev_close_all(devs, opened);
	return err;
}

/**
 * The control-plane identifier this invocation names
 *
 * --shm_id predates --cplane-id and named this too, so it still answers for it;
 * the newer spelling wins where both are given.
 */
static uint32_t
_cplane_id(struct xnvme_cli *cli)
{
	return cli->given[XNVME_CLI_OPT_CPLANE_ID] ? (uint32_t)cli->args.cplane_id
						   : (uint32_t)cli->args.shm_id;
}

/**
 * Whether this invocation named one at all
 *
 * Neither spelling can be the required one while both are accepted, so the
 * requirement moves here from the option table.
 */
static int
_cplane_id_given(struct xnvme_cli *cli)
{
	return cli->given[XNVME_CLI_OPT_CPLANE_ID] || cli->given[XNVME_CLI_OPT_SHM_ID];
}

static void
_install_stop_handler(void)
{
	struct sigaction sa = {0};

	sa.sa_handler = handle_signal;
	sa.sa_flags = 0;

	sigemptyset(&sa.sa_mask);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
}

static void
_wait_for_stop_signal(void)
{
	sigset_t mask, orig;

	_install_stop_handler();

	// Block the stop-signals before testing 'stop', and let sigsuspend() unblock them only
	// while parked, such that a signal arriving between the test and the wait cannot be lost
	sigemptyset(&mask);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &orig);

	while (!stop) {
		sigsuspend(&orig);
	}

	sigprocmask(SIG_SETMASK, &orig, NULL);
}

static int
sub_start(struct xnvme_cli *cli)
{
	struct xnvme_dev **devs;
	struct xnvme_opts opts = xnvme_opts_default();
	const char **dev_uris;
	int ndevs, err;

	if (!_cplane_id_given(cli)) {
		err = -EINVAL;
		xnvme_cli_perr("Error: --cplane-id is required", err);
		return err;
	}

	ndevs = cli->args.posn_count;
	if (ndevs <= 0) {
		err = -EINVAL;
		xnvme_cli_perr("Error: at least one device URI is required", err);
		return err;
	}
	dev_uris = cli->args.posn;

	/* Each backend reads its own: SPDK hands shm_id to DPDK for a segment,
	 * uPCIe takes cplane_id as the server to be. */
	opts.shm_id = cli->args.shm_id;
	opts.cplane_id = _cplane_id(cli);
	opts.be = cli->args.be;

	// The heap is per-process rather than per-device, so it has to cover every device
	// held. Claiming the backend default would leave nothing in the hugepage pool for
	// the clients HOMI exists to serve.
	opts.host_heap_size = cli->args.host_heap_size ? cli->args.host_heap_size
						       : HOMI_HEAP_SIZE_PER_DEV * ndevs;
	opts.device_heap_size =
		cli->args.device_heap_size ? cli->args.device_heap_size : HOMI_DEVICE_HEAP_SIZE;

	err = _xnvme_dev_open_all(dev_uris, ndevs, &opts, &devs);
	if (err) {
		xnvme_cli_perr("Failed opening all devices", err);
		return err;
	}

	xnvme_cli_pinf("HOMI started successfully, use Ctrl+C to stop");

	{
		_install_stop_handler();

		err = xnvme_cplane_serve(devs, ndevs, _cplane_id(cli), &stop);
		if (err == -ENOSYS) {
			/* A backend that shares its own way, so hold the
			 * controllers and let it do the sharing. */
			err = 0;
			_wait_for_stop_signal();
		} else if (err) {
			xnvme_cli_perr("xnvme_cplane_serve()", err);
		}
	}

	_xnvme_dev_close_all(devs, ndevs);

	return err;
}

#else

static int
sub_start(struct xnvme_cli *XNVME_UNUSED(cli))
{
	int err = -ENOTSUP;

	xnvme_cli_perr("No backend that can share a controller is available on Windows", err);

	return err;
}

#endif

/* Reads uPCIe's own state, so 'homi start --be spdk' works on hosts where
 * 'homi status' refuses */
#ifdef XNVME_BE_UPCIE_ENABLED

/**
 * Emit one YAML sequence entry per controller the runtime reports
 *
 * Totals are omitted rather than zeroed when the controller did not report
 * them: absent says unknown, where zero would say none.
 *
 * Sets `*all_up` to whether every controller finished coming up, which is the
 * difference between a server that exists and one that can be connected to.
 */
static int
_pr_held_controllers(uint32_t cplane_id, int *all_up)
{
	struct xnvme_cplane_ctrlr_info info;
	struct xnvme_cplane_info rte;
	int err;

	*all_up = 0;

	err = xnvme_cplane_get_info(cplane_id, &rte);
	if (err) {
		return (err == -ENOENT) ? 0 : err;
	}

	printf("connections: %u\n", rte.nconnections);
	if (rte.nctrlrs_held > rte.nctrlrs) {
		printf("controllers_held: %u\n", rte.nctrlrs_held);
	}

	*all_up = rte.nctrlrs ? 1 : 0;

	for (uint32_t i = 0; i < rte.nctrlrs; ++i) {
		if (!i) {
			printf("controllers:\n");
		}
		printf("  - uri: '%s'\n", rte.ctrlrs[i]);

		if (xnvme_cplane_get_ctrlr_info(rte.ctrlrs[i], &info)) {
			printf("    readable: false\n");
			*all_up = 0;
			continue;
		}

		if (!info.initialized) {
			*all_up = 0;
		}

		printf("    readable: true\n");
		printf("    initialized: %s\n", info.initialized ? "true" : "false");
		printf("    connections: %u\n", info.nconnections);
		printf("    nsq_used: %u\n", info.nsq_used);
		printf("    ncq_used: %u\n", info.ncq_used);
		if (info.nsq_total || info.ncq_total) {
			printf("    nsq_total: %u\n", info.nsq_total);
			printf("    ncq_total: %u\n", info.ncq_total);
		}
	}

	return (int)rte.nctrlrs;
}

static int
sub_status(struct xnvme_cli *cli)
{
	const uint32_t cplane_id = _cplane_id(cli);
	int running, held, all_up = 0;

	if (!_cplane_id_given(cli)) {
		xnvme_cli_perr("Error: --cplane-id is required", -EINVAL);
		return -EINVAL;
	}

	running = xnvme_cplane_server_alive(cplane_id);
	if (running < 0) {
		xnvme_cli_perr("Failed probing the runtime", running);
		return running;
	}

	printf("cplane_id: %" PRIu32 "\n", cplane_id);
	printf("server_running: %s\n", running ? "true" : "false");

	if (!running) {
		printf("ready: false\n");
		printf("controllers: []\n");
		fflush(stdout);

		return -ENODEV;
	}

	held = _pr_held_controllers(cplane_id, &all_up);
	if (held < 0) {
		fflush(stdout);
		xnvme_cli_perr("Failed listing controllers", held);
		return held;
	}
	if (!held) {
		printf("controllers: []\n");
	}
	printf("ready: %s\n", all_up ? "true" : "false");

	fflush(stdout);

	return all_up ? 0 : -EAGAIN;
}

#else

static int
sub_status(struct xnvme_cli *XNVME_UNUSED(cli))
{
	int err = -ENOTSUP;

	xnvme_cli_perr("Inspection requires the uPCIe backend, which this build lacks", err);

	return err;
}

#endif

static struct xnvme_cli_sub g_subs[] = {
	{
		"start",
		"Open the given devices and hold them open",
		"Open the given devices and hold them open",
		sub_start,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSN},
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SHM_ID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CPLANE_ID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_ORCH_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_BE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_HOST_HEAP_SIZE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DEVICE_HEAP_SIZE, XNVME_CLI_LOPT},
		},
	},
	{
		"status",
		"Report whether a server is holding devices",
		"Report whether a server is holding devices. Reads the state the "
		"uPCIe runtime keeps, taking no lock and opening no device, so it "
		"disturbs neither I/O nor a runtime that is starting. Exits "
		"non-zero until a server is up and its devices are ready.",
		sub_status,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SHM_ID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CPLANE_ID, XNVME_CLI_LOPT},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "homi - Host-Orchestrated Multi-path I/O",
	.descr_short = "Hold NVMe devices open and serve them to other processes",
	.descr_long = "Hold NVMe devices open and serve them to other processes. Client "
		      "processes attach to the same controllers by passing the same --cplane-id.",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
