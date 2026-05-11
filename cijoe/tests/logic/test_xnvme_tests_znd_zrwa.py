import pytest

from ..conftest import get_osname, xnvme_parametrize


# sync=[psync,block]: ENOSYS, cannot do mgmt send/receive; admin=[block]: same
@xnvme_parametrize(
    labels=["zrwa"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["psync", "block"], "admin": ["block"]},
)
def test_idfy(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_znd_zrwa idfy {cli_args}")
    assert not err


# sync=[psync,block]: ENOSYS, cannot do mgmt send/receive; admin=[block]: same
@xnvme_parametrize(
    labels=["zrwa"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["psync", "block"], "admin": ["block"]},
)
def test_support(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_znd_zrwa support {cli_args}")
    assert not err


# sync=[psync,block]: ENOSYS, cannot do mgmt send/receive; admin=[block]: same
@xnvme_parametrize(
    labels=["zrwa"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["psync", "block"], "admin": ["block"]},
)
def test_open_with_zrwa(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")
    err, _ = cijoe.run(f"xnvme_tests_znd_zrwa open-with-zrwa {cli_args}")
    assert not err


# sync=[psync,block]: ENOSYS, cannot do mgmt send/receive; admin=[block]: same
@xnvme_parametrize(
    labels=["zrwa"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["psync", "block"], "admin": ["block"]},
)
def test_open_without_zrwa(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")
    err, _ = cijoe.run(f"xnvme_tests_znd_zrwa open-without-zrwa {cli_args}")
    assert not err


# sync=[psync,block]: ENOSYS, cannot do mgmt send/receive; admin=[block]: same
@xnvme_parametrize(
    labels=["zrwa"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["psync", "block"], "admin": ["block"]},
)
def test_flush(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")
    err, _ = cijoe.run(f"xnvme_tests_znd_zrwa flush {cli_args}")
    assert not err


# sync=[psync,block]: ENOSYS, cannot do mgmt send/receive; admin=[block]: same
@xnvme_parametrize(
    labels=["zrwa"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["psync", "block"], "admin": ["block"]},
)
def test_flush_explicit(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")
    err, _ = cijoe.run(f"xnvme_tests_znd_zrwa flush-explicit {cli_args}")
    assert not err


# sync=[psync,block]: ENOSYS, cannot do mgmt send/receive; admin=[block]: same
@xnvme_parametrize(
    labels=["zrwa"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["psync", "block"], "admin": ["block"]},
)
def test_flush_implicit(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")
    err, _ = cijoe.run(f"xnvme_tests_znd_zrwa flush-implicit {cli_args}")
    assert not err
