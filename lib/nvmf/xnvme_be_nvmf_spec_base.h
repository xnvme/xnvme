#ifndef _INTERNAL_XNVME_BE_NVMF_SPEC_BASE_H
#define _INTERNAL_XNVME_BE_NVMF_SPEC_BASE_H

#include <stdint.h>
#include <libxnvme.h>

/**
 * Base-spec NVMe controller registers reached through NVMe-oF Property Get
 * and Property Set: Controller Capabilities (CAP), Controller Configuration
 * (CC), and Controller Status (CSTS).
 *
 * These are base-spec, not Fabrics-specific, but they are kept private to
 * lib/nvmf for now. They use `xnvme_spec_*`-style bitfield layouts, matching
 * the existing `struct xnvme_spec_ctrlr_bar`, so they can move into
 * `libxnvme_spec.h` verbatim later.
 */

struct nvme_ctrlr_cap {
	union {
		struct {
			uint64_t mqes    : 16; // Maximum Queue Entries Supported, bits 15:0
			uint64_t cqr     : 1;  // Contiguous Queues Required, bit 16
			uint64_t ams     : 2;  // Arbitration Mechanism Supported bits 18:17
			uint64_t rsvd    : 5;  // Reserved, bits 23:19
			uint64_t to      : 8;  // Timeout, bits 31:24
			uint64_t dstrd   : 4;  // Doorbell Stride, bits 35:32
			uint64_t nssrs   : 1;  // NVMe Subsystem Reset Supported, bit 36
			uint64_t ncss    : 1;  // NVMe Command Set Support, bit 37
			uint64_t rsvd2   : 5;  // Reserved, bits 42:38
			uint64_t iocss   : 1;  // I/O Command Set Supported, bit 43
			uint64_t noiocss : 1;  // No I/O Command Set Supported, bit 44
			uint64_t bps     : 1;  // Boot Partition Support, bit 45
			uint64_t cps     : 2;  // Controller Power Scope, bits 47:46
			uint64_t mpsmin  : 4;  // Memory Page Size Minimum, bits 51:48
			uint64_t mpsmax  : 4;  // Memory Page Size Maximum, bits 55:52
			uint64_t pmrs    : 1;  // Persistent Memory Region Supported, bit 56
			uint64_t cmbs    : 1;  // Controller Memory Buffer Supported, bit 57
			uint64_t nsss    : 1;  // NVMe Subsystem Shutdown Supported, bit 58
			uint64_t crwms   : 1;  // Controller Ready with Media Support, bit 59
			uint64_t crims
				: 1; // Controller Ready Independent of Media Support, bit 60
			uint64_t nsses
				: 1; // NVMe Subsystem Shutdown Enhancements Supported, bit 61
			uint64_t rsvd3 : 2; // Reserved, bits 63:62
		} __attribute__((packed));
		uint64_t raw;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct nvme_ctrlr_cap) == 8, "Incorrect size");

struct nvme_ctrlr_cc {
	union {
		struct {
			uint32_t en     : 1; // Enable, bit 0
			uint32_t rsvd   : 3; // Reserved, bits 3:1
			uint32_t css    : 3; // I/O Command Set Selected, bits 6:4
			uint32_t mps    : 4; // Memory Page Size, bits 10:7
			uint32_t ams    : 3; // Arbitration Mechanism Selected, bits 13:11
			uint32_t shn    : 2; // Shutdown Notification, bits 15:14
			uint32_t iosqes : 4; // I/O Submission Queue Entry Size, bits 19:16
			uint32_t iocqes : 4; // I/O Completion Queue Entry Size, bits 23:20
			uint32_t crime : 1; // Controller Ready Independent of Media Enable, bit 24
			uint32_t rsvd3 : 7; // Reserved, bits 31:25
		};
		uint32_t raw;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct nvme_ctrlr_cc) == 4, "Incorrect size");

struct nvme_ctrlr_csts {
	union {
		struct {
			uint32_t rdy   : 1;  // Ready, bit 0
			uint32_t cfs   : 1;  // Controller Fatal Status, bit 1
			uint32_t shst  : 2;  // Shutdown Status, bits 3:2
			uint32_t nssro : 1;  // NVMe Subsystem Shutdown Ready, bit 4
			uint32_t pp    : 1;  // Processing Paused, bit 5
			uint32_t st    : 1;  // Shutdown Type , bit 6
			uint32_t rsvd  : 25; // Reserved, bits 31:7
		};
		uint32_t raw;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct nvme_ctrlr_csts) == 4, "Incorrect size");

#endif /* _INTERNAL_XNVME_BE_NVMF_SPEC_BASE_H */
