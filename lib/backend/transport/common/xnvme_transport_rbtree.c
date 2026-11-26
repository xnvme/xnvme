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

// reentrant red-black tree

#include <stdlib.h>

#include <libxnvme.h>

#include "xnvme_transport_rbtree.h"

typedef enum { XNVME_TRANSPORT_RBT_BLACK, XNVME_TRANSPORT_RBT_RED } xnvme_transport_rbt_color;

typedef struct xnvme_transport_rbt_node_tag {
	struct xnvme_transport_rbt_node_tag *left;   // left child
	struct xnvme_transport_rbt_node_tag *right;  // right child
	struct xnvme_transport_rbt_node_tag *parent; // parent
	xnvme_transport_rbt_color
		color; // node color (XNVME_TRANSPORT_RBT_BLACK, XNVME_TRANSPORT_RBT_RED)
	void *key;     // key used for searching
	void *val;     // user data
} xnvme_transport_rbt_node;

typedef struct xnvme_transport_rbt_tag {
	xnvme_transport_rbt_node *root; // root of red-black tree
	xnvme_transport_rbt_node sentinel;
	int (*compare)(void *a, void *b); // compare keys
} xnvme_transport_rbt;

// all leafs are sentinels
#define SENTINEL &rbt->sentinel

xnvme_transport_rbt_handle
xnvme_transport_rbt_new(int (*rbtCompare)(void *a, void *b))
{
	xnvme_transport_rbt *rbt;

	if ((rbt = (xnvme_transport_rbt *)malloc(sizeof(xnvme_transport_rbt))) == NULL) {
		return NULL;
	}

	rbt->compare = rbtCompare;
	rbt->root = SENTINEL;
	rbt->sentinel.left = SENTINEL;
	rbt->sentinel.right = SENTINEL;
	rbt->sentinel.parent = NULL;
	rbt->sentinel.color = XNVME_TRANSPORT_RBT_BLACK;
	rbt->sentinel.key = NULL;
	rbt->sentinel.val = NULL;

	return rbt;
}

static void
deleteTree(xnvme_transport_rbt_handle h, xnvme_transport_rbt_node *p)
{
	xnvme_transport_rbt *rbt = h;

	// erase nodes depth-first
	if (p == SENTINEL)
		return;
	deleteTree(h, p->left);
	deleteTree(h, p->right);
	free(p);
}

int
xnvme_transport_rbt_empty(xnvme_transport_rbt_handle h)
{
	xnvme_transport_rbt *rbt = h;
	return rbt->root == SENTINEL;
}

void
xnvme_transport_rbt_delete(xnvme_transport_rbt_handle h)
{
	xnvme_transport_rbt *rbt = h;

	deleteTree(h, rbt->root);
	free(rbt);
}

static void
rotateLeft(xnvme_transport_rbt *rbt, xnvme_transport_rbt_node *x)
{

	// rotate node x to left

	xnvme_transport_rbt_node *y = x->right;

	// establish x->right link
	x->right = y->left;
	if (y->left != SENTINEL)
		y->left->parent = x;

	// establish y->parent link
	if (y != SENTINEL)
		y->parent = x->parent;
	if (x->parent) {
		if (x == x->parent->left)
			x->parent->left = y;
		else
			x->parent->right = y;
	} else {
		rbt->root = y;
	}

	// link x and y
	y->left = x;
	if (x != SENTINEL)
		x->parent = y;
}

static void
rotateRight(xnvme_transport_rbt *rbt, xnvme_transport_rbt_node *x)
{

	// rotate node x to right

	xnvme_transport_rbt_node *y = x->left;

	// establish x->left link
	x->left = y->right;
	if (y->right != SENTINEL)
		y->right->parent = x;

	// establish y->parent link
	if (y != SENTINEL)
		y->parent = x->parent;
	if (x->parent) {
		if (x == x->parent->right)
			x->parent->right = y;
		else
			x->parent->left = y;
	} else {
		rbt->root = y;
	}

	// link x and y
	y->right = x;
	if (x != SENTINEL)
		x->parent = y;
}

static void
insertFixup(xnvme_transport_rbt *rbt, xnvme_transport_rbt_node *x)
{

	// maintain red-black tree balance after inserting node x

	// check red-black properties
	while (x != rbt->root && x->parent->color == XNVME_TRANSPORT_RBT_RED) {
		// we have a violation
		if (x->parent == x->parent->parent->left) {
			xnvme_transport_rbt_node *y = x->parent->parent->right;
			if (y->color == XNVME_TRANSPORT_RBT_RED) {

				// uncle is XNVME_TRANSPORT_RBT_RED
				x->parent->color = XNVME_TRANSPORT_RBT_BLACK;
				y->color = XNVME_TRANSPORT_RBT_BLACK;
				x->parent->parent->color = XNVME_TRANSPORT_RBT_RED;
				x = x->parent->parent;
			} else {

				// uncle is XNVME_TRANSPORT_RBT_BLACK
				if (x == x->parent->right) {
					// make x a left child
					x = x->parent;
					rotateLeft(rbt, x);
				}

				// recolor and rotate
				x->parent->color = XNVME_TRANSPORT_RBT_BLACK;
				x->parent->parent->color = XNVME_TRANSPORT_RBT_RED;
				rotateRight(rbt, x->parent->parent);
			}
		} else {

			// mirror image of above code
			xnvme_transport_rbt_node *y = x->parent->parent->left;
			if (y->color == XNVME_TRANSPORT_RBT_RED) {

				// uncle is XNVME_TRANSPORT_RBT_RED
				x->parent->color = XNVME_TRANSPORT_RBT_BLACK;
				y->color = XNVME_TRANSPORT_RBT_BLACK;
				x->parent->parent->color = XNVME_TRANSPORT_RBT_RED;
				x = x->parent->parent;
			} else {

				// uncle is XNVME_TRANSPORT_RBT_BLACK
				if (x == x->parent->left) {
					x = x->parent;
					rotateRight(rbt, x);
				}
				x->parent->color = XNVME_TRANSPORT_RBT_BLACK;
				x->parent->parent->color = XNVME_TRANSPORT_RBT_RED;
				rotateLeft(rbt, x->parent->parent);
			}
		}
	}
	rbt->root->color = XNVME_TRANSPORT_RBT_BLACK;
}

xnvme_transport_rbt_status
xnvme_transport_rbt_insert(xnvme_transport_rbt_handle h, void *key, void *val)
{
	xnvme_transport_rbt_node *current, *parent, *x;
	xnvme_transport_rbt *rbt = h;

	// allocate node for data and insert in tree

	// find future parent
	current = rbt->root;
	parent = 0;
	while (current != SENTINEL) {
		int rc = rbt->compare(key, current->key);
		if (rc == 0)
			return XNVME_TRANSPORT_RBT_STATUS_DUPLICATE_KEY;
		parent = current;
		current = (rc < 0) ? current->left : current->right;
	}

	// setup new node
	if ((x = malloc(sizeof(*x))) == 0)
		return XNVME_TRANSPORT_RBT_STATUS_MEM_EXHAUSTED;
	x->parent = parent;
	x->left = SENTINEL;
	x->right = SENTINEL;
	x->color = XNVME_TRANSPORT_RBT_RED;
	x->key = key;
	x->val = val;

	// insert node in tree
	if (parent) {
		if (rbt->compare(key, parent->key) < 0)
			parent->left = x;
		else
			parent->right = x;
	} else {
		rbt->root = x;
	}

	insertFixup(rbt, x);

	return XNVME_TRANSPORT_RBT_STATUS_OK;
}

static void
deleteFixup(xnvme_transport_rbt *rbt, xnvme_transport_rbt_node *x)
{

	// maintain red-black tree balance after deleting node x

	while (x != rbt->root && x->color == XNVME_TRANSPORT_RBT_BLACK) {
		if (x == x->parent->left) {
			xnvme_transport_rbt_node *w = x->parent->right;
			if (w->color == XNVME_TRANSPORT_RBT_RED) {
				w->color = XNVME_TRANSPORT_RBT_BLACK;
				x->parent->color = XNVME_TRANSPORT_RBT_RED;
				rotateLeft(rbt, x->parent);
				w = x->parent->right;
			}
			if (w->left->color == XNVME_TRANSPORT_RBT_BLACK &&
			    w->right->color == XNVME_TRANSPORT_RBT_BLACK) {
				w->color = XNVME_TRANSPORT_RBT_RED;
				x = x->parent;
			} else {
				if (w->right->color == XNVME_TRANSPORT_RBT_BLACK) {
					w->left->color = XNVME_TRANSPORT_RBT_BLACK;
					w->color = XNVME_TRANSPORT_RBT_RED;
					rotateRight(rbt, w);
					w = x->parent->right;
				}
				w->color = x->parent->color;
				x->parent->color = XNVME_TRANSPORT_RBT_BLACK;
				w->right->color = XNVME_TRANSPORT_RBT_BLACK;
				rotateLeft(rbt, x->parent);
				x = rbt->root;
			}
		} else {
			xnvme_transport_rbt_node *w = x->parent->left;
			if (w->color == XNVME_TRANSPORT_RBT_RED) {
				w->color = XNVME_TRANSPORT_RBT_BLACK;
				x->parent->color = XNVME_TRANSPORT_RBT_RED;
				rotateRight(rbt, x->parent);
				w = x->parent->left;
			}
			if (w->right->color == XNVME_TRANSPORT_RBT_BLACK &&
			    w->left->color == XNVME_TRANSPORT_RBT_BLACK) {
				w->color = XNVME_TRANSPORT_RBT_RED;
				x = x->parent;
			} else {
				if (w->left->color == XNVME_TRANSPORT_RBT_BLACK) {
					w->right->color = XNVME_TRANSPORT_RBT_BLACK;
					w->color = XNVME_TRANSPORT_RBT_RED;
					rotateLeft(rbt, w);
					w = x->parent->left;
				}
				w->color = x->parent->color;
				x->parent->color = XNVME_TRANSPORT_RBT_BLACK;
				w->left->color = XNVME_TRANSPORT_RBT_BLACK;
				rotateRight(rbt, x->parent);
				x = rbt->root;
			}
		}
	}
	x->color = XNVME_TRANSPORT_RBT_BLACK;
}

xnvme_transport_rbt_status
xnvme_transport_rbt_erase(xnvme_transport_rbt_handle h, xnvme_transport_rbt_iterator i)
{
	xnvme_transport_rbt_node *x, *y;
	xnvme_transport_rbt *rbt = h;
	xnvme_transport_rbt_node *z = i;

	if (z->left == SENTINEL || z->right == SENTINEL) {
		// y has a SENTINEL node as a child
		y = z;
	} else {
		// find tree successor with a SENTINEL node as a child
		y = z->right;
		while (y->left != SENTINEL)
			y = y->left;
	}

	// x is y's only child
	if (y->left != SENTINEL)
		x = y->left;
	else
		x = y->right;

	// remove y from the parent chain
	x->parent = y->parent;
	if (y->parent)
		if (y == y->parent->left)
			y->parent->left = x;
		else
			y->parent->right = x;
	else
		rbt->root = x;

	if (y != z) {
		z->key = y->key;
		z->val = y->val;
	}

	if (y->color == XNVME_TRANSPORT_RBT_BLACK)
		deleteFixup(rbt, x);

	free(y);

	return XNVME_TRANSPORT_RBT_STATUS_OK;
}

xnvme_transport_rbt_iterator
xnvme_transport_rbt_next(xnvme_transport_rbt_handle h, xnvme_transport_rbt_iterator it)
{
	xnvme_transport_rbt *rbt = h;
	xnvme_transport_rbt_node *i = it;

	if (i->right != SENTINEL) {
		// go right 1, then left to the end
		for (i = i->right; i->left != SENTINEL; i = i->left)
			;
	} else {
		// while you're the right child, chain up parent link
		xnvme_transport_rbt_node *p = i->parent;
		while (p && i == p->right) {
			i = p;
			p = p->parent;
		}

		// return the "inorder" node
		i = p;
	}
	return i != SENTINEL ? i : NULL;
}

xnvme_transport_rbt_iterator
xnvme_transport_rbt_begin(xnvme_transport_rbt_handle h)
{
	xnvme_transport_rbt *rbt = h;

	// return pointer to first value
	xnvme_transport_rbt_node *i;
	for (i = rbt->root; i->left != SENTINEL; i = i->left)
		;
	return i != SENTINEL ? i : NULL;
}

xnvme_transport_rbt_iterator
xnvme_transport_rbt_end(xnvme_transport_rbt_handle XNVME_UNUSED(h))
{
	// return pointer to one past last value
	return NULL;
}

void
xnvme_transport_rbt_key_value(xnvme_transport_rbt_handle XNVME_UNUSED(h),
			      xnvme_transport_rbt_iterator it, void **key, void **val)
{
	xnvme_transport_rbt_node *i = it;

	*key = i->key;
	*val = i->val;
}

void **
xnvme_transport_rbt_value_ptr(xnvme_transport_rbt_handle XNVME_UNUSED(h),
			      xnvme_transport_rbt_iterator it)
{
	xnvme_transport_rbt_node *i = it;

	return &i->val;
}

void *
xnvme_transport_rbt_find_leftmost(xnvme_transport_rbt_handle h, void *key,
				  int (*compare)(void *a, void *b))
{
	xnvme_transport_rbt *rbt = h;
	xnvme_transport_rbt_node *current = rbt->root;
	xnvme_transport_rbt_node *found = NULL;

	while (current != SENTINEL) {
		int rc = compare(key, current->key);

		if (rc == 0) {
			found = current;
			current = current->left;
		} else if (found) {
			if (rc == 1)
				current = current->right;
			else
				return found;
		} else {
			current = (rc < 0) ? current->left : current->right;
		}
	}

	return found;
}

void *
xnvme_transport_rbt_find(xnvme_transport_rbt_handle h, void *key)
{
	xnvme_transport_rbt *rbt = h;

	xnvme_transport_rbt_node *current;
	current = rbt->root;
	while (current != SENTINEL) {
		int rc = rbt->compare(key, current->key);
		if (rc == 0)
			return current;
		current = (rc < 0) ? current->left : current->right;
	}
	return NULL;
}

void
xnvme_transport_rbt_traversal(xnvme_transport_rbt_handle h, xnvme_transport_rbt_iterator it,
			      void *handler_arg,
			      void (*handler)(void *arg, xnvme_transport_rbt_iterator it))
{
	xnvme_transport_rbt *rbt = h;
	xnvme_transport_rbt_node *root = it;

	// apply handler for:
	// -o the root of the tree/subtree
	handler(handler_arg, it);
	// - the left subtree
	if (root->left != SENTINEL)
		xnvme_transport_rbt_traversal(h, root->left, handler_arg, handler);
	// - the right subtree
	if (root->right != SENTINEL)
		xnvme_transport_rbt_traversal(h, root->right, handler_arg, handler);
}

void *
xnvme_transport_rbt_root(xnvme_transport_rbt_handle h)
{
	xnvme_transport_rbt *rbt = h;
	return rbt->root;
}