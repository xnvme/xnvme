// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <libxnvme.h>
#include <xnvme_cuse.h>
#include <xnvme_vcs.h>

// The backend default (1GiB) is sized for a process doing I/O. HOMI only needs the
// admin queue and the sync qpair that opening a device creates, so claiming the
// default is overkill. Each of those two queue pairs carries a request pool of
// NVME_REQUEST_POOL_LEN PRP pages, which is 4MiB apiece, so budget double the 8MiB
// per device that costs.
#define HOMI_HEAP_SIZE_PER_DEV (16ULL * 1024 * 1024)

// The GPU backends allocate a device heap for data buffers, which HOMI never allocates
// from; only the control structures it does need live on the host heap. Claiming the
// backend default would take a GiB of VRAM away from the secondaries. 2MiB is the dma-buf
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

static void
wait_for_stop_signal(void)
{
	// sigsuspend() unblocks the stop-signals only while parked, so one arriving between
	// the test and the wait cannot be lost
	while (!stop) {
		sigsuspend(&g_orig_sigmask);
	}

	sigprocmask(SIG_SETMASK, &g_orig_sigmask, NULL);
}

static void
sigusr1_noop(int sig __attribute__((unused)))
{
}

static bool
is_pci_bdf(const char *uri)
{
	unsigned int domain, bus, dev, func;
	int end = 0;

	return sscanf(uri, "%4x:%2x:%2x.%1x%n", &domain, &bus, &dev, &func, &end) == 4 &&
	       uri[end] == '\0';
}

static int
sub_serve(struct xnvme_cli *cli)
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

	// Before any CUSE session thread exists: a thread inherits its creator's signal mask
	block_stop_signals();

	opts.shm_id = (uint32_t)cli->args.homi_id;
	opts.be = cli->args.be;
	// homi holds whole controllers, not namespaces: nsid=0 gives dtype
	// XNVME_DEV_TYPE_NVME_CONTROLLER, which lib/cuse/xnvme_cuse.c relies on
	opts.nsid = 0;

	// The heap is per-process rather than per-device, so it has to cover every device
	// held. Claiming the backend default would leave nothing in the hugepage pool for
	// the secondaries HOMI exists to serve.
	opts.host_heap_size = cli->args.host_heap_size ? cli->args.host_heap_size
						       : HOMI_HEAP_SIZE_PER_DEV * ndevs;
	opts.device_heap_size =
		cli->args.device_heap_size ? cli->args.device_heap_size : HOMI_DEVICE_HEAP_SIZE;

	err = xnvme_cli_dev_open_multi(dev_uris, ndevs, &opts, &devs);
	if (err) {
		xnvme_cli_perr("Failed opening all devices", err);
		return err;
	}

	if (!cli->args.no_cuse) {
		// Without a handler, xnvme_cuse_stop()'s SIGUSR1 would terminate the process
		struct sigaction sa = {.sa_handler = sigusr1_noop};

		cuse_sessions = calloc(ndevs, sizeof(*cuse_sessions));
		if (!cuse_sessions) {
			xnvme_cli_perr("Failed allocating CUSE session state", -ENOMEM);
			xnvme_cli_dev_close_multi(devs, ndevs);
			return -ENOMEM;
		}

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
			if (err && err != -ENOSYS && err != -ENOTSUP) {
				xnvme_cli_perr("Failed: xnvme_cuse_start(); skipped", err);
			}
			if (!err) {
				xnvme_cli_pinf("Exposing /dev/%s", name);
			}
		}
	}

	xnvme_cli_pinf("HOMI started successfully, use Ctrl+C to stop");
	xnvme_ver_pr(XNVME_PR_DEF);
	printf("\n");
	wait_for_stop_signal();

	for (int i = 0; cuse_sessions && i < ndevs; ++i) {
		xnvme_cuse_stop(&cuse_sessions[i]);
	}
	free(cuse_sessions);

	xnvme_cli_dev_close_multi(devs, ndevs);

	return 0;
}

#else

static int
sub_serve(struct xnvme_cli *XNVME_UNUSED(cli))
{
	int err = -ENOTSUP;

	xnvme_cli_perr("No multi-process capable backend is available on Windows", err);

	return err;
}

#endif

static struct xnvme_cli_sub g_subs[] = {
	{
		"serve",
		"Open the given devices and hold them open",
		"Open the given devices and hold them open",
		sub_serve,
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
};

static struct xnvme_cli g_cli = {
	.title = "homi - Host-Orchestrated Multi-path I/O",
	.vcs = XNVME_VCS_TAG,
	.descr_short = "Hold NVMe devices open for multi-process sharing",
	.descr_long = "Hold NVMe devices open for multi-process sharing. Secondary "
		      "processes attach to the same controllers by passing the same --homi-id.",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
