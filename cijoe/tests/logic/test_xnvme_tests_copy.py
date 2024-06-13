import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["scc"], opts=["be", "admin", "sync"])
def test_copy(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["sync"] in ["block", "psync"]:
        pytest.skip(reason=f"Not supported: simple-copy via {be_opts['sync']}")
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(
            reason=f"Admin {be_opts['admin']} doesn't report MSSRL on the namespace"
        )

    err, _ = cijoe.run(f"xnvme_tests_copy copy {cli_args}")
    assert not err


@xnvme_parametrize(labels=["scc"], opts=["be", "admin", "sync"])
def test_copy_src_before_dest(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["sync"] in ["block", "psync"]:
        pytest.skip(reason=f"Not supported: simple-copy via {be_opts['sync']}")
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(
            reason=f"Admin {be_opts['admin']} doesn't report MSSRL on the namespace"
        )

    err, _ = cijoe.run(f"xnvme_tests_copy copy {cli_args} --slba 0 --nlb 7 --sdlba 8")
    assert not err


@xnvme_parametrize(labels=["scc"], opts=["be", "admin", "sync"])
def test_copy_src_after_dest(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["sync"] in ["block", "psync"]:
        pytest.skip(reason=f"Not supported: simple-copy via {be_opts['sync']}")
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(
            reason=f"Admin {be_opts['admin']} doesn't report MSSRL on the namespace"
        )

    err, _ = cijoe.run(f"xnvme_tests_copy copy {cli_args} --slba 8 --nlb 7 --sdlba 0")
    assert not err
