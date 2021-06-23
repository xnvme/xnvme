// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Niclas Hedam <n.hedam@samsung.com, nhed@itu.dk>
// SPDX-License-Identifier: Apache-2.0
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_HERMES_ENABLED
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <libxnvme_adm.h>
#include <libxnvme_file.h>
#include <libxnvme_spec_fs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <xnvme_be_compute.h>
#include <xnvme_dev.h>

int
xnvme_be_hermes_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_be_compute_state *state = (void *)dev->be.state;
	const struct xnvme_ident *ident = &dev->ident;

	state->fd = open(ident->trgt, O_RDWR);

	if (state->fd < 0) {
		XNVME_DEBUG("FAILED: open(trgt: '%s'), state->fd: '%d', errno: %d",
			    ident->trgt, state->fd, errno);
		return -errno;
	}

	XNVME_DEBUG("INFO: --- open() : OK ---");

	return 0;
}

void
xnvme_be_hermes_dev_close(struct xnvme_dev *dev)
{
	struct xnvme_be_compute_state *state;

	if (!dev) {
		return;
	}

	state = (void *)dev->be.state;
	close(state->fd);

	memset(&dev->be, 0, sizeof(dev->be));
}

int
xnvme_be_hermes_enumerate(struct xnvme_enumeration *list,
			  const char *XNVME_UNUSED(sys_uri),
			  int XNVME_UNUSED(opts))
{
	char pattern[] = "hermes";

	struct dirent **ns = NULL;
	int nns = 0;

	nns = scandir("/dev", &ns, NULL, NULL);
	for (int ni = 0; ni < nns; ++ni) {
		char uri[XNVME_IDENT_URI_LEN] = {0};
		struct xnvme_ident ident = {0};

		snprintf(uri, XNVME_IDENT_URI_LEN - 1, "file:" _PATH_DEV "%s",
			 ns[ni]->d_name);
		if (!strncmp(pattern, uri, strlen(pattern))) {
			break;
		}

		if (xnvme_enumeration_append(list, &ident)) {
			XNVME_DEBUG("FAILED: adding ident");
		}
	}

	for (int ni = 0; ni < nns; ++ni) {
		free(ns[ni]);
	}
	free(ns);

	return 0;
}
#endif

struct xnvme_be_dev g_xnvme_be_compute_dev = {
#ifdef XNVME_BE_HERMES_ENABLED
	.enumerate = xnvme_be_hermes_enumerate,
	.dev_open = xnvme_be_hermes_dev_open,
	.dev_close = xnvme_be_hermes_dev_close,
#else
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
#endif
};
