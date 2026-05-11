from ..conftest import xnvme_parametrize


# sync=[psync]: psync cannot do mgmt send/receive; os=[freebsd],be=[kqueue,thrpool,emu]: FreeBSD kernel doesn't support ZNS
@xnvme_parametrize(
    labels=["zns"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["psync"]},
    os_exclude={"freebsd": {"be": ["kqueue", "thrpool", "emu"]}},
)
def test_transition(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_znd_state transition {cli_args}")
    assert not err
