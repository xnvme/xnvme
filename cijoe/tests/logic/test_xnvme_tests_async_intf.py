import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(["dev"], opts=["be", "admin", "async"])
def test_init_term(cijoe, device, be_opts, cli_args):
    if be_opts["be"] in ("upcie", "upcie-cuda"):
        pytest.skip(
            reason=f"[be={be_opts['be']}] This test requires memory beyond what is available"
        )
    qdepth = 64
    for count in [1, 2, 4, 8, 16, 32, 64, 128]:
        err, _ = cijoe.run(
            f"xnvme_tests_async_intf init_term {cli_args} "
            f"--count {count} --qdepth {qdepth}"
        )
        assert not err


@xnvme_parametrize(["dev"], opts=["be", "admin", "async"])
def test_init_io_term(cijoe, device, be_opts, cli_args):
    """
    One queue at a time, so unlike test_init_term this runs on upcie as well.

    The --cq-gpu variant needs a GPU and is not run here.
    """

    for qdepth in [8, 64]:
        err, _ = cijoe.run(
            f"xnvme_tests_async_intf init_io_term {cli_args} "
            f"--count 4 --qdepth {qdepth}"
        )
        assert not err
