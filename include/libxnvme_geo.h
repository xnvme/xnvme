#ifndef __LIBXNVME_GEO_H
#define __LIBXNVME_GEO_H
#include <libxnvme_util.h>

/**
 * Representation of the type of device / geo / namespace
 *
 * @enum xnvme_geo_type
 */
enum xnvme_geo_type {
	XNVME_GEO_UNKNOWN      = 0x0,
	XNVME_GEO_CONVENTIONAL = 0x1,
	XNVME_GEO_ZONED        = 0x2
};

/**
 * Representation of device "geometry"
 *
 * This will remain in some, encapsulating IO parameters such as MDTS, ZONE
 * APPEND MDTS, nbytes, nsect etc. mapping to zone characteristics, as well as
 * extended LBA formats.
 *
 * @struct xnvme_geo
 */
struct xnvme_geo {
	enum xnvme_geo_type type;

	uint32_t npugrp; ///< Nr. of Parallel Unit Groups
	uint32_t npunit; ///< Nr. of Parallel Units in PUG

	uint32_t nzone;      ///< Nr. of zones in PU
	uint64_t nsect;      ///< Nr. of sectors per zone
	uint32_t nbytes;     ///< Nr. of bytes per sector
	uint32_t nbytes_oob; ///< Nr. of bytes per sector in OOB

	uint64_t tbytes; ///< Total # bytes in geometry
	uint64_t ssw;    ///< Bit-width for LBA fmt conversion

	uint32_t mdts_nbytes; ///< Maximum-data-transfer-size in unit of bytes

	uint32_t lba_nbytes;  ///< Size of an LBA in bytes
	uint8_t lba_extended; ///< Extended LBA: 1=Supported, 0=Not-Supported

	uint8_t _rsvd[7];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_geo) == 64, "Incorrect size")

#endif
