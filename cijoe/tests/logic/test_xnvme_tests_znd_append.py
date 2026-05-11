from ..conftest import xnvme_parametrize


# os=[freebsd],be=[kqueue,thrpool,emu]: FreeBSD kernel doesn't support ZNS
@xnvme_parametrize(
    labels=["zns"],
    opts=["be", "admin", "async"],
    os_exclude={"freebsd": {"be": ["kqueue", "thrpool", "emu"]}},
)
def test_verify(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_znd_append verify {cli_args}")
    assert not err
