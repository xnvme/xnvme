import ctypes
import os

import numpy as np
import pytest
import xnvme.cython_bindings as xnvme

DEVICE_PATH = os.environ.get("XNVME_URI", "").encode()
BACKEND = os.environ.get("XNVME_BE", "").encode()
DEVICE_NSID = int(os.environ.get("XNVME_DEV_NSID", "1"))
HUGETLB_PATH = os.environ.get("XNVME_HUGETLB_PATH")

NULL = xnvme.xnvme_void_p(0)
UINT16_MAX = 0xFFFF


def pytest_configure(config):
    config.addinivalue_line("markers", "xnvme_admin(arg): Select admin interface.")
    config.addinivalue_line("markers", "xnvme_async(arg): Select async interface.")
    config.addinivalue_line("markers", "xnvme_sync(arg): Select sync interface.")
    config.addinivalue_line("markers", "xnvme_mem(arg): Select mem interface.")


@pytest.fixture(scope="function")
def opts():
    _opts = xnvme.xnvme_opts_default()
    _opts.be = BACKEND
    _opts.nsid = DEVICE_NSID
    return _opts


@pytest.fixture(scope="function")
def dev(opts, request):
    marker = request.node.get_closest_marker("xnvme_admin")
    if marker:
        opts.admin = marker.args[0]

    marker = request.node.get_closest_marker("xnvme_async")
    if marker:
        opts.async_ = marker.args[0]

    marker = request.node.get_closest_marker("xnvme_sync")
    if marker:
        opts.sync = marker.args[0]

    marker = request.node.get_closest_marker("xnvme_mem")
    if marker:
        opts.mem = marker.args[0]

    opts.direct = 1
    opts.rdwr = 1

    device = xnvme.xnvme_dev_open(DEVICE_PATH, opts)
    yield device
    device = xnvme.xnvme_dev_close(device)


@pytest.fixture(scope="function")
def autofreed_buffer(dev):
    buffers = []

    def _alloc(buffer_size):
        buf = xnvme.xnvme_buf_alloc(dev, buffer_size)
        buf_memview = np.ctypeslib.as_array(
            ctypes.cast(buf.void_pointer, ctypes.POINTER(ctypes.c_uint8)),
            shape=(buffer_size,),
        )

        buf_memview[:] = 0  # Zero memory and force page allocation

        buffers.append(buf)
        return buf, buf_memview

    yield _alloc

    for buf in buffers:
        xnvme.xnvme_buf_free(dev, buf)
