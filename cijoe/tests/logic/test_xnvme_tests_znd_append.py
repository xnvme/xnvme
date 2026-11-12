import pytest

from ..conftest import ASYNC_READ_WRITE_FLUSH_ONLY, get_osname, xnvme_parametrize


@xnvme_parametrize(labels=["zns"], opts=["be", "admin", "async"])
def test_verify(cijoe, device, be_opts, cli_args):
    if get_osname() == "freebsd" and be_opts["be"] not in [
        "spdk",
        "ramdisk_emu",
        "ramdisk_thrpool",
    ]:
        pytest.skip(reason="Freebsd kernel doesn't support zns")
    if be_opts["admin"] == "block":
        pytest.skip(reason="[admin=block] does not implement zone-append")
    if be_opts["async"] in ASYNC_READ_WRITE_FLUSH_ONLY:
        pytest.skip(reason=f"[async={be_opts['async']}] carries no zone-append")
    err, _ = cijoe.run(f"xnvme test znd-append verify {cli_args}")
    assert not err
