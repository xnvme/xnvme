from ..conftest import xnvme_parametrize


# sync=[block,psync]: simple-copy not supported; admin=[block]: doesn't report MSSRL on namespace
@xnvme_parametrize(
    labels=["scc"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["block", "psync"], "admin": ["block"]},
)
def test_copy(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_copy copy {cli_args}")
    assert not err


# sync=[block,psync]: simple-copy not supported; admin=[block]: doesn't report MSSRL on namespace
@xnvme_parametrize(
    labels=["scc"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["block", "psync"], "admin": ["block"]},
)
def test_copy_src_before_dest(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_copy copy {cli_args} --slba 0 --nlb 7 --sdlba 8")
    assert not err


# sync=[block,psync]: simple-copy not supported; admin=[block]: doesn't report MSSRL on namespace
@xnvme_parametrize(
    labels=["scc"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["block", "psync"], "admin": ["block"]},
)
def test_copy_src_after_dest(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_copy copy {cli_args} --slba 8 --nlb 7 --sdlba 0")
    assert not err
