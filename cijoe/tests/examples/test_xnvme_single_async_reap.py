import pytest

from ..conftest import get_homi_id, xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_read(cijoe, device, be_opts, cli_args):
    if get_homi_id():
        pytest.skip(reason="xnvme example single-async-reap does not take a homi id")

    err, _ = cijoe.run(f"xnvme example single-async-reap {device['uri']}")
    assert not err
