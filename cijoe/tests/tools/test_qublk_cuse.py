"""
qublk's CUSE ioctl mimic (on by default; --no-cuse disables it) answers
the Linux kernel NVMe driver's ioctl() interface on /dev/ublkb<dev_id>-ctl.
Each case sends the same operation through both nvme-cli and an xNVMe
command-line tool: 'nvme id-ctrl'/'xnvme idfy-ctrlr' and 'nvme
smart-log'/'xnvme log-health' go through NVME_IOCTL_ADMIN_CMD, and 'nvme
read'/'nvme write'/'nvme flush' go through NVME_IOCTL_IO_CMD. Cases for
further ioctls are added here as lib/xnvme_cuse.c grows to answer them.

The xNVMe side's --be is fixed to 'nil': it is lib/xnvme_cuse.c being
exercised here, not the server's own backend (that one is be_opts['be'],
same as every other qublk test-case), and 'nil' reaches the same
NVMe-ioctl code as the other Linux presets without pulling in io_uring or
libaio.
"""

import pytest

from ..conftest import require_nvme_cli, xnvme_parametrize
from .qublk_session import (
    UBLK_CTL,
    qublk_session,
    qublk_teardown,
    require_cuse,
    require_ublk,
)

CTL_BE = "nil"


@pytest.fixture(autouse=True)
def qublk_cuse_cleanup(cijoe):
    """Skip when ublk, the CUSE mimic or nvme-cli are unusable; leave nothing behind"""

    require_ublk(cijoe)
    require_cuse(cijoe)
    require_nvme_cli(cijoe)

    yield

    qublk_teardown(cijoe)


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_idfy_ctrlr(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [
            f"nvme id-ctrl {UBLK_CTL}",
            f"xnvme idfy-ctrlr {UBLK_CTL} --be {CTL_BE}",
        ],
        ctl=UBLK_CTL,
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_log_health(cijoe, device, be_opts, cli_args):
    if be_opts["admin"] == "block":
        pytest.skip(reason="[admin=block] does not implement health-log")
    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason="[be=ramdisk] does not implement health-log")

    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [
            # No '-n': this drive's controller declines SMART/Health scoped to a
            # namespace, the same reason 'xnvme log-health' defaults to the broadcast nsid
            f"nvme smart-log {UBLK_CTL}",
            f"xnvme log-health {UBLK_CTL} --be {CTL_BE}",
        ],
        ctl=UBLK_CTL,
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_read(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [
            # nvme-cli needs an explicit --data-size; look up the LBA size first, captured
            # into a variable so 'set -e' catches a failing 'lblk info', not awk's exit status
            f"info=$(lblk info {UBLK_CTL} --be {CTL_BE})",
            "lba=$(echo \"$info\" | awk '/lba_nbytes:/{print $2}')",
            "rbuf=$(mktemp)",
            f"nvme read {UBLK_CTL} -n 1 -s 0 -c 0 -z $lba -o binary > $rbuf",
            "hexdump -C $rbuf | head",
            "rm -f $rbuf",
        ],
        ctl=UBLK_CTL,
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_write(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [
            f"info=$(lblk info {UBLK_CTL} --be {CTL_BE})",
            "lba=$(echo \"$info\" | awk '/lba_nbytes:/{print $2}')",
            "wbuf=$(mktemp)",
            "dd if=/dev/urandom of=$wbuf bs=$lba count=1 2>/dev/null",
            f"nvme write {UBLK_CTL} -n 1 -s 0 -c 0 -z $lba -d $wbuf",
            "rm -f $wbuf",
        ],
        ctl=UBLK_CTL,
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_flush(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [
            f"nvme flush {UBLK_CTL} -n 1",
        ],
        ctl=UBLK_CTL,
    )
    assert not err
