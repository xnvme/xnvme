/**
 * Prepare the commands of the NVM Command Set from records of their fields
 *
 * Written from the data model of senfd 0.26.0, by toolbox/xnvme_cmd_prep.py;
 * each field is named and placed as the figure it cites gives it. Do not edit:
 * see the contributor docs on producing the command preps.
 *
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_cmd_prep_nvm.h
 */

#ifndef __LIBXNVME_CMD_PREP_NVM_H
#define __LIBXNVME_CMD_PREP_NVM_H

#ifdef __cplusplus
extern "C" {
#endif

/** Defined where the preps of the NVM Command Set are declared; a program tests it with `#ifdef`
 */
#define XNVME_CMD_PREP_NVM 1

/**
 * The arguments of xnvme_cmd_prep_nvm_write(): the fields of Write
 *
 * See NVM 1.3, 3.3, Figure 22, page 27, PDF page 27.
 *
 * The fields of each dword are from:
 *
 * - CDW2-3: NVM 1.3, 3.3.6, Figure 69, page 54, PDF page 54
 * - CDW10-11: NVM 1.3, 3.3.6, Figure 70, page 54, PDF page 54
 * - CDW12: NVM 1.3, 3.3.6, Figure 71, page 54, PDF page 54
 * - CDW13, if CETYPE is cleared to 0h: NVM 1.3, 3.3.6, Figure 72, page 54, PDF page 54
 * - CDW13, if CETYPE is non-zero: NVM 1.3, 3.3.6, Figure 73, page 55, PDF page 55
 * - CDW14: NVM 1.3, 3.3.6, Figure 74, page 55, PDF page 55
 * - CDW15: NVM 1.3, 3.3.6, Figure 75, page 55, PDF page 55
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_nvm_write_args {
	uint32_t nsid;     ///< Namespace Identifier, CDW1
	uint8_t rsvd4[4];  ///< Reserved
	uint64_t slba;     ///< Starting LBA, CDW10-11 bits 63:00
	uint8_t lr;        ///< Limited Retry, CDW12 bit 31
	uint8_t fua;       ///< Force Unit Access, CDW12 bit 30
	uint8_t prinfo;    ///< Protection Information, CDW12 bits 29:26
	uint8_t stc;       ///< Storage Tag Check, CDW12 bit 24
	uint8_t dtype;     ///< Directive Type, CDW12 bits 23:20
	uint8_t cetype;    ///< Command Extension Type, CDW12 bits 19:16
	uint16_t nlb;      ///< Number of Logical Blocks, CDW12 bits 15:00
	uint16_t dspec;    ///< Directive Specific, CDW13 bits 31:16
	uint8_t dsm;       ///< Dataset Management, CDW13 bits 07:00
	uint8_t rsvd27[1]; ///< Reserved
	uint16_t cev;      ///< Command Extension Value, CDW13 bits 15:00
	uint16_t lbatm;    ///< Logical Block Application Tag Mask, CDW15 bits 31:16
	uint16_t lbat;     ///< Logical Block Application Tag, CDW15 bits 15:00
	uint8_t rsvd34[2]; ///< Reserved
	uint32_t lbtl;     ///< Logical Block Tags Lower, CDW14 bits 31:00
	uint64_t lbtu;     ///< Logical Block Tags Upper, CDW2-3 bits 47:00
};

/**
 * Prepare 'sqe' as the command Write, opcode 01h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_nvm_write(), which passes the size
 * of 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_nvm_write_args
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
xnvme_cmd_prep_nvm_write_sized(union xnvme_spec_sqe *sqe,
			       const struct xnvme_cmd_prep_nvm_write_args *args,
			       size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Write from 'args', see xnvme_cmd_prep_nvm_write_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_nvm_write_args
 *
 * @return The return value of xnvme_cmd_prep_nvm_write_sized()
 */
static inline int
xnvme_cmd_prep_nvm_write(union xnvme_spec_sqe *sqe,
			 const struct xnvme_cmd_prep_nvm_write_args *args)
{
	return xnvme_cmd_prep_nvm_write_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_nvm_read(): the fields of Read
 *
 * See NVM 1.3, 3.3, Figure 22, page 27, PDF page 27.
 *
 * The fields of each dword are from:
 *
 * - CDW2-3: NVM 1.3, 3.3.4, Figure 52, page 49, PDF page 49
 * - CDW10-11: NVM 1.3, 3.3.4, Figure 53, page 49, PDF page 49
 * - CDW12: NVM 1.3, 3.3.4, Figure 54, page 49, PDF page 49
 * - CDW13, if CETYPE is cleared to 0h: NVM 1.3, 3.3.4, Figure 55, page 50, PDF page 50
 * - CDW13, if CETYPE is non-zero: NVM 1.3, 3.3.4, Figure 56, page 50, PDF page 50
 * - CDW14: NVM 1.3, 3.3.4, Figure 57, page 50, PDF page 50
 * - CDW15: NVM 1.3, 3.3.4, Figure 58, page 51, PDF page 51
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_nvm_read_args {
	uint32_t nsid;     ///< Namespace Identifier, CDW1
	uint8_t rsvd4[4];  ///< Reserved
	uint64_t slba;     ///< Starting LBA, CDW10-11 bits 63:00
	uint8_t lr;        ///< Limited Retry, CDW12 bit 31
	uint8_t fua;       ///< Force Unit Access, CDW12 bit 30
	uint8_t prinfo;    ///< Protection Information, CDW12 bits 29:26
	uint8_t stc;       ///< Storage Tag Check, CDW12 bit 24
	uint8_t cetype;    ///< Command Extension Type, CDW12 bits 19:16
	uint8_t rsvd21[1]; ///< Reserved
	uint16_t nlb;      ///< Number of Logical Blocks, CDW12 bits 15:00
	uint8_t dsm;       ///< Dataset Management, CDW13 bits 07:00
	uint8_t rsvd25[1]; ///< Reserved
	uint16_t cev;      ///< Command Extension Value, CDW13 bits 15:00
	uint16_t elbatm;   ///< Expected Logical Block Application Tag Mask, CDW15 bits 31:16
	uint16_t elbat;    ///< Expected Logical Block Application Tag, CDW15 bits 15:00
	uint32_t elbtl;    ///< Expected Logical Block Tags Lower, CDW14 bits 31:00
	uint8_t rsvd36[4]; ///< Reserved
	uint64_t elbtu;    ///< Expected Logical Block Tags Upper, CDW2-3 bits 47:00
};

/**
 * Prepare 'sqe' as the command Read, opcode 02h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_nvm_read(), which passes the size of
 * 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_nvm_read_args
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
xnvme_cmd_prep_nvm_read_sized(union xnvme_spec_sqe *sqe,
			      const struct xnvme_cmd_prep_nvm_read_args *args, size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Read from 'args', see xnvme_cmd_prep_nvm_read_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_nvm_read_args
 *
 * @return The return value of xnvme_cmd_prep_nvm_read_sized()
 */
static inline int
xnvme_cmd_prep_nvm_read(union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_nvm_read_args *args)
{
	return xnvme_cmd_prep_nvm_read_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_nvm_write_uncorrectable(): the fields of Write Uncorrectable
 *
 * See NVM 1.3, 3.3, Figure 22, page 27, PDF page 27.
 *
 * The fields of each dword are from:
 *
 * - CDW10-11: NVM 1.3, 3.3.7, Figure 77, page 56, PDF page 56
 * - CDW12: NVM 1.3, 3.3.7, Figure 78, page 56, PDF page 56
 * - CDW13: NVM 1.3, 3.3.7, Figure 79, page 57, PDF page 57
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_nvm_write_uncorrectable_args {
	uint32_t nsid;     ///< Namespace Identifier, CDW1
	uint8_t rsvd4[4];  ///< Reserved
	uint64_t slba;     ///< Starting LBA, CDW10-11 bits 63:00
	uint8_t dtype;     ///< Directive Type, CDW12 bits 23:20
	uint8_t rsvd17[1]; ///< Reserved
	uint16_t nlb;      ///< Number of Logical Blocks, CDW12 bits 15:00
	uint16_t dspec;    ///< Directive Specific, CDW13 bits 31:16
	uint8_t rsvd22[2]; ///< Reserved
};

/**
 * Prepare 'sqe' as the command Write Uncorrectable, opcode 04h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_nvm_write_uncorrectable(), which
 * passes the size of 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_nvm_write_uncorrectable_args
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
xnvme_cmd_prep_nvm_write_uncorrectable_sized(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_nvm_write_uncorrectable_args *args,
	size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Write Uncorrectable from 'args', see
 * xnvme_cmd_prep_nvm_write_uncorrectable_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_nvm_write_uncorrectable_args
 *
 * @return The return value of xnvme_cmd_prep_nvm_write_uncorrectable_sized()
 */
static inline int
xnvme_cmd_prep_nvm_write_uncorrectable(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_nvm_write_uncorrectable_args *args)
{
	return xnvme_cmd_prep_nvm_write_uncorrectable_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_nvm_compare(): the fields of Compare
 *
 * See NVM 1.3, 3.3, Figure 22, page 27, PDF page 27.
 *
 * The fields of each dword are from:
 *
 * - CDW2-3: NVM 1.3, 3.3.1, Figure 25, page 28, PDF page 28
 * - CDW10-11: NVM 1.3, 3.3.1, Figure 26, page 28, PDF page 28
 * - CDW12: NVM 1.3, 3.3.1, Figure 27, page 28, PDF page 28
 * - CDW13, if CETYPE is non-zero: NVM 1.3, 3.3.1, Figure 28, page 29, PDF page 29
 * - CDW14: NVM 1.3, 3.3.1, Figure 29, page 29, PDF page 29
 * - CDW15: NVM 1.3, 3.3.1, Figure 30, page 29, PDF page 29
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_nvm_compare_args {
	uint32_t nsid;     ///< Namespace Identifier, CDW1
	uint8_t rsvd4[4];  ///< Reserved
	uint64_t slba;     ///< Starting LBA, CDW10-11 bits 63:00
	uint8_t lr;        ///< Limited Retry, CDW12 bit 31
	uint8_t fua;       ///< Force Unit Access, CDW12 bit 30
	uint8_t prinfo;    ///< Protection Information, CDW12 bits 29:26
	uint8_t stc;       ///< Storage Tag Check, CDW12 bit 24
	uint8_t cetype;    ///< Command Extension Type, CDW12 bits 19:16
	uint8_t rsvd21[1]; ///< Reserved
	uint16_t nlb;      ///< Number of Logical Blocks, CDW12 bits 15:00
	uint16_t cev;      ///< Command Extension Value, CDW13 bits 15:00
	uint16_t elbatm;   ///< Expected Logical Block Application Tag Mask, CDW15 bits 31:16
	uint16_t elbat;    ///< Expected Logical Block Application Tag, CDW15 bits 15:00
	uint8_t rsvd30[2]; ///< Reserved
	uint32_t elbtl;    ///< Expected Logical Block Tags Lower, CDW14 bits 31:00
	uint8_t rsvd36[4]; ///< Reserved
	uint64_t elbtu;    ///< Expected Logical Block Tags Upper, CDW2-3 bits 47:00
};

/**
 * Prepare 'sqe' as the command Compare, opcode 05h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_nvm_compare(), which passes the size
 * of 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_nvm_compare_args
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
xnvme_cmd_prep_nvm_compare_sized(union xnvme_spec_sqe *sqe,
				 const struct xnvme_cmd_prep_nvm_compare_args *args,
				 size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Compare from 'args', see xnvme_cmd_prep_nvm_compare_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_nvm_compare_args
 *
 * @return The return value of xnvme_cmd_prep_nvm_compare_sized()
 */
static inline int
xnvme_cmd_prep_nvm_compare(union xnvme_spec_sqe *sqe,
			   const struct xnvme_cmd_prep_nvm_compare_args *args)
{
	return xnvme_cmd_prep_nvm_compare_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_nvm_write_zeroes(): the fields of Write Zeroes
 *
 * See NVM 1.3, 3.3, Figure 22, page 27, PDF page 27.
 *
 * The fields of each dword are from:
 *
 * - CDW2-3: NVM 1.3, 3.3.8, Figure 81, page 59, PDF page 59
 * - CDW10-11: NVM 1.3, 3.3.8, Figure 82, page 59, PDF page 59
 * - CDW12: NVM 1.3, 3.3.8, Figure 83, page 59, PDF page 59
 * - CDW13, if CETYPE is cleared to 0h: NVM 1.3, 3.3.8, Figure 84, page 60, PDF page 60
 * - CDW13, if CETYPE is non-zero: NVM 1.3, 3.3.8, Figure 85, page 60, PDF page 60
 * - CDW14: NVM 1.3, 3.3.8, Figure 86, page 60, PDF page 60
 * - CDW15: NVM 1.3, 3.3.8, Figure 87, page 60, PDF page 60
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_nvm_write_zeroes_args {
	uint32_t nsid;     ///< Namespace Identifier, CDW1
	uint8_t rsvd4[4];  ///< Reserved
	uint64_t slba;     ///< Starting LBA, CDW10-11 bits 63:00
	uint8_t lr;        ///< Limited Retry, CDW12 bit 31
	uint8_t fua;       ///< Force Unit Access, CDW12 bit 30
	uint8_t prinfo;    ///< Protection Information, CDW12 bits 29:26
	uint8_t deac;      ///< Deallocate, CDW12 bit 25
	uint8_t stc;       ///< Storage Tag Check, CDW12 bit 24
	uint8_t nsz;       ///< Namespace Zeroes, CDW12 bit 23
	uint8_t dtype;     ///< Directive Type, CDW12 bits 22:20
	uint8_t cetype;    ///< Command Extension Type, CDW12 bits 19:16
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
 * Prepare 'sqe' as the command Write Zeroes, opcode 08h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_nvm_write_zeroes(), which passes the
 * size of 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_nvm_write_zeroes_args
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
xnvme_cmd_prep_nvm_write_zeroes_sized(union xnvme_spec_sqe *sqe,
				      const struct xnvme_cmd_prep_nvm_write_zeroes_args *args,
				      size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Write Zeroes from 'args', see
 * xnvme_cmd_prep_nvm_write_zeroes_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_nvm_write_zeroes_args
 *
 * @return The return value of xnvme_cmd_prep_nvm_write_zeroes_sized()
 */
static inline int
xnvme_cmd_prep_nvm_write_zeroes(union xnvme_spec_sqe *sqe,
				const struct xnvme_cmd_prep_nvm_write_zeroes_args *args)
{
	return xnvme_cmd_prep_nvm_write_zeroes_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_nvm_dataset_management(): the fields of Dataset Management
 *
 * See NVM 1.3, 3.3, Figure 22, page 27, PDF page 27.
 *
 * The fields of each dword are from:
 *
 * - CDW10: NVM 1.3, 3.3.3, Figure 45, page 44, PDF page 44
 * - CDW11: NVM 1.3, 3.3.3, Figure 46, page 44, PDF page 44
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_nvm_dataset_management_args {
	uint32_t nsid; ///< Namespace Identifier, CDW1
	uint8_t nr;    ///< Number of Ranges, CDW10 bits 07:00
	uint8_t ad;    ///< Attribute - Deallocate, CDW11 bit 2
	uint8_t idw;   ///< Attribute - Integral Dataset for Write, CDW11 bit 1
	uint8_t idr;   ///< Attribute - Integral Dataset for Read, CDW11 bit 0
};

/**
 * Prepare 'sqe' as the command Dataset Management, opcode 09h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_nvm_dataset_management(), which
 * passes the size of 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_nvm_dataset_management_args
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
xnvme_cmd_prep_nvm_dataset_management_sized(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_nvm_dataset_management_args *args,
	size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Dataset Management from 'args', see
 * xnvme_cmd_prep_nvm_dataset_management_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_nvm_dataset_management_args
 *
 * @return The return value of xnvme_cmd_prep_nvm_dataset_management_sized()
 */
static inline int
xnvme_cmd_prep_nvm_dataset_management(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_nvm_dataset_management_args *args)
{
	return xnvme_cmd_prep_nvm_dataset_management_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_nvm_verify(): the fields of Verify
 *
 * See NVM 1.3, 3.3, Figure 22, page 27, PDF page 27.
 *
 * The fields of each dword are from:
 *
 * - CDW2-3: NVM 1.3, 3.3.5, Figure 60, page 52, PDF page 52
 * - CDW10-11: NVM 1.3, 3.3.5, Figure 61, page 52, PDF page 52
 * - CDW12: NVM 1.3, 3.3.5, Figure 62, page 52, PDF page 52
 * - CDW13, if CETYPE is non-zero: NVM 1.3, 3.3.5, Figure 63, page 52, PDF page 52
 * - CDW14: NVM 1.3, 3.3.5, Figure 64, page 53, PDF page 53
 * - CDW15: NVM 1.3, 3.3.5, Figure 65, page 53, PDF page 53
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_nvm_verify_args {
	uint32_t nsid;     ///< Namespace Identifier, CDW1
	uint8_t rsvd4[4];  ///< Reserved
	uint64_t slba;     ///< Starting LBA, CDW10-11 bits 63:00
	uint8_t lr;        ///< Limited Retry, CDW12 bit 31
	uint8_t fua;       ///< Force Unit Access, CDW12 bit 30
	uint8_t prinfo;    ///< Protection Information, CDW12 bits 29:26
	uint8_t stc;       ///< Storage Tag Check, CDW12 bit 24
	uint8_t cetype;    ///< Command Extension Type, CDW12 bits 19:16
	uint8_t rsvd21[1]; ///< Reserved
	uint16_t nlb;      ///< Number of Logical Blocks, CDW12 bits 15:00
	uint16_t cev;      ///< Command Extension Value, CDW13 bits 15:00
	uint16_t elbatm;   ///< Expected Logical Block Application Tag Mask, CDW15 bits 31:16
	uint16_t elbat;    ///< Expected Logical Block Application Tag, CDW15 bits 15:00
	uint8_t rsvd30[2]; ///< Reserved
	uint32_t elbtl;    ///< Expected Logical Block Tags Lower, CDW14 bits 31:00
	uint8_t rsvd36[4]; ///< Reserved
	uint64_t elbtu;    ///< Expected Logical Block Tags Upper, CDW2-3 bits 47:00
};

/**
 * Prepare 'sqe' as the command Verify, opcode 0Ch, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_nvm_verify(), which passes the size
 * of 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_nvm_verify_args
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
xnvme_cmd_prep_nvm_verify_sized(union xnvme_spec_sqe *sqe,
				const struct xnvme_cmd_prep_nvm_verify_args *args,
				size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Verify from 'args', see xnvme_cmd_prep_nvm_verify_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_nvm_verify_args
 *
 * @return The return value of xnvme_cmd_prep_nvm_verify_sized()
 */
static inline int
xnvme_cmd_prep_nvm_verify(union xnvme_spec_sqe *sqe,
			  const struct xnvme_cmd_prep_nvm_verify_args *args)
{
	return xnvme_cmd_prep_nvm_verify_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_nvm_copy(): the fields of Copy
 *
 * See NVM 1.3, 3.3, Figure 22, page 27, PDF page 27.
 *
 * The fields of each dword are from:
 *
 * - CDW2-3: NVM 1.3, 3.3.2, Figure 33, page 30, PDF page 30
 * - CDW10-11: NVM 1.3, 3.3.2, Figure 34, page 30, PDF page 30
 * - CDW12: NVM 1.3, 3.3.2, Figure 35, page 30, PDF page 30
 * - CDW13: NVM 1.3, 3.3.2, Figure 36, page 31, PDF page 31
 * - CDW14: NVM 1.3, 3.3.2, Figure 37, page 31, PDF page 31
 * - CDW15: NVM 1.3, 3.3.2, Figure 38, page 32, PDF page 32
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_nvm_copy_args {
	uint32_t nsid;     ///< Namespace Identifier, CDW1
	uint8_t rsvd4[4];  ///< Reserved
	uint64_t sdlba;    ///< Starting Destination LBA, CDW10-11 bits 63:00
	uint8_t lr;        ///< Limited Retry, CDW12 bit 31
	uint8_t fua;       ///< Force Unit Access, CDW12 bit 30
	uint8_t prinfow;   ///< Protection Information Write, CDW12 bits 29:26
	uint8_t stcr;      ///< Storage Tag Check Read, CDW12 bit 25
	uint8_t stcw;      ///< Storage Tag Check Write, CDW12 bit 24
	uint8_t dtype;     ///< Directive Type, CDW12 bits 23:20
	uint8_t cetype;    ///< Command Extension Type, CDW12 bits 19:16
	uint8_t prinfor;   ///< Protection Information Read, CDW12 bits 15:12
	uint8_t desfmt;    ///< Descriptor Format, CDW12 bits 11:08
	uint8_t nr;        ///< Number of Ranges, CDW12 bits 07:00
	uint16_t dspec;    ///< Directive Specific, CDW13 bits 31:16
	uint16_t cev;      ///< Command Extension Value, CDW13 bits 15:00
	uint16_t lbatm;    ///< Logical Block Application Tag Mask, CDW15 bits 31:16
	uint16_t lbat;     ///< Logical Block Application Tag, CDW15 bits 15:00
	uint8_t rsvd34[2]; ///< Reserved
	uint32_t lbtl;     ///< Logical Block Tags Lower, CDW14 bits 31:00
	uint64_t lbtu;     ///< Logical Block Tags Upper, CDW2-3 bits 47:00
};

/**
 * Prepare 'sqe' as the command Copy, opcode 19h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_nvm_copy(), which passes the size of
 * 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_nvm_copy_args
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
xnvme_cmd_prep_nvm_copy_sized(union xnvme_spec_sqe *sqe,
			      const struct xnvme_cmd_prep_nvm_copy_args *args, size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Copy from 'args', see xnvme_cmd_prep_nvm_copy_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_nvm_copy_args
 *
 * @return The return value of xnvme_cmd_prep_nvm_copy_sized()
 */
static inline int
xnvme_cmd_prep_nvm_copy(union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_nvm_copy_args *args)
{
	return xnvme_cmd_prep_nvm_copy_sized(sqe, args, sizeof(*args));
}

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_CMD_PREP_NVM_H */
