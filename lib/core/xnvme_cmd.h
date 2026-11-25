// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_CMD_H
#define __INTERNAL_XNVME_CMD_H

/**
 * Enumeration of `xnvme_cmd` options
 *
 * @enum xnvme_cmd_opts
 */
enum xnvme_cmd_opts {
	XNVME_CMD_SYNC  = 0x1 << 0, ///< XNVME_CMD_SYNC: Synchronous command
	XNVME_CMD_ASYNC = 0x1 << 1, ///< XNVME_CMD_ASYNC: Asynchronous command

	XNVME_CMD_UPLD_SGLD = 0x1 << 2, ///< XNVME_CMD_UPLD_SGLD: User-managed SGL data
	XNVME_CMD_UPLD_SGLM = 0x1 << 3, ///< XNVME_CMD_UPLD_SGLM: User-managed SGL meta
};

#define XNVME_CMD_MASK_IOMD (XNVME_CMD_SYNC | XNVME_CMD_ASYNC)
#define XNVME_CMD_MASK_UPLD (XNVME_CMD_UPLD_SGLD | XNVME_CMD_UPLD_SGLM)
#define XNVME_CMD_MASK      (XNVME_CMD_MASK_IOMD | XNVME_CMD_MASK_UPLD)

#define XNVME_CMD_DEF_IOMD XNVME_CMD_SYNC
#define XNVME_CMD_DEF_UPLD (0x0)

/**
 * The opcode of 'sqe', XNVME_SPEC_BASE_COMMON_COMMAND_FORMAT_CDW0_OPC
 */
static inline uint8_t
xnvme_cmd_sqe_get_opc(const union xnvme_cmd_sqe *sqe)
{
	return (uint8_t)xnvme_bits_get(sqe->common.cdw0,
				       XNVME_SPEC_BASE_COMMON_COMMAND_FORMAT_CDW0_OPC);
}

/**
 * Write 'opc' into the opcode of 'sqe', as xnvme_cmd_sqe_get_opc() reads it
 */
static inline void
xnvme_cmd_sqe_set_opc(union xnvme_cmd_sqe *sqe, uint8_t opc)
{
	sqe->common.cdw0 = (uint32_t)xnvme_bits_set(
		sqe->common.cdw0, XNVME_SPEC_BASE_COMMON_COMMAND_FORMAT_CDW0_OPC, opc);
}

/**
 * The 64 bits of dwords 10 and 11 of 'sqe', where the I/O commands have their SLBA, as in
 * XNVME_SPEC_NVM_READ_CDW10_11_SLBA
 */
static inline uint64_t
xnvme_cmd_sqe_get_cdw10_11(const union xnvme_cmd_sqe *sqe)
{
	return (uint64_t)sqe->common.cdw11 << 32 | sqe->common.cdw10;
}

/**
 * Write 'val' into dwords 10 and 11 of 'sqe', as xnvme_cmd_sqe_get_cdw10_11() reads them
 */
static inline void
xnvme_cmd_sqe_set_cdw10_11(union xnvme_cmd_sqe *sqe, uint64_t val)
{
	sqe->common.cdw10 = (uint32_t)val;
	sqe->common.cdw11 = (uint32_t)(val >> 32);
}

/**
 * The 64 bits of dwords 0 and 1 of 'cpl', where a backend puts a result such as the number of
 * bytes transferred, and the Linux NVMe driver the 64-bit result of a command
 */
static inline uint64_t
xnvme_cmd_cpl_get_dw0_1(const struct xnvme_spec_base_completion_queue_entry *cpl)
{
	return (uint64_t)cpl->dw1 << 32 | cpl->dw0;
}

/**
 * Write 'val' into dwords 0 and 1 of 'cpl', as xnvme_cmd_cpl_get_dw0_1() reads them
 */
static inline void
xnvme_cmd_cpl_set_dw0_1(struct xnvme_spec_base_completion_queue_entry *cpl, uint64_t val)
{
	cpl->dw0 = (uint32_t)val;
	cpl->dw1 = (uint32_t)(val >> 32);
}

/**
 * Pass a PSEUDO-NVMe Command through to the device
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param dbuf pointer to data-payload
 * @param dbuf_nbytes size of data-payload in bytes
 * @param mbuf pointer to meta-payload
 * @param mbuf_nbytes size of the meta-payload in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_cmd_pass_pseudo(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
		      size_t mbuf_nbytes);

#endif /* __INTERNAL_XNVME_CMD_H */
