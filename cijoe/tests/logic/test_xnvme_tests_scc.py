import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["scc"], opts=["be", "admin", "sync"])
def test_idfy(cijoe, device, be_opts, cli_args):

    if be_opts["sync"] in ["block", "psync"]:
        pytest.skip(reason="Linux Block layer does not support simple-copy")

    err, _ = cijoe.run(f"xnvme_tests_scc idfy {cli_args}")
    assert not err


@xnvme_parametrize(labels=["scc"], opts=["be", "admin", "sync"])
def test_support(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="Linux Block layer does not support simple-copy")

    err, _ = cijoe.run(f"xnvme_tests_scc support {cli_args}")
    assert not err


@xnvme_parametrize(labels=["scc"], opts=["be", "admin", "sync"])
def test_scopy(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="Linux Block layer does not support simple-copy")
    if be_opts["be"] == "linux" and be_opts["sync"] in ["block", "psync"]:
        pytest.skip(reason="Linux Block layer does not support simple-copy")

    err, _ = cijoe.run(f"xnvme_tests_scc scopy {cli_args}")
    assert not err


@xnvme_parametrize(labels=["scc"], opts=["be", "admin", "sync"])
def test_scopy_clear(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="Linux Block layer does not support simple-copy")
    if be_opts["be"] == "linux" and be_opts["sync"] in ["block", "psync"]:
        pytest.skip(reason="Linux Block layer does not support simple-copy")

    err, _ = cijoe.run(f"xnvme_tests_scc scopy {cli_args} --clear")
    assert not err


@xnvme_parametrize(labels=["scc"], opts=["be", "admin", "sync"])
def test_scopy_msrc(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="Linux Block layer does not support simple-copy")
    if be_opts["be"] == "linux" and be_opts["sync"] in ["block", "psync"]:
        pytest.skip(reason="Linux Block layer does not support simple-copy")

    err, _ = cijoe.run(f"xnvme_tests_scc scopy-msrc {cli_args}")
    assert not err


@xnvme_parametrize(labels=["scc"], opts=["be", "admin", "sync"])
def test_scopy_msrc_clear(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="Linux Block layer does not support simple-copy")
    if be_opts["be"] == "linux" and be_opts["sync"] in ["block", "psync"]:
        pytest.skip(reason="Linux Block layer does not support simple-copy")

    err, _ = cijoe.run(f"xnvme_tests_scc scopy-msrc {cli_args} --clear")
    assert not err
