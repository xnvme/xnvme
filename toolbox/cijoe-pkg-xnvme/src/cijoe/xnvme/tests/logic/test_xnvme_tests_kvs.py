import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["kvs"], opts=["be", "admin"])
def test_kvs_io(cijoe, device, be_opts, cli_args):

    if be_opts["sync"] in ["block", "psync"]:
        pytest.skip(reason="Linux Block layer does not support simple-copy")

    err, _ = cijoe.run(f"xnvme_tests_kvs kvs_io {cli_args}")
    assert not err
