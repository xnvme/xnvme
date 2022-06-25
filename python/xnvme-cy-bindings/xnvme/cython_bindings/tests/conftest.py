import os

import pytest
import xnvme.cython_bindings as xnvme

DEVICE_PATH = os.environ.get("XNVME_URI", "").encode()
BACKEND = os.environ.get("XNVME_BE", "").encode()
DEVICE_NSID = int(os.environ.get("XNVME_DEV_NSID", "1"))

NULL = xnvme.xnvme_void_p(0)
UINT16_MAX = 0xFFFF


@pytest.fixture
def opts():
    return xnvme.xnvme_opts(be=BACKEND, nsid=DEVICE_NSID)


@pytest.fixture
def dev(opts):
    device_path = DEVICE_PATH
    device = xnvme.xnvme_dev_open(device_path, opts)
    yield device
    device = xnvme.xnvme_dev_close(device)
