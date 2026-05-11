import pytest

from ..conftest import get_osname, xnvme_cli_args, xnvme_parametrize


def test_enum(cijoe):
    err, _ = cijoe.run("zoned enum")
    assert not err


@xnvme_parametrize(labels=["zns"], opts=["be", "admin"])
def test_info(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"zoned info {cli_args}")
    assert not err


@xnvme_parametrize(labels=["zns", "changes_log"], opts=["be", "admin"])
def test_changes(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"zoned changes {cli_args}")
    assert not err


@xnvme_parametrize(labels=["zns"], opts=["be", "admin"])
def test_idfy_ctrlr(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"zoned idfy-ctrlr {cli_args}")
    assert not err


@xnvme_parametrize(labels=["zns"], opts=["be", "admin"])
def test_idfy_ns(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"zoned idfy-ns {cli_args}")
    assert not err


# sync=[psync,block]: ENOSYS, cannot do mgmt send/receive
@xnvme_parametrize(
    labels=["zns"], opts=["be", "admin", "sync"], exclude={"sync": ["psync", "block"]}
)
def test_append(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")

    admin_opts = {k: be_opts[k] for k in ["be", "admin"]}
    admin_args = xnvme_cli_args(device, admin_opts)

    nlb = "1"
    slba = "0x0"

    err, _ = cijoe.run(f"zoned mgmt-reset {admin_args} --slba {slba}")
    assert not err

    for _ in range(3):
        err, _ = cijoe.run(f"zoned append {cli_args} --slba {slba} --nlb {nlb}")
        assert not err


# sync=[psync]: ENOSYS, psync cannot do mgmt send/receive
@xnvme_parametrize(
    labels=["zns"], opts=["be", "admin", "sync"], exclude={"sync": ["psync"]}
)
def test_report_all(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")

    err, _ = cijoe.run(f"zoned report {cli_args}")
    assert not err


# sync=[psync]: ENOSYS, psync cannot do mgmt send/receive
@xnvme_parametrize(
    labels=["zns"], opts=["be", "admin", "sync"], exclude={"sync": ["psync"]}
)
def test_report_limit(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")

    err, _ = cijoe.run(f"zoned report {cli_args} --slba 0x0 --limit 1")
    assert not err


# sync=[psync]: ENOSYS, psync cannot do mgmt send/receive
@xnvme_parametrize(
    labels=["zns"], opts=["be", "admin", "sync"], exclude={"sync": ["psync"]}
)
def test_report_single(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")

    err, _ = cijoe.run(f"zoned report {cli_args} --slba 0x26400 --limit 1")
    assert not err


# sync=[psync]: ENOSYS, psync cannot do mgmt send/receive
@xnvme_parametrize(
    labels=["zns"], opts=["be", "admin", "sync"], exclude={"sync": ["psync"]}
)
def test_report_some(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")

    err, _ = cijoe.run(f"zoned report {cli_args} --slba 0x1dc00 --limit 10")
    assert not err


# sync=[psync,block]: ENOSYS, cannot do mgmt send/receive
@xnvme_parametrize(
    labels=["zns"], opts=["be", "admin", "sync"], exclude={"sync": ["psync", "block"]}
)
def test_read(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"zoned read {cli_args} --slba 0x0 --nlb 0")
    assert not err


# sync=[psync,block]: ENOSYS, cannot do mgmt send/receive
@xnvme_parametrize(
    labels=["zns"], opts=["be", "admin", "sync"], exclude={"sync": ["psync", "block"]}
)
def test_reset_report_write_report(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")

    slba = "0x0"
    nlb = "0"
    limit = "0"

    err, _ = cijoe.run(f"zoned mgmt-reset {cli_args} --slba {slba}")
    assert not err

    err, _ = cijoe.run(f"zoned report {cli_args} --slba {slba} --limit {limit}")
    assert not err

    err, _ = cijoe.run(f"zoned write {cli_args} --slba {slba} --nlb {nlb}")
    assert not err

    err, _ = cijoe.run(f"zoned report {cli_args} --slba {slba} --limit {limit}")
    assert not err


# sync=[psync,block]: ENOSYS, cannot do mgmt send/receive
@xnvme_parametrize(
    labels=["zns"], opts=["be", "admin", "sync"], exclude={"sync": ["psync", "block"]}
)
def test_reset_open_report(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")

    slba = "0x0"
    limit = "1"

    err, _ = cijoe.run(f"zoned mgmt-reset {cli_args} --slba {slba}")
    assert not err

    err, _ = cijoe.run(f"zoned mgmt-open {cli_args} --slba {slba}")
    assert not err

    err, _ = cijoe.run(f"zoned report {cli_args} --slba {slba} --limit {limit}")
    assert not err
