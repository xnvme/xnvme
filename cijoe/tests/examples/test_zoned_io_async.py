from ..conftest import xnvme_parametrize


# os=[freebsd],be=[kqueue,thrpool,emu]: FreeBSD kernel doesn't support ZNS
@xnvme_parametrize(
    labels=["zns"],
    opts=["be", "admin", "async"],
    os_exclude={"freebsd": {"be": ["kqueue", "thrpool", "emu"]}},
)
def test_write(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"zoned_io_async write {cli_args}")
    assert not err


# admin=[block]: block layer does not support append; async=[io_uring,libaio,posix]: these async backends do not support append; os=[freebsd],be=[kqueue,thrpool,emu]: FreeBSD kernel doesn't support ZNS
@xnvme_parametrize(
    labels=["zns"],
    opts=["be", "admin", "async"],
    exclude={"admin": ["block"], "async": ["io_uring", "libaio", "posix"]},
    os_exclude={"freebsd": {"be": ["kqueue", "thrpool", "emu"]}},
)
def test_append(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"zoned_io_async append {cli_args}")
    assert not err


# os=[freebsd],be=[kqueue,thrpool,emu]: FreeBSD kernel doesn't support ZNS
@xnvme_parametrize(
    labels=["zns"],
    opts=["be", "admin", "async"],
    os_exclude={"freebsd": {"be": ["kqueue", "thrpool", "emu"]}},
)
def test_read(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"zoned_io_async read {cli_args}")
    assert not err
