/**
 * Cross-platform I/O library for emerging storage devices
 *
 * Designed for the low-level control and performance offered by NVMe device
 * while maintaining the support for traditional storage devices and
 * interfaces.
 *
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme.h
 */

#ifndef __LIBXNVME_H
#define __LIBXNVME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>

#include "libxnvme_util.h"
#include "libxnvme_opts.h"
#include "libxnvme_dev.h"
#include "libxnvme_be.h"
#include "libxnvme_buf.h"
#include "libxnvme_mem.h"
#include "libxnvme_geo.h"
#include "libxnvme_ident.h"
#include "libxnvme_queue.h"
#include "libxnvme_spec.h"
#include "libxnvme_spec_fs.h"
#include "libxnvme_spec_pp.h"
#include "libxnvme_cmd.h"
#include "libxnvme_lba.h"
#include "libxnvme_ver.h"
#include "libxnvme_pp.h"
#include "libxnvme_file.h"
#include "libxnvme_adm.h"
#include "libxnvme_nvm.h"
#include "libxnvme_kvs.h"
#include "libxnvme_znd.h"
#include "libxnvme_topology.h"
#include "libxnvme_libconf.h"
#include "libxnvme_cli.h"

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_H */
