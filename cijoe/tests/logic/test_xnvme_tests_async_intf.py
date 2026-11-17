import pytest

from ..conftest import is_nvmet, xnvme_parametrize


@xnvme_parametrize(["dev"], opts=["be", "admin", "async"])
def test_init_term(cijoe, device, be_opts, cli_args):
    if be_opts["be"] in ("upcie", "upcie-cuda", "upcie-hip"):
        pytest.skip(
            reason=f"[be={be_opts['be']}] This test requires memory beyond what is available"
        )
    qdepth = 64
    for count in [1, 2, 4, 8, 16, 32, 64, 128]:
        err, _ = cijoe.run(
            f"xnvme test async-intf init_term {cli_args} "
            f"--count {count} --qdepth {qdepth}"
        )
        assert not err


@xnvme_parametrize(["dev"], opts=["be", "admin", "async"])
def test_init_exhaust(cijoe, device, be_opts, cli_args):
    """
    A queue past the controller's allocation of Number of Queues fails with -ENOMEM

    Runs on the uPCIe backends only: a queue of the kernel backends is not a
    queue of the controller, and SPDK and libvfn request queues on their own.
    """

    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason=f"[be={be_opts['be']}] Queues are not the controller's")

    err, _ = cijoe.run(f"xnvme test async-intf init_exhaust {cli_args} --qdepth 8")
    assert not err


@xnvme_parametrize(["dev"], opts=["be", "admin", "async"])
def test_init_io_term(cijoe, device, be_opts, cli_args):
    """
    One queue at a time, so unlike test_init_term this runs on upcie as well.
    """

    for qdepth in [8, 64]:
        err, _ = cijoe.run(
            f"xnvme test async-intf init_io_term {cli_args} "
            f"--count 4 --qdepth {qdepth}"
        )
        assert not err


@xnvme_parametrize(["dev"], opts=["be", "admin", "async"])
def test_reap(cijoe, device, be_opts, cli_args):
    """What xnvme_queue_reap() hands back: each command once, with its data"""

    for qdepth in [8, 64]:
        err, _ = cijoe.run(f"xnvme test async-intf reap {cli_args} --qdepth {qdepth}")
        assert not err


@xnvme_parametrize(["dev"], opts=["be", "admin", "async"])
def test_reap_flush(cijoe, device, be_opts, cli_args):
    """A flush that completes when it is submitted is held for the next reap"""

    if be_opts["async"] != "kqueue":
        pytest.skip(reason="Only kqueue holds a flush for the next reap")

    for qdepth in [8, 64]:
        err, _ = cijoe.run(
            f"xnvme test async-intf reap_flush {cli_args} --qdepth {qdepth}"
        )
        assert not err


@xnvme_parametrize(["dev"], opts=["be", "admin", "async"])
def test_reap_cmd_fail(cijoe, device, be_opts, cli_args):
    """A command that fails is reaped with its status, and is no error of the reap"""

    passes_nvme = be_opts["async"] == "io_uring_cmd" or be_opts["be"] in (
        "spdk",
        "libvfn",
        "upcie",
    )
    if not passes_nvme:
        pytest.skip(reason="The compare needs an interface that passes NVMe commands")
    if is_nvmet(cijoe, device):
        pytest.skip(reason="[nvmet] does not implement compare")

    for qdepth in [1, 8]:
        err, _ = cijoe.run(
            f"xnvme test async-intf reap_cmd_fail {cli_args} --qdepth {qdepth}"
        )
        assert not err


@xnvme_parametrize(["dev"], opts=["be", "admin", "async"])
def test_cb(cijoe, device, be_opts, cli_args):
    """The callback of the queue sees every completion"""

    for qdepth in [8, 64]:
        err, _ = cijoe.run(f"xnvme test async-intf cb {cli_args} --qdepth {qdepth}")
        assert not err


@xnvme_parametrize(["dev"], opts=["be", "admin", "async"])
def test_mode(cijoe, device, be_opts, cli_args):
    """A queue delivers completions only the way it was initialized for"""

    err, _ = cijoe.run(f"xnvme test async-intf mode {cli_args}")
    assert not err


@xnvme_parametrize(["dev"], opts=["be", "admin", "async"])
def test_term_outstanding(cijoe, device, be_opts, cli_args):
    """Tear down a queue with reads outstanding"""

    if be_opts["be"] != "spdk":
        pytest.skip(reason="Only SPDK completes commands while tearing down a queue")

    for qdepth in [8, 64]:
        err, _ = cijoe.run(
            f"xnvme test async-intf term_outstanding {cli_args} --qdepth {qdepth}"
        )
        assert not err


@xnvme_parametrize(["dev"], opts=["be", "admin", "async"])
def test_init_io_term_p2p_cq_mirror(cijoe, device, be_opts, cli_args):
    """Queues that complete into GPU memory, mirrored by a GPU kernel"""

    if be_opts["be"] not in ("upcie-cuda", "upcie-hip"):
        pytest.skip(reason="Only the GPU backends complete into GPU memory")

    for qdepth in [8, 64]:
        err, _ = cijoe.run(
            f"xnvme test async-intf init_io_term {cli_args} "
            f"--count 4 --qdepth {qdepth} --p2p-cq-mirror"
        )
        assert not err
