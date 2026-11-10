import ctypes

import pytest
import xnvme
from conftest import xnvme_parametrize
from utils import dev_from_params

DEVICE_COUNT = 0


@xnvme.XnvmeEnumerateCb
def callback_func(dev, cb_args):
    global DEVICE_COUNT
    DEVICE_COUNT += 1

    return xnvme.XNVME_ENUMERATE_DEV_CLOSE


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_enum(device, be_opts, cli_args):
    """Verify that returned some devices"""

    if "spdk" == be_opts["be"]:
        pytest.skip("SPDK does not support enumeration on physical devices")

    global DEVICE_COUNT
    DEVICE_COUNT = 0

    xnvme.xnvme_enumerate(None, None, callback_func, None)
    assert DEVICE_COUNT > 0


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_enum_open_first(device, be_opts, cli_args):
    """Same as test_enum() except it tries opening before enumerating"""

    if "spdk" == be_opts["be"]:
        pytest.skip("SPDK does not support enumeration post open()/close()")

    # Open/close device. This will make it disappear from the enumeration on SPDK
    dev = dev_from_params(device, be_opts)
    xnvme.xnvme_dev_close(dev)

    global DEVICE_COUNT
    DEVICE_COUNT = 0

    xnvme.xnvme_enumerate(None, None, callback_func, None)
    assert DEVICE_COUNT > 0
