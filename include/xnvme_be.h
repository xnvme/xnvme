// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_H
#define __INTERNAL_XNVME_BE_H
#include <paths.h>
#include <xnvme_be_registry.h>

#define XNVME_FREEBSD_CTRLR_SCAN _PATH_DEV "nvme%1u%[^\n]"
#define XNVME_FREEBSD_NS_SCAN _PATH_DEV "nvme%1uns%1u%[^\n]"

#define XNVME_FREEBSD_CTRLR_FMT _PATH_DEV "nvme%1u"
#define XNVME_FREEBSD_NS_FMT _PATH_DEV "nvme%1uns%1u"

#define XNVME_LINUX_CTRLR_SCAN _PATH_DEV "nvme%1u%[^\n]"
#define XNVME_LINUX_NS_SCAN _PATH_DEV "nvme%1un%1u%[^\n]"

#define XNVME_LINUX_CTRLR_FMT _PATH_DEV "nvme%1u"
#define XNVME_LINUX_NS_FMT _PATH_DEV "nvme%1un%1u"

/**
 * Backend interface
 */
struct xnvme_be {

	/**
	 * Internal backend identifier
	 */
	uint16_t bid;

	/**
	 * Backend name
	 */
	char name[64];

	/**
	 * Construct identifier from uri
	 */
	int (*ident_from_uri)(const char *, struct xnvme_ident *);

	/**
	 * Enumerate devices on the system
	 */
	int (*enumerate)(struct xnvme_enumeration *, int);

	/**
	 * Open a device
	 */
	struct xnvme_dev *(*dev_open)(const struct xnvme_ident *, int);

	/**
	 * Close the given device
	 */
	void (*dev_close)(struct xnvme_dev *);

	/**
	 * Allocate a buffer usable for NVMe commands
	 */
	void *(*buf_alloc)(const struct xnvme_dev *, size_t, uint64_t *);

	/**
	 * Re-allocate a buffer usable for NVMe commands
	 */
	void *(*buf_realloc)(const struct xnvme_dev *, void *, size_t,
			     uint64_t *);

	/**
	 * Free a buffer usable for NVMe commands
	 */
	void (*buf_free)(const struct xnvme_dev *, void *);

	/**
	 * Resolve the physical address of the given buffer
	 */
	int (*buf_vtophys)(const struct xnvme_dev *, void *, uint64_t *);

	/**
	 * Initialize an asynchronous command context with
	 */
	struct xnvme_async_ctx *(*async_init)(struct xnvme_dev *, uint32_t,
					      uint16_t);

	/**
	 * Terminate an asynchronous command context
	 */
	int (*async_term)(struct xnvme_dev *, struct xnvme_async_ctx *);

	/**
	 * Attempt to read all asynchronous events from a given context
	 */
	int (*async_poke)(struct xnvme_dev *, struct xnvme_async_ctx *,
			  uint32_t);

	/**
	 * Wait for completion of all asynchronous events on a given context
	 */
	int (*async_wait)(struct xnvme_dev *, struct xnvme_async_ctx *);

	/**
	 * Pass a NVMe I/O Command Through to the device with minimal driver
	 * intervention
	 */
	int (*cmd_pass)(struct xnvme_dev *, struct xnvme_spec_cmd *, void *,
			size_t, void *, size_t, int, struct xnvme_req *);

	/**
	 * Pass a NVMe Admin Command Through to the device with minimal driver
	 * intervention
	 */
	int (*cmd_pass_admin)(struct xnvme_dev *, struct xnvme_spec_cmd *,
			      void *, size_t, void *, size_t, int,
			      struct xnvme_req *);
};

/**
 * Auxilary helpers
 */

/**
 * Resolve the name of the given backend identifier
 */
const char *
xnvme_be_id2name(int bid);

/**
 * Resolve the internal identifier of the given backend name.
 *
 * @returns Returns numerical identifier on success. On error, e.g. the given
 * name does not exists in the list of backend implementations, -1 is returned.
 */
int
xnvme_be_name2id(const char *bname);

/**
 * Produce a device with a backend attached
 */
struct xnvme_dev *
xnvme_be_factory(const char *uri, int opts);

int
xnvme_be_dev_derive_geometry(struct xnvme_dev *dev);

int
uri_parse_prefix(const char *uri, char *prefix);

int
xnvme_ident_yaml(FILE *stream, const struct xnvme_ident *ident, int indent,
		 const char *sep, int head);

struct xnvme_enumeration *
xnvme_enumeration_alloc(uint32_t capacity);

void
xnvme_enumeration_free(struct xnvme_enumeration *list);

int
xnvme_enumeration_append(struct xnvme_enumeration *list,
			 struct xnvme_ident *entry);

#endif /* __INTERNAL_XNVME_BE_H */
