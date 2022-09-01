import pytest

from ..conftest import XnvmeDriver, xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_open(cijoe, device, be_opts, cli_args):
    pytest.skip(reason="Currently not implemented, comments this out after merge.")

    err, _ = cijoe.run(f"xnvme_tests_enum open --count 4 --be {be_opts['be']}")
    assert not err


def test_open_all_be(cijoe):

    XnvmeDriver.kernel_attach(cijoe)
    err, _ = cijoe.run("xnvme_tests_enum open --count 4")
    assert not err

    XnvmeDriver.kernel_detach(cijoe)
    err, _ = cijoe.run("xnvme_tests_enum open --count 4")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_multi(cijoe, device, be_opts, cli_args):
    pytest.skip(reason="Currently not implemented, comments this out after merge.")

    err, _ = cijoe.run(f"xnvme_tests_enum multi --count 4 --be {be_opts['be']}")
    assert not err


def test_multi_all_be(cijoe):

    XnvmeDriver.kernel_attach(cijoe)
    err, _ = cijoe.run("xnvme_tests_enum multi --count 4")
    assert not err

    XnvmeDriver.kernel_detach(cijoe)
    err, _ = cijoe.run("xnvme_tests_enum multi --count 4")
    assert not err
