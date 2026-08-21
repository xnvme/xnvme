// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Inspecting a multi-process controller without attaching to it
 *
 * Reads uPCIe's shared bookkeeping directly, opening no device and taking no
 * lock. Holding a lock is ruled out rather than merely avoided: the role
 * election is decided by testing that same lock, so a probe holding it briefly
 * could make a starting process demote itself and wait for a primary that
 * never arrives.
 *
 * Everything returned is a snapshot taken without a lock, so counts may be
 * stale and an identifier read while a controller closes may be a torn copy of
 * two.
 *
 * @note uPCIe only, so -ENOSYS elsewhere. Other backends accept the same
 * `shm_id` without keeping bookkeeping here, and read as absent.
 *
 * @note This API is experimental and may change without notice.
 *
 * @file libxnvme_mproc.h
 */

#ifndef __LIBXNVME_MPROC_H
#define __LIBXNVME_MPROC_H

#include <stdint.h>

/* XNVME_IDENT_URI_LEN comes from libxnvme_ident.h, which libxnvme.h includes
 * first; these headers carry no include guards and rely on that ordering. */

/**
 * Most controllers a single runtime records
 */
#define XNVME_MPROC_MAX_CTRLRS 64

/**
 * What a runtime says about itself and what it holds
 */
struct xnvme_mproc_info {
	uint32_t nattached; ///< Processes attached, the primary itself included

	uint32_t nctrlrs; ///< Controllers listed below

	/** Controllers held; exceeds `nctrlrs` when the list below truncated */
	uint32_t nctrlrs_held;

	/** Their identifiers, `nctrlrs` of them */
	char ctrlrs[XNVME_MPROC_MAX_CTRLRS][XNVME_IDENT_URI_LEN];
};

/**
 * Report whether a process currently holds the runtime for `shm_id`
 *
 * A segment outlives the primary that created it, so what distinguishes a live
 * runtime from debris is the role-election lock, which the kernel drops when
 * its holder dies. Asked without taking it, so a process electing at the same
 * moment is unaffected.
 *
 * @param shm_id Shared-memory id the runtime was created with
 *
 * @return 1 when a process holds it, 0 when none does, negative errno on
 *         error, including -ENOSYS where multi-process mode is unavailable.
 */
int
xnvme_mproc_primary_alive(uint32_t shm_id);

/**
 * Read what the runtime for `shm_id` holds, without attaching
 *
 * @param shm_id Shared-memory id the runtime was created with
 * @param info Filled on success
 *
 * @return On success, 0 is returned. On error, negative errno is returned:
 *         -ENOENT when no runtime exists for this id, -EAGAIN when one is
 *         mid-creation and worth retrying, -EPROTO when the segment carries
 *         another build's layout, -ENOSYS where multi-process mode is
 *         unavailable.
 */
int
xnvme_mproc_get_info(uint32_t shm_id, struct xnvme_mproc_info *info);

/**
 * What a shared controller segment says about its users
 */
struct xnvme_mproc_ctrlr_info {
	uint32_t nattached; ///< Processes attached, the primary itself included

	/** I/O queues in use; always equal, since a queue id covers the pair */
	uint32_t nsq_used;
	uint32_t ncq_used;

	/**
	 * I/O queues the controller allocated; the smaller limits queue pairs.
	 * Zero means the controller did not answer, not that it has none.
	 */
	uint32_t nsq_total;
	uint32_t ncq_total;

	uint32_t initialized; ///< Whether the primary finished bringing the controller up
};

/**
 * Read the shared state for the controller at `uri`, without attaching
 *
 * @param uri Device identifier, as passed to xnvme_dev_open()
 * @param info Filled on success
 *
 * @return On success, 0 is returned. On error, negative errno is returned:
 *         -ENOENT when no process is sharing this controller, -EAGAIN when
 *         the segment is mid-creation and worth retrying, -EPROTO when it
 *         carries another build's layout, -ENOSYS where multi-process mode is
 *         unavailable.
 *
 * @note Does not check that the creating process is still alive; a killed
 * primary leaves a segment that still reads as plausible. Pair with
 * xnvme_mproc_primary_alive() to tell the two apart.
 */
int
xnvme_mproc_get_ctrlr_info(const char *uri, struct xnvme_mproc_ctrlr_info *info);

#endif /* __LIBXNVME_MPROC_H */
