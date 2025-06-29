// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * NVMe MMIO operations wrapper: functions and bitfield accessors
 * ==============================================================
 *
 * This header provides minimal helper functions for interacting with NVMe controller registers
 * via memory-mapped I/O (MMIO). It assumes the presence of PCIe MMIO primitives defined in
 * "mmio.h".
 *
 * The goal is to keep things simple—no heuristics for timeout values, no automatic waiting after
 * enabling or disabling the controller. Each function performs a single MMIO operation, leaving
 * policy and sequencing decisions to the caller.
 *
 * Note: In some cases (e.g., CC.EN), register bits must be updated via read–modify–write
 * to avoid overwriting other configuration fields. These helpers assume the caller expects
 * only the minimal required field to be changed, and that other fields remain unmodified.
 *
 * @file nvme_mmio.h
 * @version 0.3.2
 */

#define NVME_REG_CAP 0x00
#define NVME_REG_VS 0x08
#define NVME_REG_INTMS 0x0C
#define NVME_REG_INTMC 0x10

#define NVME_REG_CC 0x14
#define NVME_REG_CSTS 0x1C
#define NVME_REG_AQA 0x24
#define NVME_REG_ASQ 0x28
#define NVME_REG_ACQ 0x30

/**
 * Controller Capabilities: Maximum Queue Entries Supported (MQES)
 */
static inline uint16_t
nvme_reg_cap_get_mqes(uint64_t cap)
{
	return bitfield_get(cap, 0, 16);
}

/**
 * Controller Capabilities: Contiguous Queues Required (CQR)
 */
static inline uint8_t
nvme_reg_cap_get_cqr(uint64_t cap)
{
	return bitfield_get(cap, 16, 1);
}

/**
 * Controller Capabilities: Arbitration Mechanism Supported (AMS)
 */
static inline uint8_t
nvme_reg_cap_get_ams(uint64_t cap)
{
	return bitfield_get(cap, 17, 2);
}

/**
 * Controller Capabilities: Timeout (TO)
 */
static inline uint8_t
nvme_reg_cap_get_to(uint64_t cap)
{
	return bitfield_get(cap, 24, 8);
}

/**
 * Controller Capabilities: Doorbell Stride (DSTRD)
 */
static inline uint8_t
nvme_reg_cap_get_dstrd(uint64_t cap)
{
	return bitfield_get(cap, 32, 4);
}

/**
 * Controller Capabilities: NVM Subsystem Reset Supported (NSSRS)
 */
static inline uint8_t
nvme_reg_cap_get_nssrs(uint64_t cap)
{
	return bitfield_get(cap, 36, 1);
}

/**
 * Controller Capabilities: Command Sets Supported (CSS)
 */
static inline uint8_t
nvme_reg_cap_get_css(uint64_t cap)
{
	return bitfield_get(cap, 37, 8);
}

/**
 * Controller Capabilities: Boot Partition Support (BPS)
 */
static inline uint8_t
nvme_reg_cap_get_bps(uint64_t cap)
{
	return bitfield_get(cap, 45, 1);
}

/**
 * Controller Capabilities: Controller Power Scope (CPS)
 */
static inline uint8_t
nvme_reg_cap_get_cps(uint64_t cap)
{
	return bitfield_get(cap, 46, 2);
}

/**
 * Controller Capabilities: Memory Page Size Minimum (MPSMIN)
 */
static inline uint8_t
nvme_reg_cap_get_mpsmin(uint64_t cap)
{
	return bitfield_get(cap, 48, 4);
}

/**
 * Controller Capabilities: Memory Page Size Maximum (MPSMAX)
 */
static inline uint8_t
nvme_reg_cap_get_mpsmax(uint64_t cap)
{
	return bitfield_get(cap, 52, 4);
}

/**
 * Controller Capabilities: Persistent Memory Region Supported (PMRS)
 */
static inline uint8_t
nvme_reg_cap_get_pmrs(uint64_t cap)
{
	return bitfield_get(cap, 56, 1);
}

/**
 * Controller Capabilities: Controller Memory Buffer Supported (CMBS)
 */
static inline uint8_t
nvme_reg_cap_get_cmbs(uint64_t cap)
{
	return bitfield_get(cap, 57, 1);
}

/**
 * Controller Capabilities: NVM Subsystem Shutdown Supported (NSSS)
 */
static inline uint8_t
nvme_reg_cap_get_nsss(uint64_t cap)
{
	return bitfield_get(cap, 58, 1);
}

/**
 * Controller Capabilities: Controller Ready Modes Supported (CRMS)
 */
static inline uint8_t
nvme_reg_cap_get_crms(uint64_t cap)
{
	return bitfield_get(cap, 59, 2);
}

/**
 * Controller Capabilities: NVM Subsystem Shutdown Enhancements Supported (NSSES)
 */
static inline uint8_t
nvme_reg_cap_get_nsses(uint64_t cap)
{
	return bitfield_get(cap, 61, 1);
}

/**
 * Setup admin-queue (aq) properties
 *
 * Assumptions
 * ===========
 *
 * - controller is disabled
 *
 * @param bar0 A memory-mapped region pointing to the start of bar0
 * @param asq DMA-able address pointing to the start of the admin submission queue
 * @param aqsize Must be power-of-two and less than or equal to MQES + 1
 */
static inline void
nvme_mmio_aq_setup(void *bar0, uint64_t asq, uint64_t acq, uint32_t aqsize)
{
	uint32_t aqa = ((aqsize - 1) << 16) | (aqsize - 1);

	mmio_write64(bar0, NVME_REG_ASQ, asq);
	mmio_write64(bar0, NVME_REG_ACQ, acq);
	mmio_write32(bar0, NVME_REG_AQA, aqa);
}

static inline uint32_t
nvme_mmio_cc_read(void *bar0)
{
	return mmio_read32(bar0, NVME_REG_CC);
}

static inline uint64_t
nvme_mmio_cap_read(void *bar0)
{
	return mmio_read64(bar0, NVME_REG_CAP);
}

static inline uint32_t
nvme_mmio_csts_read(void *bar0)
{
	return mmio_read32(bar0, NVME_REG_CSTS);
}

/**
 * Enable the current controller configuration
 */
static inline void
nvme_mmio_cc_write(void *bar0, uint32_t cc)
{
	mmio_write32(bar0, NVME_REG_CC, cc);
}

/**
 * Enable the current controller configuration
 */
static inline void
nvme_mmio_cc_enable(void *bar0)
{
	uint32_t cc = nvme_mmio_cc_read(bar0);

	nvme_mmio_cc_write(bar0, cc | 0x1);
}

/**
 * Disable the current controller-configuration.
 *
 * Note: Disabling takes effect asynchronously. Thus, you must wait for the controller to report
 * CSTS.RDY = 0 before proceeding. Use nvme_mmio_wait_until_not_ready() with a proper timeout
 * value.
 */
static inline void
nvme_mmio_cc_disable(void *bar0)
{
	uint32_t cc = nvme_mmio_cc_read(bar0);

	nvme_mmio_cc_write(bar0, cc & ~0x1);
}

/**
 * Wait until CSTS.RDY == 1 (controller is ready)
 *
 * Note: a sound choice for timeout_us would be CAP.TO, also read the spec on the other sensible
 * timeout choices based on CC.CRIME and CRTO.CRWMT.
 */
static inline int
nvme_mmio_csts_wait_until_ready(void *mmio, int timeout_ms)
{
	for (int elapsed = 0; elapsed < timeout_ms; ++elapsed) {
		if ((nvme_mmio_csts_read(mmio) & 0x1) == 0x1) {
			return 0;
		}
		usleep(1000);
	}
	return -ETIMEDOUT;
}

/**
 * Wait until CSTS.RDY == 0 (controller is not ready)
 *
 * Note: a sound choice for timeout_us would be CAP.TO, also read the spec on the other sensible
 * timeout choices based on CC.CRIME and CRTO.CRWMT.
 */
static inline int
nvme_mmio_csts_wait_until_not_ready(void *mmio, int timeout_ms)
{
	for (int elapsed = 0; elapsed < timeout_ms; ++elapsed) {
		if ((nvme_mmio_csts_read(mmio) & 0x1) == 0x0) {
			return 0;
		}
		usleep(1000);
	}
	return -ETIMEDOUT;
}

static inline int
nvme_reg_cap_pr(uint64_t cap)
{
	int wrtn = 0;

	wrtn += printf("CAP = 0x%016" PRIx64 "\n", cap);
	wrtn += printf("  mqes:   %u # max queue entries supported\n", nvme_reg_cap_get_mqes(cap));
	wrtn += printf("  cqr:    %u # contiguous queues required\n", nvme_reg_cap_get_cqr(cap));
	wrtn += printf("  ams:    %u # arbitration mechanisms supported\n",
		       nvme_reg_cap_get_ams(cap));
	wrtn += printf("  to:     %u # timeout in 500ms units (=> %u ms)\n",
		       nvme_reg_cap_get_to(cap), nvme_reg_cap_get_to(cap) * 500);
	wrtn += printf("  dstrd:  %u # doorbell stride (2^n bytes)\n",
		       nvme_reg_cap_get_dstrd(cap));
	wrtn += printf("  nssrs:  %u # NVM subsystem reset supported\n",
		       nvme_reg_cap_get_nssrs(cap));
	wrtn += printf("  css:    0x%02x # command sets supported\n", nvme_reg_cap_get_css(cap));
	wrtn += printf("  bps:    %u # boot partition support\n", nvme_reg_cap_get_bps(cap));
	wrtn += printf("  cps:    %u # controller power scope\n", nvme_reg_cap_get_cps(cap));
	wrtn += printf("  mpsmin: %u # memory page size min (2^(12+mpsmin))\n",
		       nvme_reg_cap_get_mpsmin(cap));
	wrtn += printf("  mpsmax: %u # memory page size max (2^(12+mpsmax))\n",
		       nvme_reg_cap_get_mpsmax(cap));
	wrtn += printf("  pmrs:   %u # persistent memory region supported\n",
		       nvme_reg_cap_get_pmrs(cap));
	wrtn += printf("  cmbs:   %u # controller memory buffer supported\n",
		       nvme_reg_cap_get_cmbs(cap));
	wrtn += printf("  nsss:   %u # NVM subsystem shutdown supported\n",
		       nvme_reg_cap_get_nsss(cap));
	wrtn += printf("  crms:   %u # controller ready modes supported\n",
		       nvme_reg_cap_get_crms(cap));
	wrtn += printf("  nsses:  %u # shutdown enhancements supported\n",
		       nvme_reg_cap_get_nsses(cap));

	return wrtn;
}

static inline int
nvme_reg_csts_pr(uint32_t val)
{
	int wrtn = 0;

	wrtn += printf("nvme_reg_csts:\n");
	wrtn += printf("  rdy    : %u   # Controller Ready\n", (unsigned)bitfield_get(val, 0, 1));
	wrtn += printf("  cfs    : %u   # Controller Fatal Status\n",
		       (unsigned)bitfield_get(val, 1, 1));
	wrtn += printf("  shst   : %u   # Shutdown Status\n", (unsigned)bitfield_get(val, 2, 2));
	wrtn += printf("  nssro  : %u   # NVM Subsystem Reset Occurred\n",
		       (unsigned)bitfield_get(val, 4, 1));
	wrtn += printf("  pp     : %u   # Processing Pause\n", (unsigned)bitfield_get(val, 5, 1));
	wrtn += printf("  st     : %u   # Shutdown Type\n", (unsigned)bitfield_get(val, 6, 1));

	return wrtn;
}

/**
 * Controller Configuration: Enable (EN)
 * Bit 0
 */
static inline uint8_t
nvme_reg_cc_get_en(uint32_t cc)
{
	return bitfield_get(cc, 0, 1);
}

/**
 * Controller Configuration: I/O Command Set Selected (CSS)
 * Bits 4–7
 */
static inline uint8_t
nvme_reg_cc_get_css(uint32_t cc)
{
	return bitfield_get(cc, 4, 4);
}

/**
 * Controller Configuration: Memory Page Size (MPS)
 * Bits 7–10
 */
static inline uint8_t
nvme_reg_cc_get_mps(uint32_t cc)
{
	return bitfield_get(cc, 7, 4);
}

/**
 * Controller Configuration: Arbitration Mechanism Selected (AMS)
 * Bits 11–13
 */
static inline uint8_t
nvme_reg_cc_get_ams(uint32_t cc)
{
	return bitfield_get(cc, 11, 3);
}

/**
 * Controller Configuration: Contiguous Queues Required (CQR)
 * Bit 16
 */
static inline uint8_t
nvme_reg_cc_get_cqr(uint32_t cc)
{
	return bitfield_get(cc, 16, 1);
}

/**
 * Controller Configuration: Shutdown Notification (SHN)
 * Bits 14–15
 */
static inline uint8_t
nvme_reg_cc_get_shn(uint32_t cc)
{
	return bitfield_get(cc, 14, 2);
}

/**
 * Controller Configuration: I/O Completion Queue Entry Size (IOCQES)
 * Bits 20–23
 */
static inline uint8_t
nvme_reg_cc_get_iocqes(uint32_t cc)
{
	return bitfield_get(cc, 20, 4);
}

/**
 * Controller Configuration: I/O Submission Queue Entry Size (IOSQES)
 * Bits 24–27
 */
static inline uint8_t
nvme_reg_cc_get_iosqes(uint32_t cc)
{
	return bitfield_get(cc, 24, 4);
}

static inline int
nvme_reg_cc_pr(uint32_t cc)
{
	int wrtn = 0;

	wrtn += printf("CC  = 0x%08" PRIx32 "\n", cc);
	wrtn += printf("  en:     %u # enable\n", nvme_reg_cc_get_en(cc));
	wrtn += printf("  cqr:    %u # contiguous queues required\n", nvme_reg_cc_get_cqr(cc));
	wrtn += printf("  ams:    %u # arbitration mechanism selected\n", nvme_reg_cc_get_ams(cc));
	wrtn += printf("  shn:    %u # shutdown notification\n", nvme_reg_cc_get_shn(cc));
	wrtn += printf("  iosqes: %u # I/O submission queue entry size (2^(n+2))\n",
		       nvme_reg_cc_get_iosqes(cc));
	wrtn += printf("  iocqes: %u # I/O completion queue entry size (2^(n+2))\n",
		       nvme_reg_cc_get_iocqes(cc));
	wrtn += printf("  mps:    %u # memory page size (2^(12+mps))\n", nvme_reg_cc_get_mps(cc));
	wrtn += printf("  css:    %u # command set selected\n", nvme_reg_cc_get_css(cc));

	return wrtn;
}

/**
 * Set the Enable (EN) field (bit 0)
 */
static inline uint32_t
nvme_reg_cc_set_en(uint32_t cc, uint8_t val)
{
	return bitfield_set(cc, 0, 1, val);
}

/**
 * Set the I/O Command Set Selected (CSS) field (bits 4–7)
 */
static inline uint32_t
nvme_reg_cc_set_css(uint32_t cc, uint8_t val)
{
	return bitfield_set(cc, 4, 3, val);
}

/**
 * Set the Memory Page Size (MPS) field (bits 7–10)
 */
static inline uint32_t
nvme_reg_cc_set_mps(uint32_t cc, uint8_t val)
{
	return bitfield_set(cc, 7, 4, val);
}

/**
 * Set the Arbitration Mechanism Selected (AMS) field (bits 11–13)
 */
static inline uint32_t
nvme_reg_cc_set_ams(uint32_t cc, uint8_t val)
{
	return bitfield_set(cc, 11, 3, val);
}

/**
 * Set the Shutdown Notification (SHN) field (bits 14–15)
 */
static inline uint32_t
nvme_reg_cc_set_shn(uint32_t cc, uint8_t val)
{
	return bitfield_set(cc, 14, 2, val);
}

/**
 * Set the I/O Submission Queue Entry Size (IOSQES) field (bits 16-19)
 */
static inline uint32_t
nvme_reg_cc_set_iosqes(uint32_t cc, uint8_t val)
{
	return bitfield_set(cc, 16, 4, val);
}

/**
 * Set the I/O Completion Queue Entry Size (IOCQES) field (bits 20–23)
 */
static inline uint32_t
nvme_reg_cc_set_iocqes(uint32_t cc, uint8_t val)
{
	return bitfield_set(cc, 20, 4, val);
}

/**
 * Controller Ready Independent of Media Enable (CRIME) field (bit 24)
 */
static inline uint32_t
nvme_reg_cc_set_crime(uint32_t cc, uint8_t val)
{
	return bitfield_set(cc, 24, 1, val);
}