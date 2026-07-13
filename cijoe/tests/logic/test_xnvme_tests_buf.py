from ..conftest import xnvme_parametrize


@xnvme_parametrize(["dev"], opts=["be"])
def test_buf_alloc_free(cijoe, device, be_opts, cli_args):
    # count=27 caps the largest allocation at 1<<26 (64MB). On FreeBSD a single
    # DMA buffer cannot exceed one contigmem buffer, which the CI sizes at 64MB
    # (CONTIGMEM_BUFSZ in conftest.py); the test only exercises functionality,
    # not allocation limits, so keep it at/under that ceiling.
    err, _ = cijoe.run(f"xnvme_tests_buf buf_alloc_free {cli_args} --count 27")
    assert not err


@xnvme_parametrize(["dev"], opts=["be"])
def test_buf_vtophys(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_buf buf_vtophys {cli_args}")
    assert not err


@xnvme_parametrize(["dev"], opts=["be"])
def test_buf_virt_alloc_free(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_buf buf_virt_alloc_free {cli_args} --count 28")
    assert not err
