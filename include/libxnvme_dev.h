/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @headerfile libxnvme_dev.h
 */

enum xnvme_enumerate_action {
	XNVME_ENUMERATE_DEV_KEEP_OPEN = 0x0, ///< Keep device-handle open after callback returns
	XNVME_ENUMERATE_DEV_CLOSE     = 0x1  ///< Close device-handle when callback returns
};

/**
 * Opaque device handle.
 *
 * @see xnvme_dev_open()
 *
 * @struct xnvme_dev
 */
struct xnvme_dev;

/**
 * Signature of callback function used with 'xnvme_enumerate' invoked for each discoved device
 *
 * The callback function signals whether the device-handle it receives should by closed, that is,
 * backend with invoke 'xnvme_dev_close(), after the callback returns or kept open. In the latter
 * case then it is up to the user to invoke 'xnvme_dev_close()' on the device-handle.
 *
 * Each signal is represented by the enum #xnvme_enumerate_action, and the values
 * XNVME_ENUMERATE_DEV_KEEP_OPEN or XNVME_ENUMERATE_DEV_CLOSE.
 */
typedef int (*xnvme_enumerate_cb)(struct xnvme_dev *dev, void *cb_args);

/**
 * enumerate devices
 *
 * @param sys_uri URI of the system to enumerate, when NULL, localhost/PCIe
 * @param opts Options for instrumenting the runtime during enumeration
 * @param cb_func Callback function to invoke for each yielded device
 * @param cb_args Arguments passed to the callback function
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_enumerate(const char *sys_uri, struct xnvme_opts *opts, xnvme_enumerate_cb cb_func,
		void *cb_args);

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
 * Returns the opts struct for the given `dev`
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 *
 * @return On success, device options are returned
 */
const struct xnvme_opts *
xnvme_dev_get_opts(const struct xnvme_dev *dev);

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

/**
 * Creates a device handle (::xnvme_dev) based on the given device-uri
 *
 * @param dev_uri File path "/dev/nvme0n1" or "0000:04.01"
 * @param opts Options for library backend and system-interfaces
 *
 * @return On success, a handle to the device. On error, NULL is returned and `errno` set to
 * indicate the error.
 */
struct xnvme_dev *
xnvme_dev_open(const char *dev_uri, struct xnvme_opts *opts);

/**
 * Destroy the given device handle (::xnvme_dev)
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 */
void
xnvme_dev_close(struct xnvme_dev *dev);
