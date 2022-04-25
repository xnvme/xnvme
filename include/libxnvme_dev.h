#ifndef __LIBXNVME_DEV_H
#define __LIBXNVME_DEV_H
#include <libxnvme.h>

/**
 * Opaque device handle.
 *
 * @see xnvme_dev_open()
 *
 * @struct xnvme_dev
 */
struct xnvme_dev;

/**
 * Returns the geometry of the given device
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 *
 * @return The geometry (struct xnvme_geo) of given device handle
 */
const struct xnvme_geo *
xnvme_dev_get_geo(const struct xnvme_dev *dev);

/**
 * Returns the NVMe identify controller structure associated with the given
 * device
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 *
 * @return On success, pointer to namespace structure. On error, NULL is
 * returned and `errno` is set to indicate the error
 */
const struct xnvme_spec_idfy_ctrlr *
xnvme_dev_get_ctrlr(const struct xnvme_dev *dev);

/**
 * Returns the NVMe identify controller structure specific to the Command Set
 * and Namespace associated with the given device
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 *
 * @return On success, pointer to namespace structure. On error, NULL is
 * returned and `errno` is set to indicate the error
 */
const struct xnvme_spec_idfy_ctrlr *
xnvme_dev_get_ctrlr_css(const struct xnvme_dev *dev);

/**
 * Returns the NVMe identify namespace structure associated with the given
 * device
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 *
 * @return On success, pointer to namespace structure. On error, NULL is
 * returned and `errno` is set to indicate the error
 */
const struct xnvme_spec_idfy_ns *
xnvme_dev_get_ns(const struct xnvme_dev *dev);

/**
 * Returns the NVMe identify namespace structure specific to the Command Set and
 * Namespace associated with the given device
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 *
 * @return On success, pointer to namespace structure. On error, NULL is
 * returned and `errno` is set to indicate the error
 */
const struct xnvme_spec_idfy_ns *
xnvme_dev_get_ns_css(const struct xnvme_dev *dev);

/**
 * Returns the NVMe namespace identifier associated with the given `dev`
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 *
 * @return On success, NVMe namespace identifier is returned
 */
uint32_t
xnvme_dev_get_nsid(const struct xnvme_dev *dev);

/**
 * Returns the NVMe Command Set Identifier associated with the given `dev`
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 *
 * @return On success, NVMe Command Set Identifier is returned
 */
uint8_t
xnvme_dev_get_csi(const struct xnvme_dev *dev);

/**
 * Returns the representation of device identifier once decoded from
 * text-representation for the given `dev`
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 *
 * @return On success, device identifier is returned
 */
const struct xnvme_ident *
xnvme_dev_get_ident(const struct xnvme_dev *dev);

/**
 * Returns the internal backend state of the given `dev`
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 *
 * @return On success, the internal backend state is returned.
 */
const void *
xnvme_dev_get_be_state(const struct xnvme_dev *dev);

/**
 * Returns the sector-shift-width of the device, that is, the value used for
 * converting block-device offset to lba, and vice-versa
 *
 * lba = ofz >> ssw
 * ofz = lba << ssw
 *
 * @return On success, the ssw is returned.
 */
uint64_t
xnvme_dev_get_ssw(const struct xnvme_dev *dev);

#endif /* __LIBXNVME_NVM */
