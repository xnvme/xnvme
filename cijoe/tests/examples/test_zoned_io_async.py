import pytest

from ..conftest import get_osname, xnvme_parametrize


@xnvme_parametrize(labels=["zns"], opts=["be", "admin", "async"])
def test_write(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")
    err, _ = cijoe.run(f"zoned_io_async write {cli_args}")
    assert not err


# admin=[block]: block layer does not support append; async=[io_uring,libaio,posix]: these async backends do not support append
@xnvme_parametrize(
    labels=["zns"],
    opts=["be", "admin", "async"],
    exclude={"admin": ["block"], "async": ["io_uring", "libaio", "posix"]},
)
def test_append(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")

    err, _ = cijoe.run(f"zoned_io_async append {cli_args}")
    assert not err


@xnvme_parametrize(labels=["zns"], opts=["be", "admin", "async"])
def test_read(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")
    err, _ = cijoe.run(f"zoned_io_async read {cli_args}")
    assert not err
