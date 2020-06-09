// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_H
#define __INTERNAL_XNVME_BE_H
#include <paths.h>
#include <xnvme_be_registry.h>
#include <stdbool.h>

#define XNVME_FREEBSD_CTRLR_SCAN _PATH_DEV "nvme%1u%[^\n]"
#define XNVME_FREEBSD_NS_SCAN _PATH_DEV "nvme%1uns%1u%[^\n]"

#define XNVME_FREEBSD_CTRLR_FMT _PATH_DEV "nvme%1u"
#define XNVME_FREEBSD_NS_FMT _PATH_DEV "nvme%1uns%1u"

#define XNVME_LINUX_CTRLR_SCAN _PATH_DEV "nvme%1u%[^\n]"
#define XNVME_LINUX_NS_SCAN _PATH_DEV "nvme%1un%1u%[^\n]"

#define XNVME_LINUX_CTRLR_FMT _PATH_DEV "nvme%1u"
#define XNVME_LINUX_NS_FMT _PATH_DEV "nvme%1un%1u"

#define XNVME_BE_ACTX_NBYTES 192

#define XNVME_BE_FUNC_NBYTES 104
#define XNVME_BE_ATTR_NBYTES 24
#define XNVME_BE_STATE_NBYTES 128
#define XNVME_BE_NBYTES \
	( XNVME_BE_FUNC_NBYTES + XNVME_BE_ATTR_NBYTES + XNVME_BE_STATE_NBYTES )

/**
 * Backend function-interface
 */
struct xnvme_be_func {
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

	/**
	 * Initialize an asynchronous command context
	 */
	int (*async_init)(struct xnvme_dev *, struct xnvme_async_ctx **,
			  uint16_t, int flags);

	/**
	 * Terminate an asynchronous command context
	 */
	int (*async_term)(struct xnvme_dev *, struct xnvme_async_ctx *);

	/**
	 * Attempt to read all asynchronous events from a given context
	 */
	int (*async_poke)(struct xnvme_dev *, struct xnvme_async_ctx *, uint32_t);

	/**
	 * Wait for completion of all asynchronous events on a given context
	 */
	int (*async_wait)(struct xnvme_dev *, struct xnvme_async_ctx *);

	/**
	 * Allocate a buffer usable for NVMe commands
	 */
	void *(*buf_alloc)(const struct xnvme_dev *, size_t, uint64_t *);

	/**
	 * Resolve the physical address of the given buffer
	 */
	int (*buf_vtophys)(const struct xnvme_dev *, void *, uint64_t *);

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
	 * Enumerate devices on/at the given 'sys_uri' when NULL local devices
	 */
	int (*enumerate)(struct xnvme_enumeration *, const char *, int);

	/**
	 * Construct a device from the given identifier
	 */
	int (*dev_from_ident)(const struct xnvme_ident *, struct xnvme_dev **);

	/**
	 * Close the given device
	 */
	void (*dev_close)(struct xnvme_dev *);
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_func) == XNVME_BE_FUNC_NBYTES,
		    "Incorrect size")

/**
 * Backend interface consisting of functions, attributes and instance state
 */
struct xnvme_be {
	struct xnvme_be_func func;		///< Functions
	struct xnvme_be_attr attr;		///< Attributes
	uint8_t state[XNVME_BE_STATE_NBYTES];	///< Instance state
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be) == XNVME_BE_NBYTES,
		    "Incorrect size")

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
int
xnvme_be_factory(const char *uri, struct xnvme_dev **dev);

int
xnvme_be_dev_derive_geometry(struct xnvme_dev *dev);

int
uri_parse_scheme(const char *uri, char *scheme);

int
xnvme_ident_yaml(FILE *stream, const struct xnvme_ident *ident, int indent,
		 const char *sep, int head);

int
xnvme_enumeration_alloc(struct xnvme_enumeration **list, uint32_t capacity);

void
xnvme_enumeration_free(struct xnvme_enumeration *list);

int
xnvme_enumeration_append(struct xnvme_enumeration *list,
			 struct xnvme_ident *entry);

bool
has_scheme(const char *needle, const char *haystack[], int len);

int
path_to_ll(const char *path, uint64_t *val);

bool
xnvme_ident_opt_to_val(const struct xnvme_ident *ident, const char *opt,
		       uint32_t *val);

#endif /* __INTERNAL_XNVME_BE_H */
