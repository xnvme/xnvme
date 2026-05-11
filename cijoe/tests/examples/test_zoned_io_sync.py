from ..conftest import xnvme_parametrize


# sync=[psync]: ENOSYS, psync cannot do mgmt send/receive; os=[freebsd],be=[kqueue,thrpool,emu]: FreeBSD kernel doesn't support ZNS
@xnvme_parametrize(
    labels=["zns"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["psync"]},
    os_exclude={"freebsd": {"be": ["kqueue", "thrpool", "emu"]}},
)
def test_write(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"zoned_io_sync write {cli_args}")
    assert not err


# admin=[block]: block layer does not support append; sync=[block,psync]: ENOSYS; os=[freebsd],be=[kqueue,thrpool,emu]: FreeBSD kernel doesn't support ZNS
@xnvme_parametrize(
    labels=["zns"],
    opts=["be", "admin", "sync"],
    exclude={"admin": ["block"], "sync": ["block", "psync"]},
    os_exclude={"freebsd": {"be": ["kqueue", "thrpool", "emu"]}},
)
def test_append(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"zoned_io_sync append {cli_args}")
    assert not err


# sync=[psync]: ENOSYS, psync cannot do mgmt send/receive; os=[freebsd],be=[kqueue,thrpool,emu]: FreeBSD kernel doesn't support ZNS
@xnvme_parametrize(
    labels=["zns"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["psync"]},
    os_exclude={"freebsd": {"be": ["kqueue", "thrpool", "emu"]}},
)
def test_read(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"zoned_io_sync read {cli_args}")
    assert not err
