import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(["dev"], opts=["be", "admin", "async"])
def test_init_term(cijoe, device, be_opts, cli_args):
    if be_opts["be"] in ("upcie", "upcie-cuda", "upcie-hip"):
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
    """

    for qdepth in [8, 64]:
        err, _ = cijoe.run(
            f"xnvme_tests_async_intf init_io_term {cli_args} "
            f"--count 4 --qdepth {qdepth}"
        )
        assert not err


@xnvme_parametrize(["dev"], opts=["be", "admin", "async"])
def test_init_io_term_p2p_cq_mirror(cijoe, device, be_opts, cli_args):
    """Queues that complete into GPU memory, mirrored by a GPU kernel"""

    if be_opts["be"] not in ("upcie-cuda", "upcie-hip"):
        pytest.skip(reason="Only the GPU backends complete into GPU memory")

    for qdepth in [8, 64]:
        err, _ = cijoe.run(
            f"xnvme_tests_async_intf init_io_term {cli_args} "
            f"--count 4 --qdepth {qdepth} --p2p-cq-mirror"
        )
        assert not err
