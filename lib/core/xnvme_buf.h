// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BUF_H
#define __INTERNAL_XNVME_BUF_H

/**
 * How the buffer helpers, such as xnvme_buf_fill(), move data in a range of memory
 *
 * A backend records it on the registration of a range. The helpers move data in a range without
 * a record with the CPU.
 *
 * @enum xnvme_buf_xfer
 */
enum xnvme_buf_xfer {
	XNVME_BUF_XFER_CPU  = 0, ///< Loads and stores of the CPU
	XNVME_BUF_XFER_CUDA = 1, ///< The memset and memcpy calls of the CUDA runtime
	XNVME_BUF_XFER_HIP  = 2, ///< The memset and memcpy calls of the HIP runtime
};

/**
 * Ask the CUDA and HIP runtimes whether they hold the memory at vaddr
 *
 * For a backend that registers memory which a runtime may hold. In a build without CUDA and HIP,
 * the result is XNVME_BUF_XFER_CPU.
 *
 * @return XNVME_BUF_XFER_CUDA or XNVME_BUF_XFER_HIP for device memory of that runtime, and
 * XNVME_BUF_XFER_CPU for any other memory
 */
enum xnvme_buf_xfer
xnvme_buf_xfer_probe(const void *vaddr);

/**
 * How the buffer helpers move data at vaddr, as a registration records it
 *
 * @return The xfer of the registration holding vaddr, and XNVME_BUF_XFER_CPU when none does
 */
enum xnvme_buf_xfer
xnvme_buf_xfer_of(const void *vaddr);

/**
 * Export the CUDA or HIP allocation holding vaddr as a dma-buf
 *
 * @param vaddr An address of CUDA or HIP device memory
 * @param xfer The runtime holding it, as xnvme_buf_xfer_probe() gives it
 * @param base Set to the start of the allocation
 * @param nbytes Set to the length of the allocation
 * @param fd Set to the dma-buf of the allocation, which the caller closes
 *
 * @return On success, 0 is returned. `-EINVAL` when xfer is neither CUDA nor HIP, and another
 * negative `errno` on error.
 */
int
xnvme_buf_export_dmabuf(void *vaddr, enum xnvme_buf_xfer xfer, void **base, size_t *nbytes,
			int *fd);

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
 * registration. The registrations are linked, unlinked and counted with the control-plane lock
 * held. xnvme_buf_xfer_of() reads vaddr, nbytes and xfer without the control-plane lock, so they
 * must not be written once xnvme_buf_reg_add_xfer() has linked the registration.
 *
 * @struct xnvme_buf_reg
 */
struct xnvme_buf_reg {
	const void *owner; ///< What the range is registered with, such as an IOMMU context
	void *vaddr;
	size_t nbytes;
	uint64_t priv; ///< For the backend, such as a handle to unregister by
	uint32_t refs;
	enum xnvme_buf_xfer xfer; ///< How the buffer helpers move data in the range
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
 * Add a registration of an owner with a reference of one, recording how data in it is moved
 *
 * As xnvme_buf_reg_add(), which records XNVME_BUF_XFER_CPU.
 *
 * @param owner What the range is registered with
 * @param vaddr Start of the range
 * @param nbytes Length of the range
 * @param priv For the backend, such as a handle to unregister by
 * @param xfer How the buffer helpers move data in the range
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_buf_reg_add_xfer(const void *owner, void *vaddr, size_t nbytes, uint64_t priv,
		       enum xnvme_buf_xfer xfer);

/**
 * Find any registration of an owner
 *
 * @param owner What the registrations are registered with
 *
 * @return The registration, or NULL when the owner has none
 */
struct xnvme_buf_reg *
xnvme_buf_reg_any(const void *owner);

/**
 * Remove a registration found with xnvme_buf_reg_find() or xnvme_buf_reg_any()
 *
 * @param reg The registration to remove
 */
void
xnvme_buf_reg_remove(struct xnvme_buf_reg *reg);

#endif /* __INTERNAL_XNVME_BUF_H */
