from pathlib import Path

import pytest
from cijoe.fio.wrapper import fio_fancy

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "sync", "async", "mem"])
def test_fio_engine(cijoe, device, be_opts, cli_args):
    """
    The construction of the fio-invocation is done in 'cijoe.fio.wrapper.fio_fancy'
    """

    fio_output_fpath = Path("/tmp/fio-output.txt")

    size = "64M"
    # size = "1G"

    env = {}

    hugepage_path = cijoe.config.options.get("hugetlbfs", {}).get("mount_point", "")
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
    The construction of the fio-invocation is done in 'cijoe.fio.wrapper.fio_fancy'
    """

    if be_opts["async"] == "posix":
        pytest.skip(reason="[async=posix] does not implement iovec")
    if be_opts["be"] == "linux" and be_opts["sync"] == "psync":
        pytest.skip(reason="[be=linux] and [sync=psync] does not implement iovec")
    if be_opts["be"] == "fbsd" and be_opts["sync"] == "nvme":
        pytest.skip(reason="[be=fbsd] and [sync=nvme] does not implement iovec")
    if be_opts["be"] == "driverkit":
        pytest.skip(reason="[be=driverkit] does not implement iovec")

    fio_output_fpath = Path("/tmp/fio-output.txt")

    size = "64M"
    # size = "1G"

    env = {}

    hugepage_path = cijoe.config.options.get("hugetlbfs", {}).get("mount_point", "")
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
