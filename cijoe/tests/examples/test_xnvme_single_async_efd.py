import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "async"])
def test_read(cijoe, device, be_opts, cli_args):
    if be_opts["be"] not in ["vfio", "linux"]:
        pytest.skip(reason=f"[be={be_opts['be']}] Eventfd not supported")
    if be_opts["be"] in ["linux"] and be_opts["async"] not in [
        "io_uring",
        "io_uring_cmd",
    ]:
        pytest.skip(
            reason=f"[be={be_opts['be']}, async={be_opts['async']}] Eventfd not supported"
        )
    err, _ = cijoe.run(f"xnvme_single_async_efd read {cli_args}")
    assert not err
