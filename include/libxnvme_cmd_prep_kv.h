/**
 * Prepare the commands of the Key Value Command Set from records of their fields
 *
 * Written from the data model of senfd 0.26.0, by toolbox/xnvme_cmd_prep.py;
 * each field is named and placed as the figure it cites gives it. Do not edit:
 * see the contributor docs on producing the command preps.
 *
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_cmd_prep_kv.h
 */

#ifndef __LIBXNVME_CMD_PREP_KV_H
#define __LIBXNVME_CMD_PREP_KV_H

#ifdef __cplusplus
extern "C" {
#endif

/** Defined where the preps of the Key Value Command Set are declared; a program tests it with
 * `#ifdef` */
#define XNVME_CMD_PREP_KV 1

/**
 * The arguments of xnvme_cmd_prep_kv_store(): the fields of Store
 *
 * See Key Value 1.4, 3.2, Figure 6, page 14, PDF page 14.
 *
 * The fields of each dword are from:
 *
 * - CDW2-3: Key Value 1.4, 3.2.5, Figure 33, page 20, PDF page 20
 * - CDW10: Key Value 1.4, 3.2.5, Figure 30, page 19, PDF page 19
 * - CDW11: Key Value 1.4, 3.2.5, Figure 31, page 19, PDF page 19
 * - CDW13: Key Value 1.4, 3.2.5, Figure 32, page 19, PDF page 19
 * - CDW14-15: Key Value 1.4, 3.2.5, Figure 34, page 20, PDF page 20
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_kv_store_args {
	uint32_t nsid;      ///< Namespace Identifier, CDW1
	uint32_t vs;        ///< Value Size, CDW10 bits 31:00
	uint8_t so;         ///< Store Option, CDW11 bits 15:08
	uint8_t kl;         ///< Key Length, CDW11 bits 07:00
	uint8_t cetype;     ///< Command Extension Type, CDW13 bits 19:16
	uint8_t rsvd11[1];  ///< Reserved
	uint16_t cev;       ///< Command Extension Value, CDW13 bits 15:00
	uint8_t kv_key[16]; ///< KV Key, the first byte in the lowest bits
	uint8_t rsvd30[2];  ///< Reserved
};

/**
 * Prepare 'sqe' as the command Store, opcode 01h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_kv_store(), which passes the size of
 * 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_kv_store_args
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
xnvme_cmd_prep_kv_store_sized(union xnvme_cmd_sqe *sqe,
			      const struct xnvme_cmd_prep_kv_store_args *args, size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Store from 'args', see xnvme_cmd_prep_kv_store_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_kv_store_args
 *
 * @return The return value of xnvme_cmd_prep_kv_store_sized()
 */
static inline int
xnvme_cmd_prep_kv_store(union xnvme_cmd_sqe *sqe, const struct xnvme_cmd_prep_kv_store_args *args)
{
	return xnvme_cmd_prep_kv_store_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_kv_retrieve(): the fields of Retrieve
 *
 * See Key Value 1.4, 3.2, Figure 6, page 14, PDF page 14.
 *
 * The fields of each dword are from:
 *
 * - CDW2-3: Key Value 1.4, 3.2.3, Figure 22, page 17, PDF page 17
 * - CDW10: Key Value 1.4, 3.2.3, Figure 19, page 17, PDF page 17
 * - CDW11: Key Value 1.4, 3.2.3, Figure 20, page 17, PDF page 17
 * - CDW13: Key Value 1.4, 3.2.3, Figure 21, page 17, PDF page 17
 * - CDW14-15: Key Value 1.4, 3.2.3, Figure 23, page 18, PDF page 18
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_kv_retrieve_args {
	uint32_t nsid;      ///< Namespace Identifier, CDW1
	uint32_t hbs;       ///< Host Buffer Size, CDW10 bits 31:00
	uint8_t ro;         ///< Retrieve Option, CDW11 bits 15:08
	uint8_t kl;         ///< Key Length, CDW11 bits 07:00
	uint8_t cetype;     ///< Command Extension Type, CDW13 bits 19:16
	uint8_t rsvd11[1];  ///< Reserved
	uint16_t cev;       ///< Command Extension Value, CDW13 bits 15:00
	uint8_t kv_key[16]; ///< KV Key, the first byte in the lowest bits
	uint8_t rsvd30[2];  ///< Reserved
};

/**
 * Prepare 'sqe' as the command Retrieve, opcode 02h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_kv_retrieve(), which passes the size
 * of 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_kv_retrieve_args
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
xnvme_cmd_prep_kv_retrieve_sized(union xnvme_cmd_sqe *sqe,
				 const struct xnvme_cmd_prep_kv_retrieve_args *args,
				 size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Retrieve from 'args', see xnvme_cmd_prep_kv_retrieve_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_kv_retrieve_args
 *
 * @return The return value of xnvme_cmd_prep_kv_retrieve_sized()
 */
static inline int
xnvme_cmd_prep_kv_retrieve(union xnvme_cmd_sqe *sqe,
			   const struct xnvme_cmd_prep_kv_retrieve_args *args)
{
	return xnvme_cmd_prep_kv_retrieve_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_kv_list(): the fields of List
 *
 * See Key Value 1.4, 3.2, Figure 6, page 14, PDF page 14.
 *
 * The fields of each dword are from:
 *
 * - CDW2-3: Key Value 1.4, 3.2.2, Figure 13, page 16, PDF page 16
 * - CDW10: Key Value 1.4, 3.2.2, Figure 11, page 15, PDF page 15
 * - CDW11: Key Value 1.4, 3.2.2, Figure 12, page 16, PDF page 16
 * - CDW14-15: Key Value 1.4, 3.2.2, Figure 14, page 16, PDF page 16
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_kv_list_args {
	uint32_t nsid;      ///< Namespace Identifier, CDW1
	uint32_t hbs;       ///< Host Buffer Size, CDW10 bits 31:00
	uint8_t kl;         ///< Key Length, CDW11 bits 07:00
	uint8_t kv_key[16]; ///< KV Key, the first byte in the lowest bits
	uint8_t rsvd25[7];  ///< Reserved
};

/**
 * Prepare 'sqe' as the command List, opcode 06h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_kv_list(), which passes the size of
 * 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_kv_list_args
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
xnvme_cmd_prep_kv_list_sized(union xnvme_cmd_sqe *sqe,
			     const struct xnvme_cmd_prep_kv_list_args *args, size_t args_nbytes);

/**
 * Prepare 'sqe' as the command List from 'args', see xnvme_cmd_prep_kv_list_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_kv_list_args
 *
 * @return The return value of xnvme_cmd_prep_kv_list_sized()
 */
static inline int
xnvme_cmd_prep_kv_list(union xnvme_cmd_sqe *sqe, const struct xnvme_cmd_prep_kv_list_args *args)
{
	return xnvme_cmd_prep_kv_list_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_kv_delete(): the fields of Delete
 *
 * See Key Value 1.4, 3.2, Figure 6, page 14, PDF page 14.
 *
 * The fields of each dword are from:
 *
 * - CDW2-3: Key Value 1.4, 3.2.1, Figure 8, page 15, PDF page 15
 * - CDW11: Key Value 1.4, 3.2.1, Figure 7, page 15, PDF page 15
 * - CDW14-15: Key Value 1.4, 3.2.1, Figure 9, page 15, PDF page 15
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_kv_delete_args {
	uint32_t nsid;      ///< Namespace Identifier, CDW1
	uint8_t kl;         ///< Key Length, CDW11 bits 07:00
	uint8_t kv_key[16]; ///< KV Key, the first byte in the lowest bits
	uint8_t rsvd21[3];  ///< Reserved
};

/**
 * Prepare 'sqe' as the command Delete, opcode 10h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_kv_delete(), which passes the size
 * of 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_kv_delete_args
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
xnvme_cmd_prep_kv_delete_sized(union xnvme_cmd_sqe *sqe,
			       const struct xnvme_cmd_prep_kv_delete_args *args,
			       size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Delete from 'args', see xnvme_cmd_prep_kv_delete_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_kv_delete_args
 *
 * @return The return value of xnvme_cmd_prep_kv_delete_sized()
 */
static inline int
xnvme_cmd_prep_kv_delete(union xnvme_cmd_sqe *sqe,
			 const struct xnvme_cmd_prep_kv_delete_args *args)
{
	return xnvme_cmd_prep_kv_delete_sized(sqe, args, sizeof(*args));
}

/**
 * The arguments of xnvme_cmd_prep_kv_exist(): the fields of Exist
 *
 * See Key Value 1.4, 3.2, Figure 6, page 14, PDF page 14.
 *
 * The fields of each dword are from:
 *
 * - CDW2-3: Key Value 1.4, 3.2.4, Figure 26, page 18, PDF page 18
 * - CDW11: Key Value 1.4, 3.2.4, Figure 25, page 18, PDF page 18
 * - CDW14-15: Key Value 1.4, 3.2.4, Figure 27, page 18, PDF page 18
 *
 * A later version of the library adds members only at the end. The function takes a member that
 * the record of a program lacks as zero, so zero is the default of every member. The function
 * returns -EINVAL for a reserved member that is not zero.
 */
struct xnvme_cmd_prep_kv_exist_args {
	uint32_t nsid;      ///< Namespace Identifier, CDW1
	uint8_t kl;         ///< Key Length, CDW11 bits 07:00
	uint8_t kv_key[16]; ///< KV Key, the first byte in the lowest bits
	uint8_t rsvd21[3];  ///< Reserved
};

/**
 * Prepare 'sqe' as the command Exist, opcode 14h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() or
 * xnvme_cmd_pass_iov() to set. A program calls xnvme_cmd_prep_kv_exist(), which passes the size of
 * 'args'.
 *
 * @param sqe The submission queue entry to prepare
 * @recparam args The fields of the command, see ::xnvme_cmd_prep_kv_exist_args
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
xnvme_cmd_prep_kv_exist_sized(union xnvme_cmd_sqe *sqe,
			      const struct xnvme_cmd_prep_kv_exist_args *args, size_t args_nbytes);

/**
 * Prepare 'sqe' as the command Exist from 'args', see xnvme_cmd_prep_kv_exist_sized()
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_kv_exist_args
 *
 * @return The return value of xnvme_cmd_prep_kv_exist_sized()
 */
static inline int
xnvme_cmd_prep_kv_exist(union xnvme_cmd_sqe *sqe, const struct xnvme_cmd_prep_kv_exist_args *args)
{
	return xnvme_cmd_prep_kv_exist_sized(sqe, args, sizeof(*args));
}

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_CMD_PREP_KV_H */
