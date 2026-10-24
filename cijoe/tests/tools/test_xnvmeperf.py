import pytest

from ..conftest import get_homi_id, xnvme_parametrize


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_read(cijoe, device, be_opts, cli_args):
    homi_id = get_homi_id()
    cplane_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern read --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {cplane_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_write(cijoe, device, be_opts, cli_args):
    homi_id = get_homi_id()
    cplane_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern write --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {cplane_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_randread(cijoe, device, be_opts, cli_args):
    homi_id = get_homi_id()
    cplane_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern randread --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {cplane_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_randwrite(cijoe, device, be_opts, cli_args):
    homi_id = get_homi_id()
    cplane_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern randwrite --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {cplane_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_verify(cijoe, device, be_opts, cli_args):
    homi_id = get_homi_id()
    cplane_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf verify --iosize 4096 --count 64"
        f" --be {be_opts['be']} {cplane_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["pcie", "nvm"], opts=["be"])
def test_run_qdepth_past_the_pool_says_why(cijoe, device, be_opts, cli_args):
    """
    A queue deeper than uPCIe can track is refused with a message saying so.

    The backend tracks 1024 commands per queue, so 2048 is refused when the
    queue is created; without the message the run ends on an error from deep in
    setup, and nothing points at --qdepth.
    """

    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="The per-queue command limit is uPCIe's")

    homi_id = get_homi_id()
    cplane_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, state = cijoe.run(
        f"xnvmeperf run --iopattern read --qdepth 2048 --iosize 4096 --runtime 1"
        f" --cpumask 0x1 --be {be_opts['be']} {cplane_arg} {device['uri']}"
    )
    assert err, "a queue deeper than the backend tracks was accepted"
    assert "is more than one queue" in state.output(), "the refusal does not say why"


@xnvme_parametrize(labels=["pcie", "nvm"], opts=["be"])
def test_run_uri_nsid(cijoe, device, be_opts, cli_args):
    """A device URI names its namespace with '/?nsid=<nsid>'"""

    homi_id = get_homi_id()
    cplane_arg = f"--homi-id {homi_id}" if homi_id else ""
    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern read --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {cplane_arg}"
        f" '{device['uri']}/?nsid={device['nsid']}'"
    )
    assert not err


@xnvme_parametrize(labels=["pcie", "nvm"], opts=["be"])
def test_run_uri_nsid_inactive(cijoe, device, be_opts, cli_args):
    """A namespace named in the URI that is not active is refused"""

    homi_id = get_homi_id()
    cplane_arg = f"--homi-id {homi_id}" if homi_id else ""
    # The configured controllers each have a single namespace
    nsid = int(device["nsid"]) + 1
    err, state = cijoe.run(
        f"xnvmeperf run --iopattern read --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {cplane_arg}"
        f" '{device['uri']}/?nsid={nsid}'"
    )
    assert err, f"ran against an inactive namespace: {state.output()}"
