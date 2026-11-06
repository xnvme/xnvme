from ..conftest import xnvme_parametrize
from .test_xnvme_tests_map import skip_unless_registers_host_memory


def test_deprecated(cijoe):
    err, _ = cijoe.run("xnvme_tests_compat deprecated")
    assert not err


def test_null_args(cijoe):
    err, _ = cijoe.run("xnvme_tests_compat null-args")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_deprecated_queue(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_compat deprecated-queue {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_deprecated_mem(cijoe, device, be_opts, cli_args):
    skip_unless_registers_host_memory(cijoe, device, be_opts)

    err, _ = cijoe.run(f"xnvme_tests_compat deprecated-mem {cli_args}")
    assert not err
