/**
 * Prepare the commands of the Admin Command Set from records of their fields
 *
 * Written from the data model of senfd 0.14.0, by toolbox/xnvme_cmd_prep.py;
 * each field is named and placed as the figure it cites gives it. Do not edit:
 * see the contributor docs on producing the command preps.
 *
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_cmd_prep_adm.h
 */

/** Defined where the preps of the Admin Command Set are declared; a program tests it with #ifdef
 */
#define XNVME_CMD_PREP_ADM 1

/**
 * The arguments of xnvme_cmd_prep_adm_delete_io_submission_queue(): the fields of Delete I/O
 * Submission Queue
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.3.4, Figure 582, page 528, PDF page 558
 */
struct xnvme_cmd_prep_adm_delete_io_submission_queue_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint16_t qid;         ///< Queue Identifier, CDW10 bits 15:00
	uint8_t rsvd6[2];     ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Delete I/O Submission Queue, opcode 00h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_delete_io_submission_queue_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_delete_io_submission_queue(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_delete_io_submission_queue_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_create_io_submission_queue(): the fields of Create I/O
 * Submission Queue
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.3.2, Figure 576, page 526, PDF page 555
 * - CDW11: Base 2.4, 5.3.2, Figure 577, page 526, PDF page 555
 * - CDW12: Base 2.4, 5.3.2, Figure 578, page 527, PDF page 556
 */
struct xnvme_cmd_prep_adm_create_io_submission_queue_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint16_t qsize;       ///< Queue Size, CDW10 bits 31:16
	uint16_t qid;         ///< Queue Identifier, CDW10 bits 15:00
	uint16_t cqid;        ///< Completion Queue Identifier, CDW11 bits 31:16
	uint8_t qprio;        ///< Queue Priority, CDW11 bits 02:01
	uint8_t pc;           ///< Physically Contiguous, CDW11 bit 0
	uint16_t nvmsetid;    ///< NVM Set Identifier, CDW12 bits 15:00
	uint8_t rsvd14[2];    ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Create I/O Submission Queue, opcode 01h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_create_io_submission_queue_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_create_io_submission_queue(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_create_io_submission_queue_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_get_log_page(): the fields of Get Log Page
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.13, Figure 204, page 213, PDF page 239
 * - CDW11: Base 2.4, 5.2.13, Figure 205, page 213, PDF page 240
 * - CDW12: Base 2.4, 5.2.13, Figure 206, page 214, PDF page 240
 * - CDW13: Base 2.4, 5.2.13, Figure 207, page 214, PDF page 240
 * - CDW14: Base 2.4, 5.2.13, Figure 208, page 214, PDF page 240
 */
struct xnvme_cmd_prep_adm_get_log_page_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t nsid;        ///< Namespace Identifier, CDW1
	uint8_t rae;          ///< Retain Asynchronous Event, CDW10 bit 15
	uint8_t lsp;          ///< Log Specific Parameter, CDW10 bits 14:08
	uint8_t lid;          ///< Log Page Identifier, CDW10 bits 07:00
	uint8_t rsvd11[1];    ///< Reserved; must be zero
	uint16_t lsi;         ///< Log Specific Identifier, CDW11 bits 31:16
	uint8_t csi;          ///< Command Set Identifier, CDW14 bits 31:24
	uint8_t ot;           ///< Offset Type, CDW14 bit 23
	uint8_t uidx;         ///< UUID Index, CDW14 bits 06:00
	uint8_t rsvd17[3];    ///< Reserved; must be zero
	uint32_t numd;        ///< Number of Dwords, NUMDL and NUMDU
	uint64_t lpo;         ///< Log Page Offset, LPOL and LPOU
};

/**
 * Prepare 'sqe' as the command Get Log Page, opcode 02h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_get_log_page_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_get_log_page(union xnvme_spec_sqe *sqe,
				const struct xnvme_cmd_prep_adm_get_log_page_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_delete_io_completion_queue(): the fields of Delete I/O
 * Completion Queue
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.3.3, Figure 580, page 528, PDF page 557
 */
struct xnvme_cmd_prep_adm_delete_io_completion_queue_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint16_t qid;         ///< Queue Identifier, CDW10 bits 15:00
	uint8_t rsvd6[2];     ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Delete I/O Completion Queue, opcode 04h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_delete_io_completion_queue_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_delete_io_completion_queue(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_delete_io_completion_queue_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_create_io_completion_queue(): the fields of Create I/O
 * Completion Queue
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.3.1, Figure 572, page 524, PDF page 554
 * - CDW11: Base 2.4, 5.3.1, Figure 573, page 524, PDF page 554
 * - CDW11: NVMe over PCIe Transport 1.4, 3.2, Figure 7, page 11, PDF page 11
 */
struct xnvme_cmd_prep_adm_create_io_completion_queue_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint16_t qsize;       ///< Queue Size, CDW10 bits 31:16
	uint16_t qid;         ///< Queue Identifier, CDW10 bits 15:00
	uint16_t iv;          ///< Interrupt Vector, CDW11 bits 31:16
	uint8_t ien;          ///< Interrupts Enabled, CDW11 bit 1
	uint8_t pc;           ///< Physically Contiguous, CDW11 bit 0
	uint8_t rsvd12[4];    ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Create I/O Completion Queue, opcode 05h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_create_io_completion_queue_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_create_io_completion_queue(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_create_io_completion_queue_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_identify(): the fields of Identify
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.14.1, Figure 333, page 335, PDF page 363
 * - CDW11: Base 2.4, 5.2.14.1, Figure 334, page 335, PDF page 363
 * - CDW14: Base 2.4, 5.2.14.1, Figure 335, page 335, PDF page 363
 */
struct xnvme_cmd_prep_adm_identify_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t nsid;        ///< Namespace Identifier, CDW1
	uint16_t cntid;       ///< Controller Identifier, CDW10 bits 31:16
	uint8_t cns;          ///< Controller or Namespace Structure, CDW10 bits 07:00
	uint8_t csi;          ///< Command Set Identifier, CDW11 bits 31:24
	uint16_t cnssid;      ///< CNS Specific Identifier, CDW11 bits 15:00
	uint8_t uidx;         ///< UUID Index, CDW14 bits 06:00
	uint8_t rsvd15[1];    ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Identify, opcode 06h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_identify_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_identify(union xnvme_spec_sqe *sqe,
			    const struct xnvme_cmd_prep_adm_identify_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_abort(): the fields of Abort
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.1, Figure 147, page 182, PDF page 208
 */
struct xnvme_cmd_prep_adm_abort_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint16_t cid;         ///< Command Identifier, CDW10 bits 31:16
	uint16_t sqid;        ///< Submission Queue Identifier, CDW10 bits 15:00
};

/**
 * Prepare 'sqe' as the command Abort, opcode 08h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_abort_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_abort(union xnvme_spec_sqe *sqe,
			 const struct xnvme_cmd_prep_adm_abort_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_set_features(): the fields of Set Features
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.30, Figure 464, page 454, PDF page 483
 * - CDW14: Base 2.4, 5.2.30, Figure 465, page 455, PDF page 483
 */
struct xnvme_cmd_prep_adm_set_features_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t nsid;        ///< Namespace Identifier, CDW1
	uint8_t sv;           ///< Save, CDW10 bit 31
	uint8_t fid;          ///< Feature Identifier, CDW10 bits 07:00
	uint8_t uidx;         ///< UUID Index, CDW14 bits 06:00
	uint8_t rsvd11[1];    ///< Reserved; must be zero
	uint32_t cdw11;       ///< CDW11, as the feature or operation describes it
	uint32_t cdw12;       ///< CDW12, as the feature or operation describes it
	uint32_t cdw13;       ///< CDW13, as the feature or operation describes it
	uint32_t cdw15;       ///< CDW15, as the feature or operation describes it
	uint8_t rsvd28[4];    ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Set Features, opcode 09h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_set_features_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_set_features(union xnvme_spec_sqe *sqe,
				const struct xnvme_cmd_prep_adm_set_features_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_get_features(): the fields of Get Features
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.12, Figure 198, page 209, PDF page 235
 * - CDW14: Base 2.4, 5.2.12, Figure 199, page 210, PDF page 236
 */
struct xnvme_cmd_prep_adm_get_features_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t nsid;        ///< Namespace Identifier, CDW1
	uint8_t sel;          ///< Select, CDW10 bits 10:08
	uint8_t fid;          ///< Feature Identifier, CDW10 bits 07:00
	uint8_t uidx;         ///< UUID Index, CDW14 bits 06:00
	uint8_t rsvd11[1];    ///< Reserved; must be zero
	uint32_t cdw11;       ///< CDW11, as the feature or operation describes it
};

/**
 * Prepare 'sqe' as the command Get Features, opcode 0Ah, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_get_features_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_get_features(union xnvme_spec_sqe *sqe,
				const struct xnvme_cmd_prep_adm_get_features_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_asynchronous_event_request(): the fields of Asynchronous
 * Event Request
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 */
struct xnvme_cmd_prep_adm_asynchronous_event_request_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint8_t rsvd4[4];     ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Asynchronous Event Request, opcode 0Ch, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_asynchronous_event_request_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_asynchronous_event_request(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_asynchronous_event_request_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_namespace_management(): the fields of Namespace Management
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.25, Figure 446, page 444, PDF page 472
 * - CDW11: Base 2.4, 5.2.25, Figure 447, page 445, PDF page 473
 */
struct xnvme_cmd_prep_adm_namespace_management_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t nsid;        ///< Namespace Identifier, CDW1
	uint8_t sel;          ///< Select, CDW10 bits 03:00
	uint8_t csi;          ///< Command Set Identifier, CDW11 bits 31:24
	uint8_t rsvd10[6];    ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Namespace Management, opcode 0Dh, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_namespace_management_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_namespace_management(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_namespace_management_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_firmware_commit(): the fields of Firmware Commit
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.9, Figure 187, page 203, PDF page 229
 */
struct xnvme_cmd_prep_adm_firmware_commit_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint8_t bpid;         ///< Boot Partition ID, CDW10 bit 31
	uint8_t ca;           ///< Commit Action, CDW10 bits 05:03
	uint8_t fs;           ///< Firmware Slot, CDW10 bits 02:00
	uint8_t rsvd7[1];     ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Firmware Commit, opcode 10h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_firmware_commit_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_firmware_commit(union xnvme_spec_sqe *sqe,
				   const struct xnvme_cmd_prep_adm_firmware_commit_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_firmware_image_download(): the fields of Firmware Image
 * Download
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.10, Figure 191, page 205, PDF page 231
 * - CDW11: Base 2.4, 5.2.10, Figure 192, page 206, PDF page 232
 */
struct xnvme_cmd_prep_adm_firmware_image_download_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t numd;        ///< Number of Dwords, CDW10 bits 31:00
	uint32_t ofst;        ///< Offset, CDW11 bits 31:00
	uint8_t rsvd12[4];    ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Firmware Image Download, opcode 11h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_firmware_image_download_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_firmware_image_download(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_firmware_image_download_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_device_self_test(): the fields of Device Self-test
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.6, Figure 177, page 199, PDF page 225
 * - CDW15: Base 2.4, 5.2.6, Figure 178, page 200, PDF page 226
 */
struct xnvme_cmd_prep_adm_device_self_test_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t nsid;        ///< Namespace Identifier, CDW1
	uint8_t stc;          ///< Self-test Code, CDW10 bits 03:00
	uint8_t rsvd9[3];     ///< Reserved; must be zero
	uint32_t dstp;        ///< Device Self-test Parameter, CDW15 bits 31:00
};

/**
 * Prepare 'sqe' as the command Device Self-test, opcode 14h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_device_self_test_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_device_self_test(union xnvme_spec_sqe *sqe,
				    const struct xnvme_cmd_prep_adm_device_self_test_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_namespace_attachment(): the fields of Namespace Attachment
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.24, Figure 443, page 443, PDF page 471
 */
struct xnvme_cmd_prep_adm_namespace_attachment_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t nsid;        ///< Namespace Identifier, CDW1
	uint8_t sel;          ///< Select, CDW10 bits 03:00
	uint8_t rsvd9[7];     ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Namespace Attachment, opcode 15h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_namespace_attachment_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_namespace_attachment(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_namespace_attachment_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_keep_alive(): the fields of Keep Alive
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 */
struct xnvme_cmd_prep_adm_keep_alive_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint8_t rsvd4[4];     ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Keep Alive, opcode 18h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_keep_alive_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_keep_alive(union xnvme_spec_sqe *sqe,
			      const struct xnvme_cmd_prep_adm_keep_alive_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_directive_send(): the fields of Directive Send
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.8, Figure 185, page 202, PDF page 228
 * - CDW11: Base 2.4, 5.2.8, Figure 186, page 202, PDF page 228
 * - CDW12, of Enable Directive: Base 2.4, 8.1.9.2.2.1, Figure 706, page 616, PDF page 646
 */
struct xnvme_cmd_prep_adm_directive_send_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t nsid;        ///< Namespace Identifier, CDW1
	uint32_t numd;        ///< Number of Dwords, CDW10 bits 31:00
	uint16_t dspec;       ///< Directive Specific, CDW11 bits 31:16
	uint8_t dtype;        ///< Directive Type, CDW11 bits 15:08
	uint8_t doper;        ///< Directive Operation, CDW11 bits 07:00
	uint32_t cdw12;       ///< CDW12, as the feature or operation describes it
	uint32_t cdw13;       ///< CDW13, as the feature or operation describes it
	uint32_t cdw14;       ///< CDW14, as the feature or operation describes it
	uint32_t cdw15;       ///< CDW15, as the feature or operation describes it
};

/**
 * Prepare 'sqe' as the command Directive Send, opcode 19h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_directive_send_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_directive_send(union xnvme_spec_sqe *sqe,
				  const struct xnvme_cmd_prep_adm_directive_send_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_directive_receive(): the fields of Directive Receive
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.7, Figure 182, page 201, PDF page 227
 * - CDW11: Base 2.4, 5.2.7, Figure 183, page 201, PDF page 227
 * - CDW12, of Allocate Resources: Base 2.4, 8.1.9.3.1.3, Figure 714, page 622, PDF page 652
 */
struct xnvme_cmd_prep_adm_directive_receive_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t nsid;        ///< Namespace Identifier, CDW1
	uint32_t numd;        ///< Number of Dwords, CDW10 bits 31:00
	uint16_t dspec;       ///< Directive Specific, CDW11 bits 31:16
	uint8_t dtype;        ///< Directive Type, CDW11 bits 15:08
	uint8_t doper;        ///< Directive Operation, CDW11 bits 07:00
	uint32_t cdw12;       ///< CDW12, as the feature or operation describes it
	uint32_t cdw13;       ///< CDW13, as the feature or operation describes it
	uint32_t cdw14;       ///< CDW14, as the feature or operation describes it
	uint32_t cdw15;       ///< CDW15, as the feature or operation describes it
};

/**
 * Prepare 'sqe' as the command Directive Receive, opcode 1Ah, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_directive_receive_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_directive_receive(union xnvme_spec_sqe *sqe,
				     const struct xnvme_cmd_prep_adm_directive_receive_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_virtualization_management(): the fields of Virtualization
 * Management
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.3.6, Figure 587, page 530, PDF page 559
 * - CDW11: Base 2.4, 5.3.6, Figure 588, page 531, PDF page 560
 */
struct xnvme_cmd_prep_adm_virtualization_management_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint16_t cntlid;      ///< Controller Identifier, CDW10 bits 31:16
	uint8_t rt;           ///< Resource Type, CDW10 bits 10:08
	uint8_t act;          ///< Action, CDW10 bits 03:00
	uint16_t nr;          ///< Number of Controller Resources, CDW11 bits 15:00
	uint8_t rsvd10[6];    ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Virtualization Management, opcode 1Ch, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_virtualization_management_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_virtualization_management(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_virtualization_management_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_nvme_mi_send(): the fields of NVMe-MI Send
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 */
struct xnvme_cmd_prep_adm_nvme_mi_send_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint8_t rsvd4[4];     ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command NVMe-MI Send, opcode 1Dh, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_nvme_mi_send_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_nvme_mi_send(union xnvme_spec_sqe *sqe,
				const struct xnvme_cmd_prep_adm_nvme_mi_send_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_nvme_mi_receive(): the fields of NVMe-MI Receive
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 */
struct xnvme_cmd_prep_adm_nvme_mi_receive_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint8_t rsvd4[4];     ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command NVMe-MI Receive, opcode 1Eh, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_nvme_mi_receive_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_nvme_mi_receive(union xnvme_spec_sqe *sqe,
				   const struct xnvme_cmd_prep_adm_nvme_mi_receive_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_capacity_management(): the fields of Capacity Management
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.3, Figure 162, page 191, PDF page 217
 * - CDW11: Base 2.4, 5.2.3, Figure 163, page 192, PDF page 218
 * - CDW12: Base 2.4, 5.2.3, Figure 164, page 192, PDF page 218
 */
struct xnvme_cmd_prep_adm_capacity_management_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint16_t elid;        ///< Element Identifier, CDW10 bits 31:16
	uint8_t oper;         ///< Operation, CDW10 bits 03:00
	uint8_t rsvd7[1];     ///< Reserved; must be zero
	uint64_t cap;         ///< Capacity, CAPL and CAPU
};

/**
 * Prepare 'sqe' as the command Capacity Management, opcode 20h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_capacity_management_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_capacity_management(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_adm_capacity_management_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_discovery_information_management(): the fields of Discovery
 * Information Management
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.4.2, Figure 597, page 536, PDF page 565
 */
struct xnvme_cmd_prep_adm_discovery_information_management_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint8_t tas;          ///< Task, CDW10 bits 03:00
	uint8_t rsvd5[3];     ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Discovery Information Management, opcode 21h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see
 * ::xnvme_cmd_prep_adm_discovery_information_management_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_discovery_information_management(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_discovery_information_management_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_fabric_zoning_receive(): the fields of Fabric Zoning Receive
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.4.4, Figure 607, page 542, PDF page 572
 * - CDW11: Base 2.4, 5.4.4, Figure 608, page 543, PDF page 572
 * - CDW12: Base 2.4, 5.4.4, Figure 609, page 543, PDF page 572
 */
struct xnvme_cmd_prep_adm_fabric_zoning_receive_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t zdk;         ///< Zoning Data Key, CDW10 bits 31:00
	uint32_t zdo;         ///< Zoning Data Offset, CDW11 bits 31:00
	uint8_t zdkc;         ///< ZDK Context, CDW12 bit 28
	uint8_t rsvd13[3];    ///< Reserved; must be zero
	uint32_t numd;        ///< Number of Dwords, CDW12 bits 27:00
	uint8_t rsvd20[4];    ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Fabric Zoning Receive, opcode 22h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_fabric_zoning_receive_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_fabric_zoning_receive(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_fabric_zoning_receive_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_lockdown(): the fields of Lockdown
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.16, Figure 365, page 403, PDF page 431
 * - CDW14: Base 2.4, 5.2.16, Figure 366, page 405, PDF page 433
 */
struct xnvme_cmd_prep_adm_lockdown_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint8_t csel;         ///< Controller Select, CDW10 bits 19:16
	uint8_t ofi;          ///< Opcode or Feature Identifier, CDW10 bits 15:08
	uint8_t ifc;          ///< Interface, CDW10 bits 06:05
	uint8_t prhbt;        ///< Prohibit, CDW10 bit 4
	uint8_t scp;          ///< Scope, CDW10 bits 03:00
	uint8_t rsvd9[1];     ///< Reserved; must be zero
	uint16_t css;         ///< Controller Select Specific, CDW14 bits 31:16
	uint8_t uidx;         ///< UUID Index, CDW14 bits 06:00
	uint8_t rsvd13[3];    ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Lockdown, opcode 24h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_lockdown_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_lockdown(union xnvme_spec_sqe *sqe,
			    const struct xnvme_cmd_prep_adm_lockdown_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_fabric_zoning_lookup(): the fields of Fabric Zoning Lookup
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 */
struct xnvme_cmd_prep_adm_fabric_zoning_lookup_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint8_t rsvd4[4];     ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Fabric Zoning Lookup, opcode 25h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_fabric_zoning_lookup_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_fabric_zoning_lookup(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_fabric_zoning_lookup_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_clear_exported_nvm_resource_configuration(): the fields of
 * Clear Exported NVM Resource Configuration
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 */
struct xnvme_cmd_prep_adm_clear_exported_nvm_resource_configuration_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint8_t rsvd4[4];     ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Clear Exported NVM Resource Configuration, opcode 28h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see
 * ::xnvme_cmd_prep_adm_clear_exported_nvm_resource_configuration_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_clear_exported_nvm_resource_configuration(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_clear_exported_nvm_resource_configuration_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_fabric_zoning_send(): the fields of Fabric Zoning Send
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.4.5, Figure 613, page 544, PDF page 573
 * - CDW11: Base 2.4, 5.4.5, Figure 614, page 544, PDF page 573
 * - CDW12: Base 2.4, 5.4.5, Figure 615, page 544, PDF page 573
 */
struct xnvme_cmd_prep_adm_fabric_zoning_send_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t zdk;         ///< Zoning Data Key, CDW10 bits 31:00
	uint32_t zdo;         ///< Zoning Data Offset, CDW11 bits 31:00
	uint8_t lf;           ///< Last Fragment, CDW12 bit 31
	uint8_t zdkc;         ///< ZDK Context, CDW12 bit 28
	uint8_t rsvd14[2];    ///< Reserved; must be zero
	uint32_t numd;        ///< Number of Dwords, CDW12 bits 27:00
	uint8_t rsvd20[4];    ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Fabric Zoning Send, opcode 29h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_fabric_zoning_send_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_fabric_zoning_send(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_adm_fabric_zoning_send_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_manage_exported_nvm_subsystem_receive(): the fields of
 * Manage Exported NVM Subsystem Receive
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.18, Figure 375, page 410, PDF page 438
 * - CDW12: Base 2.4, 5.2.18, Figure 376, page 410, PDF page 438
 * - CDW13: Base 2.4, 5.2.18, Figure 377, page 411, PDF page 439
 * - CDW14: Base 2.4, 5.2.18, Figure 378, page 411, PDF page 439
 * - CDW15: Base 2.4, 5.2.18, Figure 379, page 411, PDF page 439
 * - CDW11, of Create Exported NVM Subsystem: Base 2.4, 5.2.18.1.1, Figure 380, page 411, PDF
 * page 439
 */
struct xnvme_cmd_prep_adm_manage_exported_nvm_subsystem_receive_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint16_t mose;        ///< Management Operation Specific Extended, CDW10 bits 31:16
	uint8_t mos;          ///< Management Operation Specific, CDW10 bits 15:08
	uint8_t sel;          ///< Select, CDW10 bits 07:00
	uint16_t mosi;        ///< Management Operation Specific Identifier, CDW14 bits 31:16
	uint8_t rsvd10[2];    ///< Reserved; must be zero
	uint32_t numdl;       ///< Number of Dwords, CDW15 bits 31:00
	uint32_t cdw11;       ///< CDW11, operation-specific, as the operation describes it
	uint8_t rsvd20[4];    ///< Reserved; must be zero
	uint64_t offset;      ///< Offset, OL and OU
};

/**
 * Prepare 'sqe' as the command Manage Exported NVM Subsystem Receive, opcode 2Ah, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see
 * ::xnvme_cmd_prep_adm_manage_exported_nvm_subsystem_receive_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_manage_exported_nvm_subsystem_receive(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_manage_exported_nvm_subsystem_receive_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_manage_exported_nvm_subsystem_send(): the fields of Manage
 * Exported NVM Subsystem Send
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.19, Figure 397, page 418, PDF page 446
 * - CDW14: Base 2.4, 5.2.19, Figure 398, page 418, PDF page 447
 * - CDW15: Base 2.4, 5.2.19.1.6, Figure 406, page 424, PDF page 452
 */
struct xnvme_cmd_prep_adm_manage_exported_nvm_subsystem_send_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint16_t mose;        ///< Management Operation Specific Extended, CDW10 bits 31:16
	uint8_t mos;          ///< Management Operation Specific, CDW10 bits 15:08
	uint8_t sel;          ///< Select, CDW10 bits 07:00
	uint16_t esubid;      ///< Exported NVM Subsystem Identifier, CDW14 bits 31:16
	uint8_t esubidv;      ///< Exported NVM Subsystem Identifier Valid, CDW14 bit 7
	uint8_t rsvd11[1];    ///< Reserved; must be zero
	uint32_t numd;        ///< Number of Dwords, CDW15 bits 31:00
};

/**
 * Prepare 'sqe' as the command Manage Exported NVM Subsystem Send, opcode 2Dh, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see
 * ::xnvme_cmd_prep_adm_manage_exported_nvm_subsystem_send_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_manage_exported_nvm_subsystem_send(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_manage_exported_nvm_subsystem_send_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_manage_exported_namespace(): the fields of Manage Exported
 * Namespace
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.17, Figure 369, page 406, PDF page 434
 * - CDW14: Base 2.4, 5.2.17, Figure 370, page 407, PDF page 435
 */
struct xnvme_cmd_prep_adm_manage_exported_namespace_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t nsid;        ///< Namespace Identifier, CDW1
	uint8_t mos;          ///< Management Operation Specific, CDW10 bits 15:08
	uint8_t sel;          ///< Select, CDW10 bits 07:00
	uint16_t esubid;      ///< Exported NVM Subsystem Identifier, CDW14 bits 31:16
	uint8_t esubidv;      ///< Exported NVM Subsystem Identifier Valid, CDW14 bit 7
	uint8_t rsvd13[3];    ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Manage Exported Namespace, opcode 31h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_manage_exported_namespace_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_manage_exported_namespace(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_manage_exported_namespace_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_manage_exported_port(): the fields of Manage Exported Port
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 */
struct xnvme_cmd_prep_adm_manage_exported_port_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint8_t rsvd4[4];     ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Manage Exported Port, opcode 35h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_manage_exported_port_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_manage_exported_port(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_manage_exported_port_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_cross_controller_reset(): the fields of Cross-Controller
 * Reset
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.4.1, Figure 591, page 533, PDF page 562
 * - CDW12: Base 2.4, 5.4.1, Figure 592, page 533, PDF page 562
 * - CDW13: Base 2.4, 5.4.1, Figure 593, page 533, PDF page 562
 */
struct xnvme_cmd_prep_adm_cross_controller_reset_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint8_t ciu;          ///< Controller Instance Uniquifer, CDW10 bits 23:16
	uint8_t rsvd5[1];     ///< Reserved; must be zero
	uint16_t icid;        ///< Impacted Controller ID, CDW10 bits 15:00
	uint64_t cirn;        ///< Controller Instance Random Number, CIRNL and CIRNU
};

/**
 * Prepare 'sqe' as the command Cross-Controller Reset, opcode 38h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_cross_controller_reset_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_cross_controller_reset(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_cross_controller_reset_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_send_discovery_log_page(): the fields of Send Discovery Log
 * Page
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.4.7, Figure 624, page 547, PDF page 576
 * - CDW11: Base 2.4, 5.4.7, Figure 625, page 547, PDF page 577
 * - CDW12: Base 2.4, 5.4.7, Figure 626, page 547, PDF page 577
 * - CDW13: Base 2.4, 5.4.7, Figure 627, page 548, PDF page 577
 */
struct xnvme_cmd_prep_adm_send_discovery_log_page_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint8_t rlps;         ///< Requested Log Page Status, CDW10 bits 31:30
	uint8_t sct;          ///< Status Code Type, CDW10 bits 27:25
	uint8_t sc;           ///< Status Code, CDW10 bits 24:17
	uint8_t tlsp;         ///< Transferred Log Specific Parameter, CDW10 bits 14:08
	uint8_t tlid;         ///< Transferred Log Page Identifier, CDW10 bits 07:00
	uint8_t rsvd9[3];     ///< Reserved; must be zero
	uint32_t ndws;        ///< Number of Dwords, CDW11 bits 31:00
	uint64_t tlpo;        ///< Transferred Log Page Offset, TLPOL and TLPOU
};

/**
 * Prepare 'sqe' as the command Send Discovery Log Page, opcode 39h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_send_discovery_log_page_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_send_discovery_log_page(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_send_discovery_log_page_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_track_send(): the fields of Track Send
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.32, Figure 561, page 519, PDF page 549
 * - CDW11, of Log User Data Changes: Base 2.4, 5.2.32.1.1, Figure 563, page 520, PDF page 549
 * - CDW11, of Track Memory Changes: Base 2.4, 5.2.32.1.2, Figure 565, page 521, PDF page 551
 */
struct xnvme_cmd_prep_adm_track_send_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint16_t mos;         ///< Management Operation Specific, CDW10 bits 31:16
	uint8_t sel;          ///< Select, CDW10 bits 07:00
	uint8_t rsvd7[1];     ///< Reserved; must be zero
	uint32_t cdw11;       ///< CDW11, operation-specific, as the operation describes it
	uint8_t rsvd12[4];    ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Track Send, opcode 3Dh, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_track_send_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_track_send(union xnvme_spec_sqe *sqe,
			      const struct xnvme_cmd_prep_adm_track_send_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_track_receive(): the fields of Track Receive
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.31, Figure 555, page 517, PDF page 546
 * - CDW12: Base 2.4, 5.2.31, Figure 556, page 517, PDF page 546
 * - CDW11, of Tracked Memory Changes: Base 2.4, 5.2.31.1.1, Figure 557, page 517, PDF page 546
 */
struct xnvme_cmd_prep_adm_track_receive_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint8_t sel;          ///< Select, CDW10 bits 07:00
	uint8_t rsvd5[3];     ///< Reserved; must be zero
	uint32_t numdl;       ///< Number of Dwords, CDW12 bits 31:00
	uint32_t cdw11;       ///< CDW11, operation-specific, as the operation describes it
};

/**
 * Prepare 'sqe' as the command Track Receive, opcode 3Eh, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_track_receive_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_track_receive(union xnvme_spec_sqe *sqe,
				 const struct xnvme_cmd_prep_adm_track_receive_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_migration_send(): the fields of Migration Send
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.21, Figure 421, page 430, PDF page 458
 * - CDW14: Base 2.4, 5.2.21, Figure 422, page 430, PDF page 458
 * - CDW11, of Suspend: Base 2.4, 5.2.21.2.1, Figure 423, page 431, PDF page 459
 * - CDW11, of Resume: Base 2.4, 5.2.21.2.2, Figure 424, page 433, PDF page 461
 * - CDW11, of Set Controller State: Base 2.4, 5.2.21.2.3, Figure 427, page 434, PDF page 462
 * - CDW11, of Set Exported NVM Subsystem State: Base 2.4, 5.2.21.2.4, Figure 437, page 440, PDF
 * page 468
 * - CDW12, of Set Controller State: Base 2.4, 5.2.21.2.3, Figure 428, page 434, PDF page 462
 * - CDW12, of Set Exported NVM Subsystem State: Base 2.4, 5.2.21.2.4, Figure 438, page 440, PDF
 * page 469
 * - CDW13, of Set Controller State: Base 2.4, 5.2.21.2.3, Figure 429, page 434, PDF page 462
 * - CDW13, of Set Exported NVM Subsystem State: Base 2.4, 5.2.21.2.4, Figure 439, page 441, PDF
 * page 469
 * - CDW15, of Set Controller State: Base 2.4, 5.2.21.2.3, Figure 430, page 434, PDF page 462
 * - CDW15, of Set Exported NVM Subsystem State: Base 2.4, 5.2.21.2.4, Figure 440, page 441, PDF
 * page 469
 */
struct xnvme_cmd_prep_adm_migration_send_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint16_t mos;         ///< Management Operation Specific, CDW10 bits 31:16
	uint8_t sel;          ///< Select, CDW10 bits 07:00
	uint8_t uidx;         ///< UUID Index, CDW14 bits 06:00
	uint32_t cdw11;       ///< CDW11, operation-specific, as the operation describes it
	uint32_t cdw12;       ///< CDW12, operation-specific, as the operation describes it
	uint32_t cdw13;       ///< CDW13, operation-specific, as the operation describes it
	uint32_t cdw15;       ///< CDW15, operation-specific, as the operation describes it
};

/**
 * Prepare 'sqe' as the command Migration Send, opcode 41h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_migration_send_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_migration_send(union xnvme_spec_sqe *sqe,
				  const struct xnvme_cmd_prep_adm_migration_send_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_migration_receive(): the fields of Migration Receive
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.20, Figure 409, page 425, PDF page 453
 * - CDW12: Base 2.4, 5.2.20, Figure 410, page 425, PDF page 453
 * - CDW13: Base 2.4, 5.2.20, Figure 411, page 425, PDF page 453
 * - CDW14: Base 2.4, 5.2.20, Figure 412, page 426, PDF page 454
 * - CDW15: Base 2.4, 5.2.20, Figure 413, page 426, PDF page 454
 * - CDW11, of Get Controller State: Base 2.4, 5.2.20.1.1, Figure 415, page 427, PDF page 455
 * - CDW11, of Get Exported NVM Subsystem State: Base 2.4, 5.2.20.1.2, Figure 418, page 428, PDF
 * page 456
 */
struct xnvme_cmd_prep_adm_migration_receive_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint16_t mos;         ///< Management Operation Specific, CDW10 bits 31:16
	uint8_t sel;          ///< Select, CDW10 bits 07:00
	uint8_t uidx;         ///< UUID Index, CDW14 bits 06:00
	uint32_t numdl;       ///< Number of Dwords, CDW15 bits 31:00
	uint32_t cdw11;       ///< CDW11, operation-specific, as the operation describes it
	uint64_t offset;      ///< Offset, OL and OU
};

/**
 * Prepare 'sqe' as the command Migration Receive, opcode 42h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_migration_receive_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_migration_receive(union xnvme_spec_sqe *sqe,
				     const struct xnvme_cmd_prep_adm_migration_receive_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_controller_data_queue(): the fields of Controller Data Queue
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.5, Figure 167, page 196, PDF page 222
 * - CDW11, of Create Controller Data Queue: Base 2.4, 5.2.5.1.1, Figure 169, page 197, PDF page
 * 223
 * - CDW11, of Delete Controller Data Queue: Base 2.4, 5.2.5.1.2, Figure 173, page 198, PDF page
 * 224
 * - CDW12, of Create Controller Data Queue: Base 2.4, 5.2.5.1.1, Figure 170, page 197, PDF page
 * 223
 */
struct xnvme_cmd_prep_adm_controller_data_queue_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint16_t mos;         ///< Management Operation Specific, CDW10 bits 31:16
	uint8_t sel;          ///< Select, CDW10 bits 07:00
	uint8_t rsvd7[1];     ///< Reserved; must be zero
	uint32_t cdw11;       ///< CDW11, operation-specific, as the operation describes it
	uint32_t cdw12;       ///< CDW12, operation-specific, as the operation describes it
};

/**
 * Prepare 'sqe' as the command Controller Data Queue, opcode 45h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_controller_data_queue_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_controller_data_queue(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_controller_data_queue_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_doorbell_buffer_config(): the fields of Doorbell Buffer
 * Config
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 */
struct xnvme_cmd_prep_adm_doorbell_buffer_config_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint8_t rsvd4[4];     ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Doorbell Buffer Config, opcode 7Ch, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_doorbell_buffer_config_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_doorbell_buffer_config(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_doorbell_buffer_config_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_format_nvm(): the fields of Format NVM
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.11, Figure 195, page 208, PDF page 234
 * - CDW10: NVM Command Set 1.3, 4.1.2, Figure 91, page 63, PDF page 63
 */
struct xnvme_cmd_prep_adm_format_nvm_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t nsid;        ///< Namespace Identifier, CDW1
	uint8_t ses;          ///< Secure Erase Settings, CDW10 bits 11:09
	uint8_t pil;          ///< Protection Information Location, CDW10 bit 8
	uint8_t pi;           ///< Protection Information, CDW10 bits 07:05
	uint8_t mset;         ///< Metadata Settings, CDW10 bit 4
	uint8_t lbaf;         ///< LBA Format, LBAFL and LBAFU
	uint8_t rsvd13[3];    ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Format NVM, opcode 80h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_format_nvm_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_format_nvm(union xnvme_spec_sqe *sqe,
			      const struct xnvme_cmd_prep_adm_format_nvm_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_security_send(): the fields of Security Send
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.29, Figure 461, page 454, PDF page 482
 * - CDW11: Base 2.4, 5.2.29, Figure 462, page 454, PDF page 482
 */
struct xnvme_cmd_prep_adm_security_send_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t nsid;        ///< Namespace Identifier, CDW1
	uint8_t secp;         ///< Security Protocol, CDW10 bits 31:24
	uint8_t spsp1;        ///< SP Specific 1, CDW10 bits 23:16
	uint8_t spsp0;        ///< SP Specific 0, CDW10 bits 15:08
	uint8_t nssf;         ///< NVMe Security Specific Field, CDW10 bits 07:00
	uint32_t tl;          ///< Transfer Length, CDW11 bits 31:00
};

/**
 * Prepare 'sqe' as the command Security Send, opcode 81h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_security_send_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_security_send(union xnvme_spec_sqe *sqe,
				 const struct xnvme_cmd_prep_adm_security_send_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_security_receive(): the fields of Security Receive
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.28, Figure 457, page 452, PDF page 480
 * - CDW11: Base 2.4, 5.2.28, Figure 458, page 453, PDF page 481
 */
struct xnvme_cmd_prep_adm_security_receive_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t nsid;        ///< Namespace Identifier, CDW1
	uint8_t secp;         ///< Security Protocol, CDW10 bits 31:24
	uint8_t spsp1;        ///< SP Specific 1, CDW10 bits 23:16
	uint8_t spsp0;        ///< SP Specific 0, CDW10 bits 15:08
	uint8_t nssf;         ///< NVMe Security Specific Field, CDW10 bits 07:00
	uint32_t al;          ///< Allocation Length, CDW11 bits 31:00
};

/**
 * Prepare 'sqe' as the command Security Receive, opcode 82h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_security_receive_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_security_receive(union xnvme_spec_sqe *sqe,
				    const struct xnvme_cmd_prep_adm_security_receive_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_sanitize(): the fields of Sanitize
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.26, Figure 451, page 448, PDF page 476
 * - CDW11: Base 2.4, 5.2.26, Figure 452, page 449, PDF page 477
 */
struct xnvme_cmd_prep_adm_sanitize_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint8_t preq;         ///< Purge Required, CDW10 bit 11
	uint8_t emvs;         ///< Enter Media Verification State, CDW10 bit 10
	uint8_t ndas;         ///< No-Deallocate After Sanitize, CDW10 bit 9
	uint8_t oipbp;        ///< Overwrite Invert Pattern Between Passes, CDW10 bit 8
	uint8_t owpass;       ///< Overwrite Pass Count, CDW10 bits 07:04
	uint8_t ause;         ///< Allow Unrestricted Sanitize Exit, CDW10 bit 3
	uint8_t sanact;       ///< Sanitize Action, CDW10 bits 02:00
	uint8_t rsvd11[1];    ///< Reserved; must be zero
	uint32_t ovrpat;      ///< Overwrite Pattern, CDW11 bits 31:00
};

/**
 * Prepare 'sqe' as the command Sanitize, opcode 84h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_sanitize_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_sanitize(union xnvme_spec_sqe *sqe,
			    const struct xnvme_cmd_prep_adm_sanitize_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_load_program(): the fields of Load Program
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Computational Programs 1.3, 4.2.1, Figure 40, page 36, PDF page 36
 * - CDW11: Computational Programs 1.3, 4.2.1, Figure 41, page 36, PDF page 36
 * - CDW14: Computational Programs 1.3, 4.2.1, Figure 43, page 36, PDF page 36
 * - CDW15: Computational Programs 1.3, 4.2.1, Figure 44, page 37, PDF page 37
 */
struct xnvme_cmd_prep_adm_load_program_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t nsid;        ///< Namespace Identifier, CDW1
	uint8_t pit;          ///< Program Identifier Type, CDW10 bits 27:25
	uint8_t sel;          ///< Select, CDW10 bit 24
	uint8_t ptype;        ///< Program Type, CDW10 bits 23:16
	uint8_t rsvd11[1];    ///< Reserved; must be zero
	uint16_t pind;        ///< Program Index, CDW10 bits 15:00
	uint8_t rsvd14[2];    ///< Reserved; must be zero
	uint32_t psize;       ///< Program Size, CDW11 bits 31:00
	uint32_t numb;        ///< Number of Bytes, CDW14 bits 31:00
	uint32_t loff;        ///< Load Offset, CDW15 bits 31:00
	uint8_t rsvd28[4];    ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Load Program, opcode 85h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_load_program_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_load_program(union xnvme_spec_sqe *sqe,
				const struct xnvme_cmd_prep_adm_load_program_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_get_lba_status(): the fields of Get LBA Status
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10-11: NVM Command Set 1.3, 4.2.1, Figure 136, page 114, PDF page 114
 * - CDW12: NVM Command Set 1.3, 4.2.1, Figure 137, page 114, PDF page 114
 * - CDW13: NVM Command Set 1.3, 4.2.1, Figure 138, page 114, PDF page 114
 */
struct xnvme_cmd_prep_adm_get_lba_status_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t nsid;        ///< Namespace Identifier, CDW1
	uint64_t slba;        ///< Starting LBA, CDW10-11 bits 63:00
	uint32_t mndw;        ///< Maximum Number of Dwords, CDW12 bits 31:00
	uint8_t atype;        ///< Action Type, CDW13 bits 31:24
	uint8_t rsvd21[1];    ///< Reserved; must be zero
	uint16_t rl;          ///< Range Length, CDW13 bits 15:00
};

/**
 * Prepare 'sqe' as the command Get LBA Status, opcode 86h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_get_lba_status_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_get_lba_status(union xnvme_spec_sqe *sqe,
				  const struct xnvme_cmd_prep_adm_get_lba_status_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_program_activation_management(): the fields of Program
 * Activation Management
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Computational Programs 1.3, 4.2.3, Figure 52, page 39, PDF page 39
 */
struct xnvme_cmd_prep_adm_program_activation_management_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t nsid;        ///< Namespace Identifier, CDW1
	uint8_t sel;          ///< Select, CDW10 bits 19:16
	uint8_t rsvd9[1];     ///< Reserved; must be zero
	uint16_t pind;        ///< Program Index, CDW10 bits 15:00
	uint8_t rsvd12[4];    ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Program Activation Management, opcode 88h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see
 * ::xnvme_cmd_prep_adm_program_activation_management_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_program_activation_management(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_program_activation_management_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_memory_range_set_management(): the fields of Memory Range
 * Set Management
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Computational Programs 1.3, 4.2.2, Figure 47, page 38, PDF page 38
 * - CDW11: Computational Programs 1.3, 4.2.2, Figure 48, page 38, PDF page 38
 */
struct xnvme_cmd_prep_adm_memory_range_set_management_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t nsid;        ///< Namespace Identifier, CDW1
	uint16_t rsid;        ///< Memory Range Set ID, CDW10 bits 31:16
	uint8_t sel;          ///< Select, CDW10 bits 03:00
	uint8_t numr;         ///< Number of Memory Ranges, CDW11 bits 07:00
	uint8_t rsvd12[4];    ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Memory Range Set Management, opcode 89h, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_memory_range_set_management_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_memory_range_set_management(
	union xnvme_spec_sqe *sqe,
	const struct xnvme_cmd_prep_adm_memory_range_set_management_args *args);

/**
 * The arguments of xnvme_cmd_prep_adm_sanitize_namespace(): the fields of Sanitize Namespace
 *
 * See Base 2.4, 5, Figure 143, page 176, PDF page 202.
 *
 * The fields of each dword are from:
 *
 * - CDW10: Base 2.4, 5.2.27, Figure 454, page 451, PDF page 479
 */
struct xnvme_cmd_prep_adm_sanitize_namespace_args {
	uint32_t args_nbytes; ///< The size of the record, sizeof(args); must be set
	uint32_t nsid;        ///< Namespace Identifier, CDW1
	uint8_t emvs;         ///< Enter Media Verification State, CDW10 bit 10
	uint8_t preq;         ///< Purge Required, CDW10 bit 4
	uint8_t ause;         ///< Allow Unrestricted Sanitize Exit, CDW10 bit 3
	uint8_t sanact;       ///< Sanitize Action, CDW10 bits 02:00
	uint8_t rsvd12[4];    ///< Reserved; must be zero
};

/**
 * Prepare 'sqe' as the command Sanitize Namespace, opcode 8Ch, from 'args'
 *
 * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.
 * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.
 *
 * @param sqe The submission queue entry to prepare
 * @param args The fields of the command, see ::xnvme_cmd_prep_adm_sanitize_namespace_args
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned:
 * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does
 * not fit its field, or a reserved byte is not zero, and -E2BIG when
 * 'args->args_nbytes' is larger than the record and the bytes beyond it are not
 * zero.
 *
 * @errno
 */
int
xnvme_cmd_prep_adm_sanitize_namespace(
	union xnvme_spec_sqe *sqe, const struct xnvme_cmd_prep_adm_sanitize_namespace_args *args);
