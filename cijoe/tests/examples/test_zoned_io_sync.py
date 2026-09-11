import pytest

from ..conftest import get_osname, xnvme_parametrize, zoned_bdev_reset


@xnvme_parametrize(labels=["zns"], opts=["be", "admin", "sync"])
def test_write(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")
    if be_opts["sync"] == "psync":
        pytest.skip(reason="psync(pread/write) does not support mgmt. send/receive")

    # Buffered writes to a zoned block device are flushed by the page cache with no
    # write-pointer ordering guarantee; bypass it to keep the writes sequential.
    # The example resets the zone through the NVMe admin path where admin=nvme,
    # which the block layer does not see; see zoned_bdev_reset()
    zoned_bdev_reset(cijoe, device)
    err, _ = cijoe.run(f"zoned_io_sync write {cli_args} --direct 1")
    assert not err


@xnvme_parametrize(labels=["zns"], opts=["be", "admin", "sync"])
def test_append(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")
    if be_opts["admin"] in ["block"]:
        pytest.skip(reason="Linux Block layer does not support append")
    if be_opts["sync"] in ["block", "psync"]:
        pytest.skip(reason="Linux Block layer does not support append")

    err, _ = cijoe.run(f"zoned_io_sync append {cli_args}")
    assert not err


@xnvme_parametrize(labels=["zns"], opts=["be", "admin", "sync"])
def test_read(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")
    if be_opts["sync"] == "psync":
        pytest.skip(reason="psync(pread/write) does not support mgmt. send/receive")

    err, _ = cijoe.run(f"zoned_io_sync read {cli_args}")
    assert not err
