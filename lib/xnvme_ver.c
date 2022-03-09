// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <libxnvme.h>
#include <libxnvme_pp.h>
#include <stdio.h>

int
xnvme_ver_major(void)
{
	return XNVME_VERSION_MAJOR;
}

int
xnvme_ver_minor(void)
{
	return XNVME_VERSION_MINOR;
}

int
xnvme_ver_patch(void)
{
	return XNVME_VERSION_PATCH;
}

int
xnvme_ver_fpr(FILE *stream, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "ver: {major: %d, minor: %d, patch: %d}", xnvme_ver_major(),
			xnvme_ver_minor(), xnvme_ver_patch());

	return wrtn;
}

int
xnvme_ver_pr(int opts)
{
	return xnvme_ver_fpr(stdout, opts);
}
