import pytest


@pytest.mark.parametrize("sub", ["idfy-offsets", "idfy-masks"])
def test_idfy(cijoe, sub):
    err, _ = cijoe.run(f"xnvme test spec {sub}")
    assert not err
