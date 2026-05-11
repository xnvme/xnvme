import pytest

from ..conftest import get_osname, xnvme_parametrize


# sync=[psync]: ENOSYS, psync cannot do mgmt send/receive
@xnvme_parametrize(
    labels=["zns"], opts=["be", "admin", "sync"], exclude={"sync": ["psync"]}
)
def test_write(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")

    err, _ = cijoe.run(f"zoned_io_sync write {cli_args}")
    assert not err


# admin=[block]: block layer does not support append; sync=[block,psync]: ENOSYS
@xnvme_parametrize(
    labels=["zns"],
    opts=["be", "admin", "sync"],
    exclude={"admin": ["block"], "sync": ["block", "psync"]},
)
def test_append(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")

    err, _ = cijoe.run(f"zoned_io_sync append {cli_args}")
    assert not err


# sync=[psync]: ENOSYS, psync cannot do mgmt send/receive
@xnvme_parametrize(
    labels=["zns"], opts=["be", "admin", "sync"], exclude={"sync": ["psync"]}
)
def test_read(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")

    err, _ = cijoe.run(f"zoned_io_sync read {cli_args}")
    assert not err
