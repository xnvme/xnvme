from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dmabuf"], opts=["be"])
def test_run_randread(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(
        f"xnvme perf run --iopattern randread --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} --direct {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["dmabuf"], opts=["be"])
def test_run_randwrite(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(
        f"xnvme perf run --iopattern randwrite --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} --direct {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["dmabuf"], opts=["be"])
def test_open_without_direct_fails(cijoe, device, be_opts, cli_args):
    """IO on a dma-buf needs O_DIRECT, which the device open checks for"""

    err, _ = cijoe.run(
        f"xnvme perf run --iopattern randread --qdepth 1 --iosize 4096 --runtime 1"
        f" --cpumask 0x1 --be {be_opts['be']} {device['uri']}"
    )
    assert err
