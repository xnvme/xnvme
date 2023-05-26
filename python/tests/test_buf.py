import xnvme.ctypes_bindings.api as xnvme
from conftest import xnvme_parametrize
from utils import dev_from_params

COUNT = 5


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_buf_alloc_free(cijoe, device, be_opts, cli_args):
    dev = dev_from_params(device, be_opts)
    assert dev

    for i in range(COUNT):
        buf_nbytes = 1 << i
        buf = xnvme.xnvme_buf_alloc(dev, buf_nbytes)
        assert buf
        xnvme.xnvme_buf_free(dev, buf)

    xnvme.xnvme_dev_close(dev)


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_buf_virt_alloc_free(cijoe, device, be_opts, cli_args):
    dev = dev_from_params(device, be_opts)
    assert dev

    for i in range(COUNT):
        buf_nbytes = 1 << i
        buf = xnvme.xnvme_buf_virt_alloc(0x1000, buf_nbytes)
        assert buf
        xnvme.xnvme_buf_virt_free(buf)

    xnvme.xnvme_dev_close(dev)
