from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "sync", "async", "admin"])
def test_verify(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "sync", "admin"])
def test_verify_sync(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify-sync {cli_args}")
    assert not err


# async=[posix],sync=[psync]: iovec not implemented; admin=[driverkit]: iovec not implemented; os=[freebsd],sync=[nvme]: FreeBSD does not implement iovec
@xnvme_parametrize(
    labels=["dev"],
    opts=["be", "sync", "async", "admin"],
    exclude={"async": ["posix"], "sync": ["psync"], "admin": ["driverkit"]},
    os_exclude={"freebsd": {"sync": ["nvme"]}},
)
def test_verify_iovec(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args} --vec-cnt 4")

    if be_opts["admin"] == "spdk" and "nosgl" in device["labels"]:
        assert err
    else:
        assert not err


# sync=[psync],admin=[driverkit]: iovec not implemented; os=[freebsd],sync=[nvme]: FreeBSD does not implement iovec
@xnvme_parametrize(
    labels=["dev"],
    opts=["be", "sync", "admin"],
    exclude={"sync": ["psync"], "admin": ["driverkit"]},
    os_exclude={"freebsd": {"sync": ["nvme"]}},
)
def test_verify_sync_iovec(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify-sync {cli_args} --vec-cnt 4")

    if be_opts["admin"] == "spdk" and "nosgl" in device["labels"]:
        assert err
    else:
        assert not err


@xnvme_parametrize(labels=["bdev"], opts=["be", "sync", "async", "admin"])
def test_verify_direct(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args} --direct 1")
    assert not err


@xnvme_parametrize(labels=["bdev"], opts=["be", "sync", "admin"])
def test_verify_sync_direct(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify-sync {cli_args} --direct 1")
    assert not err


# async=[posix],sync=[psync]: iovec not implemented; admin=[driverkit]: iovec not implemented; os=[freebsd],sync=[nvme]: FreeBSD does not implement iovec
@xnvme_parametrize(
    labels=["dev"],
    opts=["be", "sync", "async", "admin"],
    exclude={"async": ["posix"], "sync": ["psync"], "admin": ["driverkit"]},
    os_exclude={"freebsd": {"sync": ["nvme"]}},
)
def test_verify_iovec_direct(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args} --vec-cnt 4 --direct 1")

    if be_opts["admin"] == "spdk" and "nosgl" in device["labels"]:
        assert err
    else:
        assert not err


# sync=[psync],admin=[driverkit]: iovec not implemented; os=[freebsd],sync=[nvme]: FreeBSD does not implement iovec
@xnvme_parametrize(
    labels=["dev"],
    opts=["be", "sync", "admin"],
    exclude={"sync": ["psync"], "admin": ["driverkit"]},
    os_exclude={"freebsd": {"sync": ["nvme"]}},
)
def test_verify_sync_iovec_direct(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(
        f"xnvme_tests_ioworker verify-sync {cli_args} --vec-cnt 4 --direct 1"
    )

    if be_opts["admin"] == "spdk" and "nosgl" in device["labels"]:
        assert err
    else:
        assert not err
