import pytest

from ..conftest import get_shm_id, xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_hw(cijoe, device, be_opts, cli_args):
    if get_shm_id():
        pytest.skip(reason="xnvme_hello does not take --shm_id as argument")

    err, _ = cijoe.run(f"xnvme_hello {device['uri']}")
    assert not err
