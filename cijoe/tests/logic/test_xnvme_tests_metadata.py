import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "sync"])
def test_metadata_buf(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason=f"Not supported: xnvme_adm_format via { be_opts['admin']}")
    if be_opts["sync"] in ["block", "psync"] or (
        be_opts["be"] == "fbsd" and be_opts["sync"] in ["nvme"]
    ):
        pytest.skip(reason=f"Not supported: separate mbuf via { be_opts['sync']}")
    if be_opts["be"] == "ramdisk":
        pytest.skip(reason=f"Not supported: xnvme_adm_format via { be_opts['be']}")
    err, _ = cijoe.run(f"xnvme_tests_metadata buf {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "sync"])
def test_metadata_iovec(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason=f"Not supported: xnvme_adm_format via { be_opts['admin']}")
    if be_opts["sync"] in ["block", "psync"] or (
        be_opts["be"] == "fbsd" and be_opts["sync"] in ["nvme"]
    ):
        pytest.skip(reason=f"Not supported: separate mbuf via { be_opts['sync']}")
    if be_opts["be"] == "ramdisk":
        pytest.skip(reason=f"Not supported: xnvme_adm_format via { be_opts['be']}")
    err, _ = cijoe.run(f"xnvme_tests_metadata iovec {cli_args}")
    assert not err
