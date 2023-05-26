import ctypes
from ctypes import CFUNCTYPE, POINTER, c_int, c_void_p

import pytest
import xnvme.ctypes_bindings as xnvme
from conftest import xnvme_parametrize
from utils import dev_from_params
from xnvme.ctypes_bindings.api import char_pointer_cast

DEVICE_COUNT = 0


@CFUNCTYPE(c_int, POINTER(xnvme.xnvme_dev), c_void_p)
def callback_func(dev, cb_args):
    global DEVICE_COUNT
    DEVICE_COUNT += 1

    return xnvme.XNVME_ENUMERATE_DEV_CLOSE


def test_enum():
    """Verify that returned some devices"""

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
