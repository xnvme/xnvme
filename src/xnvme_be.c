// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>
#include <paths.h>
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_be_registry.c>

bool
xnvme_ident_opt_to_val(const struct xnvme_ident *ident, const char *opt,
		       uint32_t *val)
{
	const char *ofz = NULL;
	char fmt[100] = { 0 };

	ofz = strstr(ident->opts, opt);
	if (!ofz) {
		return false;
	}

	sprintf(fmt, "%s=%%1u", opt);

	return sscanf(ofz, fmt, val) == 1;
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
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

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
xnvme_be_attr_list(struct xnvme_be_attr_list **list)
{
	const size_t list_nbytes = sizeof(**list) + xnvme_be_count * \
				   sizeof((*list)->item[0]);

	*list = malloc(list_nbytes);
	if (!*list) {
		return -1;
	}

	(*list)->count = xnvme_be_count;
	(*list)->capacity = xnvme_be_count;
	for (int i = 0; i < xnvme_be_count; ++i) {
		(*list)->item[i] = xnvme_be_registry[i]->attr;
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

	wrtn += fprintf(stream, "{");

	wrtn += fprintf(stream, "name: '%s', ", attr->name);
	wrtn += fprintf(stream, "enabled: %d, ", attr->enabled);

	wrtn += fprintf(stream, "schemes: [");
	for (int i = 0; i < attr->nschemes; ++i) {
		if (i) {
			wrtn += fprintf(stream, ", ");
		}

		wrtn += fprintf(stream, "%s", attr->schemes[i]);
	}
	wrtn += fprintf(stream, "]");

	wrtn += fprintf(stream, "}");

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
		wrtn += xnvme_be_attr_fpr(stream, &xnvme_be_registry[i]->attr,
					  opts);
		wrtn += fprintf(stream, "\n");
	}

	return wrtn;
}

int
xnvme_be_attr_list_pr(const struct xnvme_be_attr_list *list, int opts)
{
	return xnvme_be_attr_list_fpr(stdout, list, opts);
}

// TODO: select LBAF correctly, instead of the first
static inline int
_conventional_geometry(struct xnvme_dev *dev)
{
	struct xnvme_geo *geo = &dev->geo;
	struct xnvme_spec_lbaf *lbaf = NULL;

	lbaf = &dev->ns.lbaf[dev->ns.flbas.format];

	geo->type = XNVME_GEO_CONVENTIONAL;

	geo->npugrp = 1;
	geo->npunit = 1;
	geo->nzone = 1;

	geo->nsect = dev->ns.nsze;
	geo->nbytes = 2 << (lbaf->ds - 1);
	geo->nbytes_oob = lbaf->ms;

	return 0;
}

// TODO: extract basename from ident->trgt
static inline int
_blockdevice_geometry(struct xnvme_dev *dev)
{
	struct xnvme_geo *geo = &dev->geo;
	int sysfs_len = 0x1000;
	char sysfs_path[sysfs_len];
	char dev_fname[XNVME_IDENT_URI_LEN];
	uint64_t val = 1;
	int err;

	sprintf(dev_fname, "nullb0");

	geo->type = XNVME_GEO_CONVENTIONAL;

	sprintf(sysfs_path, "/sys/block/%s/size", dev_fname);
	err = path_to_ll(sysfs_path, &val);
	if (err) {
		XNVME_DEBUG("err: '%s'", strerror(err));
		return err;
	}
	geo->tbytes = val * 512;

	sprintf(sysfs_path, "/sys/block/%s/queue/minimum_io_size", dev_fname);
	err = path_to_ll(sysfs_path, &val);
	if (err) {
		XNVME_DEBUG("err: '%s'", strerror(err));
		return err;
	}
	geo->nbytes = val;
	geo->nbytes_oob = 0;

	geo->npugrp = 1;
	geo->npunit = 1;
	geo->nzone = 1;
	geo->nsect = geo->tbytes / geo->nbytes;

	sprintf(sysfs_path, "/sys/block/%s/queue/max_sectors_kb", dev_fname);
	err = path_to_ll(sysfs_path, &val);
	if (err) {
		XNVME_DEBUG("err: '%s'", strerror(err));
		return err;
	}
	geo->mdts_nbytes = val * 1024;

	dev->ssw = XNVME_ILOG2(geo->nbytes);

	return 0;
}

// TODO: add proper handling of NVMe controllers
int
xnvme_be_dev_derive_geometry(struct xnvme_dev *dev)
{
	struct xnvme_geo *geo = &dev->geo;

	switch (dev->dtype) {
	case XNVME_DEV_TYPE_BLOCK_DEVICE:
		if (_blockdevice_geometry(dev)) {
			return -EINVAL;
		}
		break;

	case XNVME_DEV_TYPE_NVME_CONTROLLER:
		return -ENOSYS;

	case XNVME_DEV_TYPE_NVME_NAMESPACE:
		if (_conventional_geometry(dev)) {
			XNVME_DEBUG("FAILED: _conventional_geometry");
			return -EINVAL;
		}
		break;
	}

	geo->tbytes = geo->npugrp * geo->npunit * geo->nzone * geo->nsect * \
		      geo->nbytes;

	/* Derive the sector-shift-width for LBA mapping */
	dev->ssw = XNVME_ILOG2(dev->geo.nbytes);

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
		size_t mdts = dev->ctrlr.mdts;

		if (!mdts) {
			geo->mdts_nbytes = 1 << 20;
		} else {
			geo->mdts_nbytes = 1 << (mdts + 12 + mpsmin);
		}
	}

	// TODO: add zamdts

	return 0;
}

int
xnvme_ident_yaml(FILE *stream, const struct xnvme_ident *ident, int indent,
		 const char *sep, int head)
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

int
xnvme_be_factory(const char *uri, struct xnvme_dev **dev)
{
	struct xnvme_ident ident = { 0 };
	int err;

	err = xnvme_ident_from_uri(uri, &ident);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_ident_from_uri()");
		return err;
	}

	for (int i = 0; xnvme_be_registry[i]; ++i) {
		struct xnvme_be *be = xnvme_be_registry[i];

		if (!be->attr.enabled) {
			continue;
		}
		if (!has_scheme(ident.schm, be->attr.schemes,
				be->attr.nschemes)) {
			continue;
		}

		if (!be->func.dev_from_ident(&ident, dev)) {
			return 0;
		}
	}

	XNVME_DEBUG("FAILED: no backend for uri: '%s'", uri);

	return -ENXIO;
}

int
xnvme_enumerate(struct xnvme_enumeration **list, int opts)
{
	int err;

	// TODO: improve in this static allocation of 100 entries should be
	// parsed on-wards as **, such that it can be re-allocated and expanded
	err = xnvme_enumeration_alloc(list, 100);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_enumeration_alloc");
		return err;
	}

	for (int i = 0; xnvme_be_registry[i]; ++i) {
		int err;

		err = xnvme_be_registry[i]->func.enumerate(*list, opts);
		if (err) {
			XNVME_DEBUG("FAILED: %s->enumerate(...), err: '%s', i: %d",
				    xnvme_be_registry[i]->attr.name,
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

	XNVME_DEBUG("ident->uri: '%s'", ident->uri);

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
	XNVME_DEBUG("ident->uri: '%s'", ident->uri);
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

	norm_uri(uri, ident, "file");

	uri_len = strlen(ident->uri);

	if (uri_len < XNVME_IDENT_URI_LEN_MIN) {
		return -EINVAL;
	}

	matches = sscanf(ident->uri, "%4[a-z]%1[:]", ident->schm, sep);
	if (matches != 2) {
		return -EINVAL;
	}

	if (strhas(ident->uri, XNVME_IDENT_OPTS_SEP, &opts_ofz)) {
		opts_len = uri_len - opts_ofz;
		strncpy(ident->opts, ident->uri + opts_ofz, opts_len);
	}

	trgt_ofz = strnlen(ident->schm, XNVME_IDENT_SCHM_LEN) + 1;
	trgt_len = uri_len - trgt_ofz - opts_len;

	strncpy(ident->trgt, ident->uri + trgt_ofz, trgt_len);

	return 0;
}
