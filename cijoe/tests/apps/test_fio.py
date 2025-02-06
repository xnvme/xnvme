from pathlib import Path

import pytest

from ..conftest import xnvme_parametrize
from .fio_fancy import fio_fancy


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "sync", "async", "mem"])
def test_fio_engine(cijoe, device, be_opts, cli_args):
    """
    The construction of the fio-invocation is done in 'fio_fancy'
    """
    is_windows = be_opts["mem"] == "windows" or be_opts["be"] == "windows"
    fio_output_fpath = Path("C:/tmp" if is_windows else "/tmp") / "fio-output.txt"

    size = "64M"
    # size = "1G"

    env = {}

    hugepage_path = cijoe.getconf("hugetlbfs.mount_point", "")
    if hugepage_path:
        env["XNVME_HUGETLB_PATH"] = hugepage_path

    err, _ = fio_fancy(
        cijoe,
        fio_output_fpath,
        "verify",
        "xnvme",
        device,
        be_opts,
        {},
        {"size": size},
        env=env,
    )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "sync", "async", "mem"])
def test_fio_engine_iov(cijoe, device, be_opts, cli_args):
    """
    The construction of the fio-invocation is done in 'fio_fancy'
    """

    if be_opts["async"] == "posix":
        pytest.skip(reason="[async=posix] does not implement iovec")
    if be_opts["be"] == "linux" and be_opts["sync"] == "psync":
        pytest.skip(reason="[be=linux] and [sync=psync] does not implement iovec")
    if be_opts["be"] == "fbsd" and be_opts["sync"] == "nvme":
        pytest.skip(reason="[be=fbsd] and [sync=nvme] does not implement iovec")
    if be_opts["be"] == "driverkit":
        pytest.skip(reason="[be=driverkit] does not implement iovec")

    is_windows = be_opts["mem"] == "windows" or be_opts["be"] == "windows"
    fio_output_fpath = Path("C:/tmp" if is_windows else "/tmp") / "fio-output.txt"

    size = "64M"
    # size = "1G"

    env = {}

    hugepage_path = cijoe.getconf("hugetlbfs.mount_point", "")
    if hugepage_path:
        env["XNVME_HUGETLB_PATH"] = hugepage_path

    err, _ = fio_fancy(
        cijoe,
        fio_output_fpath,
        "verify",
        "xnvme",
        device,
        be_opts,
        {},
        {"size": size, "xnvme_iovec": 1},
        env=env,
    )
    assert not err


@xnvme_parametrize(labels=["zns", "bdev"], opts=["be", "admin", "sync", "async", "mem"])
def test_fio_engine_zns(cijoe, device, be_opts, cli_args):
    """
    The construction of the fio-invocation is done in 'fio_fancy'
    """
    if be_opts["be"] == "fbsd":
        pytest.skip(reason="Freebsd kernel doesn't support zns")
    if be_opts["be"] == "linux" and be_opts["sync"] in ["psync", "block"]:
        pytest.skip(reason="[sync=psync,block] does not support mgmt. send/receive")
    if be_opts["be"] == "linux" and be_opts["async"] in ["thrpool"]:
        pytest.skip(reason="[async=thrpool] gives Zone Invalid Write")

    is_windows = be_opts["mem"] == "windows" or be_opts["be"] == "windows"
    fio_output_fpath = Path("C:/tmp" if is_windows else "/tmp") / "fio-output.txt"

    size = "64M"
    # size = "1G"

    env = {}

    hugepage_path = cijoe.getconf("hugetlbfs.mount_point", "")
    if hugepage_path:
        env["XNVME_HUGETLB_PATH"] = hugepage_path

    err, _ = fio_fancy(
        cijoe,
        fio_output_fpath,
        "verify",
        "xnvme",
        device,
        be_opts,
        {},
        {"size": size, "zonemode": "zbd"},
        env=env,
    )
    assert not err


@xnvme_parametrize(labels=["fdp"], opts=["be", "admin", "sync", "async", "mem"])
def test_fio_engine_fdp(cijoe, device, be_opts, cli_args):
    """
    The construction of the fio-invocation is done in 'fio_fancy'
    """
    if be_opts["be"] == "linux" and "cdev" not in device["labels"]:
        pytest.skip(reason="FIO requires a char device with [be=linux]")
    if be_opts["be"] == "fbsd" and be_opts["sync"] in ["psync"]:
        pytest.skip(reason="[sync=psync] cannot do write with directives")

    is_windows = be_opts["mem"] == "windows" or be_opts["be"] == "windows"
    fio_output_fpath = Path("C:/tmp" if is_windows else "/tmp") / "fio-output.txt"

    size = "64M"
    # size = "1G"

    env = {}

    hugepage_path = cijoe.getconf("hugetlbfs.mount_point", "")
    if hugepage_path:
        env["XNVME_HUGETLB_PATH"] = hugepage_path

    err, _ = fio_fancy(
        cijoe,
        fio_output_fpath,
        "verify",
        "xnvme",
        device,
        be_opts,
        {},
        {"size": size, "fdp": 1, "fdp_pli": "4,5"},
        env=env,
    )
    assert not err
