import pytest
from cijoe.xnvme.tests.conftest import xnvme_device_driver as device
from cijoe.xnvme.tests.conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_hw(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"xnvme_hello hw {cli_args}")
    assert not err
