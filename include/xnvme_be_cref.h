// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_CREF_H
#define __INTERNAL_XNVME_BE_CREF_H

#include <libxnvme.h>

#define XNVME_BE_CREF_MAX_ENTRIES 100

typedef int (*xnvme_be_cref_destructor_fn)(void *ctrlr);

enum xnvme_be_cref_flags {
	XNVME_BE_CREF_NONE              = 0x0,
	XNVME_BE_CREF_DESTROY_IMMEDIATE = 0x1,
};

/**
 * Lookup a controller by @uri and increment its refcount
 *
 * @param uri Device URI to match against
 *
 * @return On success, the controller handle. On error, NULL.
 */
void *
xnvme_be_cref_lookup(const char *uri);

/**
 * Insert a new controller entry with refcount=1
 *
 * Does not check for duplicate URIs. The caller must ensure the URI is not
 * already present, e.g. by calling xnvme_be_cref_lookup() first.
 *
 * @param uri Device URI to associate with the controller
 * @param be_name Backend name pointer, used for filtering in xnvme_be_cref_cleanup()
 * @param ctrlr Controller handle to store, must be non-NULL
 * @param destructor Callback invoked to destroy the controller when refcount reaches zero
 *
 * @return 0 on success, negative errno on error.
 */
int
xnvme_be_cref_insert(const char *uri, const char *be_name, void *ctrlr,
		     xnvme_be_cref_destructor_fn destructor);

/**
 * Combined lookup-or-insert
 *
 * If an entry with matching @uri exists, increment its refcount and return the
 * stored handle. Otherwise, if @ctrlr is non-NULL, insert it with refcount=1
 * and associate it with @be_name and @destructor.
 *
 * @param uri Device URI to match or insert
 * @param be_name Backend name pointer, stored for filtering in xnvme_be_cref_cleanup()
 * @param ctrlr Controller handle to insert if no match is found, may be NULL for lookup-only
 * @param destructor Callback invoked to destroy the controller when refcount reaches zero
 *
 * @return On success, the controller handle. On error, NULL.
 */
void *
xnvme_be_cref_ref(const char *uri, const char *be_name, void *ctrlr,
		  xnvme_be_cref_destructor_fn destructor);

/**
 * Decrement the refcount for the given controller
 *
 * If the refcount reaches zero and XNVME_BE_CREF_DESTROY_IMMEDIATE is set,
 * call the stored destructor and remove the entry. Otherwise the entry remains
 * with refcount=0 until xnvme_be_cref_cleanup() is called.
 *
 * @param ctrlr Controller handle to dereference, must be non-NULL
 * @param flags Bitmask of enum xnvme_be_cref_flags
 *
 * @return 0 on success, negative errno on error.
 */
int
xnvme_be_cref_deref(void *ctrlr, enum xnvme_be_cref_flags flags);

/**
 * Destroy all entries with zero refcount matching @be_name
 *
 * Iterates all entries, and for those matching @be_name with refcount=0,
 * calls the stored destructor and removes the entry.
 *
 * @param be_name Backend name pointer to match (compared by pointer equality)
 *
 * @return 0 on success, negative errno of the last failed destructor on error.
 */
int
xnvme_be_cref_cleanup(const char *be_name);

#endif /* __INTERNAL_XNVME_BE_CREF_H */
