from ..conftest import xnvme_parametrize


def test_optional_none(cijoe):

    err, _ = cijoe.run("xnvme_tests_cli optional")
    assert not err


@xnvme_parametrize(labels=[], opts=["be", "mem", "sync", "async", "admin"])
def test_optional_all(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(
        f"xnvme_tests_cli optional "
        f"--be {be_opts['be']} "
        f"--mem {be_opts['mem']} "
        f"--sync {be_opts['sync']} "
        f"--async {be_opts['async']} "
        f"--admin {be_opts['admin']} "
    )
    assert not err
