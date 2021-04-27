// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <paths.h>
#include <ctype.h>
#include <errno.h>
#include <libxnvme.h>
#include <libxnvme_spec_fs.h>
#include <libxnvme_adm.h>
#include <libxnvme_file.h>
#include <libxnvme_znd.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

static struct xnvme_be *g_xnvme_be_registry[] = {
	&xnvme_be_spdk,
	&xnvme_be_linux,
	&xnvme_be_fbsd,
	&xnvme_be_posix,
	NULL
};
static int g_xnvme_be_count = sizeof g_xnvme_be_registry / sizeof * g_xnvme_be_registry - 1;

int
xnvme_be_yaml(FILE *stream, const struct xnvme_be *be, int indent,
	      const char *sep, int head)
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

	/*
	wrtn += fprintf(stream, "%*sdev: {id: '%s'}%s", indent, "", be->dev.id, sep);
	wrtn += fprintf(stream, "%*smem: {id: '%s'}%s", indent, "", be->mem.id, sep);
	*/
	wrtn += fprintf(stream, "%*sadmin: {id: '%s'}%s", indent, "", be->admin.id, sep);
	wrtn += fprintf(stream, "%*ssync: {id: '%s'}%s", indent, "", be->sync.id, sep);
	wrtn += fprintf(stream, "%*sasync: {id: '%s'}%s", indent, "", be->async.id, sep);
	wrtn += fprintf(stream, "%*sattr: {name: '%s'}", indent, "", be->attr.name);

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

bool
xnvme_ident_opt_to_val(const struct xnvme_ident *ident, const char *opt, uint32_t *val)
{
	const char *ofz = NULL;
	char fmt[100] = { 0 };

	ofz = strstr(ident->opts, opt);
	if (!ofz) {
		return false;
	}

	sprintf(fmt, "%s=%%x", opt);

	return sscanf(ofz, fmt, val) == 1;
}

bool
xnvme_ident_optval_to_buf(const char *opts, const char *opt, char *buf, uint32_t buf_len)
{
	char haystack[XNVME_IDENT_OPTS_LEN] = { 0 };

	if (strlen(opts) > (XNVME_IDENT_OPTS_LEN - 1)) {
		XNVME_DEBUG("FAILED: len(opts) out-of-bounds");
		return false;
	}
	if (strlen(opt) > XNVME_IDENT_OPT_MAX) {
		XNVME_DEBUG("FAILED: len(opt) out-of-bounds");
		return false;
	}
	/*
	if (buf_len < (strlen(opt) + 32)) {
		return false;
	}*/

	snprintf(haystack, XNVME_IDENT_OPTS_LEN, "%s", opts);

	for (char *tok = strtok(haystack, "?"); tok; tok = strtok(NULL, "?")) {
		char fmt[100] = { 0 };

		snprintf(fmt, sizeof(fmt), "%s=%%%d[a-z_0123456789]", opt, (int)(buf_len - 1));

		if (sscanf(tok, fmt, buf) == 1) {
			return true;
		}
	}

	return false;
}

bool
check_cmask_validity(const char *cmask, int nproc)
{
	char string[100];

	if (sscanf(cmask, "%s", string) != 0) {
		return false;
	}

	char *token = strtok(string, "[,-]");
	while (token != NULL) {

		for (size_t i = 0; i < strlen(token); i ++) {
			if (!isdigit(token[i])) {
				goto not_a_number;
			}
		}

		if (atoi(token) >= nproc) {
			goto fail;
		}

		token = strtok(NULL, "[,-]");
	}

	return true;

fail:

not_a_number:
	return false;
}

int
xnvme_be_options_from_ident(const struct xnvme_ident *ident, struct xnvme_be_options *opts)
{
	char optstr[XNVME_BE_OPTION_CHAR_LEN] = { 0 };
	uint32_t optv = 0;

	memset(opts, 0, sizeof(*opts));

	// File open flags
	if ((optv = 0) || (xnvme_ident_opt_to_val(ident, "create", &optv) && (optv == 1))) {
		opts->oflags |= XNVME_FILE_OFLG_CREATE;
	}
	if ((optv = 0) || xnvme_ident_opt_to_val(ident, "direct", &optv)) {
		if (optv == 1) {
			opts->oflags |= XNVME_FILE_OFLG_DIRECT_ON;
		} else {
			opts->oflags |= XNVME_FILE_OFLG_DIRECT_OFF;
		}
	} else {
		opts->oflags |= XNVME_FILE_OFLG_DIRECT_ON;
	}

	if ((optv = 0) || (xnvme_ident_opt_to_val(ident, "rdonly", &optv) && (optv == 1))) {
		opts->oflags |= XNVME_FILE_OFLG_RDONLY;
	} else if ((optv = 0) || (xnvme_ident_opt_to_val(ident, "wronly", &optv) && (optv == 1))) {
		opts->oflags |= XNVME_FILE_OFLG_WRONLY;
	} else if ((optv = 0) || (xnvme_ident_opt_to_val(ident, "rdwr", &optv) && (optv == 1))) {
		opts->oflags |= XNVME_FILE_OFLG_RDWR;
	} else {	// None explicitly chosen, set a default
		opts->oflags |= XNVME_FILE_OFLG_RDWR;
	}

	if ((optv = 0) || (xnvme_ident_opt_to_val(ident, "trunc", &optv) && (optv == 1))) {
		opts->oflags |= XNVME_FILE_OFLG_TRUNC;
	}
	opts->provided.oflags = 1;	// A default is always set

	// File creation mode when XNVME_FILE_OFLG_CREATE is provided
	memset(optstr, 0, sizeof(optstr));
	if (xnvme_ident_optval_to_buf(ident->opts, "mode", optstr, sizeof(optstr) - 1)) {
		opts->mode = strtol(optstr, NULL, 8);
		opts->provided.mode = 1;
	}

	// io_uring
	if ((optv = 0) || xnvme_ident_opt_to_val(ident, "poll_io", &optv)) {
		opts->poll_io = (optv == 1);
		opts->provided.poll_io = 1;
	}
	if ((optv = 0) || xnvme_ident_opt_to_val(ident, "poll_sq", &optv)) {
		opts->poll_sq = (optv == 1);
		opts->provided.poll_sq = 1;
	}

	// SPDK options
	if ((optv = 0) || xnvme_ident_opt_to_val(ident, "nsid", &optv)) {
		opts->nsid = optv;
		opts->provided.nsid = 1;
		XNVME_DEBUG("INFO: opts->nsid: %d", opts->nsid);
	}
	if ((optv = 0) || xnvme_ident_opt_to_val(ident, "css", &optv)) {
		opts->css = optv;
		opts->provided.css = 1;
	}
	if ((optv = 0) || xnvme_ident_opt_to_val(ident, "cmb_sqs", &optv)) {
		opts->cmb_sqs = (optv == 1);
		opts->provided.cmb_sqs = 1;
	}
	if ((optv = 0) || xnvme_ident_opt_to_val(ident, "shm_id", &optv)) {
		opts->shm_id = optv;
		opts->provided.shm_id = 1;
	}

	memset(optstr, 0, sizeof(optstr));
	if (xnvme_ident_optval_to_buf(ident->opts, "corelist", optstr, sizeof(optstr) - 1)) {
		memcpy(opts->corelist, optstr, XNVME_BE_OPTION_CHAR_LEN - 1);
		opts->provided.corelist = 1;
	}
	memset(optstr, 0, sizeof(optstr));
	if (xnvme_ident_optval_to_buf(ident->opts, "adrfam", optstr, sizeof(optstr) - 1)) {
		memcpy(opts->adrfam, optstr, XNVME_BE_OPTION_CHAR_LEN - 1);
		opts->provided.adrfam = 1;
	}

	// Backend interface options
	if (xnvme_ident_optval_to_buf(ident->opts, "admin", opts->admin,
				      XNVME_BE_MIXIN_NAME_LEN - 1)) {
		opts->provided.admin = 1;
	}
	if (xnvme_ident_optval_to_buf(ident->opts, "dev", opts->dev,
				      XNVME_BE_MIXIN_NAME_LEN - 1)) {
		opts->provided.dev = 1;
	}
	if (xnvme_ident_optval_to_buf(ident->opts, "mem", opts->mem,
				      XNVME_BE_MIXIN_NAME_LEN - 1)) {
		opts->provided.mem = 1;
	}
	if (xnvme_ident_optval_to_buf(ident->opts, "async", opts->async,
				      XNVME_BE_MIXIN_NAME_LEN - 1)) {
		opts->provided.async = 1;
	}
	if (xnvme_ident_optval_to_buf(ident->opts, "sync", opts->sync,
				      XNVME_BE_MIXIN_NAME_LEN - 1)) {
		opts->provided.sync = 1;
	}

	return 0;
}

int
path_to_ll(const char *path, uint64_t *val)
{
	int buf_len = 0x1000;
	char buf[buf_len];
	FILE *fp;
	int c;
	int base = 10;

	fp = fopen(path, "rb");
	if (!fp) {
		return -errno;
	}

	memset(buf, 0, sizeof(char) * buf_len);
	for (int i = 0; (((c = getc(fp)) != EOF) && i < buf_len); ++i) {
		buf[i] = c;
	}

	fclose(fp);

	if ((strlen(buf) > 2) && (buf[0] == '0') && (buf[1] == 'x')) {
		base = 16;
	}

	*val = strtoll(buf, NULL, base);

	return 0;
}

int
xnvme_lba_fpr(FILE *stream, uint64_t lba, enum xnvme_pr opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "0x%016lx", lba);
		break;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		wrtn += fprintf(stream, "lba: 0x%016lx\n", lba);
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
xnvme_lba_fprn(FILE *stream, const uint64_t *lba, uint16_t nlb,
	       enum xnvme_pr opts)
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

	wrtn += fprintf(stream, "nlbas: %d\n", nlb);
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
xnvme_be_attr_list_bundled(struct xnvme_be_attr_list **list)
{
	const size_t list_nbytes = sizeof(**list) + g_xnvme_be_count * \
				   sizeof((*list)->item[0]);

	*list = malloc(list_nbytes);
	if (!*list) {
		return -1;
	}

	(*list)->count = g_xnvme_be_count;
	(*list)->capacity = g_xnvme_be_count;
	for (int i = 0; i < g_xnvme_be_count; ++i) {
		(*list)->item[i] = g_xnvme_be_registry[i]->attr;
	}

	return 0;
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
	wrtn += fprintf(stream, "    enabled: %d\n", attr->enabled);

	wrtn += fprintf(stream, "    schemes: [");
	for (uint8_t i = 0; i < attr->nschemes; ++i) {
		if (i) {
			wrtn += fprintf(stream, ", ");
		}

		wrtn += fprintf(stream, "'%s'", attr->schemes[i]);
	}
	wrtn += fprintf(stream, "]\n");

	return wrtn;
}

int
xnvme_be_attr_pr(const struct xnvme_be_attr *attr, int opts)
{
	return xnvme_be_attr_fpr(stdout, attr, opts);
}

int
xnvme_be_attr_list_fpr(FILE *stream, const struct xnvme_be_attr_list *list,
		       int opts)
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

	wrtn += fprintf(stream, "xnvme_be_attr_list:\n");
	wrtn += fprintf(stream, "  count: %d\n", list->count);
	wrtn += fprintf(stream, "  capacity: %d\n", list->capacity);
	wrtn += fprintf(stream, "  items:");

	if (!list->count) {
		wrtn += fprintf(stream, "~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	for (int i = 0; i < list->count; ++i) {
		wrtn += fprintf(stream, "  - ");
		wrtn += xnvme_be_attr_fpr(stream, &g_xnvme_be_registry[i]->attr, opts);
		wrtn += fprintf(stream, "\n");
	}

	return wrtn;
}

int
xnvme_be_attr_list_pr(const struct xnvme_be_attr_list *list, int opts)
{
	return xnvme_be_attr_list_fpr(stdout, list, opts);
}

static inline int
_zoned_geometry(struct xnvme_dev *dev)
{
	struct xnvme_spec_idfy_ns *nvm = (void *)xnvme_dev_get_ns(dev);
	struct xnvme_spec_lbaf *lbaf = &nvm->lbaf[nvm->flbas.format];
	struct xnvme_spec_znd_idfy_ns *zns = (void *)xnvme_dev_get_ns_css(dev);
	struct xnvme_spec_znd_idfy_lbafe *lbafe = &zns->lbafe[nvm->flbas.format];
	struct xnvme_geo *geo = &dev->geo;
	uint64_t nzones;
	int err;

	if (!zns->lbafe[0].zsze) {
		XNVME_DEBUG("FAILED: !zns.lbafe[0].zsze");
		return -EINVAL;
	}

	err = xnvme_znd_stat(dev, XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_ALL, &nzones);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_znd_mgmt_recv()");
		return err;
	}

	geo->type = XNVME_GEO_ZONED;

	geo->npugrp = 1;
	geo->npunit = 1;
	geo->nzone = nzones;
	geo->nsect = lbafe->zsze;

	geo->nbytes = 2 << (lbaf->ds - 1);
	geo->nbytes_oob = lbaf->ms;

	geo->lba_nbytes = geo->nbytes;
	geo->lba_extended = nvm->flbas.extended && lbaf->ms;
	if (geo->lba_extended) {
		geo->lba_nbytes += geo->nbytes_oob;
	}

	return 0;
}

// TODO: select LBAF correctly, instead of the first
static inline int
_conventional_geometry(struct xnvme_dev *dev)
{
	const struct xnvme_spec_idfy_ns *nvm = (void *)xnvme_dev_get_ns(dev);
	const struct xnvme_spec_lbaf *lbaf = &nvm->lbaf[nvm->flbas.format];
	struct xnvme_geo *geo = &dev->geo;

	geo->type = XNVME_GEO_CONVENTIONAL;

	geo->npugrp = 1;
	geo->npunit = 1;
	geo->nzone = 1;

	geo->nsect = dev->id.ns.nsze;
	geo->nbytes = 2 << (lbaf->ds - 1);
	geo->nbytes_oob = lbaf->ms;

	geo->lba_nbytes = geo->nbytes;
	geo->lba_extended = nvm->flbas.extended && lbaf->ms;
	if (geo->lba_extended) {
		geo->lba_nbytes += geo->nbytes_oob;
	}

	return 0;
}

static inline int
_fs_geometry(struct xnvme_dev *dev)
{
	const struct xnvme_spec_fs_idfy_ns *ns = (void *)xnvme_dev_get_ns_css(dev);
	struct xnvme_geo *geo = &dev->geo;

	geo->type = XNVME_GEO_CONVENTIONAL;

	geo->npugrp = 1;
	geo->npunit = 1;
	geo->nzone = 1;
	geo->nsect = 1;

	geo->nbytes = 1;
	geo->nbytes_oob = 0;

	geo->lba_nbytes = 512;
	geo->lba_extended = 0;

	geo->tbytes = ns->nuse;

	geo->mdts_nbytes = 1 << 20;
	geo->ssw = XNVME_ILOG2(geo->lba_nbytes);

	return 0;
}

// TODO: add proper handling of NVMe controllers
int
xnvme_be_dev_derive_geometry(struct xnvme_dev *dev)
{
	struct xnvme_geo *geo = &dev->geo;

	switch (dev->dtype) {
	case XNVME_DEV_TYPE_NVME_CONTROLLER:
		XNVME_DEBUG("FAILED: not supported");
		return -ENOSYS;

	case XNVME_DEV_TYPE_FS_FILE:
		return _fs_geometry(dev);

	case XNVME_DEV_TYPE_BLOCK_DEVICE:
	case XNVME_DEV_TYPE_NVME_NAMESPACE:
		switch (dev->csi) {
		case XNVME_SPEC_CSI_ZONED:
			if (_zoned_geometry(dev)) {
				XNVME_DEBUG("FAILED: _zoned_geometry");
				return -EINVAL;
			}
			break;

		case XNVME_SPEC_CSI_NVM:
			if (_conventional_geometry(dev)) {
				XNVME_DEBUG("FAILED: _conventional_geometry");
				return -EINVAL;
			}
			break;

		default:
			XNVME_DEBUG("FAILED: unhandled csi: 0x%x", dev->csi);
			return -ENOSYS;
		}
		break;

	default:
		XNVME_DEBUG("FAILED: unhandled dtype: 0x%x", dev->dtype);
		return -ENOSYS;
	}

	geo->tbytes = (unsigned long)(geo->npugrp) * geo->npunit * geo->nzone * geo->nsect * \
		      geo->nbytes;

	/* Derive the sector-shift-width for LBA mapping */
	geo->ssw = XNVME_ILOG2(dev->geo.nbytes);

	//
	// If the controller reports that MDTS is unbounded, that is, it can be
	// infinitely large then we cap it here to something that just might
	// exists in finite physical space and time withing this realm of
	// what we perceive as reality in our universe. Note, that this might
	// change with quantum devices...
	//
	// TODO: read the mpsmin register...
	{
		size_t mpsmin = 0;
		size_t mdts = dev->id.ctrlr.mdts;

		if (!mdts) {
			geo->mdts_nbytes = 1 << 20;
		} else {
			geo->mdts_nbytes = 1 << (mdts + 12 + mpsmin);
		}

		// Fabrics work-around
		if ((geo->mdts_nbytes > (16 * 1024)) && \
		    (!strncmp(dev->ident.schm, "fab", 3))) {
			geo->mdts_nbytes = 16 * 1024;
		}
	}

	// TODO: add zamdts

	return 0;
}

int
xnvme_be_dev_idfy(struct xnvme_dev *dev)
{
	struct xnvme_cmd_ctx ctx = { 0 };
	struct xnvme_spec_idfy *idfy_ctrlr = NULL, *idfy_ns = NULL;
	int err;

	//
	// Identify controller and namespace
	//

	// Allocate buffers for idfy
	idfy_ctrlr = xnvme_buf_alloc(dev, sizeof(*idfy_ctrlr));
	if (!idfy_ctrlr) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc()");
		err = -errno;
		goto exit;
	}
	idfy_ns = xnvme_buf_alloc(dev, sizeof(*idfy_ns));
	if (!idfy_ns) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc()");
		err = -errno;
		goto exit;
	}

	// Retrieve idfy-ctrlr
	memset(idfy_ctrlr, 0, sizeof(*idfy_ctrlr));
	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_idfy_ctrlr(&ctx, idfy_ctrlr);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("FAILED: identify controller");
		err = err ? err : -EIO;
		goto exit;
	}

	// Retrieve idfy-ns
	memset(idfy_ns, 0, sizeof(*idfy_ns));
	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_idfy_ns(&ctx, dev->nsid, idfy_ns);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("FAILED: identify namespace, err: %d", err);
		goto exit;
	}

	// Store idfy-ctrlr and idfy-ns in device instance
	memcpy(&dev->id.ctrlr, idfy_ctrlr, sizeof(*idfy_ctrlr));
	memcpy(&dev->id.ns, idfy_ns, sizeof(*idfy_ns));

	//
	// Command-set specific identify controller / namespace
	//

	// Attempt to identify Zoned Namespace
	{
		struct xnvme_spec_znd_idfy_ns *zns = (void *)idfy_ns;

		memset(idfy_ctrlr, 0, sizeof(*idfy_ctrlr));
		ctx = xnvme_cmd_ctx_from_dev(dev);
		err = xnvme_adm_idfy_ctrlr_csi(&ctx, XNVME_SPEC_CSI_ZONED, idfy_ctrlr);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			XNVME_DEBUG("INFO: !id-ctrlr-zns");
			goto not_zns;
		}

		memset(idfy_ns, 0, sizeof(*idfy_ns));
		ctx = xnvme_cmd_ctx_from_dev(dev);
		err = xnvme_adm_idfy_ns_csi(&ctx, dev->nsid, XNVME_SPEC_CSI_ZONED, idfy_ns);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			XNVME_DEBUG("INFO: !id-ns-zns");
			goto not_zns;
		}

		if (!zns->lbafe[0].zsze) {
			goto not_zns;
		}

		memcpy(&dev->idcss.ctrlr, idfy_ctrlr, sizeof(*idfy_ctrlr));
		memcpy(&dev->idcss.ns, idfy_ns, sizeof(*idfy_ns));
		dev->csi = XNVME_SPEC_CSI_ZONED;

		XNVME_DEBUG("INFO: looks like csi(ZNS)");
		goto exit;

not_zns:
		XNVME_DEBUG("INFO: no positive response to idfy(ZNS)");
	}

	{
		struct xnvme_spec_fs_idfy_ctrlr *fs_ctrlr = (void *)idfy_ctrlr;
		struct xnvme_spec_fs_idfy_ns *fs_ns = (void *)idfy_ns;

		memset(idfy_ctrlr, 0, sizeof(*idfy_ctrlr));
		ctx = xnvme_cmd_ctx_from_dev(dev);
		err = xnvme_adm_idfy_ctrlr_csi(&ctx, XNVME_SPEC_CSI_FS, idfy_ctrlr);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			XNVME_DEBUG("INFO: !xnvme_adm_idfy_ctrlr_csi(CSI_FS)");
			goto not_fs;
		}
		if (!(fs_ctrlr->ac == 0xAC && fs_ctrlr->dc == 0xDC)) {
			XNVME_DEBUG("INFO: invalid values in idfy-ctrlr(CSI_FS)");
			goto not_fs;
		}

		memset(idfy_ns, 0, sizeof(*idfy_ns));
		ctx = xnvme_cmd_ctx_from_dev(dev);
		err = xnvme_adm_idfy_ns_csi(&ctx, dev->nsid, XNVME_SPEC_CSI_FS, idfy_ns);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			XNVME_DEBUG("INFO: !xnvme_adm_idfy_ns_csi(CSI_FS)");
			goto not_fs;
		}
		if (!(fs_ns->nsze && fs_ns->ncap && fs_ns->ac == 0xAC && fs_ns->dc == 0xDC)) {
			XNVME_DEBUG("INFO: invalid values in idfy-ctrlr(CSI_FS)");
			goto not_fs;
		}

		memcpy(&dev->idcss.ctrlr, idfy_ctrlr, sizeof(*idfy_ctrlr));
		memcpy(&dev->idcss.ns, idfy_ns, sizeof(*idfy_ns));

		XNVME_DEBUG("INFO: looks like csi(FS)");
		dev->csi = XNVME_SPEC_CSI_FS;
		goto exit;

not_fs:
		XNVME_DEBUG("INFO: no positive response to idfy(FS)");
	}

	// Attempt to identify LBLK Namespace
	memset(idfy_ns, 0, sizeof(*idfy_ns));
	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_idfy_ns_csi(&ctx, dev->nsid, XNVME_SPEC_CSI_NVM, idfy_ns);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("INFO: not csi-specific id-NVM");
		XNVME_DEBUG("INFO: falling back to NVM assumption");
		err = 0;
		goto exit;
	}
	memcpy(&dev->idcss.ns, idfy_ns, sizeof(*idfy_ns));

exit:
	xnvme_buf_free(dev, idfy_ctrlr);
	xnvme_buf_free(dev, idfy_ns);

	return err;
}

int
xnvme_ident_yaml(FILE *stream, const struct xnvme_ident *ident, int indent, const char *sep,
		 int head)
{
	int wrtn = 0;

	if (head) {
		wrtn += fprintf(stream, "%*sxnvme_ident:", indent, "");
		indent += 2;
	}

	if (!ident) {
		wrtn += fprintf(stream, " ~");
		return wrtn;
	}

	if (head) {
		wrtn += fprintf(stream, "\n");
	}

	wrtn += fprintf(stream, "%*strgt: '%s'%s", indent, "", ident->trgt,
			sep);

	wrtn += fprintf(stream, "%*sschm: '%s'%s", indent, "", ident->schm,
			sep);

	wrtn += fprintf(stream, "%*sopts: '%s'%s", indent, "", ident->opts,
			sep);

	wrtn += fprintf(stream, "%*suri: '%s'", indent, "", ident->uri);

	return wrtn;
}

int
xnvme_ident_fpr(FILE *stream, const struct xnvme_ident *ident, int opts)
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

	wrtn += xnvme_ident_yaml(stream, ident, 0, "\n", 1);
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_ident_pr(const struct xnvme_ident *ident, int opts)
{
	return xnvme_ident_fpr(stdout, ident, opts);
}

int
xnvme_enumeration_fpr(FILE *stream, struct xnvme_enumeration *list, int opts)
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

	wrtn += fprintf(stream, "xnvme_enumeration:");

	if (!list) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  capacity: %u\n", list->capacity);
	wrtn += fprintf(stream, "  nentries: %u\n", list->nentries);
	wrtn += fprintf(stream, "  entries:");

	if (!list->nentries) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	for (uint64_t idx = 0; idx < list->nentries; ++idx) {
		struct xnvme_ident *entry = &list->entries[idx];

		wrtn += fprintf(stream, "\n  - {");
		wrtn += xnvme_ident_yaml(stream, entry, 0, ", ", 0);
		wrtn += fprintf(stream, "}");
	}
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_enumeration_pr(struct xnvme_enumeration *list, int opts)
{
	return xnvme_enumeration_fpr(stdout, list, opts);
}

void
xnvme_enumeration_free(struct xnvme_enumeration *list)
{
	free(list);
}

int
xnvme_enumeration_alloc(struct xnvme_enumeration **list, uint32_t capacity)
{
	*list = malloc(sizeof(**list) + sizeof(*(*list)->entries) * capacity);
	if (!(*list)) {
		XNVME_DEBUG("FAILED: malloc(list + entry * cap(%u))", capacity);
		return -errno;
	}
	(*list)->capacity = capacity;
	(*list)->nentries = 0;

	return 0;
}

int
xnvme_enumeration_append(struct xnvme_enumeration *list,
			 struct xnvme_ident *entry)
{
	if (!list->capacity) {
		XNVME_DEBUG("FAILED: syslist->capacity: %u", list->capacity);
		return -ENOMEM;
	}

	list->entries[(list->nentries)++] = *entry;
	list->capacity--;

	return 0;
}

/**
 * Check whether the given list has the trgt as associated with the given ident
 *
 * @return Returns 1 it is exist, 0 otherwise.
 */
static int
enumeration_has_trgt(struct xnvme_enumeration *list, struct xnvme_ident *ident,
		     uint32_t idx)
{
	uint32_t bound = XNVME_MIN(list->nentries, idx);

	for (uint32_t i = 0; i < bound; ++i) {
		struct xnvme_ident *id = &list->entries[i];

		if (!strncmp(ident->trgt, id->trgt, XNVME_IDENT_TRGT_LEN)) {
			return 1;
		}
	}

	return 0;
}

int
xnvme_enumeration_fpp(FILE *stream, struct xnvme_enumeration *list, int opts)
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

	wrtn += fprintf(stream, "xnvme_enumeration:");

	if (!list) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	if (!list->nentries) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	for (uint32_t idx = 0; idx < list->nentries; ++idx) {
		struct xnvme_ident *ident = &list->entries[idx];
		int nschemes = 0;

		if (enumeration_has_trgt(list, ident, idx)) {
			continue;
		}

		wrtn += fprintf(stream, "\n");
		wrtn += fprintf(stream, "  - trgt: %s\n", ident->trgt);
		wrtn += fprintf(stream, "    schm: [");

		for (uint32_t sidx = 0; sidx < list->nentries; ++sidx) {
			struct xnvme_ident *sident = &list->entries[sidx];

			if (strcmp(ident->trgt, sident->trgt)) {
				continue;
			}

			wrtn += fprintf(stream, "%s%s", nschemes ? "," : "",
					sident->schm);
			++nschemes;
		}
		wrtn += fprintf(stream, "]\n");
	}

	return wrtn;
}

int
xnvme_enumeration_pp(struct xnvme_enumeration *list, int opts)
{
	return xnvme_enumeration_fpp(stdout, list, opts);
}

int
be_options_pr(const struct xnvme_be_options *opts)
{
	int wrtn = 0;

	wrtn += printf("be_options:\n");
	wrtn += printf("  admin: '%s'\n", opts->admin);
	wrtn += printf("  dev: '%s'\n", opts->dev);
	wrtn += printf("  mem: '%s'\n", opts->mem);

	wrtn += printf("  sync: '%s'\n", opts->sync);
	wrtn += printf("  async: '%s'\n", opts->async);

	return wrtn;
}

int
be_options_mname_by_mtype(struct xnvme_be_options *opts, enum xnvme_be_mixin_type mtype,
			  char **mname)
{
	switch (mtype) {
	case XNVME_BE_ASYNC:
		*mname = opts->async;
		return 0;

	case XNVME_BE_SYNC:
		*mname = opts->sync;
		return 0;

	case XNVME_BE_ADMIN:
		*mname = opts->admin;
		return 0;

	case XNVME_BE_DEV:
		*mname = opts->dev;
		return 0;

	case XNVME_BE_MEM:
		*mname = opts->mem;
		return 0;

	case XNVME_BE_ATTR:
	case XNVME_BE_END:
		XNVME_DEBUG("FAILED: attr | end");
		break;
	}

	return -ENOSYS;
}

/**
 * Set up mixin of 'be' of the given 'mtype' and optionally matching 'mname'
 */
static int
be_setup(struct xnvme_be *be, enum xnvme_be_mixin_type mtype, const char *mname)
{
	if (!mname) {
		XNVME_DEBUG("FAILED: mname == NULL, must be a string, possibly empty");
		return -EINVAL;
	}

	for (uint64_t  j = 0; (j < be->nobjs); ++j) {
		const struct xnvme_be_mixin *mixin = &be->objs[j];

		if (mixin->mtype != mtype) {
			continue;
		}
		if (mname[0] != '\0' && strcmp(mname, mixin->name)) {
			continue;
		}

		switch (mtype) {
		case XNVME_BE_ASYNC:
			be->async = *mixin->async;
			break;

		case XNVME_BE_SYNC:
			be->sync = *mixin->sync;
			break;

		case XNVME_BE_ADMIN:
			be->admin = *mixin->admin;
			break;

		case XNVME_BE_DEV:
			be->dev = *mixin->dev;
			break;

		case XNVME_BE_MEM:
			be->mem = *mixin->mem;
			break;

		case XNVME_BE_ATTR:
		case XNVME_BE_END:
			XNVME_DEBUG("FAILED: attr | end");
			return -EINVAL;
		}

		return mtype;
	}

	XNVME_DEBUG("FAILED: no backend mixin");
	return -ENOSYS;
}


// TODO: add option-parsing
int
xnvme_be_factory(const char *uri, struct xnvme_dev *dev)
{
	struct xnvme_be_options *opts = &dev->opts;
	struct xnvme_ident *ident = &dev->ident;
	int err;

	err = xnvme_ident_from_uri(uri, ident);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_ident_from_uri(), err: %d", err);
		return err;
	}
	err = xnvme_be_options_from_ident(ident, opts);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_options_from_ident(), err: %d", err);
		return err;
	}

	for (int i = 0; (i < g_xnvme_be_count) && g_xnvme_be_registry[i]; ++i) {
		struct xnvme_be be = *g_xnvme_be_registry[i];
		int setup = 0;

		if (!be.attr.enabled) {
			XNVME_DEBUG("INFO: skipping be: '%s' because !enabled", be.attr.name);
			continue;
		}

		for (int j = 0; j < 5; ++j) {
			char *mname = NULL;
			int mtype = 1 << j;

			err = be_options_mname_by_mtype(opts, mtype, &mname);
			if (err) {
				XNVME_DEBUG("FAILED: be_options_mname_by_mtype()");
				continue;
			}

			err = be_setup(&be, mtype, mname);
			if (err < 0) {
				XNVME_DEBUG("FAILED: be_setup(%s); err: %d", be.attr.name, err);
				continue;
			}

			setup |= err;
		}

		if (setup != XNVME_BE_CONFIGURED) {
			XNVME_DEBUG("INFO: !configured be(%s); err: %d", be.attr.name, err);
			continue;
		}

		dev->be = be;

		XNVME_DEBUG("INFO: obtained backend instance be: '%s'", be.attr.name);

		err = be.dev.dev_open(dev);
		switch (err < 0 ? -err : err) {
		case 0:
			XNVME_DEBUG("INFO: obtained device handle");
			return 0;

		case EPERM:
			XNVME_DEBUG("FAILED: permission-error; failed and stop trying");
			return err;

		default:
			XNVME_DEBUG("INFO: skipping backend due to err: %d", err);
			break;
		}
	}

	XNVME_DEBUG("FAILED: no backend for uri: '%s'", uri);

	return err ? err : -ENXIO;
}

int
xnvme_enumerate(struct xnvme_enumeration **list, const char *sys_uri, int opts)
{
	int err;

	// TODO: improve in this static allocation of 100 entries should be
	// parsed on-wards as **, such that it can be re-allocated and expanded
	err = xnvme_enumeration_alloc(list, 100);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_enumeration_alloc()");
		return err;
	}

	for (int i = 0; g_xnvme_be_registry[i]; ++i) {
		struct xnvme_be be = *g_xnvme_be_registry[i];

		if (!be.attr.enabled) {
			XNVME_DEBUG("INFO: skipping be: '%s'; !enabled", be.attr.name);
			continue;
		}

		err = be_setup(&be, XNVME_BE_DEV, "");
		if (err < 0) {
			XNVME_DEBUG("FAILED: be_setup(); err: %d", err);
			continue;
		}

		err = be.dev.enumerate(*list, sys_uri, opts);
		if (err) {
			XNVME_DEBUG("FAILED: %s->enumerate(...), err: '%s', i: %d",
				    g_xnvme_be_registry[i]->attr.name,
				    strerror(-err), i);
		}
	}

	return 0;
}

/**
 * Copy 'uri' into 'xnvme_ident.uri'
 *
 * @note consecutive '/' are replaced by a single '/'
 * @note if 'uri' begins with '/' then 'schm_def' is prepended to
 * 'xnvme_ident.uri'
 * @note breaks copy when seeing a space (' ')
 */
static void
norm_uri(const char *uri, struct xnvme_ident *ident, const char *schm_def)
{
	int bound;
	char prev = '!';
	int uri_len = 0;

	bound = strlen(uri) > XNVME_IDENT_URI_LEN ? XNVME_IDENT_URI_LEN : strlen(uri);

	if (uri[0] == '/') {
		sprintf(ident->uri, "%s:", schm_def);
		uri_len = strlen(ident->uri);
	}

	for (int i = 0; i < bound && uri[i] != '\0'; i++) {
		char cur = uri[i];

		if (cur == ' ') {
			break;
		}

		if (i && (cur == '/') && (prev == cur)) {
			continue;
		}

		ident->uri[uri_len++] = cur;

		prev = uri[i];
	}
}

/**
 * Searches the 'haystack' for 'needle', assigns 'ofz' when found
 *
 * @returns On success, that is, needle is found, 1 is returned. On error, that
 * is when 'needle' is not in haystack, the 0 is returned.
 */
static bool
strhas(const char *haystack, char needle, int *ofz)
{
	for (int i = 0; haystack[i] != '\0'; i++) {
		if (haystack[i] == needle) {
			*ofz = i;
			return true;
		}
	}

	return false;
}

bool
has_scheme(const char *needle, const char *haystack[], int len)
{
	for (int i = 0; i < len; ++i) {
		if (!strncmp(needle, haystack[i], 4)) {
			return true;
		}
	}

	return false;
}

int
xnvme_ident_from_uri(const char *uri, struct xnvme_ident *ident)
{
	int trgt_len = 0, opts_len = 0;
	int trgt_ofz, opts_ofz;
	int uri_len;
	int matches;
	char sep[1];

	memset(ident, 0, sizeof(*ident));

	norm_uri(uri, ident, "file");

	uri_len = strlen(ident->uri);

	if (uri_len < XNVME_IDENT_URI_LEN_MIN) {
		XNVME_DEBUG("FAILED: uri is less than minimum length");
		return -EINVAL;
	}

	matches = sscanf(ident->uri, "%4[a-z]%1[:]", ident->schm, sep);
	if (matches != 2) {
		XNVME_DEBUG("FAILED: uri has no scheme");
		return -EINVAL;
	}

	if (strhas(ident->uri, XNVME_IDENT_OPTS_SEP, &opts_ofz)) {
		opts_len = uri_len - opts_ofz;
		strncpy(ident->opts, ident->uri + opts_ofz, opts_len);
	}

	trgt_ofz = strnlen(ident->schm, XNVME_IDENT_SCHM_LEN) + 1;
	trgt_len = uri_len - trgt_ofz - opts_len;

	if (trgt_len >= XNVME_IDENT_TRGT_LEN) {
		XNVME_DEBUG("FAILED: uri-target exceeds maximum length");
		return -EINVAL;
	}

	strncpy(ident->trgt, ident->uri + trgt_ofz, trgt_len);

	return 0;
}

