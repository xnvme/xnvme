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

int
xnvme_be_macos_driverkit_enumerate(const char *sys_uri, struct xnvme_opts *opts,
				   xnvme_enumerate_cb cb_func, void *cb_args)
{
	XNVME_DEBUG("INFO: xnvme_be_macos_driverkit_enumerate()");
	CFMutableDictionaryRef matching_dict;
	struct xnvme_opts tmp_opts = *opts;
	io_iterator_t iterator;
	kern_return_t ret;
	io_service_t service;
	struct xnvme_dev *dev;
	char uri[XNVME_IDENT_URI_LEN];

	if (sys_uri) {
		XNVME_DEBUG("FAILED: sys_uri: %s is not supported", sys_uri);
		return -ENOSYS;
	}

	const char *service_classes[] = {"IOUserBlockStorageDevice", "IOUserService"};
	for (int i = 0; i < 2; ++i) {
		matching_dict = IOServiceMatching(service_classes[i]);

		ret = IOServiceGetMatchingServices(kIOMasterPortDefault, matching_dict, &iterator);
		if (ret != kIOReturnSuccess || !iterator) {
			XNVME_DEBUG("FAILED: IOServiceGetMatchingServices(%s); ret(0x%08x), '%s'",
				    service_classes[i], ret, mach_error_string(ret));
			continue;
		}

		tmp_opts.be = xnvme_be_macos_driverkit.attr.name;
		tmp_opts.nsid = 1;

		while (true) {
			dev = NULL;
			memset(&uri, 0, XNVME_IDENT_URI_LEN);
			service = IOIteratorNext(iterator);

			if (!service) {
				break;
			}

			ret = IORegistryEntryGetName(service, uri);
			if (strncmp("MacVFN-", uri, 7)) {
				continue;
			}
			XNVME_DEBUG("INFO: MacVFN uri matched: %s", uri);

			dev = xnvme_dev_open(uri, &tmp_opts);
			if (!dev) {
				XNVME_DEBUG("INFO: xnvme_dev_open(): %d", errno);
				return -errno;
			}
			if (cb_func(dev, cb_args)) {
				xnvme_dev_close(dev);
			}
		}

		IOObjectRelease(iterator);
	}

	return 0;
}

void
xnvme_be_macos_driverkit_dev_close(struct xnvme_dev *dev)
{
	struct xnvme_be_macos_driverkit_state *state =
		(struct xnvme_be_macos_driverkit_state *)dev->be.state;

	if (!state->device_service) {
		XNVME_DEBUG("FAILED: !state->device_service");
		return;
	}

	IOServiceClose(state->device_service);
	IOServiceClose(state->device_connection);
}

int
_xnvme_be_driverkit_create_ioqpair(struct xnvme_be_macos_driverkit_state *state, int qd, int flags)
{
	unsigned int qid = __builtin_ffsll(state->qidmap);
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

	ret = IOConnectCallStructMethod(state->device_connection, NVME_CREATE_QUEUE_PAIR,
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
	state->qidmap &= ~(1ULL << qid);

	return qid;
}

int
_xnvme_be_driverkit_delete_ioqpair(struct xnvme_be_macos_driverkit_state *state, int qid)
{
	size_t output_cnt = 0;
	NvmeQueue ioqpair_delete = {
		.id = (uint64_t)qid,
	};
	kern_return_t ret;

	ret = IOConnectCallStructMethod(state->device_connection, NVME_DELETE_QUEUE_PAIR,
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

	state->qidmap |= 1ULL << qid;

	return 0;
}

int
xnvme_be_macos_driverkit_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_be_macos_driverkit_state *state =
		(struct xnvme_be_macos_driverkit_state *)dev->be.state;
	int qpid;
	kern_return_t err;
	io_service_t service;
	CFMutableDictionaryRef matching_dict;
	io_connect_t connection;

	matching_dict = IOServiceNameMatching(dev->ident.uri);
	service = IOServiceGetMatchingService(kIOMasterPortDefault, matching_dict);
	if (!service) {
		XNVME_DEBUG("FAILED: IOServiceGetMatchingService()");
		return -ENODEV;
	}

	err = IOServiceOpen(service, mach_task_self_, MACVFN_CLIENT_VERSION, &connection);
	if (err != kIOReturnSuccess) {
		XNVME_DEBUG("FAILED: IOServiceOpen(MACVFN_CLIENT_VERSION); connection(%u),"
			    "ret(0x%08x), '%s'",
			    connection, err, mach_error_string(err));
		return -EIO;
	}
	XNVME_DEBUG("INFO: IOServiceOpen(MACVFN_CLIENT_VERSION); connection: %u", connection);

	state->device_service = service;
	state->device_connection = connection;

	err = IOConnectCallScalarMethod(state->device_connection, NVME_INIT, NULL, 0, NULL, 0);
	if (err != kIOReturnSuccess) {
		XNVME_DEBUG("FAILED: IOConnectCallScalarMethod(NVME_INIT);"
			    "ret(0x%08x), '%s'",
			    err, mach_error_string(err));
		return -EIO;
	}
	XNVME_DEBUG("INFO: NVME_INIT done");

	dev->ident.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
	dev->ident.csi = XNVME_SPEC_CSI_NVM;
	dev->ident.nsid = dev->opts.nsid;

	state->qidmap = -1 & ~0x1;

	qpid = _xnvme_be_driverkit_create_ioqpair(state, 31, 0x0);
	if (qpid < 0) {
		XNVME_DEBUG("FAILED: initializing qpairs for sync IO: %d", err);
		return qpid;
	}

	state->buffers = (struct buffer *)malloc(MAX_BUFFERS * sizeof(struct buffer));
	memset(state->buffers, 0, MAX_BUFFERS * sizeof(struct buffer));

	return 0;
}
#endif

struct xnvme_be_dev g_xnvme_be_macos_driverkit_dev = {
#ifdef XNVME_PLATFORM_MACOS_ENABLED
	.enumerate = xnvme_be_macos_driverkit_enumerate,
	.dev_open = xnvme_be_macos_driverkit_dev_open,
	.dev_close = xnvme_be_macos_driverkit_dev_close,
#else
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
#endif
};
