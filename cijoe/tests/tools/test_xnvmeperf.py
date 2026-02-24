import pytest

from ..conftest import XnvmeDriver, xnvme_parametrize


def _be_args(be_opts):
    """Build backend CLI arguments (without device URI or nsid)"""
    return " ".join(f"--{k} {v}" for k, v in be_opts.items() if k != "label")


@xnvme_parametrize(labels=["nvm"], opts=["be", "async"])
def test_run_read(cijoe, device, be_opts, cli_args):
    be_args = _be_args(be_opts)
    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern read --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 {be_args} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be", "async"])
def test_run_write(cijoe, device, be_opts, cli_args):
    be_args = _be_args(be_opts)
    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern write --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 {be_args} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be", "async"])
def test_run_randread(cijoe, device, be_opts, cli_args):
    be_args = _be_args(be_opts)
    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern randread --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 {be_args} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be", "async"])
def test_run_randwrite(cijoe, device, be_opts, cli_args):
    be_args = _be_args(be_opts)
    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern randwrite --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 {be_args} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be", "async"])
def test_verify(cijoe, device, be_opts, cli_args):
    be_args = _be_args(be_opts)
    err, _ = cijoe.run(
        f"xnvmeperf verify --iosize 4096 --count 64 {be_args} {device['uri']}"
    )
    assert not err
