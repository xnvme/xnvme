import pytest

from ..conftest import xnvme_parametrize

pytest.skip(allow_module_level=True, reason="Not implemented")


@xnvme_parametrize(labels=["kvs"], opts=["be", "admin"])
def test_kvs_io(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_kvs kvs_io {cli_args}")
    assert not err


@xnvme_parametrize(labels=["kvs"], opts=["be", "admin"])
def test_kvs_io_custom_kv(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_kvs kvs_io {cli_args} --key tango --value charlie")
    assert not err
