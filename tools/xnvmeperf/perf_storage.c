// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>

#include "xnvmeperf.h"

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_xnvmeperf_storage_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
