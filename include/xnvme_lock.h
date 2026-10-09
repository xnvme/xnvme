/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file xnvme_lock.h
 */
#ifndef __INTERNAL_XNVME_LOCK_H
#define __INTERNAL_XNVME_LOCK_H

/**
 * Take the library's process-wide lock
 *
 * The public device, queue, buffer and memory-mapping entry points hold it for the duration of
 * the call, which serialises these control-plane operations with respect to each other on any
 * backend; the I/O path does not take it. Backend code that reaches the
 * same process-wide state from threads of its own, such as a HOMI server's handlers, takes it
 * too. It is recursive, so an entry point calling another entry point does not deadlock.
 *
 * Lock order, outermost first: a caller's own locks (the CUSE dispatch mutex, the HOMI
 * server's serve_lock), then this lock, then any backend-private or third-party lock. Nothing
 * may invoke a user callback while holding it.
 */
void
xnvme_lock(void);

/**
 * Release the lock taken by ::xnvme_lock
 */
void
xnvme_unlock(void);

#endif /* __INTERNAL_XNVME_LOCK_H */
