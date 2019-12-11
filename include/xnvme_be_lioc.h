// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_LIOC_H
#define __INTERNAL_XNVME_BE_LIOC_H
#include <xnvme_dev.h>

/**
 * @enum xnvme_be_lioc_opts
 */
enum xnvme_be_lioc_opts {
	XNVME_BE_LIOC_WRITABLE = 0x1, ///< XNVME_BE_LIOC_WRITABLE
};

/**
 * Internal representation of XNVME_BE_LIOC state
 */
struct xnvme_be_lioc {
	struct xnvme_be be;

	int fd;
};

void *
xnvme_be_lioc_buf_alloc(const struct xnvme_dev *dev, size_t nbytes,
			uint64_t *phys);

void *
xnvme_be_lioc_buf_realloc(const struct xnvme_dev *XNVME_UNUSED(dev),
			  void *XNVME_UNUSED(buf), size_t XNVME_UNUSED(nbytes),
			  uint64_t *XNVME_UNUSED(phys));

void
xnvme_be_lioc_buf_free(const struct xnvme_dev *dev, void *buf);

int
xnvme_be_lioc_buf_vtophys(const struct xnvme_dev *dev, void *buf,
			  uint64_t *phys);

int
xnvme_be_lioc_cmd_pass(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
		       void *dbuf, size_t dbuf_nbytes, void *mbuf,
		       size_t mbuf_nbytes, int opts, struct xnvme_req *req);

int
xnvme_be_lioc_cmd_pass_admin(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
			     void *dbuf, size_t dbuf_nbytes, void *mbuf,
			     size_t mbuf_nbytes, int opts,
			     struct xnvme_req *req);

struct xnvme_dev *
xnvme_be_lioc_dev_open(const struct xnvme_ident *ident, int flags);

void
xnvme_be_lioc_dev_close(struct xnvme_dev *dev);

int
xnvme_be_lioc_enumerate(struct xnvme_enumeration *list, int opts);

static inline uint64_t
xnvme_lba2off(struct xnvme_dev *dev, uint64_t lba)
{
	return lba << dev->ssw;
}

static inline uint64_t
xnvme_off2lba(struct xnvme_dev *dev, uint64_t off)
{
	return off >> dev->ssw;
}

int
xnvme_path_nvme_filter(const struct dirent *d);

#endif /* __INTERNAL_XNVME_BE_LIOC */
