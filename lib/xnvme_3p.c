// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <libxnvme.h>
#include <libxnvme_3p.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_windows.h>

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

	// TODO: this is kinda hackish... a better means for backends to provide
	// various introspective information should be provided
	wrtn += fprintf(stream, "  - '");
#ifdef XNVME_BE_WINDOWS_ENABLED
	wrtn += xnvme_be_windows_uapi_ver_fpr(stream, XNVME_PR_DEF);
#endif
#ifdef XNVME_BE_LINUX_ENABLED
	wrtn += xnvme_be_linux_uapi_ver_fpr(stream, XNVME_PR_DEF);
#endif
	wrtn += fprintf(stream, "'\n");

	return wrtn;
}

int
xnvme_3p_ver_pr(const char *ver[], enum xnvme_pr opts)
{
	return xnvme_3p_ver_fpr(stdout, ver, opts);
}
