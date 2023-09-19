/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @headerfile libxnvme_lba.h
 */

struct xnvme_lba_range_attr {
	bool is_zoned; ///< Whether the range covers a zone [zslba, zslba + cap]
	bool is_valid; ///< Whether the range is valid
};

/**
 * Representation of a range of logical-block-addresses aka LBAs
 *
 * @struct xnvme_lba_range
 */
struct xnvme_lba_range {
	uint64_t slba;   ///< Range start-LBA; begins at and includes this address
	uint64_t elba;   ///< Range end-LBA; ends at and includes this address
	uint32_t naddrs; ///< Number of addresses in range [slba, elba]
	uint64_t nbytes; ///< Number of bytes covered by [slba, elba]
	struct xnvme_lba_range_attr attr;
};

/**
 * Prints the given backend attribute to the given output stream
 *
 * @param stream output stream used for printing
 * @param range Pointer to the ::xnvme_lba_range to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_lba_range_fpr(FILE *stream, struct xnvme_lba_range *range, int opts);

/**
 * Prints the given backend attribute to stdout
 *
 * @param range Pointer to the ::xnvme_lba_range to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_lba_range_pr(struct xnvme_lba_range *range, int opts);

/**
 * Initializes a range starting at the given byte-offset and spanning nbytes
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param offset Offset in bytes satisfying offset < capacity
 * @param nbytes Number of bytes, satisfying offset + nbytes < capacity
 *
 * @return On success, an initialized range is returned by value with attr.is_valid = 1. On error,
 * a potential non, or partially, initialized struct is returned by value with attr.is_valid = 0.
 */
struct xnvme_lba_range
xnvme_lba_range_from_offset_nbytes(struct xnvme_dev *dev, uint64_t offset, uint64_t nbytes);

/**
 * Initializes a range starting at the given start-lba (slba) and spanning naddrs
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param slba A valid lba on the given dev
 * @param elba A valid lba on the given dev
 *
 * @return On success, an initialized range is returned by value with attr.is_valid = 1. On error,
 * a potential non, or partially, initialized struct is returned by value with attr.is_valid = 0.
 */
struct xnvme_lba_range
xnvme_lba_range_from_slba_elba(struct xnvme_dev *dev, uint64_t slba, uint64_t elba);

/**
 * Initializes a range starting at the given start-lba (slba) and spanning naddrs
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param slba A valid lba on the given dev
 * @param naddrs A non-zero number of addresses, satisfying slba + naddrs < device capacity
 *
 * @return On success, an initialized range is returned by value with attr.is_valid = 1. On error,
 * a potential non, or partially, initialized struct is returned by value with attr.is_valid = 0.
 */
struct xnvme_lba_range
xnvme_lba_range_from_slba_naddrs(struct xnvme_dev *dev, uint64_t slba, uint64_t naddrs);

/**
 * Initializes a range starting at the given zone-start-LBA and spanning zone-capacity
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param zdescr Pointer to a zone-descriptor valid on the given dev
 *
 * @return On success, an initialized range is returned by value with attr.is_valid = 1. On error,
 * a potential non, or partially, initialized struct is returned by value with attr.is_valid = 0.
 */
struct xnvme_lba_range
xnvme_lba_range_from_zdescr(struct xnvme_dev *dev, struct xnvme_spec_znd_descr *zdescr);
