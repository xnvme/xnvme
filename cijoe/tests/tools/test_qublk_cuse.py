"""
qublk's CUSE ioctl mimic (on by default; --no-cuse disables it) answers
the Linux kernel NVMe driver's ioctl() interface on /dev/ublkb<dev_id>-ctl.
This first case only confirms the character device itself comes up; cases
exercising specific ioctls are added here as lib/xnvme_cuse.c grows to
answer them.
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
def qublk_cuse_cleanup(cijoe):
    """Skip when ublk or the CUSE mimic are unusable; leave no daemon or device behind"""

    require_ublk(cijoe)
    require_cuse(cijoe)

    yield

    qublk_teardown(cijoe)


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_ctl_device_appears(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [],
        ctl=UBLK_CTL,
    )
    assert not err
