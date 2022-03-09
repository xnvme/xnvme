// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_GEO_H
#define __INTERNAL_XNVME_GEO_H

#include <libxnvme.h>
#include <errno.h>

int
xnvme_geo_yaml(FILE *stream, const struct xnvme_geo *geo, int indent, const char *sep, int head);

#endif /* __INTERNAL_XNVME_GEO_H */
