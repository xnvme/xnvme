// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <liblblk.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_spec.h>

const char *
lblk_cmd_opc_str(enum lblk_cmd_opc opc)
{
	switch (opc) {
	case LBLK_CMD_OPC_SCOPY:
		return "LBLK_CMD_OPC_SCOPY";
	}

	return "LBLK_CMD_OPC_ENOSYS";
}

const char *
lblk_status_code_str(enum lblk_status_code sc)
{
	switch (sc) {
	case LBLK_SC_WRITE_TO_RONLY:
		return "LBLK_SC_WRITE_TO_RONLY";
	}

	return "LBLK_SC_WRITE_TO_RONLY";
}

static int
lblk_source_range_entry_yaml(FILE *stream,
			     const struct lblk_source_range_entry *entry,
			     int indent, const char *sep)
{
	int wrtn = 0;

	wrtn += fprintf(stream, "%*sslba: 0x%016lx%s", indent, "",
			entry->slba, sep);

	wrtn += fprintf(stream, "%*snlb: %u%s", indent, "",
			entry->nlb, sep);

	wrtn += fprintf(stream, "%*seilbrt: 0x%08x%s", indent, "",
			entry->eilbrt, sep);

	wrtn += fprintf(stream, "%*selbatm: 0x%08x%s", indent, "",
			entry->elbatm, sep);

	wrtn += fprintf(stream, "%*selbat: 0x%08x", indent, "",
			entry->elbat);

	return wrtn;
}

int
lblk_source_range_entry_fpr(FILE *stream,
			    const struct lblk_source_range_entry *entry,
			    int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "lblk_source_range_entry:");
	if (!entry) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += lblk_source_range_entry_yaml(stream, entry, 2, "\n");
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
lblk_source_range_entry_pr(const struct lblk_source_range_entry *entry,
			   int opts)
{
	return lblk_source_range_entry_fpr(stdout, entry, opts);
}

int
lblk_source_range_fpr(FILE *stream, const struct lblk_source_range *srange,
		      uint8_t nr, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "lblk_source_range:");
	if (!srange) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  nranges: %d\n", nr + 1);
	wrtn += fprintf(stream, "  nr: %d\n", nr);
	wrtn += fprintf(stream, "  entries:\n");

	for (int i = 0; i < LBLK_SCOPY_NENTRY_MAX; ++i) {
		if ((nr) && (i > nr)) {
			break;
		}

		wrtn += fprintf(stream, "  - { ");
		wrtn += lblk_source_range_entry_yaml(stream, &srange->entry[i],
						     0, ", ");
		wrtn += fprintf(stream, " }\n");
	}

	return wrtn;
}

int
lblk_source_range_pr(const struct lblk_source_range *srange, uint8_t nr,
		     int opts)
{
	return lblk_source_range_fpr(stdout, srange, nr, opts);
}

// TODO: add yaml file and call it
int
lblk_idfy_ctrlr_fpr(FILE *stream, struct lblk_idfy_ctrlr *idfy, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "lblk_idfy_ctrlr:");
	if (!idfy) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  oncs:\n");
	wrtn += fprintf(stream, "    compare: %u\n",
			idfy->oncs.compare);
	wrtn += fprintf(stream, "    write_unc: %u\n",
			idfy->oncs.write_unc);
	wrtn += fprintf(stream, "    dsm: %u\n",
			idfy->oncs.dsm);
	wrtn += fprintf(stream, "    write_zeroes: %u\n",
			idfy->oncs.write_zeroes);
	wrtn += fprintf(stream, "    set_features_save: %u\n",
			idfy->oncs.set_features_save);
	wrtn += fprintf(stream, "    reservations: %u\n",
			idfy->oncs.reservations);
	wrtn += fprintf(stream, "    timestamp: %u\n",
			idfy->oncs.timestamp);
	wrtn += fprintf(stream, "    verify: %u\n",
			idfy->oncs.verify);
	wrtn += fprintf(stream, "    copy: %u\n",
			idfy->oncs.copy);

	wrtn += fprintf(stream, "  ocfs:\n");
	wrtn += fprintf(stream, "    copy_fmt0: %u\n",
			idfy->ocfs.copy_fmt0);

	return wrtn;
}

int
lblk_idfy_ctrlr_pr(struct lblk_idfy_ctrlr *idfy, int opts)
{
	return lblk_idfy_ctrlr_fpr(stdout, idfy, opts);
}

int
lblk_idfy_ns_fpr(FILE *stream, struct lblk_idfy_ns *idfy, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "lblk_idfy_ns:");
	if (!idfy) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  mcl: %d\n",  idfy->mcl);
	wrtn += fprintf(stream, "  mssrl: %d\n",  idfy->mssrl);
	wrtn += fprintf(stream, "  msrc: %d\n",  idfy->msrc);

	return wrtn;
}

int
lblk_idfy_ns_pr(struct lblk_idfy_ns *idfy, int opts)
{
	return lblk_idfy_ns_fpr(stdout, idfy, opts);
}

int
lblk_cmd_scopy(struct xnvme_dev *dev, uint32_t nsid, uint64_t sdlba,
	       struct lblk_source_range_entry *ranges, uint8_t nr, int opts,
	       struct xnvme_req *ret)
{
	size_t ranges_nbytes = (nr + 1) * sizeof(*ranges);
	struct lblk_cmd cmd = { 0 };

	cmd.common.opcode = LBLK_CMD_OPC_SCOPY;
	cmd.common.nsid = nsid;
	cmd.copy.sdlba = sdlba;
	cmd.copy.nr = nr;

	// TODO: consider the remaining command-fields

	return dev->be.func.cmd_pass(dev, &cmd.base, ranges, ranges_nbytes,
				     NULL, 0, opts, ret);
}

