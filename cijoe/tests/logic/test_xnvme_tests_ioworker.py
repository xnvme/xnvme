import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "sync", "async", "admin"])
def test_verify(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "sync", "admin"])
def test_verify_sync(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_ioworker verify-sync {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "sync", "async", "admin"])
def test_verify_iovec(cijoe, device, be_opts, cli_args):
    if be_opts["async"] == "posix":
        pytest.skip(reason="[async=posix] does not implement iovec")
    if be_opts["be"] == "linux" and be_opts["sync"] == "psync":
        pytest.skip(reason="[be=linux] and [sync=psync] does not implement iovec")
    if be_opts["be"] == "fbsd" and be_opts["sync"] == "nvme":
        pytest.skip(reason="[be=fbsd] and [sync=nvme] does not implement iovec")
    if be_opts["be"] == "driverkit":
        pytest.skip(reason="[be=driverkit] does not implement iovec")

    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args} --vec-cnt 4")

    if be_opts["be"] == "spdk" and "nosgl" in device["labels"]:
        assert err
    else:
        assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "sync", "admin"])
def test_verify_sync_iovec(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["sync"] == "psync":
        pytest.skip(reason="[be=linux] and [sync=psync] does not implement iovec")
    if be_opts["be"] == "fbsd" and be_opts["sync"] == "nvme":
        pytest.skip(reason="[be=fbsd] and [sync=nvme] does not implement iovec")
    if be_opts["be"] == "driverkit":
        pytest.skip(reason="[be=driverkit] does not implement iovec")

    err, _ = cijoe.run(f"xnvme_tests_ioworker verify-sync {cli_args} --vec-cnt 4")

    if be_opts["be"] == "spdk" and "nosgl" in device["labels"]:
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


@xnvme_parametrize(labels=["dev"], opts=["be", "sync", "async", "admin"])
def test_verify_iovec_direct(cijoe, device, be_opts, cli_args):
    if be_opts["async"] == "posix":
        pytest.skip(reason="[async=posix] does not implement iovec")
    if be_opts["be"] == "linux" and be_opts["sync"] == "psync":
        pytest.skip(reason="[be=linux] and [sync=psync] does not implement iovec")
    if be_opts["be"] == "fbsd" and be_opts["sync"] == "nvme":
        pytest.skip(reason="[be=fbsd] and [sync=nvme] does not implement iovec")
    if be_opts["be"] == "driverkit":
        pytest.skip(reason="[be=driverkit] does not implement iovec")

    err, _ = cijoe.run(f"xnvme_tests_ioworker verify {cli_args} --vec-cnt 4 --direct 1")

    if be_opts["be"] == "spdk" and "nosgl" in device["labels"]:
        assert err
    else:
        assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "sync", "admin"])
def test_verify_sync_iovec_direct(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["sync"] == "psync":
        pytest.skip(reason="[be=linux] and [sync=psync] does not implement iovec")
    if be_opts["be"] == "fbsd" and be_opts["sync"] == "nvme":
        pytest.skip(reason="[be=fbsd] and [sync=nvme] does not implement iovec")
    if be_opts["be"] == "driverkit":
        pytest.skip(reason="[be=driverkit] does not implement iovec")

    err, _ = cijoe.run(
        f"xnvme_tests_ioworker verify-sync {cli_args} --vec-cnt 4 --direct 1"
    )

    if be_opts["be"] == "spdk" and "nosgl" in device["labels"]:
        assert err
    else:
        assert not err
