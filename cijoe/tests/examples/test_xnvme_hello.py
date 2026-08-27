import pytest

from ..conftest import get_homi_id, xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_hw(cijoe, device, be_opts, cli_args):
    if get_homi_id():
        pytest.skip(reason="xnvme_hello does not take a control-plane id")

    err, _ = cijoe.run(f"xnvme_hello {device['uri']}")
    assert not err
