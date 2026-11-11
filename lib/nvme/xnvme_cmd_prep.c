// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/*
 * Written from the data model of senfd 0.14.0, by toolbox/xnvme_cmd_prep.py. Do not edit.
 */
#include <errno.h>
#include <string.h>
#include <libxnvme.h>

/**
 * Copy the caller's record into 'a', zero beyond the caller's size
 *
 * The size of the caller's record is its first member, 'args_nbytes'.
 *
 * @param a The record of the function, zero on entry
 * @param a_nbytes The size of 'a'
 * @param args The caller's record
 * @param rsvd The reserved bytes of 'a', as 'nrsvd' pairs of a byte offset and a
 * number of bytes
 * @param nrsvd The number of pairs in 'rsvd'
 *
 * @return 0 on success; -EINVAL when 'args' is NULL, its size is less than the
 * size of 'args_nbytes', or a reserved byte is not zero; -E2BIG when the caller's
 * record is larger than 'a' and the bytes beyond 'a_nbytes' are not zero.
 */
static int
prep_args(void *a, size_t a_nbytes, const void *args, const uint16_t *rsvd, unsigned int nrsvd)
{
	uint32_t nbytes;

	if (!args) {
		return -EINVAL;
	}
	memcpy(&nbytes, args, sizeof(nbytes));
	if (nbytes < sizeof(nbytes)) {
		return -EINVAL;
	}
	for (size_t i = a_nbytes; i < nbytes; ++i) {
		if (((const uint8_t *)args)[i]) {
			return -E2BIG;
		}
	}
	memcpy(a, args, nbytes < a_nbytes ? nbytes : a_nbytes);

	for (unsigned int i = 0; i < nrsvd; ++i) {
		for (unsigned int b = 0; b < rsvd[2 * i + 1]; ++b) {
			if (((const uint8_t *)a)[rsvd[2 * i] + b]) {
				return -EINVAL;
			}
		}
	}

	return 0;
}

/**
 * Write the low 'width' bits of 'value' into 'sqe'
 *
 * A field wider than what is left of dword 'cdw' continues in the dwords after
 * it, as SLBA does in CDW10 and CDW11.
 *
 * @param sqe The submission queue entry
 * @param cdw The dword of the lowest bit of the field
 * @param bit The lowest bit of the field in dword 'cdw'
 * @param width The number of bits of the field
 * @param value The value of the field
 */
static void
prep_bits(union xnvme_spec_sqe *sqe, unsigned int cdw, unsigned int bit, unsigned int width,
	  uint64_t value)
{
	bit += 32 * cdw;

	for (unsigned int i = 0; i < width;) {
		unsigned int dw = (bit + i) / 32, ofz = (bit + i) % 32;
		unsigned int n = 32 - ofz < width - i ? 32 - ofz : width - i;
		uint32_t mask = n == 32 ? 0xFFFFFFFFu : (1u << n) - 1u;

		sqe->cdw[dw] =
			(sqe->cdw[dw] & ~(mask << ofz)) | (((uint32_t)(value >> i) & mask) << ofz);
		i += n;
	}
}

/**
 * The 'nbytes' bytes from 'bytes' as an integer, the first in its lowest bits
 *
 * Gives a value of bytes, as the key of a Key Value command, the byte order of
 * the submission queue entry on any host.
 */
static uint64_t
prep_le64(const uint8_t *bytes, unsigned int nbytes)
{
	uint64_t value = 0;

	for (unsigned int i = 0; i < nbytes; ++i) {
		value |= (uint64_t)bytes[i] << (8 * i);
	}

	return value;
}

/*
 * Admin Command Set
 */

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_delete_io_submission_queue_args) == 8,
		    "The record has padding")

int
xnvme_cmd_prep_adm_delete_io_submission_queue(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_delete_io_submission_queue_args *args)
{
	static const uint16_t rsvd[] = {6, 2};
	struct xnvme_cmd_prep_adm_delete_io_submission_queue_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x00;

	prep_bits(sqe, 10, 0, 16, a.qid); ///< Queue Identifier

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_create_io_submission_queue_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_create_io_submission_queue(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_create_io_submission_queue_args *args)
{
	static const uint16_t rsvd[] = {14, 2};
	struct xnvme_cmd_prep_adm_create_io_submission_queue_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.qprio >> 2) { ///< Queue Priority: 2 bits
		return -EINVAL;
	}
	if (a.pc >> 1) { ///< Physically Contiguous: 1 bit
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x01;

	prep_bits(sqe, 10, 16, 16, a.qsize);   ///< Queue Size
	prep_bits(sqe, 10, 0, 16, a.qid);      ///< Queue Identifier
	prep_bits(sqe, 11, 16, 16, a.cqid);    ///< Completion Queue Identifier
	prep_bits(sqe, 11, 1, 2, a.qprio);     ///< Queue Priority
	prep_bits(sqe, 11, 0, 1, a.pc);        ///< Physically Contiguous
	prep_bits(sqe, 12, 0, 16, a.nvmsetid); ///< NVM Set Identifier

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_get_log_page_args) == 32,
		    "The record has padding")

int
xnvme_cmd_prep_adm_get_log_page(union xnvme_spec_sqe *sqe,
				const struct xnvme_cmd_prep_adm_get_log_page_args *args)
{
	static const uint16_t rsvd[] = {11, 1, 17, 3};
	struct xnvme_cmd_prep_adm_get_log_page_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 2);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.rae >> 1) { ///< Retain Asynchronous Event: 1 bit
		return -EINVAL;
	}
	if (a.lsp >> 7) { ///< Log Specific Parameter: 7 bits
		return -EINVAL;
	}
	if (a.ot >> 1) { ///< Offset Type: 1 bit
		return -EINVAL;
	}
	if (a.uidx >> 7) { ///< UUID Index: 7 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x02;

	prep_bits(sqe, 1, 0, 32, a.nsid);        ///< Namespace Identifier
	prep_bits(sqe, 10, 16, 16, a.numd);      ///< Number of Dwords Lower
	prep_bits(sqe, 10, 15, 1, a.rae);        ///< Retain Asynchronous Event
	prep_bits(sqe, 10, 8, 7, a.lsp);         ///< Log Specific Parameter
	prep_bits(sqe, 10, 0, 8, a.lid);         ///< Log Page Identifier
	prep_bits(sqe, 11, 16, 16, a.lsi);       ///< Log Specific Identifier
	prep_bits(sqe, 11, 0, 16, a.numd >> 16); ///< Number of Dwords Upper
	prep_bits(sqe, 12, 0, 32, a.lpo);        ///< Log Page Offset Lower
	prep_bits(sqe, 13, 0, 32, a.lpo >> 32);  ///< Log Page Offset Upper
	prep_bits(sqe, 14, 24, 8, a.csi);        ///< Command Set Identifier
	prep_bits(sqe, 14, 23, 1, a.ot);         ///< Offset Type
	prep_bits(sqe, 14, 0, 7, a.uidx);        ///< UUID Index

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_delete_io_completion_queue_args) == 8,
		    "The record has padding")

int
xnvme_cmd_prep_adm_delete_io_completion_queue(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_delete_io_completion_queue_args *args)
{
	static const uint16_t rsvd[] = {6, 2};
	struct xnvme_cmd_prep_adm_delete_io_completion_queue_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x04;

	prep_bits(sqe, 10, 0, 16, a.qid); ///< Queue Identifier

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_create_io_completion_queue_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_create_io_completion_queue(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_create_io_completion_queue_args *args)
{
	static const uint16_t rsvd[] = {12, 4};
	struct xnvme_cmd_prep_adm_create_io_completion_queue_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.ien >> 1) { ///< Interrupts Enabled: 1 bit
		return -EINVAL;
	}
	if (a.pc >> 1) { ///< Physically Contiguous: 1 bit
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x05;

	prep_bits(sqe, 10, 16, 16, a.qsize); ///< Queue Size
	prep_bits(sqe, 10, 0, 16, a.qid);    ///< Queue Identifier
	prep_bits(sqe, 11, 16, 16, a.iv);    ///< Interrupt Vector
	prep_bits(sqe, 11, 1, 1, a.ien);     ///< Interrupts Enabled
	prep_bits(sqe, 11, 0, 1, a.pc);      ///< Physically Contiguous

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_identify_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_identify(union xnvme_spec_sqe *sqe,
			    const struct xnvme_cmd_prep_adm_identify_args *args)
{
	static const uint16_t rsvd[] = {15, 1};
	struct xnvme_cmd_prep_adm_identify_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.uidx >> 7) { ///< UUID Index: 7 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x06;

	prep_bits(sqe, 1, 0, 32, a.nsid);    ///< Namespace Identifier
	prep_bits(sqe, 10, 16, 16, a.cntid); ///< Controller Identifier
	prep_bits(sqe, 10, 0, 8, a.cns);     ///< Controller or Namespace Structure
	prep_bits(sqe, 11, 24, 8, a.csi);    ///< Command Set Identifier
	prep_bits(sqe, 11, 0, 16, a.cnssid); ///< CNS Specific Identifier
	prep_bits(sqe, 14, 0, 7, a.uidx);    ///< UUID Index

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_abort_args) == 8, "The record has padding")

int
xnvme_cmd_prep_adm_abort(union xnvme_spec_sqe *sqe,
			 const struct xnvme_cmd_prep_adm_abort_args *args)
{
	struct xnvme_cmd_prep_adm_abort_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, NULL, 0);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x08;

	prep_bits(sqe, 10, 16, 16, a.cid); ///< Command Identifier
	prep_bits(sqe, 10, 0, 16, a.sqid); ///< Submission Queue Identifier

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_set_features_args) == 32,
		    "The record has padding")

int
xnvme_cmd_prep_adm_set_features(union xnvme_spec_sqe *sqe,
				const struct xnvme_cmd_prep_adm_set_features_args *args)
{
	static const uint16_t rsvd[] = {11, 1, 28, 4};
	struct xnvme_cmd_prep_adm_set_features_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 2);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.sv >> 1) { ///< Save: 1 bit
		return -EINVAL;
	}
	if (a.uidx >> 7) { ///< UUID Index: 7 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x09;

	prep_bits(sqe, 1, 0, 32, a.nsid);   ///< Namespace Identifier
	prep_bits(sqe, 10, 31, 1, a.sv);    ///< Save
	prep_bits(sqe, 10, 0, 8, a.fid);    ///< Feature Identifier
	prep_bits(sqe, 11, 0, 32, a.cdw11); ///< as the feature or operation describes it
	prep_bits(sqe, 12, 0, 32, a.cdw12); ///< as the feature or operation describes it
	prep_bits(sqe, 13, 0, 32, a.cdw13); ///< as the feature or operation describes it
	prep_bits(sqe, 14, 0, 7, a.uidx);   ///< UUID Index
	prep_bits(sqe, 15, 0, 32, a.cdw15); ///< as the feature or operation describes it

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_get_features_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_get_features(union xnvme_spec_sqe *sqe,
				const struct xnvme_cmd_prep_adm_get_features_args *args)
{
	static const uint16_t rsvd[] = {11, 1};
	struct xnvme_cmd_prep_adm_get_features_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.sel >> 3) { ///< Select: 3 bits
		return -EINVAL;
	}
	if (a.uidx >> 7) { ///< UUID Index: 7 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x0A;

	prep_bits(sqe, 1, 0, 32, a.nsid);   ///< Namespace Identifier
	prep_bits(sqe, 10, 8, 3, a.sel);    ///< Select
	prep_bits(sqe, 10, 0, 8, a.fid);    ///< Feature Identifier
	prep_bits(sqe, 11, 0, 32, a.cdw11); ///< as the feature or operation describes it
	prep_bits(sqe, 14, 0, 7, a.uidx);   ///< UUID Index

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_asynchronous_event_request_args) == 8,
		    "The record has padding")

int
xnvme_cmd_prep_adm_asynchronous_event_request(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_asynchronous_event_request_args *args)
{
	static const uint16_t rsvd[] = {4, 4};
	struct xnvme_cmd_prep_adm_asynchronous_event_request_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x0C;

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_namespace_management_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_namespace_management(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_adm_namespace_management_args *args)
{
	static const uint16_t rsvd[] = {10, 6};
	struct xnvme_cmd_prep_adm_namespace_management_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.sel >> 4) { ///< Select: 4 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x0D;

	prep_bits(sqe, 1, 0, 32, a.nsid); ///< Namespace Identifier
	prep_bits(sqe, 10, 0, 4, a.sel);  ///< Select
	prep_bits(sqe, 11, 24, 8, a.csi); ///< Command Set Identifier

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_firmware_commit_args) == 8,
		    "The record has padding")

int
xnvme_cmd_prep_adm_firmware_commit(union xnvme_spec_sqe *sqe,
				   const struct xnvme_cmd_prep_adm_firmware_commit_args *args)
{
	static const uint16_t rsvd[] = {7, 1};
	struct xnvme_cmd_prep_adm_firmware_commit_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.bpid >> 1) { ///< Boot Partition ID: 1 bit
		return -EINVAL;
	}
	if (a.ca >> 3) { ///< Commit Action: 3 bits
		return -EINVAL;
	}
	if (a.fs >> 3) { ///< Firmware Slot: 3 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x10;

	prep_bits(sqe, 10, 31, 1, a.bpid); ///< Boot Partition ID
	prep_bits(sqe, 10, 3, 3, a.ca);    ///< Commit Action
	prep_bits(sqe, 10, 0, 3, a.fs);    ///< Firmware Slot

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_firmware_image_download_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_firmware_image_download(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_firmware_image_download_args *args)
{
	static const uint16_t rsvd[] = {12, 4};
	struct xnvme_cmd_prep_adm_firmware_image_download_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x11;

	prep_bits(sqe, 10, 0, 32, a.numd); ///< Number of Dwords
	prep_bits(sqe, 11, 0, 32, a.ofst); ///< Offset

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_device_self_test_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_device_self_test(union xnvme_spec_sqe *sqe,
				    const struct xnvme_cmd_prep_adm_device_self_test_args *args)
{
	static const uint16_t rsvd[] = {9, 3};
	struct xnvme_cmd_prep_adm_device_self_test_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.stc >> 4) { ///< Self-test Code: 4 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x14;

	prep_bits(sqe, 1, 0, 32, a.nsid);  ///< Namespace Identifier
	prep_bits(sqe, 10, 0, 4, a.stc);   ///< Self-test Code
	prep_bits(sqe, 15, 0, 32, a.dstp); ///< Device Self-test Parameter

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_namespace_attachment_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_namespace_attachment(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_adm_namespace_attachment_args *args)
{
	static const uint16_t rsvd[] = {9, 7};
	struct xnvme_cmd_prep_adm_namespace_attachment_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.sel >> 4) { ///< Select: 4 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x15;

	prep_bits(sqe, 1, 0, 32, a.nsid); ///< Namespace Identifier
	prep_bits(sqe, 10, 0, 4, a.sel);  ///< Select

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_keep_alive_args) == 8,
		    "The record has padding")

int
xnvme_cmd_prep_adm_keep_alive(union xnvme_spec_sqe *sqe,
			      const struct xnvme_cmd_prep_adm_keep_alive_args *args)
{
	static const uint16_t rsvd[] = {4, 4};
	struct xnvme_cmd_prep_adm_keep_alive_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x18;

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_directive_send_args) == 32,
		    "The record has padding")

int
xnvme_cmd_prep_adm_directive_send(union xnvme_spec_sqe *sqe,
				  const struct xnvme_cmd_prep_adm_directive_send_args *args)
{
	struct xnvme_cmd_prep_adm_directive_send_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, NULL, 0);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x19;

	prep_bits(sqe, 1, 0, 32, a.nsid);    ///< Namespace Identifier
	prep_bits(sqe, 10, 0, 32, a.numd);   ///< Number of Dwords
	prep_bits(sqe, 11, 16, 16, a.dspec); ///< Directive Specific
	prep_bits(sqe, 11, 8, 8, a.dtype);   ///< Directive Type
	prep_bits(sqe, 11, 0, 8, a.doper);   ///< Directive Operation
	prep_bits(sqe, 12, 0, 32, a.cdw12);  ///< as the feature or operation describes it
	prep_bits(sqe, 13, 0, 32, a.cdw13);  ///< as the feature or operation describes it
	prep_bits(sqe, 14, 0, 32, a.cdw14);  ///< as the feature or operation describes it
	prep_bits(sqe, 15, 0, 32, a.cdw15);  ///< as the feature or operation describes it

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_directive_receive_args) == 32,
		    "The record has padding")

int
xnvme_cmd_prep_adm_directive_receive(union xnvme_spec_sqe *sqe,
				     const struct xnvme_cmd_prep_adm_directive_receive_args *args)
{
	struct xnvme_cmd_prep_adm_directive_receive_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, NULL, 0);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x1A;

	prep_bits(sqe, 1, 0, 32, a.nsid);    ///< Namespace Identifier
	prep_bits(sqe, 10, 0, 32, a.numd);   ///< Number of Dwords
	prep_bits(sqe, 11, 16, 16, a.dspec); ///< Directive Specific
	prep_bits(sqe, 11, 8, 8, a.dtype);   ///< Directive Type
	prep_bits(sqe, 11, 0, 8, a.doper);   ///< Directive Operation
	prep_bits(sqe, 12, 0, 32, a.cdw12);  ///< as the feature or operation describes it
	prep_bits(sqe, 13, 0, 32, a.cdw13);  ///< as the feature or operation describes it
	prep_bits(sqe, 14, 0, 32, a.cdw14);  ///< as the feature or operation describes it
	prep_bits(sqe, 15, 0, 32, a.cdw15);  ///< as the feature or operation describes it

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_virtualization_management_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_virtualization_management(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_virtualization_management_args *args)
{
	static const uint16_t rsvd[] = {10, 6};
	struct xnvme_cmd_prep_adm_virtualization_management_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.rt >> 3) { ///< Resource Type: 3 bits
		return -EINVAL;
	}
	if (a.act >> 4) { ///< Action: 4 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x1C;

	prep_bits(sqe, 10, 16, 16, a.cntlid); ///< Controller Identifier
	prep_bits(sqe, 10, 8, 3, a.rt);       ///< Resource Type
	prep_bits(sqe, 10, 0, 4, a.act);      ///< Action
	prep_bits(sqe, 11, 0, 16, a.nr);      ///< Number of Controller Resources

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_nvme_mi_send_args) == 8,
		    "The record has padding")

int
xnvme_cmd_prep_adm_nvme_mi_send(union xnvme_spec_sqe *sqe,
				const struct xnvme_cmd_prep_adm_nvme_mi_send_args *args)
{
	static const uint16_t rsvd[] = {4, 4};
	struct xnvme_cmd_prep_adm_nvme_mi_send_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x1D;

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_nvme_mi_receive_args) == 8,
		    "The record has padding")

int
xnvme_cmd_prep_adm_nvme_mi_receive(union xnvme_spec_sqe *sqe,
				   const struct xnvme_cmd_prep_adm_nvme_mi_receive_args *args)
{
	static const uint16_t rsvd[] = {4, 4};
	struct xnvme_cmd_prep_adm_nvme_mi_receive_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x1E;

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_capacity_management_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_capacity_management(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_adm_capacity_management_args *args)
{
	static const uint16_t rsvd[] = {7, 1};
	struct xnvme_cmd_prep_adm_capacity_management_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.oper >> 4) { ///< Operation: 4 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x20;

	prep_bits(sqe, 10, 16, 16, a.elid);     ///< Element Identifier
	prep_bits(sqe, 10, 0, 4, a.oper);       ///< Operation
	prep_bits(sqe, 11, 0, 32, a.cap);       ///< Capacity Lower
	prep_bits(sqe, 12, 0, 32, a.cap >> 32); ///< Capacity Upper

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_discovery_information_management_args) == 8,
		    "The record has padding")

int
xnvme_cmd_prep_adm_discovery_information_management(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_discovery_information_management_args *args)
{
	static const uint16_t rsvd[] = {5, 3};
	struct xnvme_cmd_prep_adm_discovery_information_management_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.tas >> 4) { ///< Task: 4 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x21;

	prep_bits(sqe, 10, 0, 4, a.tas); ///< Task

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_fabric_zoning_receive_args) == 24,
		    "The record has padding")

int
xnvme_cmd_prep_adm_fabric_zoning_receive(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_fabric_zoning_receive_args *args)
{
	static const uint16_t rsvd[] = {13, 3, 20, 4};
	struct xnvme_cmd_prep_adm_fabric_zoning_receive_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 2);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.zdkc >> 1) { ///< ZDK Context: 1 bit
		return -EINVAL;
	}
	if (a.numd >> 28) { ///< Number of Dwords: 28 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x22;

	prep_bits(sqe, 10, 0, 32, a.zdk);  ///< Zoning Data Key
	prep_bits(sqe, 11, 0, 32, a.zdo);  ///< Zoning Data Offset
	prep_bits(sqe, 12, 28, 1, a.zdkc); ///< ZDK Context
	prep_bits(sqe, 12, 0, 28, a.numd); ///< Number of Dwords

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_lockdown_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_lockdown(union xnvme_spec_sqe *sqe,
			    const struct xnvme_cmd_prep_adm_lockdown_args *args)
{
	static const uint16_t rsvd[] = {9, 1, 13, 3};
	struct xnvme_cmd_prep_adm_lockdown_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 2);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.csel >> 4) { ///< Controller Select: 4 bits
		return -EINVAL;
	}
	if (a.ifc >> 2) { ///< Interface: 2 bits
		return -EINVAL;
	}
	if (a.prhbt >> 1) { ///< Prohibit: 1 bit
		return -EINVAL;
	}
	if (a.scp >> 4) { ///< Scope: 4 bits
		return -EINVAL;
	}
	if (a.uidx >> 7) { ///< UUID Index: 7 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x24;

	prep_bits(sqe, 10, 16, 4, a.csel); ///< Controller Select
	prep_bits(sqe, 10, 8, 8, a.ofi);   ///< Opcode or Feature Identifier
	prep_bits(sqe, 10, 5, 2, a.ifc);   ///< Interface
	prep_bits(sqe, 10, 4, 1, a.prhbt); ///< Prohibit
	prep_bits(sqe, 10, 0, 4, a.scp);   ///< Scope
	prep_bits(sqe, 14, 16, 16, a.css); ///< Controller Select Specific
	prep_bits(sqe, 14, 0, 7, a.uidx);  ///< UUID Index

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_fabric_zoning_lookup_args) == 8,
		    "The record has padding")

int
xnvme_cmd_prep_adm_fabric_zoning_lookup(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_adm_fabric_zoning_lookup_args *args)
{
	static const uint16_t rsvd[] = {4, 4};
	struct xnvme_cmd_prep_adm_fabric_zoning_lookup_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x25;

	return 0;
}

XNVME_STATIC_ASSERT(
	sizeof(struct xnvme_cmd_prep_adm_clear_exported_nvm_resource_configuration_args) == 8,
	"The record has padding")

int
xnvme_cmd_prep_adm_clear_exported_nvm_resource_configuration(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_clear_exported_nvm_resource_configuration_args *args)
{
	static const uint16_t rsvd[] = {4, 4};
	struct xnvme_cmd_prep_adm_clear_exported_nvm_resource_configuration_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x28;

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_fabric_zoning_send_args) == 24,
		    "The record has padding")

int
xnvme_cmd_prep_adm_fabric_zoning_send(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_adm_fabric_zoning_send_args *args)
{
	static const uint16_t rsvd[] = {14, 2, 20, 4};
	struct xnvme_cmd_prep_adm_fabric_zoning_send_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 2);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.lf >> 1) { ///< Last Fragment: 1 bit
		return -EINVAL;
	}
	if (a.zdkc >> 1) { ///< ZDK Context: 1 bit
		return -EINVAL;
	}
	if (a.numd >> 28) { ///< Number of Dwords: 28 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x29;

	prep_bits(sqe, 10, 0, 32, a.zdk);  ///< Zoning Data Key
	prep_bits(sqe, 11, 0, 32, a.zdo);  ///< Zoning Data Offset
	prep_bits(sqe, 12, 31, 1, a.lf);   ///< Last Fragment
	prep_bits(sqe, 12, 28, 1, a.zdkc); ///< ZDK Context
	prep_bits(sqe, 12, 0, 28, a.numd); ///< Number of Dwords

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_manage_exported_nvm_subsystem_receive_args) ==
			    32,
		    "The record has padding")

int
xnvme_cmd_prep_adm_manage_exported_nvm_subsystem_receive(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_manage_exported_nvm_subsystem_receive_args *args)
{
	static const uint16_t rsvd[] = {10, 2, 20, 4};
	struct xnvme_cmd_prep_adm_manage_exported_nvm_subsystem_receive_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 2);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x2A;

	prep_bits(sqe, 10, 16, 16, a.mose);  ///< Management Operation Specific Extended
	prep_bits(sqe, 10, 8, 8, a.mos);     ///< Management Operation Specific
	prep_bits(sqe, 10, 0, 8, a.sel);     ///< Select
	prep_bits(sqe, 11, 0, 32, a.cdw11);  ///< operation-specific, as the operation describes it
	prep_bits(sqe, 12, 0, 32, a.offset); ///< Offset Lower
	prep_bits(sqe, 13, 0, 32, a.offset >> 32); ///< Offset Upper
	prep_bits(sqe, 14, 16, 16, a.mosi);        ///< Management Operation Specific Identifier
	prep_bits(sqe, 15, 0, 32, a.numdl);        ///< Number of Dwords

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_manage_exported_nvm_subsystem_send_args) ==
			    16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_manage_exported_nvm_subsystem_send(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_manage_exported_nvm_subsystem_send_args *args)
{
	static const uint16_t rsvd[] = {11, 1};
	struct xnvme_cmd_prep_adm_manage_exported_nvm_subsystem_send_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.esubidv >> 1) { ///< Exported NVM Subsystem Identifier Valid: 1 bit
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x2D;

	prep_bits(sqe, 10, 16, 16, a.mose);   ///< Management Operation Specific Extended
	prep_bits(sqe, 10, 8, 8, a.mos);      ///< Management Operation Specific
	prep_bits(sqe, 10, 0, 8, a.sel);      ///< Select
	prep_bits(sqe, 14, 16, 16, a.esubid); ///< Exported NVM Subsystem Identifier
	prep_bits(sqe, 14, 7, 1, a.esubidv);  ///< Exported NVM Subsystem Identifier Valid
	prep_bits(sqe, 15, 0, 32, a.numd);    ///< Number of Dwords

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_manage_exported_namespace_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_manage_exported_namespace(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_manage_exported_namespace_args *args)
{
	static const uint16_t rsvd[] = {13, 3};
	struct xnvme_cmd_prep_adm_manage_exported_namespace_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.esubidv >> 1) { ///< Exported NVM Subsystem Identifier Valid: 1 bit
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x31;

	prep_bits(sqe, 1, 0, 32, a.nsid);     ///< Namespace Identifier
	prep_bits(sqe, 10, 8, 8, a.mos);      ///< Management Operation Specific
	prep_bits(sqe, 10, 0, 8, a.sel);      ///< Select
	prep_bits(sqe, 14, 16, 16, a.esubid); ///< Exported NVM Subsystem Identifier
	prep_bits(sqe, 14, 7, 1, a.esubidv);  ///< Exported NVM Subsystem Identifier Valid

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_manage_exported_port_args) == 8,
		    "The record has padding")

int
xnvme_cmd_prep_adm_manage_exported_port(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_adm_manage_exported_port_args *args)
{
	static const uint16_t rsvd[] = {4, 4};
	struct xnvme_cmd_prep_adm_manage_exported_port_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x35;

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_cross_controller_reset_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_cross_controller_reset(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_cross_controller_reset_args *args)
{
	static const uint16_t rsvd[] = {5, 1};
	struct xnvme_cmd_prep_adm_cross_controller_reset_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x38;

	prep_bits(sqe, 10, 16, 8, a.ciu);        ///< Controller Instance Uniquifer
	prep_bits(sqe, 10, 0, 16, a.icid);       ///< Impacted Controller ID
	prep_bits(sqe, 12, 0, 32, a.cirn);       ///< Controller Instance Random Number Lower
	prep_bits(sqe, 13, 0, 32, a.cirn >> 32); ///< Controller Instance Random Number Upper

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_send_discovery_log_page_args) == 24,
		    "The record has padding")

int
xnvme_cmd_prep_adm_send_discovery_log_page(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_send_discovery_log_page_args *args)
{
	static const uint16_t rsvd[] = {9, 3};
	struct xnvme_cmd_prep_adm_send_discovery_log_page_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.rlps >> 2) { ///< Requested Log Page Status: 2 bits
		return -EINVAL;
	}
	if (a.sct >> 3) { ///< Status Code Type: 3 bits
		return -EINVAL;
	}
	if (a.tlsp >> 7) { ///< Transferred Log Specific Parameter: 7 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x39;

	prep_bits(sqe, 10, 30, 2, a.rlps);       ///< Requested Log Page Status
	prep_bits(sqe, 10, 25, 3, a.sct);        ///< Status Code Type
	prep_bits(sqe, 10, 17, 8, a.sc);         ///< Status Code
	prep_bits(sqe, 10, 8, 7, a.tlsp);        ///< Transferred Log Specific Parameter
	prep_bits(sqe, 10, 0, 8, a.tlid);        ///< Transferred Log Page Identifier
	prep_bits(sqe, 11, 0, 32, a.ndws);       ///< Number of Dwords
	prep_bits(sqe, 12, 0, 32, a.tlpo);       ///< Transferred Log Page Offset Lower
	prep_bits(sqe, 13, 0, 32, a.tlpo >> 32); ///< Transferred Log Page Offset Upper

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_track_send_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_track_send(union xnvme_spec_sqe *sqe,
			      const struct xnvme_cmd_prep_adm_track_send_args *args)
{
	static const uint16_t rsvd[] = {7, 1, 12, 4};
	struct xnvme_cmd_prep_adm_track_send_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 2);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x3D;

	prep_bits(sqe, 10, 16, 16, a.mos);  ///< Management Operation Specific
	prep_bits(sqe, 10, 0, 8, a.sel);    ///< Select
	prep_bits(sqe, 11, 0, 32, a.cdw11); ///< operation-specific, as the operation describes it

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_track_receive_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_track_receive(union xnvme_spec_sqe *sqe,
				 const struct xnvme_cmd_prep_adm_track_receive_args *args)
{
	static const uint16_t rsvd[] = {5, 3};
	struct xnvme_cmd_prep_adm_track_receive_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x3E;

	prep_bits(sqe, 10, 0, 8, a.sel);    ///< Select
	prep_bits(sqe, 11, 0, 32, a.cdw11); ///< operation-specific, as the operation describes it
	prep_bits(sqe, 12, 0, 32, a.numdl); ///< Number of Dwords

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_migration_send_args) == 24,
		    "The record has padding")

int
xnvme_cmd_prep_adm_migration_send(union xnvme_spec_sqe *sqe,
				  const struct xnvme_cmd_prep_adm_migration_send_args *args)
{
	struct xnvme_cmd_prep_adm_migration_send_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, NULL, 0);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.uidx >> 7) { ///< UUID Index: 7 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x41;

	prep_bits(sqe, 10, 16, 16, a.mos);  ///< Management Operation Specific
	prep_bits(sqe, 10, 0, 8, a.sel);    ///< Select
	prep_bits(sqe, 11, 0, 32, a.cdw11); ///< operation-specific, as the operation describes it
	prep_bits(sqe, 12, 0, 32, a.cdw12); ///< operation-specific, as the operation describes it
	prep_bits(sqe, 13, 0, 32, a.cdw13); ///< operation-specific, as the operation describes it
	prep_bits(sqe, 14, 0, 7, a.uidx);   ///< UUID Index
	prep_bits(sqe, 15, 0, 32, a.cdw15); ///< operation-specific, as the operation describes it

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_migration_receive_args) == 24,
		    "The record has padding")

int
xnvme_cmd_prep_adm_migration_receive(union xnvme_spec_sqe *sqe,
				     const struct xnvme_cmd_prep_adm_migration_receive_args *args)
{
	struct xnvme_cmd_prep_adm_migration_receive_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, NULL, 0);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.uidx >> 7) { ///< UUID Index: 7 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x42;

	prep_bits(sqe, 10, 16, 16, a.mos);   ///< Management Operation Specific
	prep_bits(sqe, 10, 0, 8, a.sel);     ///< Select
	prep_bits(sqe, 11, 0, 32, a.cdw11);  ///< operation-specific, as the operation describes it
	prep_bits(sqe, 12, 0, 32, a.offset); ///< Offset Lower
	prep_bits(sqe, 13, 0, 32, a.offset >> 32); ///< Offset Upper
	prep_bits(sqe, 14, 0, 7, a.uidx);          ///< UUID Index
	prep_bits(sqe, 15, 0, 32, a.numdl);        ///< Number of Dwords

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_controller_data_queue_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_controller_data_queue(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_controller_data_queue_args *args)
{
	static const uint16_t rsvd[] = {7, 1};
	struct xnvme_cmd_prep_adm_controller_data_queue_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x45;

	prep_bits(sqe, 10, 16, 16, a.mos);  ///< Management Operation Specific
	prep_bits(sqe, 10, 0, 8, a.sel);    ///< Select
	prep_bits(sqe, 11, 0, 32, a.cdw11); ///< operation-specific, as the operation describes it
	prep_bits(sqe, 12, 0, 32, a.cdw12); ///< operation-specific, as the operation describes it

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_doorbell_buffer_config_args) == 8,
		    "The record has padding")

int
xnvme_cmd_prep_adm_doorbell_buffer_config(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_doorbell_buffer_config_args *args)
{
	static const uint16_t rsvd[] = {4, 4};
	struct xnvme_cmd_prep_adm_doorbell_buffer_config_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x7C;

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_format_nvm_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_format_nvm(union xnvme_spec_sqe *sqe,
			      const struct xnvme_cmd_prep_adm_format_nvm_args *args)
{
	static const uint16_t rsvd[] = {13, 3};
	struct xnvme_cmd_prep_adm_format_nvm_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.ses >> 3) { ///< Secure Erase Settings: 3 bits
		return -EINVAL;
	}
	if (a.pil >> 1) { ///< Protection Information Location: 1 bit
		return -EINVAL;
	}
	if (a.pi >> 3) { ///< Protection Information: 3 bits
		return -EINVAL;
	}
	if (a.mset >> 1) { ///< Metadata Settings: 1 bit
		return -EINVAL;
	}
	if (a.lbaf >> 6) { ///< LBA Format: 6 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x80;

	prep_bits(sqe, 1, 0, 32, a.nsid);       ///< Namespace Identifier
	prep_bits(sqe, 10, 12, 2, a.lbaf >> 4); ///< LBA Format Upper
	prep_bits(sqe, 10, 9, 3, a.ses);        ///< Secure Erase Settings
	prep_bits(sqe, 10, 8, 1, a.pil);        ///< Protection Information Location
	prep_bits(sqe, 10, 5, 3, a.pi);         ///< Protection Information
	prep_bits(sqe, 10, 4, 1, a.mset);       ///< Metadata Settings
	prep_bits(sqe, 10, 0, 4, a.lbaf);       ///< LBA Format Lower

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_security_send_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_security_send(union xnvme_spec_sqe *sqe,
				 const struct xnvme_cmd_prep_adm_security_send_args *args)
{
	struct xnvme_cmd_prep_adm_security_send_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, NULL, 0);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x81;

	prep_bits(sqe, 1, 0, 32, a.nsid);   ///< Namespace Identifier
	prep_bits(sqe, 10, 24, 8, a.secp);  ///< Security Protocol
	prep_bits(sqe, 10, 16, 8, a.spsp1); ///< SP Specific 1
	prep_bits(sqe, 10, 8, 8, a.spsp0);  ///< SP Specific 0
	prep_bits(sqe, 10, 0, 8, a.nssf);   ///< NVMe Security Specific Field
	prep_bits(sqe, 11, 0, 32, a.tl);    ///< Transfer Length

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_security_receive_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_security_receive(union xnvme_spec_sqe *sqe,
				    const struct xnvme_cmd_prep_adm_security_receive_args *args)
{
	struct xnvme_cmd_prep_adm_security_receive_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, NULL, 0);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x82;

	prep_bits(sqe, 1, 0, 32, a.nsid);   ///< Namespace Identifier
	prep_bits(sqe, 10, 24, 8, a.secp);  ///< Security Protocol
	prep_bits(sqe, 10, 16, 8, a.spsp1); ///< SP Specific 1
	prep_bits(sqe, 10, 8, 8, a.spsp0);  ///< SP Specific 0
	prep_bits(sqe, 10, 0, 8, a.nssf);   ///< NVMe Security Specific Field
	prep_bits(sqe, 11, 0, 32, a.al);    ///< Allocation Length

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_sanitize_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_sanitize(union xnvme_spec_sqe *sqe,
			    const struct xnvme_cmd_prep_adm_sanitize_args *args)
{
	static const uint16_t rsvd[] = {11, 1};
	struct xnvme_cmd_prep_adm_sanitize_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.preq >> 1) { ///< Purge Required: 1 bit
		return -EINVAL;
	}
	if (a.emvs >> 1) { ///< Enter Media Verification State: 1 bit
		return -EINVAL;
	}
	if (a.ndas >> 1) { ///< No-Deallocate After Sanitize: 1 bit
		return -EINVAL;
	}
	if (a.oipbp >> 1) { ///< Overwrite Invert Pattern Between Passes: 1 bit
		return -EINVAL;
	}
	if (a.owpass >> 4) { ///< Overwrite Pass Count: 4 bits
		return -EINVAL;
	}
	if (a.ause >> 1) { ///< Allow Unrestricted Sanitize Exit: 1 bit
		return -EINVAL;
	}
	if (a.sanact >> 3) { ///< Sanitize Action: 3 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x84;

	prep_bits(sqe, 10, 11, 1, a.preq);   ///< Purge Required
	prep_bits(sqe, 10, 10, 1, a.emvs);   ///< Enter Media Verification State
	prep_bits(sqe, 10, 9, 1, a.ndas);    ///< No-Deallocate After Sanitize
	prep_bits(sqe, 10, 8, 1, a.oipbp);   ///< Overwrite Invert Pattern Between Passes
	prep_bits(sqe, 10, 4, 4, a.owpass);  ///< Overwrite Pass Count
	prep_bits(sqe, 10, 3, 1, a.ause);    ///< Allow Unrestricted Sanitize Exit
	prep_bits(sqe, 10, 0, 3, a.sanact);  ///< Sanitize Action
	prep_bits(sqe, 11, 0, 32, a.ovrpat); ///< Overwrite Pattern

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_load_program_args) == 32,
		    "The record has padding")

int
xnvme_cmd_prep_adm_load_program(union xnvme_spec_sqe *sqe,
				const struct xnvme_cmd_prep_adm_load_program_args *args)
{
	static const uint16_t rsvd[] = {11, 1, 14, 2, 28, 4};
	struct xnvme_cmd_prep_adm_load_program_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 3);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.pit >> 3) { ///< Program Identifier Type: 3 bits
		return -EINVAL;
	}
	if (a.sel >> 1) { ///< Select: 1 bit
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x85;

	prep_bits(sqe, 1, 0, 32, a.nsid);   ///< Namespace Identifier
	prep_bits(sqe, 10, 25, 3, a.pit);   ///< Program Identifier Type
	prep_bits(sqe, 10, 24, 1, a.sel);   ///< Select
	prep_bits(sqe, 10, 16, 8, a.ptype); ///< Program Type
	prep_bits(sqe, 10, 0, 16, a.pind);  ///< Program Index
	prep_bits(sqe, 11, 0, 32, a.psize); ///< Program Size
	prep_bits(sqe, 14, 0, 32, a.numb);  ///< Number of Bytes
	prep_bits(sqe, 15, 0, 32, a.loff);  ///< Load Offset

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_get_lba_status_args) == 24,
		    "The record has padding")

int
xnvme_cmd_prep_adm_get_lba_status(union xnvme_spec_sqe *sqe,
				  const struct xnvme_cmd_prep_adm_get_lba_status_args *args)
{
	static const uint16_t rsvd[] = {21, 1};
	struct xnvme_cmd_prep_adm_get_lba_status_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x86;

	prep_bits(sqe, 1, 0, 32, a.nsid);   ///< Namespace Identifier
	prep_bits(sqe, 10, 0, 64, a.slba);  ///< Starting LBA
	prep_bits(sqe, 12, 0, 32, a.mndw);  ///< Maximum Number of Dwords
	prep_bits(sqe, 13, 24, 8, a.atype); ///< Action Type
	prep_bits(sqe, 13, 0, 16, a.rl);    ///< Range Length

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_program_activation_management_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_program_activation_management(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_program_activation_management_args *args)
{
	static const uint16_t rsvd[] = {9, 1, 12, 4};
	struct xnvme_cmd_prep_adm_program_activation_management_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 2);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.sel >> 4) { ///< Select: 4 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x88;

	prep_bits(sqe, 1, 0, 32, a.nsid);  ///< Namespace Identifier
	prep_bits(sqe, 10, 16, 4, a.sel);  ///< Select
	prep_bits(sqe, 10, 0, 16, a.pind); ///< Program Index

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_memory_range_set_management_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_memory_range_set_management(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_memory_range_set_management_args *args)
{
	static const uint16_t rsvd[] = {12, 4};
	struct xnvme_cmd_prep_adm_memory_range_set_management_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.sel >> 4) { ///< Select: 4 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x89;

	prep_bits(sqe, 1, 0, 32, a.nsid);   ///< Namespace Identifier
	prep_bits(sqe, 10, 16, 16, a.rsid); ///< Memory Range Set ID
	prep_bits(sqe, 10, 0, 4, a.sel);    ///< Select
	prep_bits(sqe, 11, 0, 8, a.numr);   ///< Number of Memory Ranges

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_sanitize_namespace_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_adm_sanitize_namespace(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_adm_sanitize_namespace_args *args)
{
	static const uint16_t rsvd[] = {12, 4};
	struct xnvme_cmd_prep_adm_sanitize_namespace_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.emvs >> 1) { ///< Enter Media Verification State: 1 bit
		return -EINVAL;
	}
	if (a.preq >> 1) { ///< Purge Required: 1 bit
		return -EINVAL;
	}
	if (a.ause >> 1) { ///< Allow Unrestricted Sanitize Exit: 1 bit
		return -EINVAL;
	}
	if (a.sanact >> 3) { ///< Sanitize Action: 3 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x8C;

	prep_bits(sqe, 1, 0, 32, a.nsid);   ///< Namespace Identifier
	prep_bits(sqe, 10, 10, 1, a.emvs);  ///< Enter Media Verification State
	prep_bits(sqe, 10, 4, 1, a.preq);   ///< Purge Required
	prep_bits(sqe, 10, 3, 1, a.ause);   ///< Allow Unrestricted Sanitize Exit
	prep_bits(sqe, 10, 0, 3, a.sanact); ///< Sanitize Action

	return 0;
}

/*
 * I/O Commands
 */

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_io_flush_args) == 8, "The record has padding")

int
xnvme_cmd_prep_io_flush(union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_io_flush_args *args)
{
	struct xnvme_cmd_prep_io_flush_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, NULL, 0);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x00;

	prep_bits(sqe, 1, 0, 32, a.nsid); ///< Namespace Identifier

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_io_reservation_register_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_io_reservation_register(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_io_reservation_register_args *args)
{
	static const uint16_t rsvd[] = {12, 4};
	struct xnvme_cmd_prep_io_reservation_register_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.cptpl >> 2) { ///< Change Persist Through Power Loss State: 2 bits
		return -EINVAL;
	}
	if (a.disnsrs >> 1) { ///< Dispersed Namespace Reservation Support: 1 bit
		return -EINVAL;
	}
	if (a.iekey >> 1) { ///< Ignore Existing Key: 1 bit
		return -EINVAL;
	}
	if (a.rrega >> 3) { ///< Reservation Register Action: 3 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x0D;

	prep_bits(sqe, 1, 0, 32, a.nsid);    ///< Namespace Identifier
	prep_bits(sqe, 10, 30, 2, a.cptpl);  ///< Change Persist Through Power Loss State
	prep_bits(sqe, 10, 4, 1, a.disnsrs); ///< Dispersed Namespace Reservation Support
	prep_bits(sqe, 10, 3, 1, a.iekey);   ///< Ignore Existing Key
	prep_bits(sqe, 10, 0, 3, a.rrega);   ///< Reservation Register Action

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_io_reservation_report_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_io_reservation_report(union xnvme_spec_sqe *sqe,
				     const struct xnvme_cmd_prep_io_reservation_report_args *args)
{
	static const uint16_t rsvd[] = {14, 2};
	struct xnvme_cmd_prep_io_reservation_report_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.disnsrs >> 1) { ///< Dispersed Namespace Reservation Support: 1 bit
		return -EINVAL;
	}
	if (a.eds >> 1) { ///< Extended Data Structure: 1 bit
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x0E;

	prep_bits(sqe, 1, 0, 32, a.nsid);    ///< Namespace Identifier
	prep_bits(sqe, 10, 0, 32, a.numd);   ///< Number of Dwords
	prep_bits(sqe, 11, 1, 1, a.disnsrs); ///< Dispersed Namespace Reservation Support
	prep_bits(sqe, 11, 0, 1, a.eds);     ///< Extended Data Structure

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_io_reservation_acquire_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_io_reservation_acquire(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_io_reservation_acquire_args *args)
{
	static const uint16_t rsvd[] = {12, 4};
	struct xnvme_cmd_prep_io_reservation_acquire_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.disnsrs >> 1) { ///< Dispersed Namespace Reservation Support: 1 bit
		return -EINVAL;
	}
	if (a.iekey >> 1) { ///< Ignore Existing Key: 1 bit
		return -EINVAL;
	}
	if (a.racqa >> 3) { ///< Reservation Acquire Action: 3 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x11;

	prep_bits(sqe, 1, 0, 32, a.nsid);    ///< Namespace Identifier
	prep_bits(sqe, 10, 8, 8, a.rtype);   ///< Reservation Type
	prep_bits(sqe, 10, 4, 1, a.disnsrs); ///< Dispersed Namespace Reservation Support
	prep_bits(sqe, 10, 3, 1, a.iekey);   ///< Ignore Existing Key
	prep_bits(sqe, 10, 0, 3, a.racqa);   ///< Reservation Acquire Action

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_io_io_management_receive_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_io_io_management_receive(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_io_io_management_receive_args *args)
{
	static const uint16_t rsvd[] = {11, 1};
	struct xnvme_cmd_prep_io_io_management_receive_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x12;

	prep_bits(sqe, 1, 0, 32, a.nsid);  ///< Namespace Identifier
	prep_bits(sqe, 10, 16, 16, a.mos); ///< Management Operation Specific
	prep_bits(sqe, 10, 0, 8, a.mo);    ///< Management Operation
	prep_bits(sqe, 11, 0, 32, a.numd); ///< Number of Dwords

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_io_reservation_release_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_io_reservation_release(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_io_reservation_release_args *args)
{
	static const uint16_t rsvd[] = {12, 4};
	struct xnvme_cmd_prep_io_reservation_release_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.disnsrs >> 1) { ///< Dispersed Namespace Reservation Support: 1 bit
		return -EINVAL;
	}
	if (a.iekey >> 1) { ///< Ignore Existing Key: 1 bit
		return -EINVAL;
	}
	if (a.rrela >> 3) { ///< Reservation Release Action: 3 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x15;

	prep_bits(sqe, 1, 0, 32, a.nsid);    ///< Namespace Identifier
	prep_bits(sqe, 10, 8, 8, a.rtype);   ///< Reservation Type
	prep_bits(sqe, 10, 4, 1, a.disnsrs); ///< Dispersed Namespace Reservation Support
	prep_bits(sqe, 10, 3, 1, a.iekey);   ///< Ignore Existing Key
	prep_bits(sqe, 10, 0, 3, a.rrela);   ///< Reservation Release Action

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_io_cancel_args) == 16, "The record has padding")

int
xnvme_cmd_prep_io_cancel(union xnvme_spec_sqe *sqe,
			 const struct xnvme_cmd_prep_io_cancel_args *args)
{
	static const uint16_t rsvd[] = {13, 3};
	struct xnvme_cmd_prep_io_cancel_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.acode >> 2) { ///< Action Code: 2 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x18;

	prep_bits(sqe, 1, 0, 32, a.nsid);  ///< Namespace Identifier
	prep_bits(sqe, 10, 16, 16, a.cid); ///< Command Identifier
	prep_bits(sqe, 10, 0, 16, a.sqid); ///< Submission Queue Identifier
	prep_bits(sqe, 11, 0, 2, a.acode); ///< Action Code

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_io_io_management_send_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_io_io_management_send(union xnvme_spec_sqe *sqe,
				     const struct xnvme_cmd_prep_io_io_management_send_args *args)
{
	static const uint16_t rsvd[] = {11, 5};
	struct xnvme_cmd_prep_io_io_management_send_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x1D;

	prep_bits(sqe, 1, 0, 32, a.nsid);  ///< Namespace Identifier
	prep_bits(sqe, 10, 16, 16, a.mos); ///< Management Operation Specific
	prep_bits(sqe, 10, 0, 8, a.mo);    ///< Management Operation

	return 0;
}

/*
 * NVM Command Set
 */

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_nvm_write_args) == 48, "The record has padding")

int
xnvme_cmd_prep_nvm_write(union xnvme_spec_sqe *sqe,
			 const struct xnvme_cmd_prep_nvm_write_args *args)
{
	static const uint16_t rsvd[] = {27, 1, 34, 2};
	struct xnvme_cmd_prep_nvm_write_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 2);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.lbtu >> 48) { ///< Logical Block Tags Upper: 48 bits
		return -EINVAL;
	}
	if (a.lr >> 1) { ///< Limited Retry: 1 bit
		return -EINVAL;
	}
	if (a.fua >> 1) { ///< Force Unit Access: 1 bit
		return -EINVAL;
	}
	if (a.prinfo >> 4) { ///< Protection Information: 4 bits
		return -EINVAL;
	}
	if (a.stc >> 1) { ///< Storage Tag Check: 1 bit
		return -EINVAL;
	}
	if (a.dtype >> 4) { ///< Directive Type: 4 bits
		return -EINVAL;
	}
	if (a.cetype >> 4) { ///< Command Extension Type: 4 bits
		return -EINVAL;
	}
	if (a.cetype != 0 && a.dsm) { ///< Dataset Management: only if CETYPE is cleared to 0h
		return -EINVAL;
	}
	if (a.cetype == 0 && a.cev) { ///< Command Extension Value: only if CETYPE is non-zero
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x01;

	prep_bits(sqe, 1, 0, 32, a.nsid);    ///< Namespace Identifier
	prep_bits(sqe, 2, 0, 48, a.lbtu);    ///< Logical Block Tags Upper
	prep_bits(sqe, 10, 0, 64, a.slba);   ///< Starting LBA
	prep_bits(sqe, 12, 31, 1, a.lr);     ///< Limited Retry
	prep_bits(sqe, 12, 30, 1, a.fua);    ///< Force Unit Access
	prep_bits(sqe, 12, 26, 4, a.prinfo); ///< Protection Information
	prep_bits(sqe, 12, 24, 1, a.stc);    ///< Storage Tag Check
	prep_bits(sqe, 12, 20, 4, a.dtype);  ///< Directive Type
	prep_bits(sqe, 12, 16, 4, a.cetype); ///< Command Extension Type
	prep_bits(sqe, 12, 0, 16, a.nlb);    ///< Number of Logical Blocks
	prep_bits(sqe, 13, 16, 16, a.dspec); ///< Directive Specific
	if (a.cetype == 0) {
		prep_bits(sqe, 13, 0, 8, a.dsm); ///< Dataset Management
	}
	if (a.cetype != 0) {
		prep_bits(sqe, 13, 0, 16, a.cev); ///< Command Extension Value
	}
	prep_bits(sqe, 14, 0, 32, a.lbtl);   ///< Logical Block Tags Lower
	prep_bits(sqe, 15, 16, 16, a.lbatm); ///< Logical Block Application Tag Mask
	prep_bits(sqe, 15, 0, 16, a.lbat);   ///< Logical Block Application Tag

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_nvm_read_args) == 48, "The record has padding")

int
xnvme_cmd_prep_nvm_read(union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_nvm_read_args *args)
{
	static const uint16_t rsvd[] = {21, 1, 25, 1, 36, 4};
	struct xnvme_cmd_prep_nvm_read_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 3);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.elbtu >> 48) { ///< Expected Logical Block Tags Upper: 48 bits
		return -EINVAL;
	}
	if (a.lr >> 1) { ///< Limited Retry: 1 bit
		return -EINVAL;
	}
	if (a.fua >> 1) { ///< Force Unit Access: 1 bit
		return -EINVAL;
	}
	if (a.prinfo >> 4) { ///< Protection Information: 4 bits
		return -EINVAL;
	}
	if (a.stc >> 1) { ///< Storage Tag Check: 1 bit
		return -EINVAL;
	}
	if (a.cetype >> 4) { ///< Command Extension Type: 4 bits
		return -EINVAL;
	}
	if (a.cetype != 0 && a.dsm) { ///< Dataset Management: only if CETYPE is cleared to 0h
		return -EINVAL;
	}
	if (a.cetype == 0 && a.cev) { ///< Command Extension Value: only if CETYPE is non-zero
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x02;

	prep_bits(sqe, 1, 0, 32, a.nsid);    ///< Namespace Identifier
	prep_bits(sqe, 2, 0, 48, a.elbtu);   ///< Expected Logical Block Tags Upper
	prep_bits(sqe, 10, 0, 64, a.slba);   ///< Starting LBA
	prep_bits(sqe, 12, 31, 1, a.lr);     ///< Limited Retry
	prep_bits(sqe, 12, 30, 1, a.fua);    ///< Force Unit Access
	prep_bits(sqe, 12, 26, 4, a.prinfo); ///< Protection Information
	prep_bits(sqe, 12, 24, 1, a.stc);    ///< Storage Tag Check
	prep_bits(sqe, 12, 16, 4, a.cetype); ///< Command Extension Type
	prep_bits(sqe, 12, 0, 16, a.nlb);    ///< Number of Logical Blocks
	if (a.cetype == 0) {
		prep_bits(sqe, 13, 0, 8, a.dsm); ///< Dataset Management
	}
	if (a.cetype != 0) {
		prep_bits(sqe, 13, 0, 16, a.cev); ///< Command Extension Value
	}
	prep_bits(sqe, 14, 0, 32, a.elbtl);   ///< Expected Logical Block Tags Lower
	prep_bits(sqe, 15, 16, 16, a.elbatm); ///< Expected Logical Block Application Tag Mask
	prep_bits(sqe, 15, 0, 16, a.elbat);   ///< Expected Logical Block Application Tag

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_nvm_write_uncorrectable_args) == 24,
		    "The record has padding")

int
xnvme_cmd_prep_nvm_write_uncorrectable(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_nvm_write_uncorrectable_args *args)
{
	static const uint16_t rsvd[] = {17, 1, 22, 2};
	struct xnvme_cmd_prep_nvm_write_uncorrectable_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 2);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.dtype >> 4) { ///< Directive Type: 4 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x04;

	prep_bits(sqe, 1, 0, 32, a.nsid);    ///< Namespace Identifier
	prep_bits(sqe, 10, 0, 64, a.slba);   ///< Starting LBA
	prep_bits(sqe, 12, 20, 4, a.dtype);  ///< Directive Type
	prep_bits(sqe, 12, 0, 16, a.nlb);    ///< Number of Logical Blocks
	prep_bits(sqe, 13, 16, 16, a.dspec); ///< Directive Specific

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_nvm_compare_args) == 48, "The record has padding")

int
xnvme_cmd_prep_nvm_compare(union xnvme_spec_sqe *sqe,
			   const struct xnvme_cmd_prep_nvm_compare_args *args)
{
	static const uint16_t rsvd[] = {21, 1, 30, 2, 36, 4};
	struct xnvme_cmd_prep_nvm_compare_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 3);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.elbtu >> 48) { ///< Expected Logical Block Tags Upper: 48 bits
		return -EINVAL;
	}
	if (a.lr >> 1) { ///< Limited Retry: 1 bit
		return -EINVAL;
	}
	if (a.fua >> 1) { ///< Force Unit Access: 1 bit
		return -EINVAL;
	}
	if (a.prinfo >> 4) { ///< Protection Information: 4 bits
		return -EINVAL;
	}
	if (a.stc >> 1) { ///< Storage Tag Check: 1 bit
		return -EINVAL;
	}
	if (a.cetype >> 4) { ///< Command Extension Type: 4 bits
		return -EINVAL;
	}
	if (a.cetype == 0 && a.cev) { ///< Command Extension Value: only if CETYPE is non-zero
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x05;

	prep_bits(sqe, 1, 0, 32, a.nsid);    ///< Namespace Identifier
	prep_bits(sqe, 2, 0, 48, a.elbtu);   ///< Expected Logical Block Tags Upper
	prep_bits(sqe, 10, 0, 64, a.slba);   ///< Starting LBA
	prep_bits(sqe, 12, 31, 1, a.lr);     ///< Limited Retry
	prep_bits(sqe, 12, 30, 1, a.fua);    ///< Force Unit Access
	prep_bits(sqe, 12, 26, 4, a.prinfo); ///< Protection Information
	prep_bits(sqe, 12, 24, 1, a.stc);    ///< Storage Tag Check
	prep_bits(sqe, 12, 16, 4, a.cetype); ///< Command Extension Type
	prep_bits(sqe, 12, 0, 16, a.nlb);    ///< Number of Logical Blocks
	if (a.cetype != 0) {
		prep_bits(sqe, 13, 0, 16, a.cev); ///< Command Extension Value
	}
	prep_bits(sqe, 14, 0, 32, a.elbtl);   ///< Expected Logical Block Tags Lower
	prep_bits(sqe, 15, 16, 16, a.elbatm); ///< Expected Logical Block Application Tag Mask
	prep_bits(sqe, 15, 0, 16, a.elbat);   ///< Expected Logical Block Application Tag

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_nvm_write_zeroes_args) == 48,
		    "The record has padding")

int
xnvme_cmd_prep_nvm_write_zeroes(union xnvme_spec_sqe *sqe,
				const struct xnvme_cmd_prep_nvm_write_zeroes_args *args)
{
	static const uint16_t rsvd[] = {34, 2};
	struct xnvme_cmd_prep_nvm_write_zeroes_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.lbtu >> 48) { ///< Logical Block Tags Upper: 48 bits
		return -EINVAL;
	}
	if (a.lr >> 1) { ///< Limited Retry: 1 bit
		return -EINVAL;
	}
	if (a.fua >> 1) { ///< Force Unit Access: 1 bit
		return -EINVAL;
	}
	if (a.prinfo >> 4) { ///< Protection Information: 4 bits
		return -EINVAL;
	}
	if (a.deac >> 1) { ///< Deallocate: 1 bit
		return -EINVAL;
	}
	if (a.stc >> 1) { ///< Storage Tag Check: 1 bit
		return -EINVAL;
	}
	if (a.nsz >> 1) { ///< Namespace Zeroes: 1 bit
		return -EINVAL;
	}
	if (a.dtype >> 3) { ///< Directive Type: 3 bits
		return -EINVAL;
	}
	if (a.cetype >> 4) { ///< Command Extension Type: 4 bits
		return -EINVAL;
	}
	if (a.cetype == 0 && a.cev) { ///< Command Extension Value: only if CETYPE is non-zero
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x08;

	prep_bits(sqe, 1, 0, 32, a.nsid);    ///< Namespace Identifier
	prep_bits(sqe, 2, 0, 48, a.lbtu);    ///< Logical Block Tags Upper
	prep_bits(sqe, 10, 0, 64, a.slba);   ///< Starting LBA
	prep_bits(sqe, 12, 31, 1, a.lr);     ///< Limited Retry
	prep_bits(sqe, 12, 30, 1, a.fua);    ///< Force Unit Access
	prep_bits(sqe, 12, 26, 4, a.prinfo); ///< Protection Information
	prep_bits(sqe, 12, 25, 1, a.deac);   ///< Deallocate
	prep_bits(sqe, 12, 24, 1, a.stc);    ///< Storage Tag Check
	prep_bits(sqe, 12, 23, 1, a.nsz);    ///< Namespace Zeroes
	prep_bits(sqe, 12, 20, 3, a.dtype);  ///< Directive Type
	prep_bits(sqe, 12, 16, 4, a.cetype); ///< Command Extension Type
	prep_bits(sqe, 12, 0, 16, a.nlb);    ///< Number of Logical Blocks
	prep_bits(sqe, 13, 16, 16, a.dspec); ///< Directive Specific
	if (a.cetype != 0) {
		prep_bits(sqe, 13, 0, 16, a.cev); ///< Command Extension Value
	}
	prep_bits(sqe, 14, 0, 32, a.lbtl);   ///< Logical Block Tags Lower
	prep_bits(sqe, 15, 16, 16, a.lbatm); ///< Logical Block Application Tag Mask
	prep_bits(sqe, 15, 0, 16, a.lbat);   ///< Logical Block Application Tag

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_nvm_dataset_management_args) == 16,
		    "The record has padding")

int
xnvme_cmd_prep_nvm_dataset_management(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_nvm_dataset_management_args *args)
{
	static const uint16_t rsvd[] = {12, 4};
	struct xnvme_cmd_prep_nvm_dataset_management_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.ad >> 1) { ///< Attribute - Deallocate: 1 bit
		return -EINVAL;
	}
	if (a.idw >> 1) { ///< Attribute - Integral Dataset for Write: 1 bit
		return -EINVAL;
	}
	if (a.idr >> 1) { ///< Attribute - Integral Dataset for Read: 1 bit
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x09;

	prep_bits(sqe, 1, 0, 32, a.nsid); ///< Namespace Identifier
	prep_bits(sqe, 10, 0, 8, a.nr);   ///< Number of Ranges
	prep_bits(sqe, 11, 2, 1, a.ad);   ///< Attribute - Deallocate
	prep_bits(sqe, 11, 1, 1, a.idw);  ///< Attribute - Integral Dataset for Write
	prep_bits(sqe, 11, 0, 1, a.idr);  ///< Attribute - Integral Dataset for Read

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_nvm_verify_args) == 48, "The record has padding")

int
xnvme_cmd_prep_nvm_verify(union xnvme_spec_sqe *sqe,
			  const struct xnvme_cmd_prep_nvm_verify_args *args)
{
	static const uint16_t rsvd[] = {21, 1, 30, 2, 36, 4};
	struct xnvme_cmd_prep_nvm_verify_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 3);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.elbtu >> 48) { ///< Expected Logical Block Tags Upper: 48 bits
		return -EINVAL;
	}
	if (a.lr >> 1) { ///< Limited Retry: 1 bit
		return -EINVAL;
	}
	if (a.fua >> 1) { ///< Force Unit Access: 1 bit
		return -EINVAL;
	}
	if (a.prinfo >> 4) { ///< Protection Information: 4 bits
		return -EINVAL;
	}
	if (a.stc >> 1) { ///< Storage Tag Check: 1 bit
		return -EINVAL;
	}
	if (a.cetype >> 4) { ///< Command Extension Type: 4 bits
		return -EINVAL;
	}
	if (a.cetype == 0 && a.cev) { ///< Command Extension Value: only if CETYPE is non-zero
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x0C;

	prep_bits(sqe, 1, 0, 32, a.nsid);    ///< Namespace Identifier
	prep_bits(sqe, 2, 0, 48, a.elbtu);   ///< Expected Logical Block Tags Upper
	prep_bits(sqe, 10, 0, 64, a.slba);   ///< Starting LBA
	prep_bits(sqe, 12, 31, 1, a.lr);     ///< Limited Retry
	prep_bits(sqe, 12, 30, 1, a.fua);    ///< Force Unit Access
	prep_bits(sqe, 12, 26, 4, a.prinfo); ///< Protection Information
	prep_bits(sqe, 12, 24, 1, a.stc);    ///< Storage Tag Check
	prep_bits(sqe, 12, 16, 4, a.cetype); ///< Command Extension Type
	prep_bits(sqe, 12, 0, 16, a.nlb);    ///< Number of Logical Blocks
	if (a.cetype != 0) {
		prep_bits(sqe, 13, 0, 16, a.cev); ///< Command Extension Value
	}
	prep_bits(sqe, 14, 0, 32, a.elbtl);   ///< Expected Logical Block Tags Lower
	prep_bits(sqe, 15, 16, 16, a.elbatm); ///< Expected Logical Block Application Tag Mask
	prep_bits(sqe, 15, 0, 16, a.elbat);   ///< Expected Logical Block Application Tag

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_nvm_copy_args) == 48, "The record has padding")

int
xnvme_cmd_prep_nvm_copy(union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_nvm_copy_args *args)
{
	static const uint16_t rsvd[] = {34, 2};
	struct xnvme_cmd_prep_nvm_copy_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.lbtu >> 48) { ///< Logical Block Tags Upper: 48 bits
		return -EINVAL;
	}
	if (a.lr >> 1) { ///< Limited Retry: 1 bit
		return -EINVAL;
	}
	if (a.fua >> 1) { ///< Force Unit Access: 1 bit
		return -EINVAL;
	}
	if (a.prinfow >> 4) { ///< Protection Information Write: 4 bits
		return -EINVAL;
	}
	if (a.stcr >> 1) { ///< Storage Tag Check Read: 1 bit
		return -EINVAL;
	}
	if (a.stcw >> 1) { ///< Storage Tag Check Write: 1 bit
		return -EINVAL;
	}
	if (a.dtype >> 4) { ///< Directive Type: 4 bits
		return -EINVAL;
	}
	if (a.cetype >> 4) { ///< Command Extension Type: 4 bits
		return -EINVAL;
	}
	if (a.prinfor >> 4) { ///< Protection Information Read: 4 bits
		return -EINVAL;
	}
	if (a.desfmt >> 4) { ///< Descriptor Format: 4 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x19;

	prep_bits(sqe, 1, 0, 32, a.nsid);     ///< Namespace Identifier
	prep_bits(sqe, 2, 0, 48, a.lbtu);     ///< Logical Block Tags Upper
	prep_bits(sqe, 10, 0, 64, a.sdlba);   ///< Starting Destination LBA
	prep_bits(sqe, 12, 31, 1, a.lr);      ///< Limited Retry
	prep_bits(sqe, 12, 30, 1, a.fua);     ///< Force Unit Access
	prep_bits(sqe, 12, 26, 4, a.prinfow); ///< Protection Information Write
	prep_bits(sqe, 12, 25, 1, a.stcr);    ///< Storage Tag Check Read
	prep_bits(sqe, 12, 24, 1, a.stcw);    ///< Storage Tag Check Write
	prep_bits(sqe, 12, 20, 4, a.dtype);   ///< Directive Type
	prep_bits(sqe, 12, 16, 4, a.cetype);  ///< Command Extension Type
	prep_bits(sqe, 12, 12, 4, a.prinfor); ///< Protection Information Read
	prep_bits(sqe, 12, 8, 4, a.desfmt);   ///< Descriptor Format
	prep_bits(sqe, 12, 0, 8, a.nr);       ///< Number of Ranges
	prep_bits(sqe, 13, 16, 16, a.dspec);  ///< Directive Specific
	prep_bits(sqe, 13, 0, 16, a.cev);     ///< Command Extension Value
	prep_bits(sqe, 14, 0, 32, a.lbtl);    ///< Logical Block Tags Lower
	prep_bits(sqe, 15, 16, 16, a.lbatm);  ///< Logical Block Application Tag Mask
	prep_bits(sqe, 15, 0, 16, a.lbat);    ///< Logical Block Application Tag

	return 0;
}

/*
 * Zoned Namespace Command Set
 */

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_zns_zone_management_send_args) == 24,
		    "The record has padding")

int
xnvme_cmd_prep_zns_zone_management_send(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_zns_zone_management_send_args *args)
{
	static const uint16_t rsvd[] = {20, 4};
	struct xnvme_cmd_prep_zns_zone_management_send_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.zsaso >> 1) { ///< Zone Send Action Specific Option: 1 bit
		return -EINVAL;
	}
	if (a.selall >> 1) { ///< Select All: 1 bit
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x79;

	prep_bits(sqe, 1, 0, 32, a.nsid);   ///< Namespace Identifier
	prep_bits(sqe, 10, 0, 64, a.slba);  ///< Starting LBA
	prep_bits(sqe, 13, 16, 8, a.zm);    ///< Zone Management
	prep_bits(sqe, 13, 9, 1, a.zsaso);  ///< Zone Send Action Specific Option
	prep_bits(sqe, 13, 8, 1, a.selall); ///< Select All
	prep_bits(sqe, 13, 0, 8, a.zsa);    ///< Zone Send Action

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_zns_zone_management_receive_args) == 24,
		    "The record has padding")

int
xnvme_cmd_prep_zns_zone_management_receive(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_zns_zone_management_receive_args *args)
{
	static const uint16_t rsvd[] = {23, 1};
	struct xnvme_cmd_prep_zns_zone_management_receive_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.zraspf >> 1) { ///< Zone Receive Action Specific Features: 1 bit
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x7A;

	prep_bits(sqe, 1, 0, 32, a.nsid);    ///< Namespace Identifier
	prep_bits(sqe, 10, 0, 64, a.slba);   ///< Starting LBA
	prep_bits(sqe, 12, 0, 32, a.ndw);    ///< Number of Dwords
	prep_bits(sqe, 13, 16, 1, a.zraspf); ///< Zone Receive Action Specific Features
	prep_bits(sqe, 13, 8, 8, a.zras);    ///< Zone Receive Action Specific
	prep_bits(sqe, 13, 0, 8, a.zra);     ///< Zone Receive Action

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_zns_zone_append_args) == 48,
		    "The record has padding")

int
xnvme_cmd_prep_zns_zone_append(union xnvme_spec_sqe *sqe,
			       const struct xnvme_cmd_prep_zns_zone_append_args *args)
{
	static const uint16_t rsvd[] = {23, 1, 34, 2};
	struct xnvme_cmd_prep_zns_zone_append_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 2);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.lbtu >> 48) { ///< Logical Block Tags Upper: 48 bits
		return -EINVAL;
	}
	if (a.lr >> 1) { ///< Limited Retry: 1 bit
		return -EINVAL;
	}
	if (a.fua >> 1) { ///< Force Unit Access: 1 bit
		return -EINVAL;
	}
	if (a.prinfo >> 4) { ///< Protection Information: 4 bits
		return -EINVAL;
	}
	if (a.piremap >> 1) { ///< Protection Information Remap: 1 bit
		return -EINVAL;
	}
	if (a.stc >> 1) { ///< Storage Tag Check: 1 bit
		return -EINVAL;
	}
	if (a.dtype >> 4) { ///< Directive Type: 4 bits
		return -EINVAL;
	}
	if (a.cetype >> 4) { ///< Command Extension Type: 4 bits
		return -EINVAL;
	}
	if (a.cetype == 0 && a.cev) { ///< Command Extension Value: only if CETYPE is non-zero
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x7D;

	prep_bits(sqe, 1, 0, 32, a.nsid);     ///< Namespace Identifier
	prep_bits(sqe, 2, 0, 48, a.lbtu);     ///< Logical Block Tags Upper
	prep_bits(sqe, 10, 0, 64, a.zslba);   ///< Zone Start Logical Block Address
	prep_bits(sqe, 12, 31, 1, a.lr);      ///< Limited Retry
	prep_bits(sqe, 12, 30, 1, a.fua);     ///< Force Unit Access
	prep_bits(sqe, 12, 26, 4, a.prinfo);  ///< Protection Information
	prep_bits(sqe, 12, 25, 1, a.piremap); ///< Protection Information Remap
	prep_bits(sqe, 12, 24, 1, a.stc);     ///< Storage Tag Check
	prep_bits(sqe, 12, 20, 4, a.dtype);   ///< Directive Type
	prep_bits(sqe, 12, 16, 4, a.cetype);  ///< Command Extension Type
	prep_bits(sqe, 12, 0, 16, a.nlb);     ///< Number of Logical Blocks
	prep_bits(sqe, 13, 16, 16, a.dspec);  ///< Directive Specific
	if (a.cetype != 0) {
		prep_bits(sqe, 13, 0, 16, a.cev); ///< Command Extension Value
	}
	prep_bits(sqe, 14, 0, 32, a.lbtl);   ///< Logical Block Tags Lower
	prep_bits(sqe, 15, 16, 16, a.lbatm); ///< Logical Block Application Tag Mask
	prep_bits(sqe, 15, 0, 16, a.lbat);   ///< Logical Block Application Tag

	return 0;
}

/*
 * Key Value Command Set
 */

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_kv_store_args) == 40, "The record has padding")

int
xnvme_cmd_prep_kv_store(union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_kv_store_args *args)
{
	static const uint16_t rsvd[] = {15, 1, 34, 6};
	struct xnvme_cmd_prep_kv_store_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 2);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.cetype >> 4) { ///< Command Extension Type: 4 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x01;

	prep_bits(sqe, 1, 0, 32, a.nsid);                      ///< Namespace Identifier
	prep_bits(sqe, 2, 0, 64, prep_le64(&a.kv_key[0], 8));  ///< KV Key[63:00]
	prep_bits(sqe, 10, 0, 32, a.vs);                       ///< Value Size
	prep_bits(sqe, 11, 8, 8, a.so);                        ///< Store Option
	prep_bits(sqe, 11, 0, 8, a.kl);                        ///< Key Length
	prep_bits(sqe, 13, 16, 4, a.cetype);                   ///< Command Extension Type
	prep_bits(sqe, 13, 0, 16, a.cev);                      ///< Command Extension Value
	prep_bits(sqe, 14, 0, 64, prep_le64(&a.kv_key[8], 8)); ///< KV Key[127:64]

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_kv_retrieve_args) == 40, "The record has padding")

int
xnvme_cmd_prep_kv_retrieve(union xnvme_spec_sqe *sqe,
			   const struct xnvme_cmd_prep_kv_retrieve_args *args)
{
	static const uint16_t rsvd[] = {15, 1, 34, 6};
	struct xnvme_cmd_prep_kv_retrieve_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 2);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	if (a.cetype >> 4) { ///< Command Extension Type: 4 bits
		return -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x02;

	prep_bits(sqe, 1, 0, 32, a.nsid);                      ///< Namespace Identifier
	prep_bits(sqe, 2, 0, 64, prep_le64(&a.kv_key[0], 8));  ///< KV Key[63:00]
	prep_bits(sqe, 10, 0, 32, a.hbs);                      ///< Host Buffer Size
	prep_bits(sqe, 11, 8, 8, a.ro);                        ///< Retrieve Option
	prep_bits(sqe, 11, 0, 8, a.kl);                        ///< Key Length
	prep_bits(sqe, 13, 16, 4, a.cetype);                   ///< Command Extension Type
	prep_bits(sqe, 13, 0, 16, a.cev);                      ///< Command Extension Value
	prep_bits(sqe, 14, 0, 64, prep_le64(&a.kv_key[8], 8)); ///< KV Key[127:64]

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_kv_list_args) == 32, "The record has padding")

int
xnvme_cmd_prep_kv_list(union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_kv_list_args *args)
{
	static const uint16_t rsvd[] = {29, 3};
	struct xnvme_cmd_prep_kv_list_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x06;

	prep_bits(sqe, 1, 0, 32, a.nsid);                      ///< Namespace Identifier
	prep_bits(sqe, 2, 0, 64, prep_le64(&a.kv_key[0], 8));  ///< KV Key[63:00]
	prep_bits(sqe, 10, 0, 32, a.hbs);                      ///< Host Buffer Size
	prep_bits(sqe, 11, 0, 8, a.kl);                        ///< Key Length
	prep_bits(sqe, 14, 0, 64, prep_le64(&a.kv_key[8], 8)); ///< KV Key[127:64]

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_kv_delete_args) == 32, "The record has padding")

int
xnvme_cmd_prep_kv_delete(union xnvme_spec_sqe *sqe,
			 const struct xnvme_cmd_prep_kv_delete_args *args)
{
	static const uint16_t rsvd[] = {25, 7};
	struct xnvme_cmd_prep_kv_delete_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x10;

	prep_bits(sqe, 1, 0, 32, a.nsid);                      ///< Namespace Identifier
	prep_bits(sqe, 2, 0, 64, prep_le64(&a.kv_key[0], 8));  ///< KV Key[63:00]
	prep_bits(sqe, 11, 0, 8, a.kl);                        ///< Key Length
	prep_bits(sqe, 14, 0, 64, prep_le64(&a.kv_key[8], 8)); ///< KV Key[127:64]

	return 0;
}

XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_kv_exist_args) == 32, "The record has padding")

int
xnvme_cmd_prep_kv_exist(union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_kv_exist_args *args)
{
	static const uint16_t rsvd[] = {25, 7};
	struct xnvme_cmd_prep_kv_exist_args a = {0};
	int err;

	err = prep_args(&a, sizeof(a), args, rsvd, 1);
	if (err || !sqe) {
		return err ? err : -EINVAL;
	}

	memset(sqe, 0, sizeof(*sqe));
	sqe->cdw[0] = 0x14;

	prep_bits(sqe, 1, 0, 32, a.nsid);                      ///< Namespace Identifier
	prep_bits(sqe, 2, 0, 64, prep_le64(&a.kv_key[0], 8));  ///< KV Key[63:00]
	prep_bits(sqe, 11, 0, 8, a.kl);                        ///< Key Length
	prep_bits(sqe, 14, 0, 64, prep_le64(&a.kv_key[8], 8)); ///< KV Key[127:64]

	return 0;
}
