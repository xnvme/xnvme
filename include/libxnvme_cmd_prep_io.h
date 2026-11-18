/**
 * Prepare the commands of the I/O Commands from records of their fields
 *
 * Written from the data model of senfd 0.26.0, by toolbox/xnvme_cmd_prep.py;
 * each field is named and placed as the figure it cites gives it. Do not edit:
 * see the contributor docs on producing the command preps.
 *
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_cmd_prep_io.h
 */

#ifndef __LIBXNVME_CMD_PREP_IO_H
#define __LIBXNVME_CMD_PREP_IO_H

#ifdef __cplusplus
extern "C" {
#endif

/** Defined where the preps of the I/O Commands are declared; a program tests it with `#ifdef` */
#define XNVME_CMD_PREP_IO 1

/**
 * The arguments of xnvme_cmd_prep_io_flush(): the fields of Flush
 *
 * See Base 2.4, 7, Figure 645, page 560, PDF page 590.
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_io_flush_args {
	uint32_t nsid;    ///< Namespace Identifier, CDW1
	uint8_t rsvd4[4]; ///< Reserved
};

/**
 * Prepare 'sqe' as the command Flush, opcode 00h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_io_flush(), which passes the size of
 * 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_io_flush_args
 * @param args_nbytes The size of the record that the program was built with
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned: -EINVAL when 'sqe'
 * or 'args' is NULL, 'args_nbytes' is zero or not a multiple of 8, or a reserved byte is not
 * zero; -E2BIG when 'args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_io_flush_sized(union xnvme_cmd_sqe *sqe,
			      const struct xnvme_cmd_prep_io_flush_args *args, size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Flush from 'args', see xnvme_cmd_prep_io_flush_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_io_flush_args
 *
 * @return The return value of xnvme_cmd_prep_io_flush_sized()
 */
static inline int
xnvme_cmd_prep_io_flush(union xnvme_cmd_sqe *sqe, const struct xnvme_cmd_prep_io_flush_args *args)
{
	return xnvme_cmd_prep_io_flush_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_io_reservation_register(): the fields of Reservation Register
 *
 * See Base 2.4, 7, Figure 645, page 560, PDF page 590.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 7.6, Figure 663, page 569, PDF page 599
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_io_reservation_register_args {
	uint32_t nsid;   ///< Namespace Identifier, CDW1
	uint8_t cptpl;   ///< Change Persist Through Power Loss State, CDW10 bits 31:30
	uint8_t disnsrs; ///< Dispersed Namespace Reservation Support, CDW10 bit 4
	uint8_t iekey;   ///< Ignore Existing Key, CDW10 bit 3
	uint8_t rrega;   ///< Reservation Register Action, CDW10 bits 02:00
};

/**
 * Prepare 'sqe' as the command Reservation Register, opcode 0Dh, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_io_reservation_register(), which
 * passes the size of 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_io_reservation_register_args
 * @param args_nbytes The size of the record that the program was built with
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned: -EINVAL when 'sqe'
 * or 'args' is NULL, 'args_nbytes' is zero or not a multiple of 8, or a value does not fit its
 * field; -E2BIG when 'args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_io_reservation_register_sized(
	union xnvme_cmd_sqe *sqe, const struct xnvme_cmd_prep_io_reservation_register_args *args,
	size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Reservation Register from 'args', see
 * xnvme_cmd_prep_io_reservation_register_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_io_reservation_register_args
 *
 * @return The return value of xnvme_cmd_prep_io_reservation_register_sized()
 */
static inline int
xnvme_cmd_prep_io_reservation_register(
	union xnvme_cmd_sqe *sqe, const struct xnvme_cmd_prep_io_reservation_register_args *args)
{
	return xnvme_cmd_prep_io_reservation_register_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_io_reservation_report(): the fields of Reservation Report
 *
 * See Base 2.4, 7, Figure 645, page 560, PDF page 590.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 7.8, Figure 669, page 571, PDF page 601
 * - CDW11: Base 2.4, 7.8, Figure 670, page 571, PDF page 601
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_io_reservation_report_args {
	uint32_t nsid;     ///< Namespace Identifier, CDW1
	uint32_t numd;     ///< Number of Dwords, CDW10 bits 31:00
	uint8_t disnsrs;   ///< Dispersed Namespace Reservation Support, CDW11 bit 1
	uint8_t eds;       ///< Extended Data Structure, CDW11 bit 0
	uint8_t rsvd10[6]; ///< Reserved
};

/**
 * Prepare 'sqe' as the command Reservation Report, opcode 0Eh, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_io_reservation_report(), which
 * passes the size of 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_io_reservation_report_args
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
xnvme_cmd_prep_io_reservation_report_sized(
	union xnvme_cmd_sqe *sqe, const struct xnvme_cmd_prep_io_reservation_report_args *args,
	size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Reservation Report from 'args', see
 * xnvme_cmd_prep_io_reservation_report_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_io_reservation_report_args
 *
 * @return The return value of xnvme_cmd_prep_io_reservation_report_sized()
 */
static inline int
xnvme_cmd_prep_io_reservation_report(union xnvme_cmd_sqe *sqe,
				     const struct xnvme_cmd_prep_io_reservation_report_args *args)
{
	return xnvme_cmd_prep_io_reservation_report_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_io_reservation_acquire(): the fields of Reservation Acquire
 *
 * See Base 2.4, 7, Figure 645, page 560, PDF page 590.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 7.5, Figure 659, page 567, PDF page 597
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_io_reservation_acquire_args {
	uint32_t nsid;   ///< Namespace Identifier, CDW1
	uint8_t rtype;   ///< Reservation Type, CDW10 bits 15:08
	uint8_t disnsrs; ///< Dispersed Namespace Reservation Support, CDW10 bit 4
	uint8_t iekey;   ///< Ignore Existing Key, CDW10 bit 3
	uint8_t racqa;   ///< Reservation Acquire Action, CDW10 bits 02:00
};

/**
 * Prepare 'sqe' as the command Reservation Acquire, opcode 11h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_io_reservation_acquire(), which
 * passes the size of 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_io_reservation_acquire_args
 * @param args_nbytes The size of the record that the program was built with
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned: -EINVAL when 'sqe'
 * or 'args' is NULL, 'args_nbytes' is zero or not a multiple of 8, or a value does not fit its
 * field; -E2BIG when 'args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_io_reservation_acquire_sized(
	union xnvme_cmd_sqe *sqe, const struct xnvme_cmd_prep_io_reservation_acquire_args *args,
	size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Reservation Acquire from 'args', see
 * xnvme_cmd_prep_io_reservation_acquire_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_io_reservation_acquire_args
 *
 * @return The return value of xnvme_cmd_prep_io_reservation_acquire_sized()
 */
static inline int
xnvme_cmd_prep_io_reservation_acquire(
	union xnvme_cmd_sqe *sqe, const struct xnvme_cmd_prep_io_reservation_acquire_args *args)
{
	return xnvme_cmd_prep_io_reservation_acquire_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_io_io_management_receive(): the fields of I/O Management Receive
 *
 * See Base 2.4, 7, Figure 645, page 560, PDF page 590.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 7.3, Figure 651, page 564, PDF page 594
 * - CDW11: Base 2.4, 7.3, Figure 652, page 564, PDF page 594
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_io_io_management_receive_args {
	uint32_t nsid;     ///< Namespace Identifier, CDW1
	uint16_t mos;      ///< Management Operation Specific, CDW10 bits 31:16
	uint8_t mo;        ///< Management Operation, CDW10 bits 07:00
	uint8_t rsvd7[1];  ///< Reserved
	uint32_t numd;     ///< Number of Dwords, CDW11 bits 31:00
	uint8_t rsvd12[4]; ///< Reserved
};

/**
 * Prepare 'sqe' as the command I/O Management Receive, opcode 12h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_io_io_management_receive(), which
 * passes the size of 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_io_io_management_receive_args
 * @param args_nbytes The size of the record that the program was built with
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned: -EINVAL when 'sqe'
 * or 'args' is NULL, 'args_nbytes' is zero or not a multiple of 8, or a reserved byte is not
 * zero; -E2BIG when 'args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_io_io_management_receive_sized(
	union xnvme_cmd_sqe *sqe, const struct xnvme_cmd_prep_io_io_management_receive_args *args,
	size_t args_nbytes);

/**
 * Prepare 'sqe' as the command I/O Management Receive from 'args', see
 * xnvme_cmd_prep_io_io_management_receive_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_io_io_management_receive_args
 *
 * @return The return value of xnvme_cmd_prep_io_io_management_receive_sized()
 */
static inline int
xnvme_cmd_prep_io_io_management_receive(
	union xnvme_cmd_sqe *sqe, const struct xnvme_cmd_prep_io_io_management_receive_args *args)
{
	return xnvme_cmd_prep_io_io_management_receive_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_io_reservation_release(): the fields of Reservation Release
 *
 * See Base 2.4, 7, Figure 645, page 560, PDF page 590.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 7.7, Figure 666, page 570, PDF page 600
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_io_reservation_release_args {
	uint32_t nsid;   ///< Namespace Identifier, CDW1
	uint8_t rtype;   ///< Reservation Type, CDW10 bits 15:08
	uint8_t disnsrs; ///< Dispersed Namespace Reservation Support, CDW10 bit 4
	uint8_t iekey;   ///< Ignore Existing Key, CDW10 bit 3
	uint8_t rrela;   ///< Reservation Release Action, CDW10 bits 02:00
};

/**
 * Prepare 'sqe' as the command Reservation Release, opcode 15h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_io_reservation_release(), which
 * passes the size of 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_io_reservation_release_args
 * @param args_nbytes The size of the record that the program was built with
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned: -EINVAL when 'sqe'
 * or 'args' is NULL, 'args_nbytes' is zero or not a multiple of 8, or a value does not fit its
 * field; -E2BIG when 'args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_io_reservation_release_sized(
	union xnvme_cmd_sqe *sqe, const struct xnvme_cmd_prep_io_reservation_release_args *args,
	size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Reservation Release from 'args', see
 * xnvme_cmd_prep_io_reservation_release_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_io_reservation_release_args
 *
 * @return The return value of xnvme_cmd_prep_io_reservation_release_sized()
 */
static inline int
xnvme_cmd_prep_io_reservation_release(
	union xnvme_cmd_sqe *sqe, const struct xnvme_cmd_prep_io_reservation_release_args *args)
{
	return xnvme_cmd_prep_io_reservation_release_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_io_cancel(): the fields of Cancel
 *
 * See Base 2.4, 7, Figure 645, page 560, PDF page 590.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 7.1, Figure 646, page 561, PDF page 591
 * - CDW11: Base 2.4, 7.1, Figure 647, page 562, PDF page 592
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_io_cancel_args {
	uint32_t nsid;    ///< Namespace Identifier, CDW1
	uint16_t cid;     ///< Command Identifier, CDW10 bits 31:16
	uint16_t sqid;    ///< Submission Queue Identifier, CDW10 bits 15:00
	uint8_t acode;    ///< Action Code, CDW11 bits 01:00
	uint8_t rsvd9[7]; ///< Reserved
};

/**
 * Prepare 'sqe' as the command Cancel, opcode 18h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_io_cancel(), which passes the size
 * of 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_io_cancel_args
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
xnvme_cmd_prep_io_cancel_sized(union xnvme_cmd_sqe *sqe,
			       const struct xnvme_cmd_prep_io_cancel_args *args,
			       size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Cancel from 'args', see xnvme_cmd_prep_io_cancel_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_io_cancel_args
 *
 * @return The return value of xnvme_cmd_prep_io_cancel_sized()
 */
static inline int
xnvme_cmd_prep_io_cancel(union xnvme_cmd_sqe *sqe,
			 const struct xnvme_cmd_prep_io_cancel_args *args)
{
	return xnvme_cmd_prep_io_cancel_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_io_io_management_send(): the fields of I/O Management Send
 *
 * See Base 2.4, 7, Figure 645, page 560, PDF page 590.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 7.4, Figure 655, page 565, PDF page 595
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_io_io_management_send_args {
	uint32_t nsid;    ///< Namespace Identifier, CDW1
	uint16_t mos;     ///< Management Operation Specific, CDW10 bits 31:16
	uint8_t mo;       ///< Management Operation, CDW10 bits 07:00
	uint8_t rsvd7[1]; ///< Reserved
};

/**
 * Prepare 'sqe' as the command I/O Management Send, opcode 1Dh, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_io_io_management_send(), which
 * passes the size of 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_io_io_management_send_args
 * @param args_nbytes The size of the record that the program was built with
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned: -EINVAL when 'sqe'
 * or 'args' is NULL, 'args_nbytes' is zero or not a multiple of 8, or a reserved byte is not
 * zero; -E2BIG when 'args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_io_io_management_send_sized(
	union xnvme_cmd_sqe *sqe, const struct xnvme_cmd_prep_io_io_management_send_args *args,
	size_t args_nbytes);

/**
 * Prepare 'sqe' as the command I/O Management Send from 'args', see
 * xnvme_cmd_prep_io_io_management_send_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_io_io_management_send_args
 *
 * @return The return value of xnvme_cmd_prep_io_io_management_send_sized()
 */
static inline int
xnvme_cmd_prep_io_io_management_send(union xnvme_cmd_sqe *sqe,
				     const struct xnvme_cmd_prep_io_io_management_send_args *args)
{
	return xnvme_cmd_prep_io_io_management_send_sized(sqe, args, sizeof(*args));
}

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_CMD_PREP_IO_H */
