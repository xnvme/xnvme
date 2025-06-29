// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * Helpers for 32-bit and 64-bit MMIO read/write access
 * ====================================================
 *
 * This header provides simple functions for accessing memory-mapped I/O (MMIO) regions, typically
 * used for interacting with device registers on PCIe devices. The accessors use `volatile`
 * semantics to prevent the compiler from reordering or eliminating memory operations that may have
 * side effects at the hardware level.
 *
 * These functions assume that MMIO registers are little-endian, as is standard on most PCIe
 * devices. No byte-swapping is performed. As such, this library is not suitable for use on
 * big-endian systems without modification.
 *
 * This implementation has been verified only on x86 systems and may require additional memory
 * barriers or platform-specific instructions to work reliably on weakly ordered architectures
 * (e.g., ARM). The user is responsible for ensuring correct ordering when porting to such
 * platforms.
 *
 * The `region` pointer typically refers to the base of a memory-mapped PCI BAR (e.g., from `struct
 * pci_func_bar.region`), obtained via UIO, VFIO, or similar mechanisms.
 *
 * @file mmio.h
 * @version 0.3.2
 */

/**
 * Read a 32-bit value from an MMIO region at the given offset
 *
 * @param region  Pointer to the base of the MMIO region
 * @param offset  Byte offset from the base
 * @return        32-bit value read from the region
 */
static inline uint32_t
mmio_read32(void *region, uint32_t offset)
{
	return *(volatile uint32_t *)((uintptr_t)region + offset);
}

/**
 * Write a 32-bit value to an MMIO region at the given offset
 *
 * @param region  Pointer to the base of the MMIO region
 * @param offset  Byte offset from the base
 * @param value   32-bit value to write
 */
static inline void
mmio_write32(void *region, uint32_t offset, uint32_t value)
{
	*(volatile uint32_t *)((uintptr_t)region + offset) = value;
}

/**
 * Read a 64-bit value from an MMIO region at the given offset
 *
 * For portability, this performs two 32-bit accesses instead of a single 64-bit access.
 *
 * @param region  Pointer to the base of the MMIO region
 * @param offset  Byte offset from the base
 * @return        64-bit value read from the region
 */
static inline uint64_t
mmio_read64(void *region, uint32_t offset)
{
	uint32_t lo = mmio_read32(region, offset + 0);
	uint32_t hi = mmio_read32(region, offset + 4);

	return ((uint64_t)hi << 32) | lo;
}

/**
 * Write a 64-bit value to an MMIO region at the given offset
 *
 * For portability, this performs two 32-bit accesses instead of a single 64-bit access.
 *
 * @param region  Pointer to the base of the MMIO region
 * @param offset  Byte offset from the base
 * @param value   64-bit value to write
 */
static inline void
mmio_write64(void *region, uint32_t offset, uint64_t val)
{
	mmio_write32(region, offset + 0, (uint32_t)(val & 0xFFFFFFFF));
	mmio_write32(region, offset + 4, (uint32_t)(val >> 32));
}
