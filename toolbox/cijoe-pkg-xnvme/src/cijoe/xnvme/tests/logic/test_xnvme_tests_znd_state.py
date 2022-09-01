import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["zns"], opts=["be", "admin", "sync"])
def test_transition(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["sync"] in ["psync"]:
        pytest.skip(reason="Cannot do mgmt send/receive via psync")

    err, _ = cijoe.run(f"xnvme_tests_znd_state transition {cli_args}")
    assert not err
