import pytest

from ..conftest import get_osname, xnvme_parametrize


# sync=[psync]: psync cannot do mgmt send/receive
@xnvme_parametrize(
    labels=["zns"], opts=["be", "admin", "sync"], exclude={"sync": ["psync"]}
)
def test_transition(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")

    err, _ = cijoe.run(f"xnvme_tests_znd_state transition {cli_args}")
    assert not err
