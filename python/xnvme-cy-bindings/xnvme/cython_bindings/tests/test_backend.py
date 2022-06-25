import conftest
import pytest
import xnvme.cython_bindings as xnvme

enumerated_devices = 0


@pytest.mark.xfail(
    conftest.BACKEND == b"spdk",
    reason="SPDK cannot handle a dev open/close followed by enumerate",
)
@pytest.mark.skipif(
    conftest.BACKEND == b"ramdisk",
    reason="ramdisk doesn't support enumeration",
)
def test_enum_spdk_fail(opts):
    def callback_func(dev, cb_args):
        global enumerated_devices
        print(xnvme.xnvme_dev_get_ident(dev).to_dict(), cb_args)
        enumerated_devices += 1
        return xnvme.XNVME_ENUMERATE_DEV_CLOSE

    # Open/close device. This will make it disappear from the enumeration on SPDK
    device_path = conftest.DEVICE_PATH
    device = xnvme.xnvme_dev_open(device_path, opts)
    device = xnvme.xnvme_dev_close(device)

    blank_opts = xnvme.xnvme_opts()
    xnvme.xnvme_enumerate(None, blank_opts, callback_func, "Awesome context!")
    assert enumerated_devices > 0
