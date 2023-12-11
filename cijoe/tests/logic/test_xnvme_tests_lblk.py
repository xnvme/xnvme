import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "sync"])
def test_io(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_lblk io {cli_args}")
    assert not err


@xnvme_parametrize(labels=["write_uncor"], opts=["be", "admin", "sync"])
def test_write_uncorrectable(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["sync"] in ["block", "psync"]:
        pytest.skip(reason=f"Not supported: write_uncor via { be_opts['sync'] }")

    err, _ = cijoe.run(f"xnvme_tests_lblk write_uncorrectable {cli_args}")
    assert not err


@xnvme_parametrize(labels=["write_zeroes"], opts=["be", "admin", "sync"])
def test_write_zeroes(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["sync"] in ["block", "psync"]:
        pytest.skip(reason=f"Not supported: write_zeroes via { be_opts['sync'] }")

    err, _ = cijoe.run(f"xnvme_tests_lblk write_zeroes {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "sync"])
def test_lba_format(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux":
        pytest.skip(
            reason="Linux Kernel cannot handle formatting namespaces in quick succession"
        )
    if be_opts["be"] == "fbsd" and be_opts["sync"] in ["nvme"]:
        pytest.skip(
            reason=f"Not supported: separate mbuf via { be_opts['sync']} on {be_opts['be']}"
        )
    if be_opts["be"] == "ramdisk":
        pytest.skip(reason=f"Not supported: xnvme_adm_format via { be_opts['be']}")
    err, _ = cijoe.run(f"xnvme_tests_lblk lba_format {cli_args}")
    assert not err
