import base64

import pytest
import yaml

from ..conftest import (
    cijoe_config_inactive_nsid,
    get_homi_id,
    get_osname,
    xnvme_parametrize,
)


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


@pytest.mark.parametrize("fd", ["999", "4294967296"])
@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_dmabuf_fd_not_a_dmabuf(cijoe, device, be_opts, cli_args, fd):
    """--dmabuf-fd names no dma-buf, or no file descriptor: the run fails"""

    # Descriptor 999 is not open; the shell of the Windows job has no /dev/null to redirect
    err, state = cijoe.run(
        f"xnvme perf-storage run --iopattern read --qdepth 32 --iosize 4096 --runtime 1"
        f" --cpumask 0x1 --be {be_opts['be']} --dmabuf-fd {fd} {device['uri']}"
    )
    assert err, f"ran with --dmabuf-fd {fd}"
    assert "--dmabuf-fd" in state.output()
    if fd == "4294967296":
        assert "INT_MAX" in state.output()


# Each case: the arguments, the size of a regular file inherited as descriptor 3 (0 for none),
# and what the error message names.
_DMABUF_FD_ERRORS = {
    "host-bounce": (
        "--iosize 4096 --dmabuf-fd 999 --buf-host-bounce",
        0,
        "--buf-host-bounce",
    ),
    "not-whole-pages": ("--iosize 4096 --dmabuf-fd 3", 4097, "whole pages"),
    "smaller-than-a-slice": ("--iosize 8192 --dmabuf-fd 3", 4096, "whole pages"),
}


@pytest.mark.parametrize("case", list(_DMABUF_FD_ERRORS))
@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_dmabuf_fd_errors(cijoe, device, be_opts, cli_args, case):
    """--dmabuf-fd with --buf-host-bounce, or of a size that is not whole pages of at least a
    slice: the run fails"""

    if get_osname() != "linux":
        pytest.skip(reason="Other platforms return -ENOTSUP for --dmabuf-fd first")
    if be_opts["be"] == "io_uring_dmabuf":
        pytest.skip(
            reason="io_uring_dmabuf's payloads are its own; --dmabuf-fd fails first"
        )

    args, nbytes, expected = _DMABUF_FD_ERRORS[case]
    cmd = (
        f"xnvme perf-storage run --iopattern read --qdepth 32 --runtime 1 --cpumask 0x1"
        f" --be {be_opts['be']} {args} {device['uri']}"
    )
    if nbytes:
        # lseek() takes a regular file's size as it takes a dma-buf's
        cmd = (
            f"f=$(mktemp) && truncate -s {nbytes} $f && {cmd} 3<$f;"
            " rc=$?; rm -f $f; exit $rc"
        )

    err, state = cijoe.run(cmd)
    assert err, f"ran with {args}"
    assert expected in state.output(), state.output()


# Creates a udmabuf over a memfd and runs the command of its arguments, with @FD@
# replaced by the number of the dma-buf, which the command inherits.
_UDMABUF_RUN_PY = r"""
import fcntl, os, struct, subprocess, sys

nbytes = 2 * 1024 * 1024
memfd = os.memfd_create("perf-dmabuf", os.MFD_ALLOW_SEALING)
os.ftruncate(memfd, nbytes)
fcntl.fcntl(memfd, fcntl.F_ADD_SEALS, fcntl.F_SEAL_SHRINK)
udev = os.open("/dev/udmabuf", os.O_RDWR)
create = bytearray(struct.pack("<IIQQ", memfd, 0x01, 0, nbytes))
dmabuf = fcntl.ioctl(udev, 0x40187542, create, True)  # UDMABUF_CREATE
args = [arg.replace("@FD@", str(dmabuf)) for arg in sys.argv[1:]]
sys.exit(subprocess.run(args, pass_fds=(dmabuf,)).returncode)
"""


def _run_dmabuf_fd(cijoe, device, be_opts, iopattern, iosize):
    """Runs I/O of iosize with its payloads in slices of a udmabuf and checks the totals

    Host memory through udmabuf, so no GPU is needed, but dmabuf_import is: without
    it the upcie interface registers no udmabuf.
    """

    if be_opts["be"] != "upcie":
        pytest.skip(reason="xnvme_buf_register_dmabuf() of host memory is upcie's")

    err, _ = cijoe.run("test -c /dev/udmabuf && test -c /dev/dmabuf_import")
    if err:
        pytest.skip(reason="Requires /dev/udmabuf and the dmabuf_import module")

    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""
    if iosize == "lba":
        err, state = cijoe.run(
            f"xnvme info {device['uri']} --be {be_opts['be']} {homi_arg}"
        )
        assert not err, state.output()
        lba = [
            line.split()
            for line in state.output().splitlines()
            if "lba_nbytes:" in line
        ]
        assert lba, state.output()
        iosize = lba[0][1]
        if int(iosize) >= 4096:
            pytest.skip(reason=f"An LBA of {iosize} bytes is no smaller than a page")
    # Four queues on one device: four jobs, which register the dma-buf four times and
    # each take a slice of their own
    script = base64.b64encode(_UDMABUF_RUN_PY.encode()).decode()
    err, state = cijoe.run(
        f"echo {script} | base64 -d > /tmp/xnvme-perf-udmabuf.py"
        f" && python3 /tmp/xnvme-perf-udmabuf.py xnvme perf-storage run"
        f" --iopattern {iopattern} --qdepth 32 --iosize {iosize} --runtime 3 --cpumask 0x1"
        f" --nqueues 4 --be {be_opts['be']} {homi_arg} --dmabuf-fd @FD@ {device['uri']}"
    )
    assert not err, state.output()
    assert "payloads in dma-buf" in state.output()
    assert "slices of 4096 bytes" in state.output()

    # Total: <IOPS> <MiB/s> <failed>
    total = [line.split() for line in state.output().splitlines() if "Total:" in line]
    assert total, state.output()
    assert float(total[-1][1]) > 0, "completed no I/O"
    assert int(total[-1][3]) == 0, "I/O failed"


@pytest.mark.parametrize("iopattern", ["randread", "randwrite"])
@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_run_dmabuf_fd(cijoe, device, be_opts, cli_args, iopattern):
    """I/O with its payloads in slices of an inherited dma-buf"""

    _run_dmabuf_fd(cijoe, device, be_opts, iopattern, "4096")


@pytest.mark.parametrize("iopattern", ["randread", "randwrite"])
@xnvme_parametrize(labels=["second_ctrlr"], opts=["be"])
def test_run_dmabuf_fd_lba(cijoe, device, be_opts, cli_args, iopattern):
    """I/O of one LBA in slices of a dma-buf, a page apart where the LBA is smaller

    The second controller of the rig 'second-controller' has 512-byte LBAs.
    """

    _run_dmabuf_fd(cijoe, device, be_opts, iopattern, "lba")


@pytest.mark.parametrize("count", ["0", "4294967296"])
@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_xfer_register_count_zero(cijoe, device, be_opts, cli_args, count):
    """'perf-xfer register' fails for a --count of 0 or above UINT32_MAX"""

    err, state = cijoe.run(
        f"xnvme perf-xfer register {device['uri']} --be {be_opts['be']}"
        f" --dmabuf-fd 999 --count {count}"
    )
    assert err, f"timed --count {count} registrations"
    assert "--count" in state.output()


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_xfer_register(cijoe, device, be_opts, cli_args):
    """'perf-xfer register' times registering an inherited dma-buf and unregistering it

    Host memory through udmabuf, as for test_run_dmabuf_fd.
    """

    if be_opts["be"] != "upcie":
        pytest.skip(reason="xnvme_buf_register_dmabuf() of host memory is upcie's")

    err, _ = cijoe.run("test -c /dev/udmabuf && test -c /dev/dmabuf_import")
    if err:
        pytest.skip(reason="Requires /dev/udmabuf and the dmabuf_import module")

    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""
    script = base64.b64encode(_UDMABUF_RUN_PY.encode()).decode()
    err, state = cijoe.run(
        f"echo {script} | base64 -d > /tmp/xnvme-perf-udmabuf.py"
        f" && python3 /tmp/xnvme-perf-udmabuf.py xnvme perf-xfer register {device['uri']}"
        f" --be {be_opts['be']} {homi_arg} --dmabuf-fd @FD@ --count 16"
    )
    assert not err, state.output()

    result = yaml.safe_load(state.output())["xnvme_perf_xfer_register"]
    assert result["count"] == 16, state.output()
    assert result["dmabuf_nbytes"] == 2 * 1024 * 1024, state.output()
    assert result["first_register_usecs"] > 0, state.output()
    assert result["first_unregister_usecs"] > 0, state.output()
    for name in ("register_usecs", "unregister_usecs"):
        usecs = result[name]
        assert 0 < usecs["min"] <= usecs["median"] <= usecs["max"], state.output()


def _check_times(result, names, count, output):
    """The times 'perf-xfer' prints of a pair of calls, such as register and unregister"""

    for name in names:
        assert result[f"first_{name}_usecs"] >= 0, output
        usecs = result[f"{name}_usecs"]
        assert 0 <= usecs["min"] <= usecs["median"] <= usecs["max"], output


@pytest.mark.parametrize(
    "args",
    [
        "--count 2",
        "--mib 2 --dmabuf-fd 999",
        "--mib 2,,4",
        "--mib 0",
        "--mib 2,x",
        "--mib 3 --huge 2M",
        "--mib 2 --huge 4M",
        "--huge 2M --dmabuf-fd 999",
    ],
)
@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_xfer_register_host_args(cijoe, device, be_opts, cli_args, args):
    """'perf-xfer register' fails, before opening the device, for arguments it cannot use"""

    if get_osname() != "linux":
        pytest.skip(reason="Other platforms return -ENOTSUP for register")

    err, state = cijoe.run(
        f"xnvme perf-xfer register {device['uri']} --be {be_opts['be']} {args}"
    )
    assert err, f"registered with {args}"
    assert "Invalid argument" in state.output(), state.output()


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_xfer_register_host(cijoe, device, be_opts, cli_args):
    """'perf-xfer register --mib' times registering host memory of base pages

    For each size, on memory faulted in before the first registration, and on memory
    mapped afresh and untouched for each. upcie registers no host memory, and fails with
    ENOSYS. SPDK registers only memory it can translate, which test_xfer_register_host_huge
    covers.
    """

    if get_osname() != "linux":
        pytest.skip(reason="Other platforms return -ENOTSUP for register")

    if be_opts["be"] == "spdk":
        pytest.skip(
            reason="SPDK registers hugetlb memory; see test_xfer_register_host_huge"
        )

    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""
    err, state = cijoe.run(
        f"xnvme perf-xfer register {device['uri']} --be {be_opts['be']} {homi_arg}"
        " --mib 2,4 --count 4"
    )
    if be_opts["be"] == "upcie":
        assert err, state.output()
        assert "Function not implemented" in state.output(), state.output()
        return
    assert not err, state.output()

    result = yaml.safe_load(state.output())["xnvme_perf_xfer_register"]
    assert result["count"] == 4, state.output()
    assert [size["nbytes"] for size in result["sizes"]] == [2 << 20, 4 << 20]
    for size in result["sizes"]:
        for case in ("faulted", "untouched"):
            _check_times(size[case], ("register", "unregister"), 4, state.output())


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_xfer_register_host_huge(cijoe, device, be_opts, cli_args):
    """'perf-xfer register --mib --huge 2M' times registering host memory of hugetlb pages

    Skipped where the guest has no free hugetlb pages of 2 MiB left for it.
    """

    if get_osname() != "linux":
        pytest.skip(reason="Other platforms return -ENOTSUP for register")

    if be_opts["be"] == "upcie":
        pytest.skip(
            reason="upcie registers no host memory; see test_xfer_register_host"
        )

    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""
    err, state = cijoe.run(
        f"xnvme perf-xfer register {device['uri']} --be {be_opts['be']} {homi_arg}"
        " --mib 4 --huge 2M --count 2"
    )
    if err and "Cannot allocate memory" in state.output():
        pytest.skip(reason="No free hugetlb pages of 2 MiB")
    assert not err, state.output()

    result = yaml.safe_load(state.output())["xnvme_perf_xfer_register"]
    assert result["page_nbytes"] == 2 << 20, state.output()
    assert [size["nbytes"] for size in result["sizes"]] == [4 << 20]
    for case in ("faulted", "untouched"):
        _check_times(
            result["sizes"][0][case], ("register", "unregister"), 2, state.output()
        )


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_xfer_heap(cijoe, device, be_opts, cli_args):
    """'perf-xfer heap' times opening the device with a host heap of each size, and closing it"""

    if be_opts["be"] != "upcie":
        pytest.skip(reason="The host heap is upcie's")

    homi_id = get_homi_id()
    homi_arg = f"--homi-id {homi_id}" if homi_id else ""
    err, state = cijoe.run(
        f"xnvme perf-xfer heap {device['uri']} --be {be_opts['be']} {homi_arg}"
        " --mib 64,128 --count 2"
    )
    assert not err, state.output()

    result = yaml.safe_load(state.output())["xnvme_perf_xfer_heap"]
    assert result["count"] == 2, state.output()
    sizes = result["sizes"]
    assert [size["host_heap_nbytes"] for size in sizes] == [64 << 20, 128 << 20]
    for size in sizes:
        _check_times(size, ("open", "close"), 2, state.output())
