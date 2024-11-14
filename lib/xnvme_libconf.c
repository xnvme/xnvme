// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <libxnvme_libconf.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_windows.h>
#ifdef XNVME_BE_LINUX_ENABLED
#include <linux/nvme_ioctl.h>
#endif
#include "xnvme_libconf_entries.c"

const struct xnvme_libconf *
xnvme_libconf_get(void)
{
	return &g_libconf;
}

int
xnvme_libconf_fpr(FILE *stream, const struct xnvme_libconf *libconf, enum xnvme_pr opts)
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

	wrtn += fprintf(stream, "xnvme_libconf:");

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  entries:\n");
	for (int i = 0; libconf->entries[i]; ++i) {
		fprintf(stream, "  - '%s'\n", libconf->entries[i]);
	}

#ifdef XNVME_BE_WINDOWS_ENABLED
	wrtn += fprintf(stream, "  - '3p: ");
	wrtn += xnvme_be_windows_uapi_ver_fpr(stream, XNVME_PR_DEF);
	wrtn += fprintf(stream, "'\n");
#endif
#ifdef XNVME_BE_LINUX_ENABLED
	wrtn += fprintf(stream, "  - '3p: ");
	wrtn += xnvme_be_linux_uapi_ver_fpr(stream, XNVME_PR_DEF);
	wrtn += fprintf(stream, "'\n");
#endif

	return wrtn;
}

int
xnvme_libconf_pr(const struct xnvme_libconf *libconf, enum xnvme_pr opts)
{
	return xnvme_libconf_fpr(stdout, libconf, opts);
}
