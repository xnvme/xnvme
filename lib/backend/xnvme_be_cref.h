// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_CREF_H
#define __INTERNAL_XNVME_BE_CREF_H

#include <libxnvme.h>

#define XNVME_BE_CREF_MAX_ENTRIES 100

typedef int (*xnvme_be_cref_destructor_fn)(void *ctrlr);

/**
 * A controller reference entry
 *
 * @ctrlr holds the controller handle and @be_name holds the backend name the
 * entry was inserted with.
 */
struct xnvme_be_cref_entry {
	void *ctrlr;
	xnvme_be_cref_destructor_fn destructor;
	const char *be_name;
	int refcount;
	char uri[XNVME_IDENT_URI_LEN + 1];
};

/**
 * Lookup a controller by @uri and increment its refcount
 *
 * @param uri Device URI to match against
 *
 * @return On a hit, a const pointer to the matched entry. On a miss, NULL.
 */
const struct xnvme_be_cref_entry *
xnvme_be_cref_get(const char *uri);

/**
 * Insert a new controller entry with refcount=1
 *
 * Does not check for duplicate URIs. The caller must ensure the URI is not
 * already present, e.g. by calling xnvme_be_cref_get() first.
 *
 * @param uri Device URI to associate with the controller
 * @param be_name Backend name pointer, stored for later matching
 * @param ctrlr Controller handle to store, must be non-NULL
 * @param destructor Callback invoked to destroy the controller when refcount reaches zero
 *
 * @return 0 on success, negative errno on error.
 */
int
xnvme_be_cref_insert(const char *uri, const char *be_name, void *ctrlr,
		     xnvme_be_cref_destructor_fn destructor);

/**
 * Decrement the refcount for the given controller
 *
 * When the refcount reaches zero, call the stored destructor and remove the
 * entry.
 *
 * @param ctrlr Controller handle to dereference, must be non-NULL
 *
 * @return 0 on success, negative errno on error.
 */
int
xnvme_be_cref_put(void *ctrlr);

#endif /* __INTERNAL_XNVME_BE_CREF_H */
