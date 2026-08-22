// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Inspecting a multi-process controller without attaching to it
 *
 * Multi-process mode keeps its bookkeeping in shared memory so that the
 * processes sharing a controller can find each other. The same bookkeeping
 * answers questions an operator has, how many processes are attached and how
 * many I/O queue pairs they have taken, and answering them should not require
 * becoming one of those processes: attaching costs a heap, bumps the refcount
 * being reported, and needs the hugepages and driver bindings that a monitoring
 * tool has no business requiring.
 *
 * So this reads the shared segment directly, read-only. It opens no device,
 * takes no lock at all, and needs no privilege beyond reading the segment.
 * Taking a lock is ruled out rather than merely avoided: the role-election
 * lock is what a starting process tests to decide whether it is the primary,
 * so a probe that held it even briefly could make that process demote itself
 * and then wait for a primary that never arrives.
 *
 * What is returned is a snapshot taken without any lock, so a count may be
 * stale by the time it is read, and an identifier read while a controller is
 * closing may be a torn copy of two. That is the right trade for monitoring
 * and the wrong one for anything making decisions on it.
 *
 * @note This covers the uPCIe backend only, and is therefore available only
 * where that backend is built, which is Linux and only when it was enabled;
 * elsewhere every call here reports -ENOSYS. Other backends accept the same
 * `shm_id` without keeping their bookkeeping where this can read it, so a
 * runtime belonging to one of those is reported as absent rather than as
 * running.
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

	/**
	 * Controllers held, which exceeds `nctrlrs` past XNVME_MPROC_MAX_CTRLRS.
	 *
	 * Reported so a truncated list is visible as one rather than passing
	 * for the whole of what the runtime holds.
	 */
	uint32_t nctrlrs_held;

	/** Their identifiers, `nctrlrs` of them */
	char ctrlrs[XNVME_MPROC_MAX_CTRLRS][XNVME_IDENT_URI_LEN];
};

/**
 * Report whether a process currently holds the runtime for `shm_id`
 *
 * A segment outlives the primary that created it: a process killed without
 * cleanup never unlinks it, so what it recorded stays readable afterwards.
 * Holding the role-election lock is what distinguishes a live runtime from
 * that debris, and the kernel drops that lock when its holder dies. This asks
 * whether it is held without taking it, so a process electing at the same
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
 * The primary records its controllers as it opens them, so this reports what
 * is actually held rather than what some configuration asked for, and it works
 * whichever process claimed the role.
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

	/**
	 * I/O queues in use, submission and completion.
	 *
	 * These are equal: a queue pair is created as one submission and one
	 * completion queue sharing a queue identifier, and the identifier is
	 * what is tracked. They are reported separately so the comparison with
	 * the totals below reads without arithmetic.
	 */
	uint32_t nsq_used;
	uint32_t ncq_used;

	/**
	 * I/O queues the controller has allocated, as counts.
	 *
	 * The two need not agree, and the smaller is what limits queue pairs;
	 * both are given rather than their minimum so it is visible which one
	 * binds. Zero when the controller did not answer, meaning unknown
	 * rather than none.
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
 * @note This does not check that the process which created the segment is
 * still alive; a killed primary leaves one behind that still reads as
 * plausible. Pair it with xnvme_mproc_primary_alive() to tell the two apart.
 */
int
xnvme_mproc_get_ctrlr_info(const char *uri, struct xnvme_mproc_ctrlr_info *info);

#endif /* __LIBXNVME_MPROC_H */
