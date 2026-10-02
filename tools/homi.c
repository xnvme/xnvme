// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libxnvme.h>
#include <xnvme_cuse.h>

// This heap is the pool every client draws from, so it is sized for the I/O they
// do rather than for what HOMI does itself. It used to be 16MiB, which was right when
// a client brought its own memory: it now has none of its own, and asks for all of
// it here, so the old figure left clients unable to allocate a working buffer.
// Tunable with --host_heap_size for a machine with less to spare, or more to serve.
#define HOMI_HEAP_SIZE_PER_DEV (64ULL * 1024 * 1024)

// What HOMI itself takes from that heap for each device held: the admin queue and
// the sync queue pair that opening it creates, each carrying a 4 MiB request pool.
// Budgeted on top of the pool rather than out of it, so a client can be handed a
// buffer the size of the pool even when a single device is held.
#define HOMI_HEAP_SIZE_SELF_PER_DEV (16ULL * 1024 * 1024)

// The GPU backends allocate a device heap for data buffers, which HOMI never allocates
// from; only the control structures it does need live on the host heap. Claiming the
// backend default would take a GiB of VRAM away from the clients. 2MiB is the dma-buf
// granularity that AMD requires, so it is the smallest heap both GPU backends accept.
#define HOMI_DEVICE_HEAP_SIZE (2ULL * 1024 * 1024)

#ifndef XNVME_PLATFORM_WINDOWS_ENABLED

static volatile sig_atomic_t stop = 0;
static sigset_t g_orig_sigmask;

static void
handle_signal(int sig __attribute__((unused)))
{
	stop = 1;
}

/**
 * Must run before any CUSE session thread is created: a thread inherits the
 * creating thread's signal mask, so blocking these here first is what keeps
 * them out of every later thread's mask too
 */
static void
block_stop_signals(void)
{
	struct sigaction sa = {0};
	sigset_t mask;

	sa.sa_handler = handle_signal;
	sa.sa_flags = 0;

	sigemptyset(&sa.sa_mask);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);

	sigemptyset(&mask);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &g_orig_sigmask);
}

/**
 * Let sigsuspend() unblock the stop-signals only while parked, such that
 * one arriving between the 'stop' test and the wait cannot be lost; restores
 * the mask block_stop_signals() saved before returning, so SIGTERM/SIGINT
 * are unblocked again for the rest of teardown
 */
static void
wait_for_stop_signal(void)
{
	while (!stop) {
		sigsuspend(&g_orig_sigmask);
	}

	sigprocmask(SIG_SETMASK, &g_orig_sigmask, NULL);
}

static void
sigusr1_noop(int sig __attribute__((unused)))
{
}

/**
 * A controller opened via a PCIe-attached backend (upcie, spdk) carries its
 * bus:device.function address as its URI verbatim (see
 * '_scan_pci_report_ctrlr()' in xnvme_platform_linux.c); that is the only
 * identifier HOMI's CUSE mimic uses, so a controller reached any other way
 * (fabrics, a bare device-file path) has none and is left unexposed
 */
static bool
is_pci_bdf(const char *uri)
{
	unsigned int domain, bus, dev, func;
	int end = 0;

	return sscanf(uri, "%4x:%2x:%2x.%1x%n", &domain, &bus, &dev, &func, &end) == 4 &&
	       uri[end] == '\0';
}

static int
sub_start(struct xnvme_cli *cli)
{
	struct xnvme_dev **devs;
	struct xnvme_cuse *cuse_sessions = NULL;
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

	block_stop_signals();

	/* SPDK needs the identifier at open, as the DPDK segment to be. uPCIe
	 * must not see it there: to the backend it names a server to connect
	 * to, and this process is the one that will serve, so it opens the
	 * controllers as their owner and takes the identifier at serve time. */
	opts.shm_id = (uint32_t)cli->args.homi_id;
	opts.homi_id = 0;
	opts.be = cli->args.be;
	// homi holds whole controllers, not namespaces: nsid=0 gives dtype
	// XNVME_DEV_TYPE_NVME_CONTROLLER, which lib/xnvme_cuse.c relies on
	opts.nsid = 0;

	// The heap is per-process rather than per-device, so it has to cover every device
	// held. Claiming the backend default would leave nothing in the hugepage pool for
	// the clients HOMI exists to serve.
	opts.host_heap_size =
		cli->args.host_heap_size
			? cli->args.host_heap_size
			: (HOMI_HEAP_SIZE_PER_DEV + HOMI_HEAP_SIZE_SELF_PER_DEV) * ndevs;
	opts.device_heap_size =
		cli->args.device_heap_size ? cli->args.device_heap_size : HOMI_DEVICE_HEAP_SIZE;

	err = xnvme_cli_dev_open_multi(dev_uris, ndevs, &opts, &devs);
	if (err) {
		xnvme_cli_perr("Failed opening all devices", err);
		return err;
	}

	cuse_sessions = calloc(ndevs, sizeof(*cuse_sessions));
	if (!cuse_sessions) {
		xnvme_cli_perr("Failed allocating CUSE session state", -ENOMEM);
		xnvme_cli_dev_close_multi(devs, ndevs);
		return -ENOMEM;
	}

	if (!cli->args.no_cuse) {
		// Without a handler, xnvme_cuse_stop()'s SIGUSR1 would terminate the process
		struct sigaction sa = {.sa_handler = sigusr1_noop};

		sigemptyset(&sa.sa_mask);
		sigaction(SIGUSR1, &sa, NULL);

		for (int i = 0; i < ndevs; ++i) {
			const struct xnvme_ident *ident = xnvme_dev_get_ident(devs[i]);
			char name[64];

			if (!is_pci_bdf(ident->uri)) {
				continue; // No BDF for this controller; nothing to expose
			}

			snprintf(name, sizeof(name), "xnvme/%s", ident->uri);

			err = xnvme_cuse_start(&cuse_sessions[i], devs[i], name);
			if (err == -ENOSYS) {
				continue; // Built without the CUSE mimic (Linux-only); quiet
			}
			if (err) {
				xnvme_cli_perr("Failed: xnvme_cuse_start(); continuing without it",
					       err);
				continue; // Non-fatal: HOMI still serves the controller
			}
			xnvme_cli_pinf("Exposing /dev/%s", name);
		}
	}

	xnvme_cli_pinf("HOMI started successfully, use Ctrl+C to stop");

	// cplane_serve() polls its own flag, so handle_signal() must run somewhere to
	// set it; unblock here only, CUSE threads keep it blocked from their creation
	sigprocmask(SIG_SETMASK, &g_orig_sigmask, NULL);

	err = xnvme_cplane_serve(devs, ndevs, (uint32_t)cli->args.homi_id, &stop);
	if (err == -ENOSYS) {
		/* A backend that shares its own way, so hold the
		 * controllers and let it do the sharing. */
		err = 0;
		block_stop_signals();
		wait_for_stop_signal();
	} else if (err) {
		xnvme_cli_perr("xnvme_cplane_serve()", err);
	}

	for (int i = 0; i < ndevs; ++i) {
		xnvme_cuse_stop(&cuse_sessions[i]);
	}
	free(cuse_sessions);

	xnvme_cli_dev_close_multi(devs, ndevs);

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
			{XNVME_CLI_OPT_NO_CUSE, XNVME_CLI_LFLG},
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
