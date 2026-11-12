/**
 * What the rdma specification defines, by xnvme_spec
 *
 * Written from the data model of senfd 0.19.0, by toolbox/xnvme_spec_headers.py;
 * each name and mask is as the figure it cites gives it. Do not
 * edit: see the contributor docs on producing the spec headers.
 *
 * @file libxnvme_spec_rdma.h
 */
#ifndef LIBXNVME_SPEC_RDMA_H
#define LIBXNVME_SPEC_RDMA_H

#include <stdint.h>

/**
 * Transport Specific Address Subtype Definition for RDMA Transport (NVMe over RDMA Transport 1.3, 3.1.1, Figure 3, page 10, PDF page 10)
 */
struct __attribute__((packed)) xnvme_spec_rdma_transport_specific_address_subtype_definition_for_rdma_transport {
	uint8_t rdma_qp_service_type; ///< RDMA QP Service Type, byte 0
	uint8_t rdma_provider_type; ///< RDMA Provider Type, byte 1
	uint8_t rdma_connection_management_service; ///< RDMA Connection Management Service, byte 2
	uint8_t rsvd3[5]; ///< Reserved, bytes 7:3
	uint16_t rdma_pkey; ///< RDMA Partition Key, bytes 9:8
	uint8_t rsvd10[246]; ///< Reserved, bytes 255:10
};

/**
 * RDMA_CM_REQUEST Private Data Format (NVMe over RDMA Transport 1.3, 3.1.4, Figure 5, page 12, PDF page 12)
 */
struct __attribute__((packed)) xnvme_spec_rdma_rdma_cm_request_private_data_format {
	uint16_t recfmt; ///< Record Format, bytes 1:0
	uint16_t qid; ///< Queue ID, bytes 3:2
	uint16_t hrqsize; ///< RDMA QP Host Receive Queue Size, bytes 5:4
	uint16_t hsqsize; ///< RDMA QP Host Send Queue Size, bytes 7:6
	uint16_t cntlid; ///< Controller ID, bytes 9:8
	uint8_t rsvd10[22]; ///< Reserved, bytes 31:10
};

/**
 * RDMA_CM_ACCEPT Private Data Format (NVMe over RDMA Transport 1.3, 3.1.4, Figure 6, page 12, PDF page 12)
 */
struct __attribute__((packed)) xnvme_spec_rdma_rdma_cm_accept_private_data_format {
	uint16_t recfmt; ///< Record Format, bytes 1:0
	uint16_t crqsize; ///< RDMA QP Controller Receive Queue Size, bytes 3:2
	uint8_t rsvd4[28]; ///< Reserved, bytes 31:4
};

/**
 * RDMA_CM_REJECT Private Data Format (NVMe over RDMA Transport 1.3, 3.1.4, Figure 7, page 12, PDF page 12)
 */
struct __attribute__((packed)) xnvme_spec_rdma_rdma_cm_reject_private_data_format {
	uint16_t recfmt; ///< Record Format, bytes 1:0
	uint16_t sts; ///< Status, bytes 3:2
};

#endif /* LIBXNVME_SPEC_RDMA_H */
