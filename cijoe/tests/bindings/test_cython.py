from pathlib import Path

import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["large_mdts"], opts=["be"])
def test_cython_bindings(cijoe, device, be_opts, cli_args):
    if be_opts["be"] not in ["linux", "spdk"]:
        pytest.skip(reason=f"Backend not supported: {be_opts['be']}")

    env = {
        "XNVME_URI": device["uri"],
        "XNVME_BE": be_opts["be"],
        "XNVME_DEV_NSID": f"{device['nsid']}",
        "XNVME_HUGETLB_PATH": cijoe.config.options.get("hugetlbfs", {}).get(
            "mount_point", ""
        ),
    }

    err, _ = cijoe.run(
        "python3 -m pytest --pyargs xnvme.cython_bindings -v -s -k 'not TestKeyValue'",
        env=env,
    )
    assert not err


@xnvme_parametrize(labels=["kvs"], opts=["be"])
def test_cython_bindings_kv(cijoe, device, be_opts, cli_args):

    if be_opts["be"] not in ["linux", "spdk"]:
        pytest.skip(reason=f"Backend not supported: {be_opts['be']}")

    env = {
        "XNVME_URI": device["uri"],
        "XNVME_BE": be_opts["be"],
        "XNVME_DEV_NSID": f"{device['nsid']}",
        # "XNVME_HUGETLB_PATH": cijoe.config.options.get("hugetlbfs", {}).get(
        #     "mount_point", ""
        # ),
    }

    err, _ = cijoe.run(
        "python3 -m pytest --pyargs xnvme.cython_bindings -v -s -k 'TestKeyValue'",
        env=env,
    )
    assert not err


@xnvme_parametrize(labels=["bdev"], opts=["be"])
def test_cython_header(cijoe, device, be_opts, cli_args):
    if be_opts["be"] not in ["linux", "spdk"]:
        pytest.skip(reason=f"Backend not supported: {be_opts['be']}")

    repos_path = (
        cijoe.config.options.get("xnvme", {}).get("source", {}).get("path", None)
    )
    assert repos_path, "Configuration must have xnvme repository path"

    header_path = (
        Path(repos_path)
        / "python"
        / "xnvme-cy-header"
        / "xnvme"
        / "cython_header"
        / "tests"
    )

    env = {
        "XNVME_URI": f"{device['uri']}",
        "XNVME_BE": f"{be_opts['be']}",
        "XNVME_DEV_NSID": f"{device['nsid']}",
        "XNVME_HUGETLB_PATH": cijoe.config.options.get("hugetlbfs", {}).get(
            "mount_point", ""
        ),
    }

    err, _ = cijoe.run(
        f"python3 -m pytest --cython-collect {header_path} -v -s", env=env
    )
    assert not err
