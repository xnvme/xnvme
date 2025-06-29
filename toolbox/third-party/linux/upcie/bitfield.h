// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * Bitfield utilities
 * ==================
 *
 * Helpers for working with individual bits and bitfields in low-level code. Designed for clarity
 * and performance in systems, protocol, and register-level programming.
 *
 * Features
 * --------
 * - bitfield_mask(): Returns a bitmask for a given offset and width
 * - bitfield_get():  Extracts a bitfield from a 64-bit value
 * - bitfield_set():  Inserts a bitfield into a 64-bit value
 *
 * Intended usage includes manipulating hardware registers, encoding protocol fields, or
 * packing/unpacking structured data into compact bit layouts.
 *
 * Example usage:
 *
 *   uint64_t reg = 0;
 *   reg = bitfield_set(reg, 0, 4, 0x5);    // Set bits 0–3
 *   reg = bitfield_set(reg, 4, 4, 0xA);    // Set bits 4–7
 *   reg = bitfield_set(reg, 8, 8, 0xFF);   // Set bits 8–15
 *
 *   uint64_t x = bitfield_get(reg, 4, 4);  // Extract bits 4–7
 *
 * This header is intended to grow with additional bit manipulation utilities over time.
 *
 * @file bitfield.h
 * @version 0.3.2
 */

/**
 * Returns a bitmask for a given offset and width.
 */
static inline uint64_t
bitfield_mask(uint8_t offset, uint8_t width)
{
	return (width == 64) ? ~0ULL : ((1ULL << width) - 1) << offset;
}

/**
 * Extracts a bitfield from a 64-bit integer.
 */
static inline uint64_t
bitfield_get(uint64_t val, uint8_t offset, uint8_t width)
{
	return (val & bitfield_mask(offset, width)) >> offset;
}

/**
 * Sets a bitfield in a 64-bit integer.
 */
static inline uint64_t
bitfield_set(uint64_t val, uint8_t offset, uint8_t width, uint64_t field)
{
	return (val & ~bitfield_mask(offset, width)) |
	       ((field << offset) & bitfield_mask(offset, width));
}
