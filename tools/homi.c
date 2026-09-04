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

/**
 * Say it is up once it actually is
 *
 * Announced from the server rather than before it, since until the socket is
 * bound there is nothing for a client to reach and saying otherwise is how a
 * server that never started reads as one that did.
 */
static void
_announce_serving(void *XNVME_UNUSED(arg))
{
	xnvme_cli_pinf("HOMI started successfully, use Ctrl+C to stop");
}

static int
sub_start(struct xnvme_cli *cli)
{
	struct xnvme_dev **devs;
	struct xnvme_opts opts = xnvme_opts_default();
	const char **dev_uris;
	int ndevs, err;

	ndevs = cli->args.posn_count;
	if (ndevs <= 0) {
		err = -EINVAL;
		xnvme_cli_perr("Error: at least one device URI is required", err);
		return err;
	}
	dev_uris = cli->args.posn;

	/* One identifier, but two fields to put it in: SPDK hands shm_id to
	 * DPDK for a segment, uPCIe takes homi_id as the server to be. Both
	 * come from --homi-id, which is the only spelling here. */
	opts.shm_id = (uint32_t)cli->args.homi_id;
	opts.homi_id = (uint32_t)cli->args.homi_id;
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

	{
		_install_stop_handler();

		err = xnvme_cplane_serve(devs, ndevs, (uint32_t)cli->args.homi_id, &stop,
					 _announce_serving, NULL);
		if (err == -ENOSYS) {
			/* A backend that shares its own way, so hold the
			 * controllers and let it do the sharing. */
			xnvme_cli_pinf("HOMI holding %d controller(s); %s shares them by its own "
				       "means, use Ctrl+C to stop",
				       ndevs, cli->args.be ? cli->args.be : "the backend");
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
_pr_held_controllers(uint32_t homi_id, int *all_up)
{
	struct xnvme_cplane_ctrlr_info info;
	struct xnvme_cplane_info rte;
	int err;

	*all_up = 0;

	err = xnvme_cplane_get_info(homi_id, &rte);
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
	const uint32_t homi_id = (uint32_t)cli->args.homi_id;
	int running, held, all_up = 0;

	running = xnvme_cplane_server_alive(homi_id);
	if (running < 0) {
		xnvme_cli_perr("Failed probing the runtime", running);
		return running;
	}

	printf("homi_id: %" PRIu32 "\n", homi_id);
	printf("server_running: %s\n", running ? "true" : "false");

	if (!running) {
		printf("ready: false\n");
		printf("controllers: []\n");
		fflush(stdout);

		return -ENODEV;
	}

	held = _pr_held_controllers(homi_id, &all_up);
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
			{XNVME_CLI_OPT_HOMI_ID, XNVME_CLI_LREQ},
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
			{XNVME_CLI_OPT_HOMI_ID, XNVME_CLI_LREQ},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "homi - Host-Orchestrated Multi-path I/O",
	.descr_short = "Hold NVMe devices open and serve them to clients",
	.descr_long = "Hold NVMe devices open and serve them to clients, which may be other "
		      "processes or accelerators. A client attaches to the same controllers "
		      "by passing the same --homi-id.",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
