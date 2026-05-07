// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_CREF_H
#define __INTERNAL_XNVME_BE_CREF_H

#include <libxnvme.h>

#define XNVME_BE_CREF_MAX_ENTRIES 100

typedef int (*xnvme_be_cref_destructor_fn)(void *ctrlr);

/**
 * Lookup a controller by @uri, increment its refcount, and return it
 *
 * @param uri Device URI to match against
 * @param be_name If non-NULL, receives the backend name of the matched entry
 *
 * @return On success, the controller handle. On error, NULL.
 */
void *
xnvme_be_cref_lookup(const char *uri, const char **be_name);

/**
 * Insert a new controller entry with refcount=1
 *
 * Does not check for duplicate URIs. The caller must ensure the URI is not
 * already present, e.g. by calling xnvme_be_cref_lookup() first.
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
 * Mark the controller as deferred, preventing destruction when refcount reaches zero
 *
 * When deferred, xnvme_be_cref_deref() will leave the entry alive at refcount=0
 * rather than calling its destructor. The entry is destroyed by xnvme_be_cref_cleanup().
 *
 * @param ctrlr Controller handle to mark, must be non-NULL
 *
 * @return 0 on success, negative errno on error.
 */
int
xnvme_be_cref_defer(void *ctrlr);

/**
 * Decrement the refcount for the given controller
 *
 * If the refcount reaches zero and the entry is not deferred, call the stored
 * destructor and remove the entry. Deferred entries remain at refcount=0 until
 * xnvme_be_cref_cleanup() is called.
 *
 * @param ctrlr Controller handle to dereference, must be non-NULL
 *
 * @return 0 on success, negative errno on error.
 */
int
xnvme_be_cref_deref(void *ctrlr);

/**
 * Destroy all deferred entries with refcount=0, calling each entry's destructor
 *
 * For entries with refcount>0 (still held by open namespace devices), clears
 * the deferred flag so xnvme_be_cref_put() will destroy them normally when
 * the last reference is dropped.
 *
 * Must be called only after all deferred xnvme_dev_close() calls for the
 * current enumeration round have completed, so that refcounts reflect live
 * namespace devices only.
 *
 * @return 0 on success, negative errno of the last failed destructor on error.
 */
int
xnvme_be_cref_cleanup(void);

#endif /* __INTERNAL_XNVME_BE_CREF_H */
