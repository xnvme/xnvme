"""
NVME_IOCTL_IO64_CMD_VEC, qublk's CUSE ioctl mimic's vectored passthru, has
no nvme-cli equivalent to pair it with, so it gets its own file: unlike
test_qublk_cuse.py's cases, this one does not require nvme-cli.

The xNVMe side's --be is fixed to 'nil': it is lib/xnvme_cuse.c being
exercised here, not the server's own backend (that one is be_opts['be'],
same as every other qublk test-case), and 'nil' reaches the same
NVMe-ioctl code as the other Linux presets without pulling in io_uring or
libaio.
"""

import pytest

from ..conftest import xnvme_parametrize
from .qublk_session import (
    UBLK_CTL,
    qublk_session,
    qublk_teardown,
    require_cuse,
    require_ublk,
)

CTL_BE = "nil"


@pytest.fixture(autouse=True)
def qublk_cuse_vec_cleanup(cijoe):
    """Skip when ublk or the CUSE mimic are unusable; leave nothing behind"""

    require_ublk(cijoe)
    require_cuse(cijoe)

    yield

    qublk_teardown(cijoe)


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_write_vec(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [
            f"xnvme_file dump-sync-iovec {UBLK_CTL} --be {CTL_BE} --data-nbytes 8192 "
            "--sync nvme --vec-cnt 2",
        ],
        ctl=UBLK_CTL,
    )
    assert not err
