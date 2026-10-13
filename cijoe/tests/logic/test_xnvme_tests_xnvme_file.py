import pytest

from ..conftest import xnvme_setup_device


@pytest.mark.parametrize(
    "device", xnvme_setup_device(labels=["file"]), indirect=["device"]
)
def test_write_fsync(cijoe, device):
    err, _ = cijoe.run(f"xnvme test xnvme-file write-fsync {device['uri']}")
    assert not err


@pytest.mark.parametrize(
    "device", xnvme_setup_device(labels=["file"]), indirect=["device"]
)
def test_file_trunc(cijoe, device):
    err, _ = cijoe.run(f"xnvme test xnvme-file file-trunc {device['uri']}")
    assert not err
