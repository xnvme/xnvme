// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <libxnvme.h>
#include <libxnvme_lba.h>
#include <libxnvme_pp.h>

int
xnvme_lba_range_fpr(FILE *stream, struct xnvme_lba_range *range, int opts)
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

	wrtn += fprintf(stream, "xnvme_lba_range:");
	if (!range) {
		wrtn += fprintf(stream, "~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");

	wrtn += fprintf(stream, "  slba: 0x%016lx\n", range->slba);
	wrtn += fprintf(stream, "  elba: 0x%016lx\n", range->elba);
	wrtn += fprintf(stream, "  naddrs: %u\n", range->naddrs);
	wrtn += fprintf(stream, "  nbytes: %zu\n", range->nbytes);
	wrtn += fprintf(stream, "  attr: { is_zones: %d, is_valid: %d}\n", range->attr.is_zoned,
			range->attr.is_valid);

	return wrtn;
}

int
xnvme_lba_range_pr(struct xnvme_lba_range *range, int opts)
{
	return xnvme_lba_range_fpr(stdout, range, opts);
}

struct xnvme_lba_range
xnvme_lba_range_from_slba_naddrs(struct xnvme_dev *dev, uint64_t slba, uint64_t naddrs)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	struct xnvme_lba_range rng = {0};

	if (!naddrs) {
		XNVME_DEBUG("FAILED: !naddrs => the range must be non-empty");
		return rng;
	}

	rng.slba = slba;
	rng.elba = slba + naddrs - 1;
	rng.naddrs = naddrs;
	rng.nbytes = naddrs * geo->nbytes;
	rng.attr.is_valid = 1;

	return rng;
}

struct xnvme_lba_range
xnvme_lba_range_from_slba_elba(struct xnvme_dev *dev, uint64_t slba, uint64_t elba)
{
	struct xnvme_lba_range rng = {0};

	if (slba > elba) {
		XNVME_DEBUG("FAILED: invalid range; slba > elba");
		return rng;
	}

	return xnvme_lba_range_from_slba_naddrs(dev, slba, (elba - slba) + 1);
}

struct xnvme_lba_range
xnvme_lba_range_from_offset_nbytes(struct xnvme_dev *dev, uint64_t offset, uint64_t nbytes)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	struct xnvme_lba_range rng = {0};

	if (offset % geo->nbytes) {
		XNVME_DEBUG("FAILED: offset: %zu, does not align to lba-width: %u", offset,
			    geo->nbytes);
		return rng;
	}
	if (nbytes % geo->nbytes) {
		XNVME_DEBUG("FAILED: nbytes: %zu, does not align to lba-width: %u", nbytes,
			    geo->nbytes);
		return rng;
	}

	return xnvme_lba_range_from_slba_naddrs(dev, offset / geo->nbytes, nbytes / geo->nbytes);
}

struct xnvme_lba_range
xnvme_lba_range_from_zdescr(struct xnvme_dev *dev, struct xnvme_spec_znd_descr *zdescr)
{
	return xnvme_lba_range_from_slba_naddrs(dev, zdescr->zslba, zdescr->zcap);
}
