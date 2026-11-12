// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BUF_H
#define __INTERNAL_XNVME_BUF_H

/**
 * Export the CUDA or HIP allocation holding vaddr as a dma-buf
 *
 * @param vaddr An address of CUDA or HIP device memory
 * @param base Set to the start of the allocation
 * @param nbytes Set to the length of the allocation
 * @param fd Set to the dma-buf of the allocation, which the caller closes
 *
 * @return On success, 0 is returned. `-EINVAL` when vaddr is neither CUDA nor HIP device memory,
 * and another negative `errno` on error.
 */
int
xnvme_buf_export_dmabuf(void *vaddr, void **base, size_t *nbytes, int *fd);

/**
 * Whether the memory at vaddr is host memory: memory of neither the CUDA nor the HIP runtime
 *
 * Host memory is told by what it is not, as there is no telling it apart by itself; the address
 * range of device memory is mapped in the process too. In a build without CUDA and HIP, all
 * memory is host memory.
 */
bool
xnvme_buf_is_host(const void *vaddr);

/**
 * Register memory where the OS pins and translates the payload of each command itself
 *
 * Any memory then serves as command payload, so there is nothing to register; the arguments are
 * checked and nothing is recorded.
 */
int
xnvme_buf_noreg_register(const struct xnvme_dev *dev, void *vaddr, size_t nbytes);

/**
 * Unregister memory registered with xnvme_buf_noreg_register()
 */
int
xnvme_buf_noreg_unregister(const struct xnvme_dev *dev, void *vaddr);

/**
 * The alignment of memory registered with xnvme_buf_noreg_register(): that of the I/O path
 */
int
xnvme_buf_noreg_get_alignment(const struct xnvme_dev *dev, size_t *align);

/**
 * A range of memory a backend registered for an owner, counted
 *
 * Registering the range again, or a range within it, takes a reference rather than a
 * registration. The registrations are touched only with the control-plane lock held.
 *
 * @struct xnvme_buf_reg
 */
struct xnvme_buf_reg {
	const void *owner; ///< What the range is registered with, such as an IOMMU context
	void *vaddr;
	size_t nbytes;
	uint64_t priv; ///< For the backend, such as a handle to unregister by
	uint32_t refs;
	struct xnvme_buf_reg *next;
};

/**
 * Find the registration of an owner holding a range
 *
 * @param owner What the range is registered with
 * @param vaddr Start of the range
 * @param nbytes Length of the range
 * @param reg Set to the registration holding the range, on success
 *
 * @return On success, 0 is returned. `-EBUSY` when a registration overlaps the range without
 * holding it, and `-ENOENT` when none does.
 */
int
xnvme_buf_reg_find(const void *owner, const void *vaddr, size_t nbytes,
		   struct xnvme_buf_reg **reg);

/**
 * Add a registration of an owner with a reference of one
 *
 * @param owner What the range is registered with
 * @param vaddr Start of the range
 * @param nbytes Length of the range
 * @param priv For the backend, such as a handle to unregister by
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_buf_reg_add(const void *owner, void *vaddr, size_t nbytes, uint64_t priv);

/**
 * Remove a registration found with xnvme_buf_reg_find()
 *
 * @param reg The registration to remove
 */
void
xnvme_buf_reg_remove(struct xnvme_buf_reg *reg);

/**
 * Remove every registration of an owner, as when what they are registered with is torn down
 *
 * @param owner What the ranges are registered with
 */
void
xnvme_buf_reg_remove_owner(const void *owner);

#endif /* __INTERNAL_XNVME_BUF_H */
