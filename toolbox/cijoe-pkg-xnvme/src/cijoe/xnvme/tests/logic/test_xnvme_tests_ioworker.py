import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "sync", "async", "admin"])
def test_verify(cijoe, device, be_opts, cli_args):

    if be_opts["async"] in ["libaio"]:
        pytest.skip(reason="FIXME: [async=libaio] needs investigation")

    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args}")
    assert not err


@xnvme_parametrize(labels=["bdev"], opts=["be", "sync", "async", "admin"])
@pytest.mark.skip(reason="FIXME: --direct=1 needs investigation")
def test_verify_direct(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args} --direct 1")
    assert not err
