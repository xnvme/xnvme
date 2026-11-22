import pytest

from ..conftest import cijoe_config_inactive_nsid, get_homi_id, xnvme_parametrize


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_read(cijoe, device, be_opts, cli_args):
    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, _ = cijoe.run(
        f"xnvme perf-storage run --iopattern read --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {homi_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_write(cijoe, device, be_opts, cli_args):
    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, _ = cijoe.run(
        f"xnvme perf-storage run --iopattern write --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {homi_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_randread(cijoe, device, be_opts, cli_args):
    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, _ = cijoe.run(
        f"xnvme perf-storage run --iopattern randread --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {homi_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_randwrite(cijoe, device, be_opts, cli_args):
    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, _ = cijoe.run(
        f"xnvme perf-storage run --iopattern randwrite --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {homi_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_randread_reap(cijoe, device, be_opts, cli_args):
    """Completions retrieved with xnvme_queue_reap() instead of called back"""

    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, state = cijoe.run(
        f"xnvme perf-storage run --iopattern randread --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --completion reap --be {be_opts['be']} {homi_arg}"
        f" {device['uri']}"
    )
    assert not err

    # The exit status is zero also when no reap returned an I/O, so read the result
    total = [line.split() for line in state.output().splitlines() if "Total:" in line]
    assert total, state.output()
    assert float(total[-1][1]) > 0 and int(total[-1][3]) == 0, state.output()


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_completion_unknown(cijoe, device, be_opts, cli_args):
    """--completion takes 'cb' or 'reap', and the run fails for anything else"""

    err, state = cijoe.run(
        f"xnvme perf-storage run --iopattern randread --qdepth 32 --iosize 4096 --runtime 1"
        f" --cpumask 0x1 --completion nosuch --be {be_opts['be']} {device['uri']}"
    )
    assert err, "an unknown --completion was accepted"
    assert "unknown completion 'nosuch'" in state.output(), state.output()


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_verify(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "ramdisk":
        pytest.skip(
            reason="The ramdisk's async interface, nil, completes IO moving no data"
        )

    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, _ = cijoe.run(
        f"xnvme perf-storage verify --iosize 4096 --count 64"
        f" --be {be_opts['be']} {homi_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_verify_reap(cijoe, device, be_opts, cli_args):
    """Data written and read back with completions reaped"""

    if be_opts["be"] == "ramdisk":
        pytest.skip(
            reason="The ramdisk's async interface, nil, completes IO moving no data"
        )

    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""

    err, _ = cijoe.run(
        f"xnvme perf-storage verify --iosize 4096 --count 64 --completion reap"
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
        f"xnvme perf-storage verify --iosize 4096 --count 16"
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
        f"xnvme perf-storage verify --iosize {iosize} --count 16 {extra}"
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
        f"xnvme perf-storage run --iopattern read --qdepth 2048 --iosize 4096 --runtime 1"
        f" --cpumask 0x1 --be {be_opts['be']} {homi_arg} {device['uri']}"
    )
    assert err, "a queue deeper than the backend tracks was accepted"


@xnvme_parametrize(labels=["pcie", "nvm"], opts=["be"])
def test_run_uri_nsid(cijoe, device, be_opts, cli_args):
    """A device URI names its namespace with '/?nsid=<nsid>'"""

    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""
    err, _ = cijoe.run(
        f"xnvme perf-storage run --iopattern read --qdepth 32 --iosize 4096 --runtime 3"
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
        f"xnvme perf-storage run --iopattern read --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {homi_arg}"
        f" '{device['uri']}/?nsid={nsid}'"
    )
    assert err, f"ran against an inactive namespace: {state.output()}"


@pytest.mark.parametrize("completion", ["cb", "reap"])
@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_polled_stop_continue(cijoe, device, be_opts, cli_args, completion):
    """A polled run that is stopped and continued still ends with its result

    With --poll_io, io_uring_peek_cqe() enters the kernel, and a signal makes it
    return -EINTR without a completion. The run is stopped and continued 20
    times; a poke or a reap reading the completion that is not there ends it
    with SIGSEGV.
    """

    if be_opts["be"] not in ("io_uring", "io_uring_bdev", "io_uring_cmd"):
        pytest.skip(reason="Only io_uring polls for completions with --poll_io")

    err, state = cijoe.run(
        f"xnvme perf-storage run --iopattern randread --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --poll_io 1 --direct --completion {completion}"
        f" --be {be_opts['be']} {device['uri']}"
        " > /tmp/xnvme-perf-polled.out 2>&1 & pid=$!;"
        " for i in $(seq 20); do sleep 0.1; kill -STOP $pid; kill -CONT $pid; done;"
        " wait $pid; rc=$?; cat /tmp/xnvme-perf-polled.out; exit $rc"
    )
    assert not err, state.output()

    total = [line.split() for line in state.output().splitlines() if "Total:" in line]
    if total and float(total[-1][1]) == 0:
        pytest.skip(reason="No polled I/O completed; the device has no poll queues")
