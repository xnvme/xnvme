import pytest

from ..conftest import cijoe_config_inactive_nsid, get_homi_id, xnvme_parametrize


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_read(cijoe, device, be_opts, cli_args):
    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern read --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {homi_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_write(cijoe, device, be_opts, cli_args):
    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern write --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {homi_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_randread(cijoe, device, be_opts, cli_args):
    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern randread --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {homi_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_randwrite(cijoe, device, be_opts, cli_args):
    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern randwrite --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {homi_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_verify(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "ramdisk":
        pytest.skip(
            reason="The ramdisk's async interface, nil, completes IO moving no data"
        )

    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf verify --iosize 4096 --count 64"
        f" --be {be_opts['be']} {homi_arg} {device['uri']}"
    )
    assert not err


METADATA_BUFFER_BACKENDS = ("upcie", "upcie-cuda", "upcie-hip", "spdk", "libvfn")


@xnvme_parametrize(labels=["pi1"], opts=["be"])
def test_verify_separate_metadata(cijoe, device, be_opts, cli_args):
    """No backend fails verify on a namespace with separate metadata: one taking a
    metadata buffer verifies it too, one that does not, or hides the metadata, the
    data alone"""

    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, state = cijoe.run(
        f"xnvmeperf verify --iosize 4096 --count 16"
        f" --be {be_opts['be']} {homi_arg} '{device['uri']}/?nsid={device['nsid']}'"
    )
    assert not err, state.output()
    if be_opts["be"] in METADATA_BUFFER_BACKENDS:
        assert "in a buffer of its own" in state.output(), state.output()


GPU_BACKENDS = ("upcie-cuda", "upcie-hip")


def _verify_gpu(cijoe, device, be_opts, iosize, extra=""):
    """Write and read back from buffers in GPU memory; returns what 'verify' printed"""

    if be_opts["be"] not in GPU_BACKENDS:
        pytest.skip(reason="Only the GPU backends put data buffers in device memory")

    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, state = cijoe.run(
        f"xnvmeperf verify --iosize {iosize} --count 16 {extra}"
        f" --be {be_opts['be']} {homi_arg} '{device['uri']}/?nsid={device['nsid']}'"
    )
    assert not err, state.output()

    return state.output()


@pytest.mark.parametrize("iosize", [4096, 8192, 16384, 131072])
@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_verify_gpu_payload(cijoe, device, be_opts, cli_args, iosize):
    """
    Each way a command points at its data, with the data in GPU memory

    With 4 KiB pages: PRP1 alone, PRP1 and PRP2, and PRP1 with a PRP list of three and
    of 31 entries.
    """

    _verify_gpu(cijoe, device, be_opts, iosize)


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_verify_gpu_p2p_cq_mirror(cijoe, device, be_opts, cli_args):
    """Completions land in GPU memory and a GPU kernel mirrors them to the host"""

    if get_homi_id():
        pytest.skip(reason="A homi client with the P2P CQ mirror hangs in verify")

    _verify_gpu(cijoe, device, be_opts, 16384, "--p2p-cq-mirror")


@xnvme_parametrize(labels=["meta_sep"], opts=["be"])
def test_verify_gpu_metadata(cijoe, device, be_opts, cli_args):
    """Metadata in a buffer of its own in GPU memory, through the metadata pointer"""

    output = _verify_gpu(cijoe, device, be_opts, 16384)
    assert "in a buffer of its own" in output, "the namespace has no separate metadata"


@xnvme_parametrize(labels=["pcie", "nvm"], opts=["be"])
def test_run_qdepth_past_the_pool_is_refused(cijoe, device, be_opts, cli_args):
    """A queue deeper than the 1024 commands uPCIe tracks per queue is refused"""

    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="The per-queue command limit is uPCIe's")

    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern read --qdepth 2048 --iosize 4096 --runtime 1"
        f" --cpumask 0x1 --be {be_opts['be']} {homi_arg} {device['uri']}"
    )
    assert err, "a queue deeper than the backend tracks was accepted"


@xnvme_parametrize(labels=["pcie", "nvm"], opts=["be"])
def test_run_uri_nsid(cijoe, device, be_opts, cli_args):
    """A device URI names its namespace with '/?nsid=<nsid>'"""

    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""
    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern read --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {homi_arg}"
        f" '{device['uri']}/?nsid={device['nsid']}'"
    )
    assert not err


@xnvme_parametrize(labels=["pcie", "nvm"], opts=["be"])
def test_run_uri_nsid_inactive(cijoe, device, be_opts, cli_args):
    """A namespace named in the URI that is not active is refused"""

    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""
    nsid = cijoe_config_inactive_nsid(device)
    err, state = cijoe.run(
        f"xnvmeperf run --iopattern read --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {homi_arg}"
        f" '{device['uri']}/?nsid={nsid}'"
    )
    assert err, f"ran against an inactive namespace: {state.output()}"
