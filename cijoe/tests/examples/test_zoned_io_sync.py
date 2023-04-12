import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["zns"], opts=["be", "admin", "sync"])
def test_write(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["sync"] in ["psync"]:
        pytest.skip(reason="psync(pread/write) does not support mgmt. send/receive")

    err, _ = cijoe.run(f"zoned_io_sync write {cli_args}")
    assert not err


@xnvme_parametrize(labels=["zns"], opts=["be", "admin", "sync"])
def test_append(cijoe, device, be_opts, cli_args):

    if be_opts["admin"] in ["block"]:
        pytest.skip(reason="Linux Block layer does not support append")
    if be_opts["sync"] in ["block", "psync"]:
        pytest.skip(reason="Linux Block layer does not support append")

    err, _ = cijoe.run(f"zoned_io_sync append {cli_args}")
    assert not err


@xnvme_parametrize(labels=["zns"], opts=["be", "admin", "sync"])
def test_read(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["sync"] in ["psync"]:
        pytest.skip(reason="psync(pread/write) does not support mgmt. send/receive")

    err, _ = cijoe.run(f"zoned_io_sync read {cli_args}")
    assert not err
