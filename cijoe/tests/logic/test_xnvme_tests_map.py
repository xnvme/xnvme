import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_mem_map_unmap(cijoe, device, be_opts, cli_args):
    if be_opts["be"] != "vfio":
        pytest.skip(reason="Backend does not support memory-mapping")

    err, _ = cijoe.run(f"xnvme_tests_map mem_map_unmap {cli_args} --count 31")
    assert not err
