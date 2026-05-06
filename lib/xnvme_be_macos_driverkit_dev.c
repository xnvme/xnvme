// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifdef XNVME_PLATFORM_MACOS_ENABLED
#include <xnvme_be_macos_driverkit.h>
#endif

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_MACOS_ENABLED
#include <mach/mach_error.h>
#include <CoreFoundation/CoreFoundation.h>

#include <xnvme_dev.h>

void
xnvme_be_macos_driverkit_dev_close(struct xnvme_dev *dev)
{
	struct xnvme_be_macos_driverkit_state *state =
		(struct xnvme_be_macos_driverkit_state *)dev->be.state;

	if (state->qid_sync) {
		_xnvme_be_driverkit_delete_ioqpair(state, state->qid_sync);
	}
	free(state->buffers);
}

int
_xnvme_be_driverkit_create_ioqpair(struct xnvme_be_macos_driverkit_state *state, int qd, int flags)
{
	struct xnvme_be_macos_driverkit_ctrlr *ctrlr = state->ctrlr;
	unsigned int qid = __builtin_ffsll(ctrlr->qidmap);
	NvmeQueue ioqpair_return;
	NvmeQueue ioqpair_create;
	kern_return_t ret;
	size_t output_cnt;

	if (qid == 0) {
		XNVME_DEBUG("FAILED: no free queue identifiers");
		return -EBUSY;
	}

	qid--;

	XNVME_DEBUG("INFO: nvme_create_ioqpair(%d, %d, %d)", qid, qd + 1, flags);

	// nvme queue capacity must be one larger than the requested capacity
	// since only n-1 slots in an NVMe queue may be used
	ioqpair_create = (NvmeQueue){
		.id = (uint64_t)qid,
		.depth = (uint64_t)qd + 1,
		.vector = -1,
		.flags = (uint64_t)flags,
	};
	output_cnt = sizeof(NvmeQueue);

	ret = IOConnectCallStructMethod(ctrlr->device_connection, NVME_CREATE_QUEUE_PAIR,
					&ioqpair_create, sizeof(NvmeQueue), &ioqpair_return,
					&output_cnt);
	if (ret != kIOReturnSuccess) {
		XNVME_DEBUG("FAILED: IOConnectCallStructMethod(NVME_CREATE_QUEUE_PAIR); "
			    "ret(0x%08x), '%s'",
			    ret, mach_error_string(ret));
		return -EAGAIN;
	}
	XNVME_DEBUG("INFO: IOConnectCallStructMethod(NVME_CREATE_QUEUE_PAIR); success");

	state->qid_sync = ioqpair_return.id;
	ctrlr->qidmap &= ~(1ULL << qid);

	return qid;
}

int
_xnvme_be_driverkit_delete_ioqpair(struct xnvme_be_macos_driverkit_state *state, int qid)
{
	struct xnvme_be_macos_driverkit_ctrlr *ctrlr = state->ctrlr;
	size_t output_cnt = 0;
	NvmeQueue ioqpair_delete = {
		.id = (uint64_t)qid,
	};
	kern_return_t ret;

	ret = IOConnectCallStructMethod(ctrlr->device_connection, NVME_DELETE_QUEUE_PAIR,
					&ioqpair_delete, sizeof(NvmeQueue), NULL, &output_cnt);
	if (ret != kIOReturnSuccess) {
		XNVME_DEBUG("FAILED: IOConnectCallStructMethod(NVME_DELETE_QUEUE_PAIR); "
			    "ret(0x%08x), '%s'",
			    ret, mach_error_string(ret));
		return -EIO;
	}
	XNVME_DEBUG("INFO: IOConnectCallStructMethod(NVME_DELETE_QUEUE_PAIR); success;"
		    "ret(0x%08x), '%s'",
		    ret, mach_error_string(ret));

	ctrlr->qidmap |= 1ULL << qid;

	return 0;
}

/**
 * Initialize the DriverKit controller.
 *
 * Allocates a shared xnvme_be_macos_driverkit_ctrlr, opens the IOKit service
 * and connection, initializes the NVMe controller. The returned handle is stored
 * in cref and written to dev->be.state[0] by the platform.
 */
static void *
xnvme_be_macos_driverkit_ctrlr_init(struct xnvme_dev *dev)
{
	struct xnvme_be_macos_driverkit_ctrlr *ctrlr;
	CFMutableDictionaryRef matching_dict;
	io_service_t service;
	io_connect_t connection;
	kern_return_t err;

	ctrlr = calloc(1, sizeof(*ctrlr));
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: calloc(ctrlr)");
		return NULL;
	}

	matching_dict = IOServiceNameMatching(dev->ident.uri);
	service = IOServiceGetMatchingService(kIOMasterPortDefault, matching_dict);
	if (!service) {
		XNVME_DEBUG("FAILED: IOServiceGetMatchingService()");
		free(ctrlr);
		return NULL;
	}

	err = IOServiceOpen(service, mach_task_self_, MACVFN_CLIENT_VERSION, &connection);
	if (err != kIOReturnSuccess) {
		XNVME_DEBUG("FAILED: IOServiceOpen(MACVFN_CLIENT_VERSION); connection(%u),"
			    "ret(0x%08x), '%s'",
			    connection, err, mach_error_string(err));
		free(ctrlr);
		return NULL;
	}
	XNVME_DEBUG("INFO: IOServiceOpen(MACVFN_CLIENT_VERSION); connection: %u", connection);

	ctrlr->device_service = service;
	ctrlr->device_connection = connection;

	err = IOConnectCallScalarMethod(ctrlr->device_connection, NVME_INIT, NULL, 0, NULL, 0);
	if (err != kIOReturnSuccess) {
		XNVME_DEBUG("FAILED: IOConnectCallScalarMethod(NVME_INIT);"
			    "ret(0x%08x), '%s'",
			    err, mach_error_string(err));
		IOServiceClose(service);
		IOServiceClose(connection);
		free(ctrlr);
		return NULL;
	}
	XNVME_DEBUG("INFO: NVME_INIT done");

	ctrlr->qidmap = -1 & ~0x1;

	return ctrlr;
}

static int
xnvme_be_macos_driverkit_ctrlr_term(void *handle)
{
	struct xnvme_be_macos_driverkit_ctrlr *ctrlr = handle;

	IOServiceClose(ctrlr->device_service);
	IOServiceClose(ctrlr->device_connection);
	free(ctrlr);

	return 0;
}

int
xnvme_be_macos_driverkit_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_be_macos_driverkit_state *state =
		(struct xnvme_be_macos_driverkit_state *)dev->be.state;
	int qpid;

	dev->ident.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
	dev->ident.csi = XNVME_SPEC_CSI_NVM;
	dev->ident.nsid = dev->opts.nsid;

	qpid = _xnvme_be_driverkit_create_ioqpair(state, 31, 0x0);
	if (qpid < 0) {
		XNVME_DEBUG("FAILED: initializing qpairs for sync IO: %d", qpid);
		return qpid;
	}

	state->buffers = (struct buffer *)malloc(MAX_BUFFERS * sizeof(struct buffer));
	if (!state->buffers) {
		XNVME_DEBUG("FAILED: malloc(buffers)");
		return -ENOMEM;
	}
	memset(state->buffers, 0, MAX_BUFFERS * sizeof(struct buffer));

	return 0;
}
#endif

struct xnvme_be_dev g_xnvme_be_macos_driverkit_dev = {
#ifdef XNVME_PLATFORM_MACOS_ENABLED
	.dev_open = xnvme_be_macos_driverkit_dev_open,
	.dev_close = xnvme_be_macos_driverkit_dev_close,
	.id = "driverkit",
	.ctrlr_init = xnvme_be_macos_driverkit_ctrlr_init,
	.ctrlr_term = xnvme_be_macos_driverkit_ctrlr_term,
#else
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
	.id = "nosys",
	.ctrlr_init = NULL,
	.ctrlr_term = NULL,
#endif
};
