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
