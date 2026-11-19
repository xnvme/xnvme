import pytest


@pytest.mark.parametrize("sub", ["fields", "conditions", "ranges", "sizes"])
def test_cmd_prep(cijoe, sub):
    err, _ = cijoe.run(f"xnvme test cmd-prep {sub}")
    assert not err
