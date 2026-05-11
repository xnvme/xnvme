from pathlib import Path

import pytest

from ..conftest import get_osname, xnvme_parametrize
from .fio_fancy import fio_fancy

FIO_OUTPUT_FPATH = (
    Path("C:/tmp" if get_osname() == "windows" else "/tmp") / "fio-output.txt"
)


# mem=[upcie-cuda]: fio engine does not support CUDA memory
@xnvme_parametrize(
    labels=["dev"],
    opts=["be", "admin", "sync", "async", "mem"],
    exclude={"mem": ["upcie-cuda"]},
)
def test_fio_engine(cijoe, device, be_opts, cli_args):
    """
    The construction of the fio-invocation is done in 'fio_fancy'
    """
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


# async=[posix],sync=[psync]: iovec not implemented; admin=[driverkit]: iovec not implemented; mem=[upcie-cuda]: CUDA memory not supported; os=[freebsd],sync=[nvme]: FreeBSD does not implement iovec
@xnvme_parametrize(
    labels=["dev"],
    opts=["be", "admin", "sync", "async", "mem"],
    exclude={
        "async": ["posix"],
        "sync": ["psync"],
        "admin": ["driverkit"],
        "mem": ["upcie-cuda"],
    },
    os_exclude={"freebsd": {"sync": ["nvme"]}},
)
def test_fio_engine_iov(cijoe, device, be_opts, cli_args):
    """
    The construction of the fio-invocation is done in 'fio_fancy'
    """

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


# sync=[psync,block]: ENOSYS, cannot do mgmt send/receive; async=[thrpool]: thrpool does not support zbd/zns; os=[freebsd],be=[kqueue,thrpool,emu]: FreeBSD kernel doesn't support ZNS
@xnvme_parametrize(
    labels=["zns", "bdev"],
    opts=["be", "admin", "sync", "async", "mem"],
    exclude={"sync": ["psync", "block"], "async": ["thrpool"]},
    os_exclude={"freebsd": {"be": ["kqueue", "thrpool", "emu"]}},
)
def test_fio_engine_zns(cijoe, device, be_opts, cli_args):
    """
    The construction of the fio-invocation is done in 'fio_fancy'
    """

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


# sync=[psync]: ENOSYS, psync cannot do mgmt send/receive
@xnvme_parametrize(
    labels=["fdp"],
    opts=["be", "admin", "sync", "async", "mem"],
    exclude={"sync": ["psync"]},
)
def test_fio_engine_fdp(cijoe, device, be_opts, cli_args):
    """
    The construction of the fio-invocation is done in 'fio_fancy'
    """
    if "cdev" not in device["labels"]:
        pytest.skip(reason="FIO requires a char device")

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
