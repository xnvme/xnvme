/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_driver.h
 *
 * Moving an NVMe controller function between the kernel NVMe driver and a userspace one. The
 * functions and their drivers are found with xnvme_scan(); a dry run of an attach or a detach
 * says what holds each. How a function is moved is the platform's: Linux binds vfio-pci or
 * uio_pci_generic through sysfs, FreeBSD nic_uio. On other platforms every function returns
 * -ENOSYS.
 */

/**
 * Flags for xnvme_driver_attach() and xnvme_driver_detach()
 *
 * @enum xnvme_driver_flag
 */
enum xnvme_driver_flag {
	XNVME_DRIVER_FORCE   = 0x1, ///< Attach a function whose namespaces carry a signature
	XNVME_DRIVER_DRY_RUN = 0x2, ///< Run the checks, change nothing
};

/**
 * The outcome of an attach or detach of one function
 *
 * @struct xnvme_driver_result
 */
struct xnvme_driver_result {
	int err;       ///< 0 when done, or a negative errno; see xnvme_driver_attach()
	char why[508]; ///< Why it was refused or failed, empty when done
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_driver_result) == 512, "Incorrect size")

/**
 * The userspace driver xnvme_driver_attach() binds when given none
 *
 * @return The driver name, or NULL where the platform has none
 */
const char *
xnvme_driver_default(void);

/**
 * The driver an NVMe controller function must be on for the backend 'be' to use it
 *
 * A backend that drives the function itself, such as spdk, libvfn or upcie, needs a userspace
 * driver: vfio-pci for libvfn, else xnvme_driver_default(). Every other backend reaches the
 * device through the kernel, and needs the platform's NVMe driver. 'be' is a backend config's
 * name, as xnvme_be_attr lists it, or the platform's own name for its kernel backends.
 *
 * @param be Name of a backend config, or of the platform
 *
 * @return The driver name. NULL with errno set to ENOENT when 'be' names no backend of the
 * platform, to EINVAL when the backend uses no PCIe function, such as ramdisk or a file, and to
 * ENOSYS where the platform does not move functions between drivers.
 */
const char *
xnvme_driver_for_be(const char *be);

/**
 * Move each of the functions in 'bdfs' off the kernel NVMe driver and onto 'driver'
 *
 * The functions are attached as one set: one sharing an IOMMU group with another in the set is
 * not refused for it. Each function gets an entry in 'results', with 'err' set to 0 when it was
 * attached, or for a dry run would be, and otherwise to:
 *
 * -ENODEV, not an NVMe controller function; -EALREADY, already on 'driver'; -EISCONN, on another
 * userspace driver; -EBUSY, held; -EACCES, open files could not be checked; -EEXIST, a namespace
 * carries a filesystem, partition table or volume signature, which XNVME_DRIVER_FORCE overrides;
 * -EXDEV, its IOMMU group has a member that cannot go along; or the errno of the binding.
 *
 * @param bdfs PCI addresses of the functions
 * @param nbdfs Number of entries in 'bdfs' and 'results'
 * @param driver The userspace driver, or NULL for xnvme_driver_default()
 * @param flags Bitwise OR of #xnvme_driver_flag
 * @param results Array of 'nbdfs' entries to fill
 *
 * @return On success, the number of functions refused or failed, not counting -EALREADY. On error,
 * negative `errno` is returned, -ENOSYS where the platform does not implement it.
 */
int
xnvme_driver_attach(const char *const *bdfs, int nbdfs, const char *driver, int flags,
		    struct xnvme_driver_result *results);

/**
 * Return each of the functions in 'bdfs' from a userspace driver to 'driver'
 *
 * Each function gets an entry in 'results', with 'err' set to 0 when it was detached, or for a
 * dry run would be, and otherwise to -ENODEV, not an NVMe controller function; -EALREADY, not on
 * a userspace driver; -EBUSY, held; -EACCES, open files could not be checked; or the errno of the
 * binding.
 *
 * @param bdfs PCI addresses of the functions
 * @param nbdfs Number of entries in 'bdfs' and 'results'
 * @param driver The kernel driver to return them to, or NULL for the platform's NVMe driver
 * @param flags Bitwise OR of #xnvme_driver_flag
 * @param results Array of 'nbdfs' entries to fill
 *
 * @return On success, the number of functions refused or failed, not counting -EALREADY. On error,
 * negative `errno` is returned, -ENOSYS where the platform does not implement it.
 */
int
xnvme_driver_detach(const char *const *bdfs, int nbdfs, const char *driver, int flags,
		    struct xnvme_driver_result *results);
