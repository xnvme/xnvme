// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <libxnvme.h>
#include <libxnvme_libconf.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_windows.h>
#ifdef XNVME_BE_LINUX_ENABLED
#include <linux/nvme_ioctl.h>
#endif

int
xnvme_libconf_fpr(FILE *stream, enum xnvme_pr opts)
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
	for (int i = 0; xnvme_libconf[i]; ++i) {
		fprintf(stream, "  - '%s'\n", xnvme_libconf[i]);
	}

	// TODO: this is kinda hackish... a better means for backends to provide
	// various introspective information should be provided
	wrtn += fprintf(stream, "  - '3p: ");
#ifdef XNVME_BE_WINDOWS_ENABLED
	wrtn += xnvme_be_windows_uapi_ver_fpr(stream, XNVME_PR_DEF);
#endif
#ifdef XNVME_BE_LINUX_ENABLED
	wrtn += xnvme_be_linux_uapi_ver_fpr(stream, XNVME_PR_DEF);
#endif
	wrtn += fprintf(stream, "'\n");
#ifdef XNVME_BE_LINUX_ENABLED
#ifdef NVME_IOCTL_IO64_CMD
	wrtn += fprintf(stream, "  - '3p: NVME_IOCTL_IO64_CMD'\n");
#endif
#ifdef NVME_IOCTL_IO64_CMD_VEC
	wrtn += fprintf(stream, "  - '3p: NVME_IOCTL_IO64_CMD_VEC'\n");
#endif
#ifdef NVME_IOCTL_ADMIN64_CMD
	wrtn += fprintf(stream, "  - '3p: NVME_IOCTL_ADMIN64_CMD'\n");
#endif
#endif

	return wrtn;
}

int
xnvme_libconf_pr(enum xnvme_pr opts)
{
	return xnvme_libconf_fpr(stdout, opts);
}
