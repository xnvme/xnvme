"""
qublk exposes an xNVMe device as a Linux ublk block-device.

Unlike the other command-line tools, qublk is a blocking daemon; it requires root and
the 'ublk_drv' module, and it runs until signalled. It therefore cannot be exercised by
a single command returning a status; see qublk_session.py for how a test-case drives it.

The cases here exercise the raw block-device; filesystem-level coverage is in
test_qublk_fs.py.
"""

import pytest

from ..conftest import xnvme_parametrize
from .qublk_session import UBLK_NODE, qublk_session, qublk_teardown, require_ublk


@pytest.fixture(autouse=True)
def qublk_cleanup(cijoe):
    """Skip when ublk is unusable; leave no daemon or ublk device behind"""

    require_ublk(cijoe)

    yield

    qublk_teardown(cijoe)


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_dd(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [f"dd if={UBLK_NODE} of=/dev/null bs=1M count=32 iflag=direct"],
        args="--qdepth 64",
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_multi_queue(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [f"dd if={UBLK_NODE} of=/dev/null bs=1M count=32 iflag=direct"],
        args="--qdepth 64 --nqueues 4",
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_max_io_bytes(cijoe, device, be_opts, cli_args):
    err, _ = qublk_session(
        cijoe,
        device["uri"],
        be_opts["be"],
        [f"dd if={UBLK_NODE} of=/dev/null bs=128k count=64 iflag=direct"],
        args="--qdepth 64 --max-io-bytes 131072",
    )
    assert not err
