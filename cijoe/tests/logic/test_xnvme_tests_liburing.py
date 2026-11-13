import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "async"])
def test_ring_flags(cijoe, device, be_opts, cli_args):
    if be_opts["async"] not in ["io_uring", "io_uring_cmd"]:
        pytest.skip(reason=f"[async={be_opts['async']}] Has no io_uring")

    err, _ = cijoe.run(f"xnvme test liburing ring-flags {cli_args}")
    assert not err
