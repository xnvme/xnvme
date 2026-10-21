// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_MACOS_STATE_H
#define __INTERNAL_XNVME_BE_MACOS_STATE_H
#include <xnvme_be.h>
#include <xnvme_be_posix_state.h>

/**
 * Internal representation of XNVME_BE_MACOS state
 *
 * The IOKit handles are held opaquely, so the header needs no IOKit: the
 * io_object_t of the device, its IONVMeSMARTInterface ** and its
 * IOCFPlugInInterface **.
 */
struct xnvme_be_macos_state {
	struct xnvme_be_posix_state posix;
	uint32_t ioservice_device;
	void *nvme_smart_interface;
	void *plugin_interface;

	uint8_t _rsvd[104];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_macos_state) == XNVME_BE_STATE_NBYTES, "Incorrect size")
XNVME_BE_POSIX_STATE_FIRST(xnvme_be_macos_state);

#endif /* __INTERNAL_XNVME_BE_MACOS_STATE_H */
