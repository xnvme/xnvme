// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <pthread.h>
#include <xnvme_lock.h>

static pthread_once_t g_lock_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock;

static void
_lock_init(void)
{
	pthread_mutexattr_t attr;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&g_lock, &attr);
	pthread_mutexattr_destroy(&attr);
}

void
xnvme_lock(void)
{
	pthread_once(&g_lock_once, _lock_init);
	pthread_mutex_lock(&g_lock);
}

void
xnvme_unlock(void)
{
	pthread_mutex_unlock(&g_lock);
}
