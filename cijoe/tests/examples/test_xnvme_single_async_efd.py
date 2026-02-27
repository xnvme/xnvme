import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "async"])
def test_read(cijoe, device, be_opts, cli_args):
    if be_opts["admin"] != "libvfn" and be_opts["async"] not in [
        "libaio",
        "io_uring",
        "io_uring_cmd",
    ]:
        pytest.skip(reason=f"[async={be_opts['async']}] Eventfd not supported")
    err, _ = cijoe.run(f"xnvme_single_async_efd read {cli_args}")
    assert not err
