from ..conftest import xnvme_parametrize


@xnvme_parametrize(["dev"], opts=["be"])
def test_threads_open_close(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_threads open_close {cli_args} --count 3")
    assert not err


@xnvme_parametrize(["dev"], opts=["be"])
def test_threads_init_term(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(
        f"xnvme_tests_threads init_term {cli_args} --count 50 --qdepth 64"
    )
    assert not err
