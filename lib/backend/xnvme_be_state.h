// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_STATE_H
#define __INTERNAL_XNVME_BE_STATE_H
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_be_posix_state.h>
#include <xnvme_be_linux_state.h>
#include <xnvme_be_freebsd_state.h>
#include <xnvme_be_macos_state.h>
#include <xnvme_be_windows_state.h>
#include <xnvme_be_driverkit_state.h>
#include <xnvme_be_libvfn_state.h>
#include <xnvme_be_spdk_state.h>
#include <xnvme_be_upcie_state.h>
#include <xnvme_be_ramdisk_state.h>

union xnvme_be_state {
	uint8_t raw[XNVME_BE_STATE_NBYTES];
	void *ctrlr;

	struct xnvme_be_posix_state posix_state;
	struct xnvme_be_linux_state linux_state;
	struct xnvme_be_freebsd_state freebsd_state;
	struct xnvme_be_macos_state macos_state;
	struct xnvme_be_windows_state windows_state;
	struct xnvme_be_driverkit_state driverkit_state;
	struct xnvme_be_libvfn_state libvfn_state;
	struct xnvme_be_spdk_state spdk_state;
	struct xnvme_be_upcie_state upcie_state;
	struct xnvme_be_ramdisk_state ramdisk_state;
};
XNVME_STATIC_ASSERT(sizeof(union xnvme_be_state) == XNVME_BE_STATE_NBYTES, "Incorrect size")

static inline union xnvme_be_state *
xnvme_be_state_of(const struct xnvme_be *be)
{
	return (void *)be->state;
}

static inline union xnvme_be_state *
xnvme_be_state(const struct xnvme_dev *dev)
{
	return xnvme_be_state_of(&dev->be);
}

#endif /* __INTERNAL_XNVME_BE_STATE_H */
