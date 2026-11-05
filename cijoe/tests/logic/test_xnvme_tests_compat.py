from ..conftest import xnvme_parametrize


def test_deprecated(cijoe):
    err, _ = cijoe.run("xnvme_tests_compat deprecated")
    assert not err


def test_deprecated_znd(cijoe):
    err, _ = cijoe.run("xnvme_tests_compat deprecated-znd")
    assert not err


def test_null_args(cijoe):
    err, _ = cijoe.run("xnvme_tests_compat null-args")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_deprecated_queue(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_compat deprecated-queue {cli_args}")
    assert not err
