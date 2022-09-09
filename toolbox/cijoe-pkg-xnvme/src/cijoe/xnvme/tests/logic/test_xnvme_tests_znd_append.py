from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["zns"], opts=["be", "admin", "async"])
def test_verify(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"xnvme_tests_znd_append verify {cli_args}")
    assert not err
