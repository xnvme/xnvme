// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <libxnvme.h>

#include "xnvme_3p/xnvme_3p_fio.c"
#ifdef XNVME_BE_SPDK_ENABLED
#include "xnvme_3p/xnvme_3p_spdk.c"
#endif
#ifdef XNVME_BE_LINUX_ENABLED
#include "xnvme_3p/xnvme_3p_libnvme.c"
#endif
#ifdef XNVME_BE_LINUX_IOU_ENABLED
#include "xnvme_3p/xnvme_3p_liburing.c"
#endif

const char *xnvme_3p_ver[] = {
	xnvme_3p_fio,
#ifdef XNVME_BE_SPDK_ENABLED
	xnvme_3p_spdk,
#endif
#ifdef XNVME_BE_LINUX_ENABLED
	xnvme_3p_libnvme,
#endif
#ifdef XNVME_BE_LINUX_IOU_ENABLED
	xnvme_3p_liburing,
#endif
	0,
};

int
xnvme_3p_ver_fpr(FILE *stream, const char *ver[], enum xnvme_pr opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_3p:");

	if ((!ver) || (!ver[0])) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	for (int i = 0; xnvme_3p_ver[i]; ++i) {
		fprintf(stream, "  - '%s'\n", xnvme_3p_ver[i]);
	}

	return wrtn;
}

int
xnvme_3p_ver_pr(const char *ver[], enum xnvme_pr opts)
{
	return xnvme_3p_ver_fpr(stdout, ver, opts);
}
