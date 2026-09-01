// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <signal.h>
#include <stdlib.h>

#include <libxnvme.h>

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
_wait_for_stop_signal(void)
{
	struct sigaction sa = {0};
	sigset_t mask, orig;

	sa.sa_handler = handle_signal;
	sa.sa_flags = 0;

	sigemptyset(&sa.sa_mask);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);

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

	ndevs = cli->args.posn_count;
	if (ndevs <= 0) {
		err = -EINVAL;
		xnvme_cli_perr("Error: at least one device URI is required", err);
		return err;
	}
	dev_uris = cli->args.posn;

	opts.shm_id = cli->args.shm_id;
	opts.be = cli->args.be;

	// The heap is per-process rather than per-device, so it has to cover every device
	// held. Claiming the backend default would leave nothing in the hugepage pool for
	// the secondaries HOMI exists to serve.
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
	_wait_for_stop_signal();

	_xnvme_dev_close_all(devs, ndevs);

	return 0;
}

#else

static int
sub_start(struct xnvme_cli *XNVME_UNUSED(cli))
{
	int err = -ENOTSUP;

	xnvme_cli_perr("No multi-process capable backend is available on Windows", err);

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
			{XNVME_CLI_OPT_SHM_ID, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_ORCH_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_BE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_HOST_HEAP_SIZE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DEVICE_HEAP_SIZE, XNVME_CLI_LOPT},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "homi - Host-Orchestrated Multi-path I/O",
	.descr_short = "Hold NVMe devices open for multi-process sharing",
	.descr_long = "Hold NVMe devices open for multi-process sharing. Secondary "
		      "processes attach to the same controllers by passing the same --shm_id.",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
