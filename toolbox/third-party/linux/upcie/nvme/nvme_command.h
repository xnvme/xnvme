// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * Rudimentary Representation of Commands and Completions
 * ======================================================
 *
 * This header defines minimal representations of NVMe commands and their completions,
 * suitable for low-level or embedded NVMe driver implementations.
 *
 * @file nvme_command.h
 * @version 0.3.2
 */

struct nvme_completion {
	uint32_t cdw0;
	uint32_t rsvd;
	uint16_t sqhd;
	uint16_t sqid;
	uint16_t cid;
	uint16_t status;
};

struct nvme_command {
	uint8_t opc;
	uint8_t fuse;
	uint16_t cid;
	uint32_t nsid;
	uint64_t rsvd2;
	uint64_t mptr;
	uint64_t prp1;
	uint64_t prp2;
	uint32_t cdw10;
	uint32_t cdw11;
	uint32_t cdw12;
	uint32_t cdw13;
	uint32_t cdw14;
	uint32_t cdw15;
};
