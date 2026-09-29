import pytest

from ..conftest import is_nvmet, xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "sync"])
def test_compare(cijoe, device, be_opts, cli_args):
    if be_opts["sync"] in ["block", "psync"]:
        pytest.skip(reason=f"Not supported: compare via {be_opts['sync']}")
    if is_nvmet(cijoe, device):
        pytest.skip(reason="[nvmet] does not implement compare")

    err, _ = cijoe.run(f"xnvme_tests_compare compare {cli_args}")
    assert not err
