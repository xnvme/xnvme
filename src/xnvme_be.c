// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <paths.h>
#include <libxnvme.h>
#include <libznd.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_be_registry.c>

static inline uint64_t
_ilog2(uint64_t x)
{
	uint64_t val = 0;

	while (x >>= 1) {
		val++;
	}

	return val;
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
	return xnvme_lba_pr(lba, opts);
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

/**
 * Resolve the internal identifier of the given backend name.
 *
 * @returns Returns numerical identifier on success. On error, e.g. the given
 * name does not exists in the list of backend implementations, -1 is returned.
 */
int
xnvme_be_name2id(const char *bname)
{
	for (int i = 0; xnvme_be_impl[i]; ++i) {
		if (strcasecmp(bname, xnvme_be_impl[i]->name)) {
			continue;
		}
		return xnvme_be_impl[i]->bid;
	}

	XNVME_DEBUG("FAILED: no backend with name: '%s'", bname);
	return -1;
}

const char *
xnvme_be_id2name(int bid)
{
	for (int i = 0; xnvme_be_impl[i]; ++i) {
		if (xnvme_be_impl[i]->bid != bid) {
			continue;
		}

		return xnvme_be_impl[i]->name;
	}

	XNVME_DEBUG("FAILED: no backend with bid: '%d'", bid);
	return "";
}

static inline int
_zoned_geometry(struct xnvme_dev *dev)
{
	struct xnvme_spec_idfy_ns *ns = &dev->ns;
	struct znd_idfy_ns *zns = (void *)&dev->ns;
	struct xnvme_geo *geo = &dev->geo;

	struct xnvme_spec_lbaf *lbaf = NULL;
	struct znd_idfy_zonef *zonef = NULL;
	struct znd_rprt_hdr *hdr = NULL;
	uint32_t hdr_nbytes = 0x1000;
	struct xnvme_req req = { 0 };
	int err;

	if (!zns->zonef[0].zs) {
		return -1;
	}

	lbaf = &ns->lbaf[ns->flbas.format];
	zonef = &zns->zonef[zns->fzsze];

	hdr = xnvme_buf_alloc(dev, hdr_nbytes, NULL);
	if (!hdr) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc()");
		return -1;
	}
	memset(hdr, 0, hdr_nbytes);

	err = znd_cmd_mgmt_recv(dev, xnvme_dev_get_nsid(dev), 0x0,
				ZND_RECV_REPORT, ZND_RECV_SF_ALL, 0,
				hdr, hdr_nbytes,
				XNVME_CMD_SYNC, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		XNVME_DEBUG("znd_cmd_mgmt_recv()");
		xnvme_buf_free(dev, hdr);
		return -1;
	}

	geo->type = XNVME_GEO_ZONED;

	geo->npugrp = 1;
	geo->npunit = 1;
	geo->nzone = hdr->nzones;

	geo->nbytes = 2 << (lbaf->ds - 1);
	geo->nbytes_oob = lbaf->ms;

	geo->nsect = zonef->zs / geo->nbytes;

	xnvme_buf_free(dev, hdr);

	return 0;
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

int
xnvme_be_dev_derive_geometry(struct xnvme_dev *dev)
{
	struct xnvme_geo *geo = &dev->geo;
	int err = 0;

	err = _zoned_geometry(dev);
	if (!err) {
		goto rest;
	}

	err = _conventional_geometry(dev);
	if (err) {
		XNVME_DEBUG("FAILED: _conventional_geometry");
		return -1;
	}

rest:
	geo->tbytes = geo->npugrp * geo->npunit * geo->nzone * geo->nsect * \
		      geo->nbytes;

	/* Derive the sector-shift-width for LBA mapping */
	dev->ssw = _ilog2(dev->geo.nbytes);

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
uri_parse_prefix(const char *uri, char *prefix)
{
	int matches;

	matches = sscanf(uri, "%4[a-z]://", prefix);
	if (matches != 1) {
		return -1;
	}

	return 0;
}

const char *
xnvme_ident_type_str(enum xnvme_ident_type idt)
{
	switch (idt) {
	case XNVME_IDENT_CTRLR:
		return "XNVME_IDENT_CTRLR";

	case XNVME_IDENT_NS:
		return "XNVME_IDENT_NS";
	}

	return "XNVME_IDENT_ENOSYS";
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

	wrtn += fprintf(stream, "%*suri: '%s'%s", indent, "",
			ident->uri, sep);

	wrtn += fprintf(stream, "%*stype: '%s'%s", indent, "",
			xnvme_ident_type_str(ident->type), sep);

	wrtn += fprintf(stream, "%*snst: '%s'%s", indent, "",
			ident->nsid ? xnvme_spec_nstype_str(ident->nst) : "~",
			sep);

	wrtn += fprintf(stream, "%*snsid: 0x%x%s", indent, "",
			ident->nsid, sep);

	wrtn += fprintf(stream, "%*sbe_uri: '%s'%s", indent, "",
			ident->be_uri, sep);

	wrtn += fprintf(stream, "%*sbe_name: '%s'%s", indent, "",
			xnvme_be_id2name(ident->bid), sep);

	wrtn += fprintf(stream, "%*sbe_id: 0x%x", indent, "",
			ident->bid);

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

struct xnvme_enumeration *
xnvme_enumeration_alloc(uint32_t capacity)
{
	struct xnvme_enumeration *list = NULL;

	list = malloc(sizeof(*list) + sizeof(*list->entries) * capacity);
	if (!list) {
		XNVME_DEBUG("FAILED: malloc(list + entry * cap(%u))", capacity);
		return NULL;
	}
	list->capacity = capacity;
	list->nentries = 0;

	return list;
}

int
xnvme_enumeration_append(struct xnvme_enumeration *list,
			 struct xnvme_ident *entry)
{
	if (!list->capacity) {
		XNVME_DEBUG("FAILED: syslist->capacity: %u", list->capacity);
		return -1;
	}

	list->entries[(list->nentries)++] = *entry;
	list->capacity--;

	return 0;
}

int
xnvme_ident_from_uri(const char *uri, struct xnvme_ident *ident)
{
	for (int i = 0; xnvme_be_impl[i]; ++i) {
		memset(ident, 0, sizeof(*ident));

		if (!xnvme_be_impl[i]->ident_from_uri(uri, ident)) {
			return 0;
		}
	}

	errno = EINVAL;
	return -1;
}

struct xnvme_dev *
xnvme_be_factory(const char *uri, int opts)
{
	struct xnvme_ident ident = { 0 };

	if (xnvme_ident_from_uri(uri, &ident)) {
		XNVME_DEBUG("FAILED: parsing uri: '%s'", uri);
		errno = EINVAL;
		return NULL;
	}

	for (int i = 0; xnvme_be_impl[i]; ++i) {
		struct xnvme_dev *dev = NULL;

		if (xnvme_be_impl[i]->bid != ident.bid) {
			continue;
		}

		dev = xnvme_be_impl[i]->dev_open(&ident, opts);
		if (!dev) {
			continue;
		}

		return dev;
	}

	XNVME_DEBUG("FAILED: no backend for uri: '%s'", uri);
	errno = ENXIO;

	return NULL;
}

struct xnvme_enumeration *
xnvme_enumerate(int opts)
{
	struct xnvme_enumeration *list = NULL;

	// TODO: improve in this static allocation of 100 entries should be
	// parsed on-wards as **, such that it can be re-allocated and expanded
	list = xnvme_enumeration_alloc(100);
	if (!list) {
		XNVME_DEBUG("FAILED: xnvme_enumeration_alloc");
		return NULL;
	}

	for (int i = 0; xnvme_be_impl[i]; ++i) {
		int err;

		err = xnvme_be_impl[i]->enumerate(list, opts);
		if (err) {
			XNVME_DEBUG("FAILED: be_id: 0x%x, name: %s}, err: '%s'",
				    xnvme_be_impl[i]->bid,
				    xnvme_be_id2name(xnvme_be_impl[i]->bid),
				    strerror(errno));
		}
	}

	return list;
}
