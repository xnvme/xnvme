from ctypes import byref, pointer

import numpy
import xnvme.ctypes_bindings.api as xnvme
from conftest import xnvme_parametrize
from utils import buf_and_view, dev_from_params, xnvme_cmd_ctx_cpl_status


@xnvme_parametrize(labels=["dev"], opts=["be", "sync"])
def test_io_sync(cijoe, device, be_opts, cli_args):
    """
    Verify that writing/reading works as expected
    """

    dev = dev_from_params(device, be_opts)
    assert dev

    nsid = xnvme.xnvme_dev_get_nsid(dev)
    lba_nbytes = xnvme.xnvme_dev_get_geo(dev).contents.lba_nbytes
    nlb = 0

    ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)

    data, data_view = buf_and_view(dev, lba_nbytes)
    assert data
    data_view[:] = 42

    err = xnvme.xnvme_nvm_write(byref(ctx), nsid, 0x0, nlb, data, None)
    assert not err and not xnvme_cmd_ctx_cpl_status(pointer(ctx))

    buf, buf_view = buf_and_view(dev, lba_nbytes)
    assert buf
    buf_view[:] = 0

    err = xnvme.xnvme_nvm_read(byref(ctx), nsid, 0x0, nlb, buf, None)
    assert not err and not xnvme_cmd_ctx_cpl_status(pointer(ctx))

    assert numpy.all(buf_view == data_view)
    assert numpy.all(buf_view == 42)
    assert buf_view[10] == data_view[20]

    xnvme.xnvme_buf_free(dev, buf)
    xnvme.xnvme_buf_free(dev, data)
    xnvme.xnvme_dev_close(dev)
