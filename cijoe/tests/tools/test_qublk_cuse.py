"""
qublk's NVMe-driver ioctl() mimic on /dev/ublkb<dev_id>-nvme

The xNVMe tools use --be nil: what is under test is lib/cuse/xnvme_cuse.c, not
the backend qublk serves the device with.
"""

import pytest

from ..conftest import require_nvme_cli, xnvme_parametrize
from .qublk_session import (
    UBLK_NVME,
    is_userspace_nvme,
    qublk_session,
    qublk_teardown,
    require_cuse,
    require_ublk,
)

CUSE_BE = "nil"


@pytest.fixture(autouse=True)
def qublk_cuse_cleanup(cijoe, request):
    """Skip unless the -nvme device, ublk, CUSE and nvme-cli are all usable; clean up"""

    be = request.node.callspec.params["be_opts"]["be"]
    if not is_userspace_nvme(be):
        pytest.skip(f"[be={be}] gets no /dev/ublkb<N>-nvme, see is_userspace_nvme()")

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
            f"nvme id-ctrl {UBLK_NVME}",
            f"xnvme idfy-ctrlr {UBLK_NVME} --be {CUSE_BE}",
        ],
        nvme=UBLK_NVME,
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_log_health(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [
            # No '-n': this drive's controller declines SMART/Health scoped to a
            # namespace, the same reason 'xnvme log-health' defaults to the broadcast nsid
            f"nvme smart-log {UBLK_NVME}",
            f"xnvme log-health {UBLK_NVME} --be {CUSE_BE}",
        ],
        nvme=UBLK_NVME,
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
            f"info=$(lblk info {UBLK_NVME} --be {CUSE_BE})",
            "lba=$(echo \"$info\" | awk '/lba_nbytes:/{print $2}')",
            "rbuf=$(mktemp)",
            f"nvme read {UBLK_NVME} -n 1 -s 0 -c 0 -z $lba -o binary > $rbuf",
            "hexdump -C $rbuf | head",
            "rm -f $rbuf",
            f"lblk read {UBLK_NVME} --be {CUSE_BE} --slba 0x0 --nlb 0",
        ],
        nvme=UBLK_NVME,
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_write(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [
            f"info=$(lblk info {UBLK_NVME} --be {CUSE_BE})",
            "lba=$(echo \"$info\" | awk '/lba_nbytes:/{print $2}')",
            "wbuf=$(mktemp)",
            "dd if=/dev/urandom of=$wbuf bs=$lba count=1 2>/dev/null",
            f"nvme write {UBLK_NVME} -n 1 -s 0 -c 0 -z $lba -d $wbuf",
            "rm -f $wbuf",
            f"lblk write {UBLK_NVME} --be {CUSE_BE} --slba 0x0 --nlb 0",
        ],
        nvme=UBLK_NVME,
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_flush(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [
            f"nvme flush {UBLK_NVME} -n 1",
            f"xnvme pioc {UBLK_NVME} --be {CUSE_BE} --opcode 0x0 --nsid 1",
        ],
        nvme=UBLK_NVME,
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_idfy_ctrlr64(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [
            f"xnvme_tests_cuse_admin64_identify identify64 {UBLK_NVME}",
        ],
        nvme=UBLK_NVME,
    )
    assert not err
