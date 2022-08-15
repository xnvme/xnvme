// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>
#include <libxnvme_spec.h>
#include <libxnvme_spec_pp.h>
#include <libxnvme_util.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

static long double
bytes2double(const uint8_t bytes[], int n)
{
	long double result = 0;

	for (int i = 0; i < n; i++) {
		result *= 256;
		result += bytes[n - 1 - i];
	}
	return result;
}

int
xnvme_spec_log_health_fpr(FILE *stream, const struct xnvme_spec_log_health_entry *log, int opts)
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

	wrtn += fprintf(stream, "xnvme_spec_log_health_entry:");

	if (!log) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  crit_warn: %u\n", log->crit_warn);
	wrtn += fprintf(stream, "  comp_temp: %u\n", log->comp_temp - 273u);
	wrtn += fprintf(stream, "  avail_spare: %u\n", log->avail_spare);
	wrtn += fprintf(stream, "  avail_spare_thresh: %u\n", log->avail_spare_thresh);
	wrtn += fprintf(stream, "  pct_used: %u\n", log->pct_used);
	wrtn += fprintf(stream, "  eg_crit_warn_sum: %u\n", log->eg_crit_warn_sum);
	wrtn += fprintf(stream, "  data_units_read: %.0Lf\n",
			bytes2double(log->data_units_read, 16));
	wrtn += fprintf(stream, "  data_units_written: %.0Lf\n",
			bytes2double(log->data_units_written, 16));
	wrtn += fprintf(stream, "  host_read_cmds: %.0Lf\n",
			bytes2double(log->host_read_cmds, 16));
	wrtn += fprintf(stream, "  host_write_cmds: %.0Lf\n",
			bytes2double(log->host_write_cmds, 16));
	wrtn += fprintf(stream, "  ctrlr_busy_time: %.0Lf\n",
			bytes2double(log->ctrlr_busy_time, 16));
	wrtn += fprintf(stream, "  pwr_cycles: %.0Lf\n", bytes2double(log->pwr_cycles, 16));
	wrtn += fprintf(stream, "  pwr_on_hours: %.0Lf\n", bytes2double(log->pwr_on_hours, 16));
	wrtn += fprintf(stream, "  unsafe_shutdowns: %.0Lf\n",
			bytes2double(log->unsafe_shutdowns, 16));
	wrtn += fprintf(stream, "  nr_err_logs: %.0Lf\n", bytes2double(log->nr_err_logs, 16));
	wrtn += fprintf(stream, "  warn_comp_temp_time: %u\n", log->warn_comp_temp_time);
	wrtn += fprintf(stream, "  crit_comp_temp_time: %u\n", log->crit_comp_temp_time);
	for (int i = 0; i < 8; ++i) {
		wrtn += fprintf(stream, "  temp_sens%u: %u\n", i + 1,
				log->temp_sens[i] ? log->temp_sens[i] - 273u : 0);
	}
	wrtn += fprintf(stream, "  tmt1tc: %u\n", log->tmt1tc);
	wrtn += fprintf(stream, "  tmt2tc: %u\n", log->tmt2tc);
	wrtn += fprintf(stream, "  tttmt1: %u\n", log->tttmt1);
	wrtn += fprintf(stream, "  tttmt2: %u\n", log->tttmt2);

	return wrtn;
}

int
xnvme_spec_log_health_pr(const struct xnvme_spec_log_health_entry *log, int opts)
{
	return xnvme_spec_log_health_fpr(stdout, log, opts);
}

static inline int
log_erri_entry_fpr_yaml(FILE *stream, const struct xnvme_spec_log_erri_entry *entry, int indent,
			const char *sep)
{
	int wrtn = 0;

	wrtn += fprintf(stream, "%*secnt: %zu%s", indent, "", entry->ecnt, sep);
	wrtn += fprintf(stream, "%*ssqid: %u%s", indent, "", entry->sqid, sep);
	wrtn += fprintf(stream, "%*scid: %u%s", indent, "", entry->cid, sep);
	wrtn += fprintf(stream, "%*sstatus: %#x%s", indent, "", entry->status.val, sep);
	wrtn += fprintf(stream, "%*seloc: %#x%s", indent, "", entry->eloc, sep);
	wrtn += fprintf(stream, "%*slba: %#lx%s", indent, "", entry->lba, sep);
	wrtn += fprintf(stream, "%*snsid: %u%s", indent, "", entry->nsid, sep);
	wrtn += fprintf(stream, "%*sven_si: %#x%s", indent, "", entry->ven_si, sep);
	wrtn += fprintf(stream, "%*strtype: %#x%s", indent, "", entry->trtype, sep);
	wrtn += fprintf(stream, "%*scmd_si: %#lx%s", indent, "", entry->cmd_si, sep);
	wrtn += fprintf(stream, "%*strtype_si: %#x", indent, "", entry->trtype_si);

	return wrtn;
}

int
xnvme_spec_log_erri_entry_fpr(FILE *stream, const struct xnvme_spec_log_erri_entry *entry,
			      int opts)
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

	wrtn += fprintf(stream, "xnvme_spec_log_erri_entry:");

	if (!entry) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += log_erri_entry_fpr_yaml(stream, entry, 2, "\n");
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_spec_log_erri_entry_pr(const struct xnvme_spec_log_erri_entry *entry, int opts)
{
	return xnvme_spec_log_erri_entry_fpr(stdout, entry, opts);
}

int
xnvme_spec_log_erri_fpr(FILE *stream, const struct xnvme_spec_log_erri_entry *log, int limit,
			int opts)
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

	wrtn += fprintf(stream, "xnvme_spec_log_erri:\n");

	if (!log) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	for (int i = 0; i < limit; ++i) {
		// TODO: consider whether it should show or not... perhaps
		// optional?
		// Skip printing invalid entries, that is, ewrtn == 0
		// if (!log[i].ewrtn) {
		//	break;
		//}

		wrtn += fprintf(stream, "  - {");
		wrtn += log_erri_entry_fpr_yaml(stream, &log[i], 0, ", ");
		wrtn += fprintf(stream, "}\n");
	}

	return wrtn;
}

int
xnvme_spec_log_erri_pr(const struct xnvme_spec_log_erri_entry *log, int limit, int opts)
{
	return xnvme_spec_log_erri_fpr(stdout, log, limit, opts);
}

int
xnvme_spec_idfy_ctrl_fpr(FILE *stream, const struct xnvme_spec_idfy_ctrlr *idfy, int opts)
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

	wrtn += fprintf(stream, "xnvme_spec_idfy_ctrlr:");

	if (!idfy) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  vid: %#x\n", idfy->vid);
	wrtn += fprintf(stream, "  ssvid: %#x\n", idfy->ssvid);
	wrtn += fprintf(stream, "  sn: '%-.*s'\n",
			XNVME_MIN(XNVME_SPEC_CTRLR_SN_LEN,
				  strnlen((const char *)idfy->sn, XNVME_SPEC_CTRLR_SN_LEN)),
			idfy->sn);
	wrtn += fprintf(stream, "  mn: '%-.*s'\n",
			XNVME_MIN(XNVME_SPEC_CTRLR_MN_LEN,
				  strnlen((const char *)idfy->mn, XNVME_SPEC_CTRLR_MN_LEN)),
			idfy->mn);
	wrtn += fprintf(stream, "  fr: '%-.*s'\n",
			XNVME_MIN(XNVME_SPEC_CTRLR_FR_LEN,
				  strnlen((const char *)idfy->fr, XNVME_SPEC_CTRLR_FR_LEN)),
			idfy->fr);

	wrtn += fprintf(stream, "  rab: %d\n", idfy->rab);
	wrtn += fprintf(stream, "  ieee: %02x%02x%02x\n", idfy->ieee[2], idfy->ieee[1],
			idfy->ieee[0]);
	wrtn += fprintf(stream, "  cmic: %#x\n", idfy->cmic.val);
	wrtn += fprintf(stream, "  mdts: %d\n", idfy->mdts);
	wrtn += fprintf(stream, "  cntlid: %#x\n", idfy->cntlid);
	wrtn += fprintf(stream, "  ver: %#x\n", idfy->ver.val);
	wrtn += fprintf(stream, "  rtd3r: %#x\n", idfy->rtd3r);
	wrtn += fprintf(stream, "  rtd3e: %#x\n", idfy->rtd3e);
	wrtn += fprintf(stream, "  oaes: %#x\n", idfy->oaes.val);
	wrtn += fprintf(stream, "  ctratt: %#x\n", idfy->ctratt.val);
	wrtn += fprintf(stream, "  rrls: %#x\n", idfy->rrls);
	wrtn += fprintf(stream, "  cntrltype: %#x\n", idfy->cntrltype);
	// TODO: fguid
	wrtn += fprintf(stream, "  crdt1: %#x\n", idfy->crdt1);
	wrtn += fprintf(stream, "  crdt2: %#x\n", idfy->crdt2);
	wrtn += fprintf(stream, "  crdt3: %#x\n", idfy->crdt3);
	wrtn += fprintf(stream, "  nvmsr: %#x\n", idfy->nvmsr.val);
	wrtn += fprintf(stream, "  vwci: %#x\n", idfy->vwci.val);
	wrtn += fprintf(stream, "  mec: %#x\n", idfy->mec.val);
	wrtn += fprintf(stream, "  oacs: %#x\n", idfy->oacs.val);
	wrtn += fprintf(stream, "  acl: %d\n", idfy->acl);
	wrtn += fprintf(stream, "  aerl: %d\n", idfy->aerl);
	wrtn += fprintf(stream, "  frmw: %#x\n", idfy->frmw.val);

	wrtn += fprintf(stream, "  lpa: {");
	wrtn += fprintf(stream, " ns_smart: %u,", idfy->lpa.ns_smart);
	wrtn += fprintf(stream, " celp: %u,", idfy->lpa.celp);
	wrtn += fprintf(stream, " edlp: %u,", idfy->lpa.edlp);
	wrtn += fprintf(stream, " telemetry: %u,", idfy->lpa.telemetry);
	wrtn += fprintf(stream, " pel: %u,", idfy->lpa.pel);
	wrtn += fprintf(stream, " mel: %u,", idfy->lpa.mel);
	wrtn += fprintf(stream, " tel_da4: %u,", idfy->lpa.tel_da4);
	wrtn += fprintf(stream, " val: %#x", idfy->lpa.val);
	wrtn += fprintf(stream, "}\n");

	wrtn += fprintf(stream, "  elpe: %d\n", idfy->elpe);
	wrtn += fprintf(stream, "  npss: %d\n", idfy->npss);
	wrtn += fprintf(stream, "  avscc: %#x\n", idfy->avscc.val);
	wrtn += fprintf(stream, "  apsta: %#x\n", idfy->apsta.val);
	wrtn += fprintf(stream, "  wctemp: %d\n", idfy->wctemp);
	wrtn += fprintf(stream, "  cctemp: %d\n", idfy->cctemp);
	wrtn += fprintf(stream, "  mtfa: %d\n", idfy->mtfa);
	wrtn += fprintf(stream, "  hmpre: %d\n", idfy->hmpre);
	wrtn += fprintf(stream, "  hmmin: %d\n", idfy->hmmin);
	// TODO: present these better
	wrtn += fprintf(stream, "  tnvmcap: [%zu, %zu]\n", idfy->tnvmcap[0], idfy->tnvmcap[1]);
	wrtn += fprintf(stream, "  unvmcap: [%zu, %zu]\n", idfy->unvmcap[0], idfy->unvmcap[1]);
	wrtn += fprintf(stream, "  rpmbs: %#x\n", idfy->rpmbs.val);
	wrtn += fprintf(stream, "  edstt: %d\n", idfy->edstt);
	wrtn += fprintf(stream, "  dsto: %d\n", idfy->dsto.val);
	wrtn += fprintf(stream, "  fwug: %d\n", idfy->fwug);
	wrtn += fprintf(stream, "  kas: %d\n", idfy->kas);
	wrtn += fprintf(stream, "  hctma: %#x\n", idfy->hctma.val);
	wrtn += fprintf(stream, "  mntmt: %d\n", idfy->mntmt);
	wrtn += fprintf(stream, "  mxtmt: %d\n", idfy->mxtmt);
	wrtn += fprintf(stream, "  sanicap: %#x\n", idfy->sanicap.val);
	wrtn += fprintf(stream, "  hmminds: %d\n", idfy->hmminds);
	wrtn += fprintf(stream, "  hmmaxd: %d\n", idfy->hmmaxd);
	wrtn += fprintf(stream, "  nsetidmax: %d\n", idfy->nsetidmax);
	wrtn += fprintf(stream, "  endgidmax: %d\n", idfy->endgidmax);
	wrtn += fprintf(stream, "  anatt: %d\n", idfy->anatt);
	wrtn += fprintf(stream, "  anacap: %#x\n", idfy->anacap.val);
	wrtn += fprintf(stream, "  anagrpmax: %d\n", idfy->anagrpmax);
	wrtn += fprintf(stream, "  nanagrpid: %d\n", idfy->nanagrpid);
	wrtn += fprintf(stream, "  pels: %d\n", idfy->pels);
	wrtn += fprintf(stream, "  domain_identifier: %d\n", idfy->domain_identifier);
	// TODO: present these better
	wrtn += fprintf(stream, "  megcap: [%zu, %zu]\n", idfy->megcap[0], idfy->megcap[1]);
	wrtn += fprintf(stream, "  sqes: %#x\n", idfy->sqes.val);
	wrtn += fprintf(stream, "  cqes: %#x\n", idfy->cqes.val);
	wrtn += fprintf(stream, "  maxcmd: %d\n", idfy->maxcmd);
	wrtn += fprintf(stream, "  nn: %d\n", idfy->nn);
	wrtn += fprintf(stream, "  oncs: %#x\n", idfy->oncs.val);
	wrtn += fprintf(stream, "  fuses: %#x\n", idfy->fuses);
	wrtn += fprintf(stream, "  fna: %#x\n", idfy->fna.val);
	wrtn += fprintf(stream, "  vwc: %#x\n", idfy->vwc.val);
	wrtn += fprintf(stream, "  awun: %d\n", idfy->awun);
	wrtn += fprintf(stream, "  awupf: %d\n", idfy->awupf);
	wrtn += fprintf(stream, "  nvscc: %d\n", idfy->nvscc);
	wrtn += fprintf(stream, "  acwu: %d\n", idfy->acwu);
	wrtn += fprintf(stream, "  cdfs: %#x\n", idfy->cdfs.val);
	wrtn += fprintf(stream, "  sgls: %#x\n", idfy->sgls.val);
	wrtn += fprintf(stream, "  mnan: %d\n", idfy->mnan);
	// TODO: present these better
	wrtn += fprintf(stream, "  maxdna: [%zu, %zu]\n", idfy->maxdna[0], idfy->maxdna[1]);
	wrtn += fprintf(stream, "  maxcna: %d\n", idfy->maxcna);
	wrtn += fprintf(stream, "  subnqn: '%-.*s'\n", (int)sizeof(idfy->subnqn), idfy->subnqn);

	// TODO: add print for remaining fields
	return wrtn;
}

int
xnvme_spec_idfy_ctrl_pr(const struct xnvme_spec_idfy_ctrlr *idfy, int opts)
{
	return xnvme_spec_idfy_ctrl_fpr(stdout, idfy, opts);
}

int
xnvme_spec_idfy_ns_fpr(FILE *stream, const struct xnvme_spec_idfy_ns *idfy, int opts)
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

	wrtn += fprintf(stream, "xnvme_spec_idfy_ns:");

	if (!idfy) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  nsze: %zu\n", idfy->nsze);
	wrtn += fprintf(stream, "  ncap: %zu\n", idfy->ncap);
	wrtn += fprintf(stream, "  nuse: %zu\n", idfy->nuse);
	wrtn += fprintf(stream, "  nlbaf: %d\n", idfy->nlbaf);

	wrtn += fprintf(stream, "  nsfeat:\n");
	wrtn += fprintf(stream, "    thin_prov: %d\n", idfy->nsfeat.thin_prov);
	wrtn += fprintf(stream, "    ns_atomic_write_unit: %d\n",
			idfy->nsfeat.ns_atomic_write_unit);
	wrtn += fprintf(stream, "    dealloc_or_unwritten_error: %d\n",
			idfy->nsfeat.dealloc_or_unwritten_error);
	wrtn += fprintf(stream, "    guid_never_reused: %d\n", idfy->nsfeat.guid_never_reused);
	wrtn += fprintf(stream, "    optimal_performance: %d\n", idfy->nsfeat.optimal_performance);
	wrtn += fprintf(stream, "    reserved1: %d\n", idfy->nsfeat.reserved1);

	wrtn += fprintf(stream, "  flbas:\n");
	wrtn += fprintf(stream, "    format_lsb: %d\n", idfy->flbas.format);
	wrtn += fprintf(stream, "    extended: %d\n", idfy->flbas.extended);
	wrtn += fprintf(stream, "    format_msb: %d\n", idfy->flbas.format_msb);
	wrtn += fprintf(stream, "    reserved2: %d\n", idfy->flbas.reserved2);

	wrtn += fprintf(stream, "  mc:\n");
	wrtn += fprintf(stream, "    extended: %d\n", idfy->mc.extended);
	wrtn += fprintf(stream, "    pointer: %d\n", idfy->mc.pointer);
	wrtn += fprintf(stream, "    reserved3: %d\n", idfy->mc.reserved3);

	wrtn += fprintf(stream, "  dpc: %#x\n", idfy->dpc.val);
	wrtn += fprintf(stream, "  dps: %#x\n", idfy->dps.val);
	wrtn += fprintf(stream, "  nsrescap: %#x\n", idfy->nsrescap.val);

	wrtn += fprintf(stream, "  fpi: %#x\n", idfy->fpi.val);
	wrtn += fprintf(stream, "  dlfeat: %#x\n", idfy->dlfeat.val);

	wrtn += fprintf(stream, "  nawun: %#x\n", idfy->nawun);
	wrtn += fprintf(stream, "  nawupf: %#x\n", idfy->nawupf);
	wrtn += fprintf(stream, "  nacwu: %#x\n", idfy->nacwu);
	wrtn += fprintf(stream, "  nabsn: %#x\n", idfy->nabsn);
	wrtn += fprintf(stream, "  nabspf: %#x\n", idfy->nabspf);
	wrtn += fprintf(stream, "  noiob: %#x\n", idfy->noiob);
	wrtn += fprintf(stream, "  nvmcap:\n");
	wrtn += fprintf(stream, "    - %zu\n", idfy->nvmcap[0]);
	wrtn += fprintf(stream, "    - %zu\n", idfy->nvmcap[1]);
	wrtn += fprintf(stream, "  npwg: %#x\n", idfy->npwg);
	wrtn += fprintf(stream, "  npwa: %#x\n", idfy->npwa);
	wrtn += fprintf(stream, "  npdg: %#x\n", idfy->npdg);
	wrtn += fprintf(stream, "  npda: %#x\n", idfy->npda);
	wrtn += fprintf(stream, "  nows: %#x\n", idfy->nows);
	wrtn += fprintf(stream, "  mssrl: %d\n", idfy->mssrl);
	wrtn += fprintf(stream, "  mcl: %d\n", idfy->mcl);
	wrtn += fprintf(stream, "  msrc: %d\n", idfy->msrc);
	wrtn += fprintf(stream, "  anagrpid: %#x\n", idfy->anagrpid);
	wrtn += fprintf(stream, "  nsattr: %#x\n", idfy->nsattr);
	wrtn += fprintf(stream, "  nvmsetid: %#x\n", idfy->nvmsetid);
	wrtn += fprintf(stream, "  endgid: %#x\n", idfy->endgid);
	wrtn += fprintf(stream, "  nguid: [");
	for (int i = 0; i < 16; ++i) {
		if (i) {
			wrtn += fprintf(stream, ", ");
		}
		wrtn += fprintf(stream, "%#x", idfy->nguid[i]);
	}
	wrtn += fprintf(stream, "]\n");

	wrtn += fprintf(stream, "  eui64: %zu\n", idfy->eui64);
	wrtn += fprintf(stream, "  # ms: meta-data-size-in-nbytes\n");
	wrtn += fprintf(stream, "  # ds: data-size in 2^ds bytes\n");
	wrtn += fprintf(stream, "  # rp: relative performance 00b, 1b 10b, 11b\n");

	wrtn += fprintf(stream, "  lbaf:\n");
	for (int i = 0; i <= idfy->nlbaf; ++i) {
		wrtn += fprintf(stream, "    - {ms: %u, ds: %u, rp: %u}\n", idfy->lbaf[i].ms,
				idfy->lbaf[i].ds, idfy->lbaf[i].rp);
	}

	return wrtn;
}

int
xnvme_spec_idfy_ns_pr(const struct xnvme_spec_idfy_ns *idfy, int opts)
{
	return xnvme_spec_idfy_ns_fpr(stdout, idfy, opts);
}

int
xnvme_spec_idfy_cs_fpr(FILE *stream, const struct xnvme_spec_idfy_cs *idfy, int opts)
{
	int count = 0;
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_spec_idfy_cs:");

	if (!idfy) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	for (int i = 0; i < XNVME_SPEC_IDFY_CS_IOCSC_LEN; ++i) {
		const struct xnvme_spec_cs_vector *iocscv = &idfy->iocsc[i];

		if (!iocscv->val) {
			continue;
		}

		wrtn += fprintf(stream, "\n");
		wrtn += fprintf(stream, "  - { ");
		wrtn += fprintf(stream, "iocsci: %d, ", i);
		wrtn += fprintf(stream, "val: 0x%lx, ", iocscv->val);
		wrtn += fprintf(stream, "nvm: %d, ", iocscv->nvm);
		wrtn += fprintf(stream, "zns: %d", iocscv->zns);
		wrtn += fprintf(stream, " }");

		count++;
	}

	wrtn += fprintf(stream, count ? "\n" : " ~\n");

	return wrtn;
}

int
xnvme_spec_idfy_cs_pr(const struct xnvme_spec_idfy_cs *idfy, int opts)
{
	return xnvme_spec_idfy_cs_fpr(stdout, idfy, opts);
}

int
xnvme_spec_cmd_fpr(FILE *stream, struct xnvme_spec_cmd *cmd, int opts)
{
	int wrtn = 0;
	uint32_t *cdw = (void *)cmd;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_spec_cmd:");

	if (!cmd) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	switch (opts) {
	case 0x0:
		for (int i = 0; i < 16; ++i) {
			wrtn += fprintf(stream, "  - '" XNVME_I32_FMT "'\n",
					XNVME_I32_TO_STR(cdw[i]));
		}
		break;

	case 0x1:
		for (int i = 0; i < 16; ++i) {
			wrtn += fprintf(stream, "  - %#04x\n", cdw[i]);
		}
		break;

	case 0x2:
		for (int i = 0; i < 16; ++i) {
			wrtn += fprintf(stream, "  cdw%01d: %#04x\n", i, cdw[i]);
		}
		break;

	case 0x3:
		wrtn += fprintf(stream, "  opcode: %#02x\n", cmd->common.opcode);
		wrtn += fprintf(stream, "  fuse: %#x\n", cmd->common.fuse);
		wrtn += fprintf(stream, "  rsvd: %#x\n", cmd->common.rsvd);
		wrtn += fprintf(stream, "  psdt: %#x\n", cmd->common.psdt);
		wrtn += fprintf(stream, "  cid: %#04x\n", cmd->common.cid);
		wrtn += fprintf(stream, "  nsid: %#04x\n", cmd->common.nsid);
		wrtn += fprintf(stream, "  cdw02: %#04x\n", cdw[2]);
		wrtn += fprintf(stream, "  cdw03: %#04x\n", cdw[3]);
		wrtn += fprintf(stream, "  mptr: %#08lx\n", cmd->common.mptr);
		wrtn += fprintf(stream, "  prp1: %#08lx\n", cmd->common.dptr.prp.prp1);
		wrtn += fprintf(stream, "  prp2: %#08lx\n", cmd->common.dptr.prp.prp2);
		wrtn += fprintf(stream, "  cdw10: %#04x\n", cdw[10]);
		wrtn += fprintf(stream, "  cdw11: %#04x\n", cdw[11]);
		wrtn += fprintf(stream, "  cdw12: %#04x\n", cdw[12]);
		wrtn += fprintf(stream, "  cdw13: %#04x\n", cdw[13]);
		wrtn += fprintf(stream, "  cdw14: %#04x\n", cdw[14]);
		wrtn += fprintf(stream, "  cdw15: %#04x\n", cdw[15]);
		break;
	}

	return wrtn;
}

int
xnvme_spec_cmd_pr(struct xnvme_spec_cmd *cmd, int opts)
{
	return xnvme_spec_cmd_fpr(stdout, cmd, opts);
}

int
xnvme_spec_feat_fpr(FILE *stream, uint8_t fid, struct xnvme_spec_feat feat, int opts)
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

	switch (fid) {
	case XNVME_SPEC_FEAT_ERROR_RECOVERY:
		wrtn += fprintf(stream, "feat: {dulbe: %x, tler: %x)\n", feat.error_recovery.dulbe,
				feat.error_recovery.tler);
		return wrtn;

	case XNVME_SPEC_FEAT_TEMP_THRESHOLD:
		wrtn += fprintf(stream,
				"feat: {"
				"tmpth: %u, tmpsel: 0x%x, thsel: 0x%x}\n",
				feat.temp_threshold.tmpth, feat.temp_threshold.tmpsel,
				feat.temp_threshold.thsel);
		return wrtn;

	case XNVME_SPEC_FEAT_NQUEUES:
		wrtn += fprintf(stream, "feat: { nsqa: %u, ncqa: %u }\n", feat.nqueues.nsqa,
				feat.nqueues.ncqa);
		return wrtn;

	case XNVME_SPEC_FEAT_ARBITRATION:
	case XNVME_SPEC_FEAT_PWR_MGMT:
	case XNVME_SPEC_FEAT_LBA_RANGETYPE:
	default:
		wrtn += fprintf(stream, "# ENOSYS: printer not implemented for fid: %x", fid);
	}

	return wrtn;
}

int
xnvme_spec_feat_pr(uint8_t fid, struct xnvme_spec_feat feat, int opts)
{
	return xnvme_spec_feat_fpr(stdout, fid, feat, opts);
}

static int
lblk_scopy_fmt_zero_yaml(FILE *stream, const struct xnvme_spec_nvm_scopy_fmt_zero *entry,
			 int indent, const char *sep)
{
	int wrtn = 0;

	wrtn += fprintf(stream, "%*sslba: 0x%016lx%s", indent, "", entry->slba, sep);

	wrtn += fprintf(stream, "%*snlb: %u%s", indent, "", entry->nlb, sep);

	wrtn += fprintf(stream, "%*seilbrt: 0x%08x%s", indent, "", entry->eilbrt, sep);

	wrtn += fprintf(stream, "%*selbatm: 0x%08x%s", indent, "", entry->elbatm, sep);

	wrtn += fprintf(stream, "%*selbat: 0x%08x", indent, "", entry->elbat);

	return wrtn;
}

int
xnvme_spec_nvm_scopy_fmt_zero_fpr(FILE *stream, const struct xnvme_spec_nvm_scopy_fmt_zero *entry,
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

	wrtn += fprintf(stream, "xnvme_spec_nvm_scopy_fmt_zero:");
	if (!entry) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += lblk_scopy_fmt_zero_yaml(stream, entry, 2, "\n");
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_spec_nvm_scopy_fmt_zero_pr(const struct xnvme_spec_nvm_scopy_fmt_zero *entry, int opts)
{
	return xnvme_spec_nvm_scopy_fmt_zero_fpr(stdout, entry, opts);
}

int
xnvme_spec_nvm_scopy_source_range_fpr(FILE *stream,
				      const struct xnvme_spec_nvm_scopy_source_range *srange,
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

	wrtn += fprintf(stream, "xnvme_spec_nvm_scopy_source_range:");
	if (!srange) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  nranges: %d\n", nr + 1);
	wrtn += fprintf(stream, "  nr: %d\n", nr);
	wrtn += fprintf(stream, "  entries:\n");

	for (int i = 0; i < XNVME_SPEC_NVM_SCOPY_NENTRY_MAX; ++i) {
		if (i > nr) {
			break;
		}

		wrtn += fprintf(stream, "  - { ");
		wrtn += lblk_scopy_fmt_zero_yaml(stream, &srange->entry[i], 0, ", ");
		wrtn += fprintf(stream, " }\n");
	}

	return wrtn;
}

int
xnvme_spec_nvm_scopy_source_range_pr(const struct xnvme_spec_nvm_scopy_source_range *srange,
				     uint8_t nr, int opts)
{
	return xnvme_spec_nvm_scopy_source_range_fpr(stdout, srange, nr, opts);
}

// TODO: add yaml file and call it
int
xnvme_spec_idfy_ctrlr_fpr(FILE *stream, struct xnvme_spec_nvm_idfy_ctrlr *idfy, int opts)
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

	wrtn += fprintf(stream, "xnvme_spec_nvm_idfy_ctrlr:");
	if (!idfy) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  oncs:\n");
	wrtn += fprintf(stream, "    compare: %u\n", idfy->oncs.compare);
	wrtn += fprintf(stream, "    write_unc: %u\n", idfy->oncs.write_unc);
	wrtn += fprintf(stream, "    dsm: %u\n", idfy->oncs.dsm);
	wrtn += fprintf(stream, "    write_zeroes: %u\n", idfy->oncs.write_zeroes);
	wrtn += fprintf(stream, "    set_features_save: %u\n", idfy->oncs.set_features_save);
	wrtn += fprintf(stream, "    reservations: %u\n", idfy->oncs.reservations);
	wrtn += fprintf(stream, "    timestamp: %u\n", idfy->oncs.timestamp);
	wrtn += fprintf(stream, "    verify: %u\n", idfy->oncs.verify);
	wrtn += fprintf(stream, "    copy: %u\n", idfy->oncs.copy);

	wrtn += fprintf(stream, "  ocfs:\n");
	wrtn += fprintf(stream, "    copy_fmt0: %u\n", idfy->ocfs.copy_fmt0);

	return wrtn;
}

int
xnvme_spec_nvm_idfy_ctrlr_pr(struct xnvme_spec_nvm_idfy_ctrlr *idfy, int opts)
{
	return xnvme_spec_idfy_ctrlr_fpr(stdout, idfy, opts);
}

int
xnvme_spec_nvm_idfy_ns_fpr(FILE *stream, struct xnvme_spec_nvm_idfy_ns *idfy, int opts)
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

	wrtn += fprintf(stream, "xnvme_spec_nvm_idfy_ns:");
	if (!idfy) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  mcl: %d\n", idfy->mcl);
	wrtn += fprintf(stream, "  mssrl: %d\n", idfy->mssrl);
	wrtn += fprintf(stream, "  msrc: %d\n", idfy->msrc);

	return wrtn;
}

int
xnvme_spec_nvm_idfy_ns_pr(struct xnvme_spec_nvm_idfy_ns *idfy, int opts)
{
	return xnvme_spec_nvm_idfy_ns_fpr(stdout, idfy, opts);
}

int
xnvme_spec_drecv_idfy_fpr(FILE *stream, struct xnvme_spec_idfy_dir_rp *idfy, int opts)
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

	wrtn += fprintf(stream, "xnvme_spec_idfy_dir_rp:");
	if (!idfy) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  directives_supported:\n");
	wrtn += fprintf(stream, "    identify: %d\n", idfy->directives_supported.identify);
	wrtn += fprintf(stream, "    streams: %d\n", idfy->directives_supported.streams);
	wrtn += fprintf(stream, "  directives_enabled:\n");
	wrtn += fprintf(stream, "    identify: %d\n", idfy->directives_enabled.identify);
	wrtn += fprintf(stream, "    streams: %d\n", idfy->directives_enabled.streams);

	return wrtn;
}

int
xnvme_spec_drecv_idfy_pr(struct xnvme_spec_idfy_dir_rp *idfy, int opts)
{
	return xnvme_spec_drecv_idfy_fpr(stdout, idfy, opts);
}

int
xnvme_spec_drecv_srp_fpr(FILE *stream, struct xnvme_spec_streams_dir_rp *srp, int opts)
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

	wrtn += fprintf(stream, "xnvme_spec_streams_dir_rp:");
	if (!srp) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  msl: %d\n", srp->msl);
	wrtn += fprintf(stream, "  nssa: %d\n", srp->nssa);
	wrtn += fprintf(stream, "  nsso: %d\n", srp->nsso);
	wrtn += fprintf(stream, "  multi_host: %d\n", srp->nssc.bits.multi_host);
	wrtn += fprintf(stream, "  sws: %d\n", srp->sws);
	wrtn += fprintf(stream, "  sgs: %d\n", srp->sgs);
	wrtn += fprintf(stream, "  nsa: %d\n", srp->nsa);
	wrtn += fprintf(stream, "  nso: %d\n", srp->nso);

	return wrtn;
}

int
xnvme_spec_drecv_srp_pr(struct xnvme_spec_streams_dir_rp *srp, int opts)
{
	return xnvme_spec_drecv_srp_fpr(stdout, srp, opts);
}

int
xnvme_spec_drecv_sgs_fpr(FILE *stream, struct xnvme_spec_streams_dir_gs *sgs, int opts)
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

	wrtn += fprintf(stream, "xnvme_spec_streams_dir_rp:");
	if (!sgs) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  open_sc: %d\n", sgs->open_sc);
	for (int osc = 0; osc < sgs->open_sc; ++osc) {
		wrtn += fprintf(stream, "  - sid[%d]: %d\n", osc, sgs->sid[osc]);
	}

	return wrtn;
}

int
xnvme_spec_drecv_sgs_pr(struct xnvme_spec_streams_dir_gs *sgs, int opts)
{
	return xnvme_spec_drecv_sgs_fpr(stdout, sgs, opts);
}

int
xnvme_spec_drecv_sar_fpr(FILE *stream, struct xnvme_spec_alloc_resource sar, int opts)
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

	wrtn += fprintf(stream, "xnvme_spec_alloc_resource:");
	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  nsa: %d\n", sar.bits.nsa);

	return wrtn;
}

int
xnvme_spec_drecv_sar_pr(struct xnvme_spec_alloc_resource sar, int opts)
{
	return xnvme_spec_drecv_sar_fpr(stdout, sar, opts);
}

int
xnvme_spec_znd_descr_fpr_yaml(FILE *stream, const struct xnvme_spec_znd_descr *descr, int indent,
			      const char *sep)
{
	int wrtn = 0;

	wrtn += fprintf(stream, "%*szslba: 0x%016lx%s", indent, "", descr->zslba, sep);

	wrtn += fprintf(stream, "%*swp: 0x%016lx%s", indent, "", descr->wp, sep);

	wrtn += fprintf(stream, "%*szcap: %zu%s", indent, "", descr->zcap, sep);

	wrtn += fprintf(stream, "%*szt: %#x%s", indent, "", descr->zt, sep);

	wrtn += fprintf(stream, "%*szs: %*s%s", indent, "", 17,
			xnvme_spec_znd_state_str(descr->zs), sep);

	wrtn += fprintf(stream, "%*sza: '0b" XNVME_I8_FMT "'", indent, "",
			XNVME_I8_TO_STR(descr->za.val));

	return wrtn;
}

int
xnvme_spec_znd_descr_fpr(FILE *stream, const struct xnvme_spec_znd_descr *descr, int opts)
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

	wrtn += fprintf(stream, "xnvme_spec_znd_descr:");
	if (!descr) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += xnvme_spec_znd_descr_fpr_yaml(stream, descr, 2, "\n");
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_spec_znd_descr_pr(const struct xnvme_spec_znd_descr *descr, int opts)
{
	return xnvme_spec_znd_descr_fpr(stdout, descr, opts);
}

int
xnvme_spec_znd_log_changes_fpr(FILE *stream, const struct xnvme_spec_znd_log_changes *changes,
			       int opts)
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

	wrtn += fprintf(stream, "xnvme_spec_znd_log_changes:");
	if (!changes) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  nidents: %u\n", changes->nidents);

	if (changes->nidents) {
		return wrtn;
	}

	wrtn += fprintf(stream, "  idents:");
	if (!changes->nidents) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	for (uint16_t idx = 0; idx < changes->nidents; ++idx) {
		wrtn += fprintf(stream, "    - 0x%016lx\n", changes->idents[idx]);
	}

	return wrtn;
}

int
xnvme_spec_znd_log_changes_pr(const struct xnvme_spec_znd_log_changes *changes, int opts)
{
	return xnvme_spec_znd_log_changes_fpr(stdout, changes, opts);
}

int
xnvme_spec_znd_report_hdr_fpr(FILE *stream, const struct xnvme_spec_znd_report_hdr *hdr, int opts)
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

	wrtn += fprintf(stream, "xnvme_spec_znd_report_hdr:");
	if (!hdr) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  nzones: %zu\n", hdr->nzones);
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_spec_znd_report_hdr_pr(const struct xnvme_spec_znd_report_hdr *hdr, int opts)
{
	return xnvme_spec_znd_report_hdr_fpr(stdout, hdr, opts);
}

int
xnvme_spec_znd_idfy_ctrlr_fpr(FILE *stream, struct xnvme_spec_znd_idfy_ctrlr *zctrlr, int opts)
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

	wrtn += fprintf(stream, "xnvme_spec_znd_idfy_ctrlr:");
	if (!zctrlr) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  zasl: %d\n", zctrlr->zasl);

	return wrtn;
}

int
xnvme_spec_znd_idfy_ctrlr_pr(struct xnvme_spec_znd_idfy_ctrlr *zctrlr, int opts)
{
	return xnvme_spec_znd_idfy_ctrlr_fpr(stdout, zctrlr, opts);
}

int
xnvme_spec_znd_idfy_lbafe_fpr(FILE *stream, struct xnvme_spec_znd_idfy_lbafe *zonef, int opts)
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

	if (!zonef) {
		wrtn += fprintf(stream, "~");
		return wrtn;
	}

	wrtn += fprintf(stream, "{ zsze: %zu, zdes: %d }", zonef->zsze, zonef->zdes);

	return wrtn;
}

int
xnvme_spec_znd_idfy_ns_fpr(FILE *stream, struct xnvme_spec_znd_idfy_ns *zns, int opts)
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

	wrtn += fprintf(stream, "xnvme_spec_znd_idfy_ns:");
	if (!zns) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  zoc: { vzcap: %d, zae: %d }\n", zns->zoc.bits.vzcap,
			zns->zoc.bits.zae);

	wrtn += fprintf(stream, "  ozcs:\n");
	wrtn += fprintf(stream, "    val: 0x%x\n", zns->ozcs.val);
	wrtn += fprintf(stream, "    razb: %d\n", zns->ozcs.bits.razb);
	wrtn += fprintf(stream, "    zrwasup: %d\n", zns->ozcs.bits.zrwasup);

	wrtn += fprintf(stream, "  mar: %u\n", zns->mar);
	wrtn += fprintf(stream, "  mor: %u\n", zns->mor);

	wrtn += fprintf(stream, "  rrl: %u\n", zns->rrl);
	wrtn += fprintf(stream, "  frl: %u\n", zns->frl);

	wrtn += fprintf(stream, "  lbafe:\n");
	for (int nfmt = 0; nfmt < 16; ++nfmt) {
		wrtn += fprintf(stream, "  - ");
		wrtn += xnvme_spec_znd_idfy_lbafe_fpr(stream, &zns->lbafe[nfmt], opts);
		wrtn += fprintf(stream, "\n");
	}

	wrtn += fprintf(stream, "  numzrwa: %u\n", zns->numzrwa);
	wrtn += fprintf(stream, "  zrwas: %u\n", zns->zrwas);
	wrtn += fprintf(stream, "  zrwafg: %u\n", zns->zrwafg);

	wrtn += fprintf(stream, "  zrwacap:\n");
	wrtn += fprintf(stream, "    val: 0x%x\n", zns->zrwacap.val);
	wrtn += fprintf(stream, "    expflushsup: %d\n", zns->zrwacap.bits.expflushsup);

	return wrtn;
}

int
xnvme_spec_znd_idfy_ns_pr(struct xnvme_spec_znd_idfy_ns *zns, int opts)
{
	return xnvme_spec_znd_idfy_ns_fpr(stdout, zns, opts);
}
