from ..conftest import xnvme_parametrize


@xnvme_parametrize(["dev"], opts=["be"])
def test_buf_alloc_free(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"xnvme_tests_buf buf_alloc_free {cli_args} --count 31")
    assert not err


@xnvme_parametrize(["dev"], opts=["be"])
def test_buf_virt_alloc_free(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"xnvme_tests_buf buf_virt_alloc_free {cli_args} --count 31")
    assert not err
