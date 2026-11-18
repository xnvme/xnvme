/**
 * Prepare the commands of the Zoned Namespace Command Set from records of their fields
 *
 * Written from the data model of senfd 0.26.0, by toolbox/xnvme_cmd_prep.py;
 * each field is named and placed as the figure it cites gives it. Do not edit:
 * see the contributor docs on producing the command preps.
 *
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_cmd_prep_zns.h
 */

#ifndef __LIBXNVME_CMD_PREP_ZNS_H
#define __LIBXNVME_CMD_PREP_ZNS_H

#ifdef __cplusplus
extern "C" {
#endif

/** Defined where the preps of the Zoned Namespace Command Set are declared; a program tests it
 * with `#ifdef` */
#define XNVME_CMD_PREP_ZNS 1

/**
 * The arguments of xnvme_cmd_prep_zns_zone_management_send(): the fields of Zone Management Send
 *
 * See Zoned Namespace 1.5, 3.2, Figure 12, page 20, PDF page 20.
 *
 * The fields of each dword are from:
 *
 * - CDW10-11: Zoned Namespace 1.5, 3.4.3, Figure 39, page 32, PDF page 32
 * - CDW13: Zoned Namespace 1.5, 3.4.3, Figure 40, page 32, PDF page 32
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_zns_zone_management_send_args {
	uint32_t nsid;     ///< Namespace Identifier, CDW1
	uint8_t rsvd4[4];  ///< Reserved
	uint64_t slba;     ///< Starting LBA, CDW10-11 bits 63:00
	uint8_t zm;        ///< Zone Management, CDW13 bits 23:16
	uint8_t zsaso;     ///< Zone Send Action Specific Option, CDW13 bit 9
	uint8_t selall;    ///< Select All, CDW13 bit 8
	uint8_t zsa;       ///< Zone Send Action, CDW13 bits 07:00
	uint8_t rsvd20[4]; ///< Reserved
};

/**
 * Prepare 'sqe' as the command Zone Management Send, opcode 79h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_zns_zone_management_send(), which
 * passes the size of 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_zns_zone_management_send_args
 * @param args_nbytes The size of the record that the program was built with
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned: -EINVAL when 'sqe'
 * or 'args' is NULL, 'args_nbytes' is zero or not a multiple of 8, a reserved byte is not zero,
 * or a value does not fit its field; -E2BIG when 'args_nbytes' is larger than the record and
 * the bytes beyond it are not zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_zns_zone_management_send_sized(
	union xnvme_cmd_sqe *sqe, const struct xnvme_cmd_prep_zns_zone_management_send_args *args,
	size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Zone Management Send from 'args', see
 * xnvme_cmd_prep_zns_zone_management_send_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_zns_zone_management_send_args
 *
 * @return The return value of xnvme_cmd_prep_zns_zone_management_send_sized()
 */
static inline int
xnvme_cmd_prep_zns_zone_management_send(
	union xnvme_cmd_sqe *sqe, const struct xnvme_cmd_prep_zns_zone_management_send_args *args)
{
	return xnvme_cmd_prep_zns_zone_management_send_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_zns_zone_management_receive(): the fields of Zone Management
 * Receive
 *
 * See Zoned Namespace 1.5, 3.2, Figure 12, page 20, PDF page 20.
 *
 * The fields of each dword are from:
 *
 * - CDW10-11: Zoned Namespace 1.5, 3.4.2, Figure 33, page 28, PDF page 28
 * - CDW12: Zoned Namespace 1.5, 3.4.2, Figure 34, page 28, PDF page 28
 * - CDW13: Zoned Namespace 1.5, 3.4.2, Figure 35, page 28, PDF page 28
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_zns_zone_management_receive_args {
	uint32_t nsid;     ///< Namespace Identifier, CDW1
	uint8_t rsvd4[4];  ///< Reserved
	uint64_t slba;     ///< Starting LBA, CDW10-11 bits 63:00
	uint32_t ndw;      ///< Number of Dwords, CDW12 bits 31:00
	uint8_t zraspf;    ///< Zone Receive Action Specific Features, CDW13 bit 16
	uint8_t zras;      ///< Zone Receive Action Specific, CDW13 bits 15:08
	uint8_t zra;       ///< Zone Receive Action, CDW13 bits 07:00
	uint8_t rsvd23[1]; ///< Reserved
};

/**
 * Prepare 'sqe' as the command Zone Management Receive, opcode 7Ah, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_zns_zone_management_receive(), which
 * passes the size of 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_zns_zone_management_receive_args
 * @param args_nbytes The size of the record that the program was built with
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned: -EINVAL when 'sqe'
 * or 'args' is NULL, 'args_nbytes' is zero or not a multiple of 8, a reserved byte is not zero,
 * or a value does not fit its field; -E2BIG when 'args_nbytes' is larger than the record and
 * the bytes beyond it are not zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_zns_zone_management_receive_sized(
	union xnvme_cmd_sqe *sqe,
	const struct xnvme_cmd_prep_zns_zone_management_receive_args *args, size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Zone Management Receive from 'args', see
 * xnvme_cmd_prep_zns_zone_management_receive_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_zns_zone_management_receive_args
 *
 * @return The return value of xnvme_cmd_prep_zns_zone_management_receive_sized()
 */
static inline int
xnvme_cmd_prep_zns_zone_management_receive(
	union xnvme_cmd_sqe *sqe,
	const struct xnvme_cmd_prep_zns_zone_management_receive_args *args)
{
	return xnvme_cmd_prep_zns_zone_management_receive_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_zns_zone_append(): the fields of Zone Append
 *
 * See Zoned Namespace 1.5, 3.2, Figure 12, page 20, PDF page 20.
 *
 * The fields of each dword are from:
 *
 * - CDW2-3: Zoned Namespace 1.5, 3.4.1, Figure 24, page 25, PDF page 25
 * - CDW10-11: Zoned Namespace 1.5, 3.4.1, Figure 25, page 25, PDF page 25
 * - CDW12: Zoned Namespace 1.5, 3.4.1, Figure 26, page 25, PDF page 25
 * - CDW13, if CETYPE is cleared to 0h: Zoned Namespace 1.5, 3.4.1, Figure 27, page 26, PDF page
 * 26
 * - CDW13, if CETYPE is non-zero: Zoned Namespace 1.5, 3.4.1, Figure 28, page 26, PDF page 26
 * - CDW14: Zoned Namespace 1.5, 3.4.1, Figure 29, page 26, PDF page 26
 * - CDW15: Zoned Namespace 1.5, 3.4.1, Figure 30, page 26, PDF page 26
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_zns_zone_append_args {
	uint32_t nsid;     ///< Namespace Identifier, CDW1
	uint8_t rsvd4[4];  ///< Reserved
	uint64_t zslba;    ///< Zone Start Logical Block Address, CDW10-11 bits 63:00
	uint8_t lr;        ///< Limited Retry, CDW12 bit 31
	uint8_t fua;       ///< Force Unit Access, CDW12 bit 30
	uint8_t prinfo;    ///< Protection Information, CDW12 bits 29:26
	uint8_t piremap;   ///< Protection Information Remap, CDW12 bit 25
	uint8_t stc;       ///< Storage Tag Check, CDW12 bit 24
	uint8_t dtype;     ///< Directive Type, CDW12 bits 23:20
	uint8_t cetype;    ///< Command Extension Type, CDW12 bits 19:16
	uint8_t rsvd23[1]; ///< Reserved
	uint16_t nlb;      ///< Number of Logical Blocks, CDW12 bits 15:00
	uint16_t dspec;    ///< Directive Specific, CDW13 bits 31:16
	uint16_t cev;      ///< Command Extension Value, CDW13 bits 15:00
	uint16_t lbatm;    ///< Logical Block Application Tag Mask, CDW15 bits 31:16
	uint16_t lbat;     ///< Logical Block Application Tag, CDW15 bits 15:00
	uint8_t rsvd34[2]; ///< Reserved
	uint32_t lbtl;     ///< Logical Block Tags Lower, CDW14 bits 31:00
	uint64_t lbtu;     ///< Logical Block Tags Upper, CDW2-3 bits 47:00
};

/**
 * Prepare 'sqe' as the command Zone Append, opcode 7Dh, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_zns_zone_append(), which passes the
 * size of 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_zns_zone_append_args
 * @param args_nbytes The size of the record that the program was built with
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned: -EINVAL when 'sqe'
 * or 'args' is NULL, 'args_nbytes' is zero or not a multiple of 8, a reserved byte is not zero,
 * a value does not fit its field, or a field is not zero while its condition does not hold;
 * -E2BIG when 'args_nbytes' is larger than the record and the bytes beyond it are not zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_zns_zone_append_sized(union xnvme_cmd_sqe *sqe,
				     const struct xnvme_cmd_prep_zns_zone_append_args *args,
				     size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Zone Append from 'args', see xnvme_cmd_prep_zns_zone_append_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_zns_zone_append_args
 *
 * @return The return value of xnvme_cmd_prep_zns_zone_append_sized()
 */
static inline int
xnvme_cmd_prep_zns_zone_append(union xnvme_cmd_sqe *sqe,
			       const struct xnvme_cmd_prep_zns_zone_append_args *args)
{
	return xnvme_cmd_prep_zns_zone_append_sized(sqe, args, sizeof(*args));
}

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_CMD_PREP_ZNS_H */
