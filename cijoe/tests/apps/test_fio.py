from pathlib import Path

import pytest

from ..conftest import get_osname, xnvme_parametrize
from .fio_fancy import fio_fancy

FIO_OUTPUT_FPATH = (
    Path("C:/tmp" if get_osname() == "windows" else "/tmp") / "fio-output.txt"
)


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "sync", "async", "mem"])
def test_fio_engine(cijoe, device, be_opts, cli_args):
    """
    The construction of the fio-invocation is done in 'fio_fancy'
    """
    if be_opts["mem"] == "upcie-cuda":
        pytest.skip(
            reason="[mem=upcie-cuda] fio xNVMe ioengine does not support CUDA memory"
        )

    size = "64M"
    # size = "1G"

    err, _ = fio_fancy(
        cijoe,
        FIO_OUTPUT_FPATH,
        "verify",
        "xnvme",
        device,
        be_opts,
        {},
        {"size": size},
    )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "sync", "async", "mem"])
def test_fio_engine_iov(cijoe, device, be_opts, cli_args):
    """
    The construction of the fio-invocation is done in 'fio_fancy'
    """

    if be_opts["async"] == "posix":
        pytest.skip(reason="[async=posix] does not implement iovec")
    if be_opts["sync"] == "psync":
        pytest.skip(reason="[sync=psync] does not implement iovec")
    if get_osname() == "freebsd" and be_opts["sync"] == "nvme":
        pytest.skip(reason="[sync=nvme] on FreeBSD does not implement iovec")
    if be_opts["admin"] == "driverkit":
        pytest.skip(reason="[admin=driverkit] does not implement iovec")
    if be_opts["mem"] == "upcie-cuda":
        pytest.skip(
            reason="[mem=upcie-cuda] fio xNVMe ioengine does not support CUDA memory"
        )

    size = "64M"
    # size = "1G"

    err, _ = fio_fancy(
        cijoe,
        FIO_OUTPUT_FPATH,
        "verify",
        "xnvme",
        device,
        be_opts,
        {},
        {"size": size, "xnvme_iovec": 1},
    )
    assert not err


@xnvme_parametrize(labels=["zns", "bdev"], opts=["be", "admin", "sync", "async", "mem"])
def test_fio_engine_zns(cijoe, device, be_opts, cli_args):
    """
    The construction of the fio-invocation is done in 'fio_fancy'
    """
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")
    if be_opts["sync"] in ["psync", "block"]:
        pytest.skip(reason="[sync=psync,block] does not support mgmt. send/receive")
    if be_opts["async"] == "thrpool":
        pytest.skip(reason="[async=thrpool] gives Zone Invalid Write")

    size = "64M"
    # size = "1G"

    err, _ = fio_fancy(
        cijoe,
        FIO_OUTPUT_FPATH,
        "verify",
        "xnvme",
        device,
        be_opts,
        {},
        {"size": size, "zonemode": "zbd"},
    )
    assert not err


@xnvme_parametrize(labels=["fdp"], opts=["be", "admin", "sync", "async", "mem"])
def test_fio_engine_fdp(cijoe, device, be_opts, cli_args):
    """
    The construction of the fio-invocation is done in 'fio_fancy'
    """
    if "cdev" not in device["labels"]:
        pytest.skip(reason="FIO requires a char device")
    if be_opts["sync"] == "psync":
        pytest.skip(reason="[sync=psync] cannot do write with directives")

    size = "64M"
    # size = "1G"

    err, _ = fio_fancy(
        cijoe,
        FIO_OUTPUT_FPATH,
        "verify",
        "xnvme",
        device,
        be_opts,
        {},
        {"size": size, "fdp": 1, "fdp_pli": "4,5"},
    )
    assert not err
