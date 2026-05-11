from ..conftest import xnvme_parametrize


# sync=[block,psync]: Linux block layer does not support simple-copy; admin=[block]: same
@xnvme_parametrize(
    labels=["scc"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["block", "psync"], "admin": ["block"]},
)
def test_idfy(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_scc idfy {cli_args}")
    assert not err


# sync=[block,psync]: Linux block layer does not support simple-copy; admin=[block]: same
@xnvme_parametrize(
    labels=["scc"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["block", "psync"], "admin": ["block"]},
)
def test_support(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_scc support {cli_args}")
    assert not err


# sync=[block,psync]: Linux block layer does not support simple-copy; admin=[block]: same
@xnvme_parametrize(
    labels=["scc"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["block", "psync"], "admin": ["block"]},
)
def test_scopy(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_scc scopy {cli_args}")
    assert not err


# sync=[block,psync]: Linux block layer does not support simple-copy; admin=[block]: same
@xnvme_parametrize(
    labels=["scc"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["block", "psync"], "admin": ["block"]},
)
def test_scopy_clear(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_scc scopy {cli_args} --clear")
    assert not err


# sync=[block,psync]: Linux block layer does not support simple-copy; admin=[block]: same
@xnvme_parametrize(
    labels=["scc"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["block", "psync"], "admin": ["block"]},
)
def test_scopy_msrc(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_scc scopy-msrc {cli_args}")
    assert not err


# sync=[block,psync]: Linux block layer does not support simple-copy; admin=[block]: same
@xnvme_parametrize(
    labels=["scc"],
    opts=["be", "admin", "sync"],
    exclude={"sync": ["block", "psync"], "admin": ["block"]},
)
def test_scopy_msrc_clear(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme_tests_scc scopy-msrc {cli_args} --clear")
    assert not err
