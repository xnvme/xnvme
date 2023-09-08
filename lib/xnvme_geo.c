// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_geo.h>

static inline const char *
xnvme_geo_type_str(int type)
{
	switch (type) {
	case XNVME_GEO_UNKNOWN:
		return "XNVME_GEO_UNKNOWN";
	case XNVME_GEO_CONVENTIONAL:
		return "XNVME_GEO_CONVENTIONAL";
	case XNVME_GEO_ZONED:
		return "XNVME_GEO_ZONED";
	default:
		return "XNVME_GEO_ENOSYS";
	}
}

int
xnvme_geo_yaml(FILE *stream, const struct xnvme_geo *geo, int indent, const char *sep, int head)
{
	int wrtn = 0;

	if (head) {
		wrtn += fprintf(stream, "%*sxnvme_geo:", indent, "");
		indent += 2;
	}
	if (!geo) {
		wrtn += fprintf(stream, " ~");
		return wrtn;
	}
	if (head) {
		wrtn += fprintf(stream, "\n");
	}

	wrtn += fprintf(stream, "%*stype: %s%s", indent, "", xnvme_geo_type_str(geo->type), sep);
	wrtn += fprintf(stream, "%*snpugrp: %" PRIu32 "%s", indent, "", geo->npugrp, sep);
	wrtn += fprintf(stream, "%*snpunit: %" PRIu32 "%s", indent, "", geo->npunit, sep);
	wrtn += fprintf(stream, "%*snzone: %" PRIu32 "%s", indent, "", geo->nzone, sep);
	wrtn += fprintf(stream, "%*snsect: %" PRIu64 "%s", indent, "", geo->nsect, sep);
	wrtn += fprintf(stream, "%*snbytes: %" PRIu32 "%s", indent, "", geo->nbytes, sep);
	wrtn += fprintf(stream, "%*snbytes_oob: %" PRIu32 "%s", indent, "", geo->nbytes_oob, sep);

	wrtn += fprintf(stream, "%*stbytes: %" PRIu64 "%s", indent, "", geo->tbytes, sep);

	wrtn += fprintf(stream, "%*smdts_nbytes: %" PRIu32 "%s", indent, "", geo->mdts_nbytes,
			sep);

	wrtn += fprintf(stream, "%*slba_nbytes: %" PRIu32 "%s", indent, "", geo->lba_nbytes, sep);
	wrtn += fprintf(stream, "%*slba_extended: %" PRIu8 "%s", indent, "", geo->lba_extended,
			sep);

	wrtn += fprintf(stream, "%*sssw: %" PRIu64, indent, "", geo->ssw);

	return wrtn;
}

int
xnvme_geo_fpr(FILE *stream, const struct xnvme_geo *geo, int opts)
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

	wrtn += xnvme_geo_yaml(stream, geo, 0, "\n", 1);
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_geo_pr(const struct xnvme_geo *geo, int opts)
{
	return xnvme_geo_fpr(stdout, geo, opts);
}
