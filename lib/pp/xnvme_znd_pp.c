// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>

int
xnvme_znd_report_fpr(FILE *stream, const struct xnvme_znd_report *report, int flags)
{
	int wrtn = 0;

	switch (flags) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", flags);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_znd_report:\n");
	if (!report) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "  report_nbytes: %" PRIu64 "\n", report->report_nbytes);
	wrtn += fprintf(stream, "  entries_nbytes: %" PRIu64 "\n", report->entries_nbytes);

	wrtn += fprintf(stream, "  zd_nbytes: %" PRIu32 "\n", report->zd_nbytes);
	wrtn += fprintf(stream, "  zdext_nbytes: %" PRIu32 "\n", report->zdext_nbytes);
	wrtn += fprintf(stream, "  zrent_nbytes: %" PRIu64 "\n", report->zrent_nbytes);

	wrtn += fprintf(stream, "  zslba: 0x%016" PRIx64 "\n", report->zslba);
	wrtn += fprintf(stream, "  zelba: 0x%016" PRIx64 "\n", report->zelba);

	wrtn += fprintf(stream, "  nzones: %" PRIu64 "\n", report->nzones);
	wrtn += fprintf(stream, "  nentries: %" PRIu32 "\n", report->nentries);
	wrtn += fprintf(stream, "  extended: %" PRIu8 "\n", report->extended);

	wrtn += fprintf(stream, "  entries:");

	if (!report->nentries) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	for (uint32_t idx = 0; idx < report->nentries; ++idx) {
		struct xnvme_spec_znd_descr *descr;
		int err;

		err = xnvme_znd_report_get_descr(report, idx, &descr, NULL);
		if (err) {
			XNVME_DEBUG("xnvme_znd_report_get_descr(), err: %d", err);
			return err;
		}

		wrtn += fprintf(stream, "    - {");
		wrtn += xnvme_spec_znd_descr_fpr_yaml(stream, descr, 0, ", ");
		wrtn += fprintf(stream, "}\n");
	}

	return wrtn;
}

int
xnvme_znd_report_pr(const struct xnvme_znd_report *report, int flags)
{
	return xnvme_znd_report_fpr(stdout, report, flags);
}
