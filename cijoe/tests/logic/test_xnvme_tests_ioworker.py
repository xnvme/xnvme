import pytest

from ..conftest import get_osname, xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "sync", "async", "admin"])
def test_verify(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args}")
    assert not err

    # small nlb
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args} --nlb 1")
    assert not err

    # large nlb
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args} --nlb 7")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "sync", "admin"])
def test_verify_sync(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify-sync {cli_args}")
    assert not err

    # small nlb
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify-sync {cli_args} --nlb 1")
    assert not err

    # large nlb
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify-sync {cli_args} --nlb 7")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "sync", "async", "admin"])
def test_verify_iovec(cijoe, device, be_opts, cli_args):
    if be_opts["async"] == "posix":
        pytest.skip(reason="[async=posix] does not implement iovec")
    if be_opts["sync"] == "psync":
        pytest.skip(reason="[sync=psync] does not implement iovec")
    if get_osname() == "freebsd" and be_opts["sync"] == "nvme":
        pytest.skip(reason="[sync=nvme] on FreeBSD does not implement iovec")
    if be_opts["admin"] == "driverkit":
        pytest.skip(reason="[admin=driverkit] does not implement iovec")

    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args} --vec-cnt 4")

    if be_opts["admin"] == "spdk" and "nosgl" in device["labels"]:
        assert err
    else:
        assert not err

    # small nlb: sub-page segments require an SGL, rejected on nosgl
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args} --nlb 1 --vec-cnt 4")

    if be_opts["be"] == "spdk" and "nosgl" in device["labels"]:
        assert err
    else:
        assert not err

    # large nlb: page-sized segments are PRP-expressible
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args} --nlb 7 --vec-cnt 4")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "sync", "admin"])
def test_verify_sync_iovec(cijoe, device, be_opts, cli_args):
    if be_opts["sync"] == "psync":
        pytest.skip(reason="[sync=psync] does not implement iovec")
    if get_osname() == "freebsd" and be_opts["sync"] == "nvme":
        pytest.skip(reason="[sync=nvme] on FreeBSD does not implement iovec")
    if be_opts["admin"] == "driverkit":
        pytest.skip(reason="[admin=driverkit] does not implement iovec")

    err, _ = cijoe.run(f"xnvme_tests_ioworker verify-sync {cli_args} --vec-cnt 4")

    if be_opts["admin"] == "spdk" and "nosgl" in device["labels"]:
        assert err
    else:
        assert not err

    # small nlb: sub-page segments require an SGL, rejected on nosgl
    err, _ = cijoe.run(
        f"xnvme_tests_ioworker verify-sync {cli_args} --nlb 1 --vec-cnt 4"
    )

    if be_opts["be"] == "spdk" and "nosgl" in device["labels"]:
        assert err
    else:
        assert not err

    # large nlb: page-sized segments are PRP-expressible
    err, _ = cijoe.run(
        f"xnvme_tests_ioworker verify-sync {cli_args} --nlb 7 --vec-cnt 4"
    )
    assert not err


@xnvme_parametrize(labels=["bdev"], opts=["be", "sync", "async", "admin"])
def test_verify_direct(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args} --direct 1")
    assert not err

    # small nlb
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args} --nlb 1 --direct 1")
    assert not err

    # large nlb
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args} --nlb 7  --direct 1")
    assert not err


@xnvme_parametrize(labels=["bdev"], opts=["be", "sync", "admin"])
def test_verify_sync_direct(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify-sync {cli_args} --direct 1")
    assert not err

    # small nlb
    err, _ = cijoe.run(
        f"xnvme_tests_ioworker verify-sync {cli_args} --nlb 1 --direct 1"
    )
    assert not err

    # large nlb
    err, _ = cijoe.run(
        f"xnvme_tests_ioworker verify-sync {cli_args} --nlb 7  --direct 1"
    )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "sync", "async", "admin"])
def test_verify_iovec_direct(cijoe, device, be_opts, cli_args):
    if be_opts["async"] == "posix":
        pytest.skip(reason="[async=posix] does not implement iovec")
    if be_opts["sync"] == "psync":
        pytest.skip(reason="[sync=psync] does not implement iovec")
    if get_osname() == "freebsd" and be_opts["sync"] == "nvme":
        pytest.skip(reason="[sync=nvme] on FreeBSD does not implement iovec")
    if be_opts["admin"] == "driverkit":
        pytest.skip(reason="[admin=driverkit] does not implement iovec")

    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args} --vec-cnt 4 --direct 1")

    if be_opts["admin"] == "spdk" and "nosgl" in device["labels"]:
        assert err
    else:
        assert not err

    # small nlb: sub-page segments require an SGL, rejected on nosgl
    err, _ = cijoe.run(
        f"xnvme_tests_ioworker verify {cli_args} --nlb 1 --vec-cnt 4 --direct 1"
    )

    if be_opts["be"] == "spdk" and "nosgl" in device["labels"]:
        assert err
    else:
        assert not err

    # large nlb: page-sized segments are PRP-expressible
    err, _ = cijoe.run(
        f"xnvme_tests_ioworker verify {cli_args} --nlb 7 --vec-cnt 4 --direct 1"
    )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "sync", "admin"])
def test_verify_sync_iovec_direct(cijoe, device, be_opts, cli_args):
    if be_opts["sync"] == "psync":
        pytest.skip(reason="[sync=psync] does not implement iovec")
    if get_osname() == "freebsd" and be_opts["sync"] == "nvme":
        pytest.skip(reason="[sync=nvme] on FreeBSD does not implement iovec")
    if be_opts["admin"] == "driverkit":
        pytest.skip(reason="[admin=driverkit] does not implement iovec")

    err, _ = cijoe.run(
        f"xnvme_tests_ioworker verify-sync {cli_args} --vec-cnt 4 --direct 1"
    )

    if be_opts["admin"] == "spdk" and "nosgl" in device["labels"]:
        assert err
    else:
        assert not err

    # small nlb: sub-page segments require an SGL, rejected on nosgl
    err, _ = cijoe.run(
        f"xnvme_tests_ioworker verify-sync {cli_args} --nlb 1 --vec-cnt 4 --direct 1"
    )

    if be_opts["be"] == "spdk" and "nosgl" in device["labels"]:
        assert err
    else:
        assert not err

    # large nlb: page-sized segments are PRP-expressible
    err, _ = cijoe.run(
        f"xnvme_tests_ioworker verify-sync {cli_args} --nlb 7 --vec-cnt 4 --direct 1"
    )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "sync", "async", "admin"])
def test_verify_flush(cijoe, device, be_opts, cli_args):
    # Async-interfaces without an implementation of NVM FLUSH: FreeBSD 'kqueue'
    # and Windows 'ioring' reject it with -ENOSYS, Windows 'iocp'/'iocp_th'
    # handle only the FS FLUSH opcode
    if be_opts["async"] in ["kqueue", "iocp", "iocp_th", "ioring"]:
        pytest.skip(reason=f"[async={be_opts['async']}] does not implement NVM FLUSH")

    # The FUA-writes make this the first fabrics case sustaining I/O beyond the
    # keep-alive timeout, and the spdk backend only services the admin-queue --
    # and thereby keep-alive -- inside admin commands, so the target reaps the
    # controller mid-run and the queue dies with -ENXIO. A backend concern, not
    # a FLUSH one; skip until the backend keeps the connection alive
    if "fabrics" in device["labels"]:
        pytest.skip(reason="[fabrics] spdk backend sends no keep-alive during I/O")

    err, _ = cijoe.run(f"xnvme_tests_ioworker verify-flush {cli_args}")
    assert not err


@xnvme_parametrize(labels=["bdev"], opts=["be", "sync", "async", "admin"])
def test_verify_sqpoll(cijoe, device, be_opts, cli_args):
    if be_opts["async"] != "io_uring":
        pytest.skip(reason="poll_sq (SQPOLL) is io_uring-only")

    for subcmd, args in [
        ("verify", "--poll_sq 1"),
        ("verify", "--poll_sq 1 --vec-cnt 4 --nlb 7"),
        ("verify", "--poll_sq 1 --vec-cnt 8 --nlb 3 --qdepth 8"),
        ("verify-flush", "--poll_sq 1 --vec-cnt 4 --nlb 7"),
    ]:
        err, _ = cijoe.run(f"xnvme_tests_ioworker {subcmd} {cli_args} {args}")
        assert not err


@xnvme_parametrize(labels=["bdev"], opts=["be", "sync", "async", "admin"])
def test_verify_flush_iopoll_rejected(cijoe, device, be_opts, cli_args):
    if be_opts["async"] != "io_uring":
        pytest.skip(reason="poll_io (IOPOLL) is io_uring-only")

    # Control: the same invocation without IOPOLL must succeed, so that the
    # failure below can only be the submission-time rejection
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify-flush {cli_args} --direct 1")
    assert not err

    # IORING_OP_FSYNC has no iopoll-handler; the backend rejects FLUSH on an
    # IOPOLL queue at submission-time, so the run must fail
    err, _ = cijoe.run(
        f"xnvme_tests_ioworker verify-flush {cli_args} --poll_io 1 --direct 1"
    )
    assert err
