import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["zns"], opts=["be", "admin", "async"])
def test_write(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"zoned_io_async write {cli_args}")
    assert not err


@xnvme_parametrize(labels=["zns"], opts=["be", "admin", "async"])
def test_append(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["admin"] == "block":
        pytest.skip(reason="Linux block-layer does not support append")

    if be_opts["be"] == "linux" and be_opts["async"] in ["io_uring", "libaio", "posix"]:
        pytest.skip(reason="Linux block-layer does not support append")

    err, _ = cijoe.run(f"zoned_io_async append {cli_args}")
    assert not err


@xnvme_parametrize(labels=["zns"], opts=["be", "admin", "async"])
def test_read(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"zoned_io_async read {cli_args}")
    assert not err
