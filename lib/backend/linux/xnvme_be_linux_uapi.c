// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libxnvme.h>
#include <xnvme_be.h>
#ifdef XNVME_PLATFORM_LINUX_ENABLED
#include <stdio.h>
#include <linux/version.h>
#include <xnvme_be_linux.h>

int
xnvme_be_linux_uapi_ver_fpr(FILE *stream, enum xnvme_pr opts)
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

	wrtn += fprintf(stream, "linux;LINUX_VERSION_CODE-UAPI/%d-%d.%d.%d", LINUX_VERSION_CODE,
			(LINUX_VERSION_CODE & (0xff << 16)) >> 16,
			(LINUX_VERSION_CODE & (0xff << 8)) >> 8, LINUX_VERSION_CODE & 0xff);

	return wrtn;
}
#else
int
xnvme_be_linux_uapi_ver_fpr(FILE *stream, enum xnvme_pr XNVME_UNUSED(opts))
{
	return fprintf(stream, "linux;LINUX_VERSION_CODE-UAPI/NOSYS\n");
}
#endif
