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
    err, _ = cijoe.run(f"xnvme example single-async-efd read {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "async"])
def test_read_queue_init_fails(cijoe, device, be_opts, cli_args):
    """Exits with the error of xnvme_queue_init(), with the device still open"""

    if be_opts["async"] != "thrpool":
        pytest.skip(
            reason=f"[async={be_opts['async']}] Fails its queue init only on thrpool"
        )

    # A thread pool of no threads makes xnvme_queue_init() return -EINVAL
    err, _ = cijoe.run(
        "XNVME_BE_THRPOOL_ASYNC_NTHREADS=0 "
        f"xnvme example single-async-efd read {cli_args}"
    )
    assert err and err < 128, f"exit status {err}, not the error of the queue init"
