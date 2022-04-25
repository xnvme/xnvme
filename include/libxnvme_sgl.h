/**
 * libxnvme_sgl - NVMe Scatter-Gather Lists
 *
 * Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
 * Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * @headerfile libxnvme_sgl.h
 */
#ifndef __LIBXNVME_SGL_H
#define __LIBXNVME_SGL_H
#include <libxnvme.h>

/**
 * Opaque handle for Scatter Gather List (SGL).
 *
 * @struct xnvme_sgl
 */
struct xnvme_sgl;

/**
 * Opaque handle for Scatter Gather List (SGL) pool
 *
 * @note A separate pool should be used for each asynchronous context
 *
 * @struct xnvme_sgl_pool
 */
struct xnvme_sgl_pool;

/**
 * Create an pool of Scather-Gather-List (SGL)
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 *
 * @return An initialized pool that can be used to amortize the cost of repeated
 * SGL allocations
 */
struct xnvme_sgl_pool *
xnvme_sgl_pool_create(struct xnvme_dev *dev);

/**
 * Destroy an SGL pool (and all SGLs in the pool).
 *
 * @param pool Pointer to the #xnvme_sgl_pool to destroy
 */
void
xnvme_sgl_pool_destroy(struct xnvme_sgl_pool *pool);

/**
 * Create a Scatter-Gather-List (SGL)
 *
 * @see xnvme_sgl_destroy
 * @see xnvme_sgl_alloc
 * @see xnvme_sgl_free
 * @see xnvme_sgl_add
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param hint Allocation hint.
 *
 * @return On success, an initialized (empty) SGL is returned. On error, NULL is
 * returned and `errno` is set to indicate any error.
 */
struct xnvme_sgl *
xnvme_sgl_create(struct xnvme_dev *dev, int hint);

/**
 * Destroy an SGL, freeing the memory used
 *
 * @see xnvme_sgl_free
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param sgl Pointer to SGL as allocated by xnvme_sgl_alloc() or
 * xnvme_sgl_create()
 */
void
xnvme_sgl_destroy(struct xnvme_dev *dev, struct xnvme_sgl *sgl);

/**
 * Allocate an SGL from a pool
 *
 * @see xnvme_sgl_create
 *
 * @param pool Pool to allocate SGL from
 *
 * @return On success, an initialized (empty) SGL is returned. On error, NULL is
 * returned and `errno` is set to indicate any error.
 */
struct xnvme_sgl *
xnvme_sgl_alloc(struct xnvme_sgl_pool *pool);

/**
 * Free an SGL, returning it to the pool
 *
 * @see xnvme_sgl_destroy
 *
 * @param pool Pool to return the given `sgl` to
 * @param sgl Pointer to SGL as allocated by xnvme_sgl_alloc() or
 *  xnvme_sgl_create()
 */
void
xnvme_sgl_free(struct xnvme_sgl_pool *pool, struct xnvme_sgl *sgl);

/**
 * Reset an SGL
 *
 * @param sgl Pointer to SGL as allocated by xnvme_sgl_alloc() or
 * xnvme_sgl_create()
 */
void
xnvme_sgl_reset(struct xnvme_sgl *sgl);

/**
 * Add an entry to the SGL
 *
 * @see xnvme_sgl_alloc
 * @see xnvme_buf_alloc
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param sgl Pointer to sgl as allocated by xnvme_sgl_alloc()
 * @param buf Pointer to buffer as allocated with xnvme_buf_alloc()
 * @param nbytes Size of the given buffer in bytes
 */
int
xnvme_sgl_add(struct xnvme_dev *dev, struct xnvme_sgl *sgl, void *buf, size_t nbytes);

#endif
