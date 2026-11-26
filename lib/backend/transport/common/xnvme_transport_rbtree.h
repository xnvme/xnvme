// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/*
 * Copied from
 * http://oopweb.com/Algorithms/Documents/Sman/VolumeFrames.html?/Algorithms/Documents/Sman/Volume/RedBlackTrees_files/s_rbt.htm
 *
 * Disclosure from the author's main page:
 * (http://oopweb.com/Algorithms/Documents/Sman/VolumeFrames.html?/Algorithms/Documents/Sman/Volume/RedBlackTrees_files/s_rbt.htm)
 *
 *     Source code when part of a software project may be used freely
 *     without reference to the author.
 *
 */

#ifndef __INTERNAL_XNVME_TRANSPORT_RBTREE_H
#define __INTERNAL_XNVME_TRANSPORT_RBTREE_H

typedef enum {
	XNVME_TRANSPORT_RBT_STATUS_OK,
	XNVME_TRANSPORT_RBT_STATUS_MEM_EXHAUSTED,
	XNVME_TRANSPORT_RBT_STATUS_DUPLICATE_KEY,
	XNVME_TRANSPORT_RBT_STATUS_KEY_NOT_FOUND
} xnvme_transport_rbt_status;

typedef void *xnvme_transport_rbt_iterator;
typedef void *xnvme_transport_rbt_handle;

xnvme_transport_rbt_handle
xnvme_transport_rbt_new(int (*compare)(void *a, void *b));
// create red-black tree
// parameters:
//     compare  pointer to function that compares keys
//              return 0   if a == b
//              return < 0 if a < b
//              return > 0 if a > b
// returns:
//     handle   use handle in calls to rbt functions

void
xnvme_transport_rbt_delete(xnvme_transport_rbt_handle h);
// destroy red-black tree

xnvme_transport_rbt_status
xnvme_transport_rbt_insert(xnvme_transport_rbt_handle h, void *key, void *value);
// insert key/value pair

xnvme_transport_rbt_status
xnvme_transport_rbt_erase(xnvme_transport_rbt_handle h, xnvme_transport_rbt_iterator i);
// delete node in tree associated with iterator
// this function does not free the key/value pointers

xnvme_transport_rbt_iterator
xnvme_transport_rbt_next(xnvme_transport_rbt_handle h, xnvme_transport_rbt_iterator i);
// return ++i

xnvme_transport_rbt_iterator
xnvme_transport_rbt_begin(xnvme_transport_rbt_handle h);
// return pointer to first node

xnvme_transport_rbt_iterator
xnvme_transport_rbt_end(xnvme_transport_rbt_handle h);
// return pointer to one past last node

void
xnvme_transport_rbt_key_value(xnvme_transport_rbt_handle h, xnvme_transport_rbt_iterator i,
			      void **key, void **value);
// returns key/value pair associated with iterator

void **
xnvme_transport_rbt_value_ptr(xnvme_transport_rbt_handle h, xnvme_transport_rbt_iterator it);
// returns pointer to the value associated with iterator

xnvme_transport_rbt_iterator
xnvme_transport_rbt_find_leftmost(xnvme_transport_rbt_handle h, void *key,
				  int (*compare)(void *a, void *b));
// returns iterator associated with left-most match. This is useful when a new
//   key might invalidate the uniqueness property of the tree.

xnvme_transport_rbt_iterator
xnvme_transport_rbt_find(xnvme_transport_rbt_handle h, void *key);
// returns iterator associated with key

void
xnvme_transport_rbt_traversal(xnvme_transport_rbt_handle h, xnvme_transport_rbt_iterator it,
			      void *handler_arg,
			      void (*handler)(void *arg, xnvme_transport_rbt_iterator it));
// tree traversal that visits (applies handler()) each node in the tree data
//   strucutre exactly once.

void *
xnvme_transport_rbt_root(xnvme_transport_rbt_handle h);
// returns the root of the tree

int
xnvme_transport_rbt_empty(xnvme_transport_rbt_handle h);
// check if red-black tree is empty
// returns 1 if empty, 0 otherwise

#endif /* __INTERNAL_XNVME_TRANSPORT_RBTREE_H */