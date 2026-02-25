// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_platform.h>

int
xnvme_be_yaml(FILE *stream, const struct xnvme_be *be, int indent, const char *sep, int head)
{
	int wrtn = 0;

	if (head) {
		wrtn += fprintf(stream, "%*sxnvme_be:", indent, "");
		indent += 2;
	}
	if (!be) {
		wrtn += fprintf(stream, " ~");
		return wrtn;
	}
	if (head) {
		wrtn += fprintf(stream, "\n");
	}

	wrtn += fprintf(stream, "%*sdev: {id: '%s'}%s", indent, "", be->dev.id, sep);
	wrtn += fprintf(stream, "%*sadmin: {id: '%s'}%s", indent, "", be->admin.id, sep);
	wrtn += fprintf(stream, "%*ssync: {id: '%s'}%s", indent, "", be->sync.id, sep);
	wrtn += fprintf(stream, "%*sasync: {id: '%s'}%s", indent, "", be->async.id, sep);
	wrtn += fprintf(stream, "%*smem: {id: '%s'}%s", indent, "", be->mem.id, sep);
	wrtn += fprintf(stream, "%*sattr: {name: '%s', descr: '%s', caps: 0x%x}", indent, "",
			be->attr.name, be->attr.descr ? be->attr.descr : "", be->attr.caps);

	return wrtn;
}

int
xnvme_be_fpr(FILE *stream, const struct xnvme_be *be, enum xnvme_pr opts)
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

	wrtn += xnvme_be_yaml(stream, be, 2, "\n", 1);
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_be_pr(const struct xnvme_be *be, enum xnvme_pr opts)
{
	return xnvme_be_fpr(stdout, be, opts);
}

int
xnvme_be_attr_list_fpr(FILE *stream, enum xnvme_pr XNVME_UNUSED(opts))
{
	int wrtn = 0;

	wrtn += fprintf(stream, "xnvme_platform:\n");
	wrtn += fprintf(stream, "  name: '%s'\n", g_xnvme_platform->name);
	wrtn += fprintf(stream, "  drivers:\n");
	for (int i = 0; g_xnvme_platform->backends[i]; ++i) {
		const struct xnvme_be_attr *attr = &g_xnvme_platform->backends[i]->attr;
		int seen = 0;

		for (int j = 0; j < i; ++j) {
			if (!strcmp(g_xnvme_platform->backends[j]->attr.name, attr->name)) {
				seen = 1;
				break;
			}
		}
		if (seen) {
			continue;
		}

		wrtn += fprintf(stream, "  - {name: '%s', descr: '%s', caps: 0x%x}\n",
				attr->name, attr->descr ? attr->descr : "", attr->caps,
	}

	return wrtn;
}

int
xnvme_be_attr_list_pr(enum xnvme_pr opts)
{
	return xnvme_be_attr_list_fpr(stdout, opts);
}

const struct xnvme_be_attr *
xnvme_be_attr_get(int idx)
{
	for (int i = 0; g_xnvme_platform->backends[i]; ++i) {
		if (i == idx) {
			return &g_xnvme_platform->backends[i]->attr;
		}
	}

	return NULL;
}

int
xnvme_lba_fpr(FILE *stream, uint64_t lba, enum xnvme_pr opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "0x%016" PRIx64, lba);
		break;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		wrtn += fprintf(stream, "lba: 0x%016" PRIx64 "\n", lba);
		break;
	}

	return wrtn;
}

int
xnvme_lba_pr(uint64_t lba, enum xnvme_pr opts)
{
	return xnvme_lba_fpr(stdout, lba, opts);
}

int
xnvme_lba_fprn(FILE *stream, const uint64_t *lba, uint16_t nlb, enum xnvme_pr opts)
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

	wrtn += fprintf(stream, "lbas:");

	if (!lba) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");

	wrtn += fprintf(stream, "nlbas: %" PRIu16 "\n", nlb);
	wrtn += fprintf(stream, "lbas:\n");
	for (unsigned int i = 0; i < nlb; ++i) {
		wrtn += fprintf(stream, "  - ");
		xnvme_lba_pr(lba[i], XNVME_PR_TERSE);
		wrtn += fprintf(stream, "\n");
	}

	return wrtn;
}

int
xnvme_lba_prn(const uint64_t *lba, uint16_t nlb, enum xnvme_pr opts)
{
	return xnvme_lba_fprn(stdout, lba, nlb, opts);
}

int
xnvme_be_attr_fpr(FILE *stream, const struct xnvme_be_attr *attr, int opts)
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

	wrtn += fprintf(stream, "name: '%s'\n", attr->name);
	wrtn += fprintf(stream, "    descr: '%s'\n", attr->descr ? attr->descr : "");
	wrtn += fprintf(stream, "    caps: 0x%x\n", attr->caps);

	return wrtn;
}

int
xnvme_be_attr_pr(const struct xnvme_be_attr *attr, int opts)
{
	return xnvme_be_attr_fpr(stdout, attr, opts);
}
