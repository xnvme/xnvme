"""
NVME_IOCTL_ADMIN64_CMD, qublk's CUSE ioctl mimic's 64-bit admin passthru,
has no nvme-cli equivalent to pair it with either: nvme-cli and the xNVMe
CLI both always issue the 32-bit NVME_IOCTL_ADMIN_CMD.
'xnvme_tests_cuse_admin64_identify' (tests/cuse_admin64_identify.c) is built
for exactly this: it issues the raw ioctl() itself. Like test_qublk_cuse_vec.py,
this does not require nvme-cli.
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


@pytest.fixture(autouse=True)
def qublk_cuse_admin64_cleanup(cijoe):
    """Skip when ublk or the CUSE mimic are unusable; leave nothing behind"""

    require_ublk(cijoe)
    require_cuse(cijoe)

    yield

    qublk_teardown(cijoe)


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_idfy_ctrlr64(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [
            f"xnvme_tests_cuse_admin64_identify identify64 {UBLK_CTL}",
        ],
        ctl=UBLK_CTL,
    )
    assert not err
