from pathlib import Path

import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_cython_bindings(cijoe, device, be_opts, cli_args):

    if be_opts["be"] not in ["linux"]:
        pytest.skip(reason="The env.setup does not work on FreeBSD")

    env = {
        "XNVME_URI": f"{device['uri']}",
        "XNVME_BE": f"{be_opts['be']}",
        "XNVME_DEV_NSID": f"{device['nsid']}",
    }

    err, _ = cijoe.run(
        "python3 -m pytest --pyargs xnvme.cython_bindings -v -s", env=env
    )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_cython_header(cijoe, device, be_opts, cli_args):

    if be_opts["be"] not in ["linux"]:
        pytest.skip(reason="The env.setup does not work on FreeBSD")

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
    }

    err, _ = cijoe.run(
        f"python3 -m pytest --cython-collect {header_path} -v -s", env=env
    )
    assert not err
