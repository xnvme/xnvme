"""
qublk's NVMe-driver ioctl() mimic on /dev/ublkb<dev_id>-nvme
"""

import pytest

from ..conftest import xnvme_parametrize
from .qublk_session import (
    UBLK_NVME,
    is_userspace_nvme,
    qublk_session,
    qublk_teardown,
    require_cuse,
    require_ublk,
)


@pytest.fixture(autouse=True)
def qublk_cuse_cleanup(cijoe, request):
    """Skip unless the -nvme device, ublk and CUSE are all usable; clean up"""

    be = request.node.callspec.params["be_opts"]["be"]
    if not is_userspace_nvme(be):
        pytest.skip(f"[be={be}] gets no /dev/ublkb<N>-nvme, see is_userspace_nvme()")

    require_ublk(cijoe)
    require_cuse(cijoe)

    yield

    qublk_teardown(cijoe)


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_nvme_device_appears(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [],
        nvme=UBLK_NVME,
    )
    assert not err
