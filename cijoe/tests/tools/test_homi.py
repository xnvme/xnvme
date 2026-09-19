import re
from time import sleep, time

import pytest
import yaml

from ..conftest import (
    HomiServer,
    cijoe_config_get_all_devices,
    get_homi_id,
    xnvme_parametrize,
)

# Selection is '-k' against the test id, so the cases taking no device carry
# 'upcie' in their name to run beside their parametrized siblings.

# Well above what the suite hands out, so no server claims it
_UNCLAIMED_ID = 4242


def _require_upcie(cijoe):
    """
    Skip unless this build has the uPCIe backend, whose runtime 'homi status' reads

    Asked of the library rather than decided from the platform, since a Linux
    build configured with -Dbe_upcie=false is in the same position.
    """

    err, state = cijoe.run("xnvme library-info")
    assert not err, "could not ask the library what backends it has"

    if "name: 'upcie'" not in state.output():
        pytest.skip(reason="Requires the uPCIe backend; this build does not have it")


def _served(cijoe, be_opts):
    """Skip unless a homi server serves this uPCIe backend; return its homi id"""

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a homi server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="Only uPCIe serves clients over a socket")

    return homi_id


def _sock(homi_id):
    return f"/tmp/xnvme-homi-{homi_id}.sock"


def _status(cijoe, homi_id):
    """Run 'homi status' and return its exit code and parsed document"""

    err, state = cijoe.run(f"homi status --homi-id {homi_id}")
    output = state.output()
    lines = output.split("\n")

    # A debug build logs before the document, and the CLI appends an error line
    start = next((i for i, ln in enumerate(lines) if ln.startswith("homi_id:")), None)
    assert start is not None, f"status emitted no document; output was: {output!r}"

    body = [ln for ln in lines[start:] if not ln.startswith("# ERR")]
    doc = yaml.safe_load("\n".join(body))
    assert doc is not None, f"status document did not parse; output was: {output!r}"

    return err, doc


def _ctrlr_of(doc, uri):
    """The entry for `uri` in a status document, or None"""

    for ctrlr in doc.get("controllers") or []:
        if ctrlr["uri"] == uri:
            return ctrlr
    return None


def _counts(cijoe, homi_id):
    """Connections, and queues in use per controller, from one status call"""

    _, doc = _status(cijoe, homi_id)

    return (
        doc.get("connections", 0),
        {c["uri"]: c["nsq_used"] for c in (doc.get("controllers") or [])},
    )


def _wait_for(cijoe, homi_id, predicate, timeout):
    """Poll _counts() until predicate(connections, used) holds"""

    connections, used = _counts(cijoe, homi_id)
    for _ in range(timeout):
        if predicate(connections, used):
            return True, connections, used
        sleep(1)
        connections, used = _counts(cijoe, homi_id)

    return False, connections, used


def _queues_on(cijoe, homi_id, uri):
    """Queues the server has handed out on `uri`"""

    _, doc = _status(cijoe, homi_id)
    ctrlr = _ctrlr_of(doc, uri)
    assert ctrlr, f"{uri} is not among the controllers the server holds"

    return ctrlr["nsq_used"]


def _alone(cijoe, homi_id):
    """Stop every client, so the only connection left is the one asking"""

    # The brackets stop pkill matching its own command line
    cijoe.run("pkill -f '[x]nvmeperf run ' || true")
    cijoe.run("pkill -f '[s]ocket.AF_UNIX' || true")
    ok, connections, _ = _wait_for(cijoe, homi_id, lambda c, _u: c == 1, 60)
    assert ok, f"the server reports {connections} connections with no clients running"


def _bg(cijoe, name, cmd):
    """
    Run `cmd` detached, its output in /tmp/homi-<name>.out and its exit status in .rc

    'setsid' and the closed stdin keep a remote transport from waiting on it.
    """

    cijoe.run(f"rm -f /tmp/homi-{name}.rc")
    cijoe.run(
        f"setsid sh -c '{cmd}; echo $? > /tmp/homi-{name}.rc'"
        f" < /dev/null > /tmp/homi-{name}.out 2>&1 &"
    )


def _bg_rc(cijoe, name):
    """The exit status of a _bg() job, or None while it runs"""

    err, state = cijoe.run(f"cat /tmp/homi-{name}.rc")

    return None if err else state.output().strip()


def _bg_wait(cijoe, name, timeout):
    """The exit status of a _bg() job, or None when it outlives `timeout` seconds"""

    for _ in range(timeout):
        rc = _bg_rc(cijoe, name)
        if rc is not None:
            return rc
        sleep(1)

    cijoe.run(f"cat /tmp/homi-{name}.out")

    return None


def _perf(uris, be, homi_id, runtime, nqueues=1):
    """An xnvmeperf client of `uris`, holding `nqueues` queues for `runtime` seconds"""

    return (
        f"xnvmeperf run {' '.join(uris)} --be {be} --homi-id {homi_id}"
        f" --iopattern randread --iosize 4096 --qdepth 8 --nqueues {nqueues}"
        f" --runtime {runtime} --cpulist 0"
    )


def _stop_clients(cijoe, uris):
    for uri in uris:
        cijoe.run(f"pkill -f '[x]nvmeperf run {uri} ' || true")


def _lblk_read(cijoe, dev, be_opts, homi_id, id_opt="--homi-id", timeout=None):
    """Read a block of `dev` as a client, which takes a queue and gives it back"""

    prefix = f"timeout {timeout} " if timeout else ""

    return cijoe.run(
        f"{prefix}lblk read {dev['uri']} --dev-nsid {dev['nsid']} --slba 0x0 --nlb 0"
        f" --be {be_opts['be']} --admin {be_opts['admin']} --sync {be_opts['sync']}"
        f" {id_opt} {homi_id}"
    )


def _held_devices():
    """
    The plain-block controllers the server holds

    Zoned and extended-LBA controllers refuse the 4 KiB reads clients here issue.
    Taken from the configuration rather than from status, which is served over
    the same socket and would shrink to the controllers that still work.
    """

    return [
        d
        for d in cijoe_config_get_all_devices(["pcie"])
        if "nvm" in (d.get("labels") or [])
        and not any(lbl.startswith("pi") for lbl in (d.get("labels") or []))
    ]


def _held_uris():
    return [d["uri"] for d in _held_devices()]


# The wire format, as nvme_cplane.h lays it out. Duplicated on purpose: a change
# to the message that forgets to bump NVME_CPLANE_VERSION fails these cases.
_MSG_NBYTES = 104
_MSG_HDR_NBYTES = 24  # op, version, status, nfds, index, reserved
_MSG_VERSION = 1  # NVME_CPLANE_VERSION
_OP_INIT_CONNECTION = 1
_OP_ALLOC_IOQPAIR = 2
_OP_FREE_IOQPAIR = 3
_OP_ALLOC_BUF = 5
_OP_FREE_BUF = 6
_OP_STATUS = 7

# Reads one reply into r; recv() without a control buffer drops its descriptors
_PY_RECV = (
    f"r=b'';[r := r + s.recv({_MSG_NBYTES} - len(r))"
    f" for _ in range(8) if len(r) < {_MSG_NBYTES}];"
)


def _py_msg(op, index=0, version=_MSG_VERSION, var="m"):
    """Python building a message in `var`"""

    return (
        f"{var}=bytearray({_MSG_NBYTES});"
        f"{var}[0:{_MSG_HDR_NBYTES}]="
        f"struct.pack('<IIiIII', {op}, {version}, 0, 0, {index}, 0);"
    )


def _py_client(homi_id, body, rcvbuf=None):
    """A python3 command connected to the server's socket as `s`, running `body`"""

    shrink = (
        f"s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, {rcvbuf});"
        if rcvbuf
        else ""
    )

    return (
        'python3 -c "import socket,struct,time;'
        "s=socket.socket(socket.AF_UNIX,socket.SOCK_SEQPACKET);"
        f"{shrink}s.connect('{_sock(homi_id)}');{body}\""
    )


def _hold(cijoe, homi_id, name, seconds, setup="", then="", rcvbuf=None):
    """
    Hold a connection for `seconds` in the background, running `setup` before it
    says it is connected and `then` after

    The shell returns zero for launching it whether or not it ran, and a server
    nobody speaks to passes every assertion here, so this waits for the marker.
    Matched whole-line, since a traceback quotes the line carrying the print.
    """

    out = f"/tmp/homi-{name}.out"
    body = f"{setup}print('connected',flush=True);{then}time.sleep({seconds})"

    err, _ = cijoe.run(
        f"setsid {_py_client(homi_id, body, rcvbuf)} < /dev/null > {out} 2>&1 &"
    )
    assert not err, f"could not start the {name} client"

    for _ in range(15):
        err, _ = cijoe.run(f"grep -qx connected {out}")
        if not err:
            return
        sleep(1)

    _, state = cijoe.run(f"cat {out}")
    pytest.fail(f"the {name} client never reached the server: {state.output()!r}")


def _stall(cijoe, homi_id, seconds):
    """Connect, send a byte that is neither a message nor a disconnect, and hold on"""

    _hold(cijoe, homi_id, "stalled", seconds, then="s.send(b'x');")


def _speak(cijoe, homi_id, op, version=_MSG_VERSION, payload_at=None, index=0):
    """
    Send one message and return the status of the reply

    `payload_at` is an (offset into the message body, 32-bit value) to write.
    No tool asks for what it knows is wrong, so the refusals are driven here.
    """

    poke = ""
    if payload_at is not None:
        at = _MSG_HDR_NBYTES + payload_at[0]
        poke = f"m[{at}:{at} + 4]=struct.pack('<I', {payload_at[1]});"

    err, state = cijoe.run(
        _py_client(
            homi_id,
            _py_msg(op, index, version)
            + poke
            + "s.sendall(bytes(m));"
            + _PY_RECV
            + "print('status', struct.unpack('<i', r[8:12])[0])",
        )
    )
    assert not err, f"the server did not answer: {state.output()!r}"

    m = re.search(r"status (-?\d+)", state.output())
    assert m, f"no status in the reply; output was {state.output()!r}"

    return int(m.group(1))


def _alloc_buffers(cijoe, homi_id, count, nbytes=4096):
    """
    Ask for `count` buffers on one connection, and return how many were granted

    Every request goes out before any reply is read. The replies fit the socket
    buffer at these counts, so the server never waits to write them.
    """

    total = count * _MSG_NBYTES
    body = (
        _py_msg(_OP_INIT_CONNECTION)
        + "s.sendall(bytes(m));"
        + _PY_RECV
        + _py_msg(_OP_ALLOC_BUF, var="a")
        + f"a[{_MSG_HDR_NBYTES}:{_MSG_HDR_NBYTES + 8}]=struct.pack('<Q', {nbytes});"
        + f"[s.sendall(bytes(a)) for _ in range({count})];b=b'';"
        + f"[b := b + s.recv({total} - len(b)) for _ in range(4000) if len(b) < {total}];"
        + f"print('allocated', sum(1 for k in range({count})"
        + f" if struct.unpack('<i', b[k * {_MSG_NBYTES} + 8:k * {_MSG_NBYTES} + 12])[0]"
        + " == 0))"
    )

    err, state = cijoe.run(_py_client(homi_id, body))
    assert not err, f"the allocating client failed: {state.output()!r}"

    m = re.search(r"allocated (\d+)", state.output())
    assert m, f"no count in the output; got {state.output()!r}"

    return int(m.group(1))


def test_upcie_status_refuses_when_absent(cijoe):
    """A build without uPCIe refuses to inspect, rather than report no server"""

    err, state = cijoe.run("xnvme library-info")
    assert not err, "could not ask the library what backends it has"

    if "name: 'upcie'" in state.output():
        pytest.skip(reason="This build has uPCIe; the refusal is a build without it")

    err, state = cijoe.run(f"homi status --homi-id {_UNCLAIMED_ID}")

    assert err, "status exits zero on a build that cannot inspect"
    # A document, even an empty one, would read as 'no server'
    assert "homi_id:" not in state.output()


def test_upcie_status_without_server(cijoe):
    """An id no server claimed reports as absent, and exits non-zero"""

    _require_upcie(cijoe)

    err, doc = _status(cijoe, _UNCLAIMED_ID)

    assert err, "status exits zero with no server running"
    assert doc["homi_id"] == _UNCLAIMED_ID
    assert doc["server_running"] is False
    assert doc["ready"] is False
    assert doc["controllers"] == []


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_client_refuses_without_server(cijoe, device, be_opts, cli_args):
    """An id no server claimed fails the open rather than running direct"""

    if be_opts["be"] not in ("upcie", "upcie-cuda", "upcie-hip"):
        pytest.skip(reason="Only the uPCIe backends connect to a server by id")

    err, _ = cijoe.run(
        f"xnvme info {device['uri']} --be {be_opts['be']} --homi-id {_UNCLAIMED_ID}"
    )

    assert err, "the open succeeded with nobody serving the id"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_status_with_server(cijoe, device, be_opts, cli_args):
    """A ready server reports what it holds, and exits zero"""

    homi_id = _served(cijoe, be_opts)

    err, doc = _status(cijoe, homi_id)

    assert not err, "status exits non-zero while a ready server holds the runtime"
    assert doc["server_running"] is True
    assert doc["ready"] is True
    assert doc["connections"] >= 1

    assert doc["controllers"], "a running server reports no controllers"
    for ctrlr in doc["controllers"]:
        assert ctrlr["readable"] is True
        assert ctrlr["initialized"] is True

        # The admin queue is subtracted, so a wrap shows as a huge count
        assert ctrlr["nsq_used"] < 65536, "queue count looks like an unsigned wrap"
        assert ctrlr["ncq_used"] == ctrlr["nsq_used"]

        if "nsq_total" in ctrlr:
            assert ctrlr["nsq_used"] <= ctrlr["nsq_total"]
            assert ctrlr["ncq_used"] <= ctrlr["ncq_total"]


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_queue_count_tracks_a_client(cijoe, device, be_opts, cli_args):
    """
    A client asking for N queues takes exactly N, and gives them back on exit

    Exactly N: the client submits asynchronously, so it takes no sync queue,
    and admin carries its own PRP scratch.
    """

    homi_id = _served(cijoe, be_opts)
    uri = device["uri"]
    nqueues = 2

    baseline = _queues_on(cijoe, homi_id, uri)

    _bg(cijoe, "qcount", _perf([uri], be_opts["be"], homi_id, 20, nqueues))

    came, _, used = _wait_for(
        cijoe, homi_id, lambda _c, u: u.get(uri, 0) > baseline, 30
    )
    if not came:
        cijoe.run("cat /tmp/homi-qcount.out")
    assert came, "the client never showed up in the queue count"
    assert used[uri] == baseline + nqueues

    went, _, used = _wait_for(
        cijoe, homi_id, lambda _c, u: u.get(uri, 0) <= baseline, 60
    )
    assert went, f"queues were not released; {used.get(uri)} against {baseline}"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_admin_costs_no_ioqueue(cijoe, device, be_opts, cli_args):
    """A client issuing only admin commands takes no I/O queue"""

    homi_id = _served(cijoe, be_opts)
    uri = device["uri"]

    baseline = _queues_on(cijoe, homi_id, uri)

    _bg(
        cijoe,
        "admin",
        f"for i in $(seq 60); do xnvme info {uri} --be {be_opts['be']}"
        f" --homi-id {homi_id} >/dev/null 2>&1; done",
    )

    # Sampled while the clients run, since a queue goes back on exit
    peak = baseline
    for _ in range(90):
        _, used = _counts(cijoe, homi_id)
        peak = max(peak, used.get(uri, 0))
        if _bg_rc(cijoe, "admin") is not None:
            break
        sleep(0.2)
    else:
        cijoe.run("cat /tmp/homi-admin.out")
        pytest.fail("the admin clients never finished")

    assert (
        peak == baseline
    ), f"an admin-only client took an I/O queue: {baseline} -> {peak}"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_totals_come_from_the_controller(cijoe, device, be_opts, cli_args):
    """Each controller's totals are its own answer to Number of Queues"""

    homi_id = _served(cijoe, be_opts)

    _, doc = _status(cijoe, homi_id)
    ctrlr = _ctrlr_of(doc, device["uri"])
    assert ctrlr, f"{device['uri']} is not among the controllers the server holds"

    if "nsq_total" not in ctrlr:
        pytest.skip(reason="the controller did not report its queue allocation")

    err, state = cijoe.run(f"xnvme feature-get {cli_args} --fid 0x7 --sel 0")
    assert not err, "could not ask the controller for Number of Queues"

    # NSQA and NCQA are zero-based
    m = re.search(r"nsqa:\s*(\d+),\s*ncqa:\s*(\d+)", state.output())
    assert m, f"no nqueues feature in output: {state.output()[-400:]}"

    assert ctrlr["nsq_total"] == int(m.group(1)) + 1
    assert ctrlr["ncq_total"] == int(m.group(2)) + 1


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_status_does_not_disturb_election(cijoe, device, be_opts, cli_args):
    """
    Polling status neither steals nor breaks the role a running server holds

    A probe racing a starting server is not covered.
    """

    homi_id = _served(cijoe, be_opts)

    # 'sh -c' keeps it a simple command, which a transport prefixing
    # environment assignments ('cijoe.run.env') needs
    err, _ = cijoe.run(
        "sh -c 'for i in $(seq 1 200); do homi status"
        f" --homi-id {homi_id} >/dev/null || exit 1; done'"
    )
    assert not err, "status stopped reporting the server while being polled"

    err, doc = _status(cijoe, homi_id)
    assert not err
    assert doc["server_running"] is True


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_server_terminates_promptly(cijoe, device, be_opts, cli_args):
    """
    A server asked to stop does so within seconds

    A server outliving its signal passes every later case and stalls the run at
    session teardown instead. The device fixture starts a fresh one afterwards.
    """

    homi_id = _served(cijoe, be_opts)

    assert HomiServer.is_running(cijoe), "no server to stop"

    start = time()
    HomiServer.stop(cijoe)
    elapsed = time() - start

    # Generous next to a teardown of about a second
    assert elapsed < 15, f"server took {elapsed:.1f}s to terminate"

    _, doc = _status(cijoe, homi_id)
    assert doc["server_running"] is False, "status still reports a live server"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_server_runs_on_its_drives_node(cijoe, device, be_opts, cli_args):
    """
    A server whose drives share a NUMA node runs on that node's cores

    Skipped where sysfs places the drives on no node, or on several, since the
    server then leaves its placement alone.
    """

    _served(cijoe, be_opts)
    if HomiServer.RUNNING is None:
        pytest.skip(reason="No server started by this run to inspect")

    _, labels = HomiServer.RUNNING
    nodes = set()
    for dev in cijoe_config_get_all_devices(list(labels)):
        err, state = cijoe.run(f"cat /sys/bus/pci/devices/{dev['uri']}/numa_node")
        nodes.add(-1 if err else int(state.output().strip()))
    if len(nodes) != 1 or min(nodes) < 0:
        pytest.skip(reason=f"The drives are on NUMA nodes {sorted(nodes)}")

    err, want = cijoe.run(f"cat /sys/devices/system/node/node{nodes.pop()}/cpulist")
    assert not err, "sysfs places the drives on a node it does not describe"

    err, got = cijoe.run("grep Cpus_allowed_list /proc/$(pgrep -x homi)/status")
    assert not err, "could not read the server's affinity"
    assert got.output().split(":")[-1].strip() == want.output().strip()


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_runtime_is_reachable_by_its_name(cijoe, device, be_opts, cli_args):
    """The server listens on the socket clients derive from --homi-id"""

    homi_id = _served(cijoe, be_opts)

    err, _ = cijoe.run(f"test -S {_sock(homi_id)}")
    assert not err, "a running server has no socket at the name clients use"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_a_killed_server_leaves_nothing_to_clean_up(cijoe, device, be_opts, cli_args):
    """A server killed outright leaves nothing that reads as a live server"""

    homi_id = _served(cijoe, be_opts)

    assert HomiServer.is_running(cijoe), "no server to kill"

    cijoe.run("pkill -9 -x homi")
    HomiServer.forget()

    for _ in range(30):
        sleep(1)
        if not HomiServer.is_running(cijoe):
            break
    else:
        pytest.fail("the server survived SIGKILL")

    err, doc = _status(cijoe, homi_id)

    assert err, "status exits zero with no server running"
    assert doc["server_running"] is False
    assert doc["ready"] is False
    assert doc["controllers"] == []


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_io_lands_on_the_controller_it_was_asked_for(cijoe, device, be_opts, cli_args):
    """
    A client of two controllers puts a queue on each, not both on the first

    The I/O succeeds either way, so where the queues appear is what is asserted.
    """

    homi_id = _served(cijoe, be_opts)

    uris = _held_uris()
    if len(uris) < 2:
        pytest.skip(reason="Requires two controllers to tell one from the other")

    pair = uris[:2]
    runtime = 20

    _bg(cijoe, "spread", _perf(pair, be_opts["be"], homi_id, runtime))

    # A client that never reached the server leaves every count at zero, which
    # says nothing about placement
    reached, _, used = _wait_for(
        cijoe, homi_id, lambda _c, u: any(u.get(uri, 0) > 0 for uri in pair), 30
    )
    if not reached:
        cijoe.run("cat /tmp/homi-spread.out")
    assert reached, f"the client never took a queue from the server; saw {used}"

    spread, _, used = _wait_for(
        cijoe, homi_id, lambda _c, u: all(u.get(uri, 0) > 0 for uri in pair), 20
    )
    assert spread, f"a client of {pair} put queues on {used}"

    # A client still holding a queue moves the counts the next case reads
    assert (
        _bg_wait(cijoe, "spread", runtime + 30) is not None
    ), f"the client of {pair} never finished"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_clients_come_and_go_across_controllers(cijoe, device, be_opts, cli_args):
    """
    Clients of one, some and all controllers reach those and no others, and let go

    A client that cannot reach the server opens the controller itself and its I/O
    succeeds, so the server's counts are asserted rather than exit status.
    """

    homi_id = _served(cijoe, be_opts)
    be = be_opts["be"]

    uris = _held_uris()
    assert uris, "a running server reports no controllers"

    shapes = [[uri] for uri in uris]
    if len(uris) > 2:
        shapes.append(uris[: len(uris) // 2])
    if len(uris) > 1:
        shapes.append(uris)

    # Leftover clients would count into the baseline and then leave, which
    # reads as the new client never arriving
    _alone(cijoe, homi_id)

    try:
        for shape in shapes:
            connections, used = _counts(cijoe, homi_id)

            # Running until killed, so leaving is not a race against a timer
            for uri in shape:
                tag = uri.replace(":", "-").replace(".", "-")
                _bg(cijoe, f"client-{tag}", _perf([uri], be, homi_id, 3600))

            # A client connects before it allocates its queue, so wait for both
            came, now_connected, now_used = _wait_for(
                cijoe,
                homi_id,
                lambda c, u, want=connections + len(shape), was=used, asked=shape: (
                    c == want and all(u.get(x, 0) > was.get(x, 0) for x in asked)
                ),
                30,
            )
            assert came, (
                f"the server reports {now_connected} clients and queues {now_used}"
                f" for {sorted(shape)}, expected {connections + len(shape)} clients"
                f" and a queue more than {used} on each"
            )

            for uri in uris:
                gained = now_used.get(uri, 0) - used.get(uri, 0)
                if uri in shape:
                    assert gained > 0, f"{uri} was asked for but gained no queue"
                else:
                    assert (
                        gained == 0
                    ), f"{uri} gained a queue while only {sorted(shape)} were asked for"

            _stop_clients(cijoe, shape)

            went, now_connected, now_used = _wait_for(
                cijoe,
                homi_id,
                lambda c, u, wc=connections, wu=used: c == wc and u == wu,
                60,
            )
            assert went, (
                f"the server did not take {sorted(shape)} back; it reports"
                f" {now_connected} clients and {now_used}, expected {connections}"
                f" and {used}"
            )
    finally:
        _stop_clients(cijoe, uris)


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_a_queue_survives_a_release_on_another_controller(
    cijoe, device, be_opts, cli_args
):
    """
    Releasing a queue on one controller leaves the same qid on another alone

    Queue identifiers are per controller, so one looked up by qid alone frees
    another client's queue under it. The counts stay right either way, so the
    holder's exit status is what tells.
    """

    homi_id = _served(cijoe, be_opts)

    devices = _held_devices()
    if len(devices) < 2:
        pytest.skip(reason="Requires two controllers to release on the other one")

    holder, visitor = devices[0], devices[1]
    runtime = 20

    baseline = _queues_on(cijoe, homi_id, holder["uri"])

    _bg(cijoe, "holder", _perf([holder["uri"]], be_opts["be"], homi_id, runtime))

    took, _, used = _wait_for(
        cijoe, homi_id, lambda _c, u: u.get(holder["uri"], 0) > baseline, 30
    )
    assert took, f"the client never took a queue on {holder['uri']}; saw {used}"

    for _ in range(5):
        err, _ = _lblk_read(cijoe, visitor, be_opts, homi_id)
        assert not err, f"a client of {visitor['uri']} could not read"

    rc = _bg_wait(cijoe, "holder", runtime + 30)
    assert (
        rc is not None
    ), f"the client holding a queue on {holder['uri']} never finished"
    assert rc == "0", (
        f"the client holding a queue on {holder['uri']} exited {rc} while clients"
        f" came and went on {visitor['uri']}"
    )


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_a_client_outlives_the_server_shutting_down(cijoe, device, be_opts, cli_args):
    """
    A client whose server stops mid-run gives up its in-flight I/O and exits zero

    The server deletes its clients' queues as it goes, so a client waiting out
    those commands would hang and pin the heap. The device fixture starts a
    fresh server afterwards.
    """

    homi_id = _served(cijoe, be_opts)
    uri = device["uri"]
    runtime = 15

    baseline = _queues_on(cijoe, homi_id, uri)

    _bg(cijoe, "shutdown", _perf([uri], be_opts["be"], homi_id, runtime))

    took, _, used = _wait_for(
        cijoe, homi_id, lambda _c, u: u.get(uri, 0) > baseline, 30
    )
    assert took, f"the client never took a queue on {uri}; saw {used}"

    HomiServer.stop(cijoe)

    # The runtime, the command timeout for noticing, and slack for a loaded host
    rc = _bg_wait(cijoe, "shutdown", runtime + 45)
    if rc is None:
        cijoe.run("pgrep -a -x xnvmeperf")
    assert rc is not None, f"the client of {uri} never terminated after its server did"
    assert rc == "0", f"the client of {uri} exited {rc} after its server shut down"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_the_two_spellings_name_one_identifier(cijoe, device, be_opts, cli_args):
    """A client passing --shm_id reaches a server started with --homi-id"""

    homi_id = _served(cijoe, be_opts)
    uri = device["uri"]

    baseline = _queues_on(cijoe, homi_id, uri)

    err, _ = _lblk_read(cijoe, device, be_opts, homi_id, id_opt="--shm_id")
    assert not err, f"a client of {uri} could not read with --shm_id"

    returned, _, used = _wait_for(
        cijoe, homi_id, lambda _c, u: u.get(uri, 0) == baseline, 30
    )
    assert returned, f"queues were not released; {used.get(uri)} against {baseline}"

    err, doc = _status(cijoe, homi_id)
    assert not err, "status did not answer for the identifier the client used"
    assert doc["homi_id"] == homi_id


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_a_stalled_client_holds_up_nobody(cijoe, device, be_opts, cli_args):
    """A connection that sends a stray byte and stays open does not block others"""

    homi_id = _served(cijoe, be_opts)
    uri = device["uri"]

    _stall(cijoe, homi_id, 20)

    # Well inside the stall, where a server waiting on it for more still waits
    sleep(2)

    started = time()
    err, doc = _status(cijoe, homi_id)
    elapsed = time() - started

    assert not err, "status did not answer while a connection was stalled"
    assert _ctrlr_of(doc, uri), f"{uri} is not among the controllers the server holds"
    assert elapsed < 10, f"status took {elapsed:.1f}s while a connection was stalled"

    for _ in range(30):
        sleep(1)
        err, _ = _status(cijoe, homi_id)
        if not err:
            break
    else:
        pytest.fail("status stopped answering after the stalled connection ended")


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_two_clients_hold_queues_at_once(cijoe, device, be_opts, cli_args):
    """
    Two clients of one controller are both served, and both let go of

    Requests are short, so this says nothing about whether they are answered
    side by side or one after the other.
    """

    homi_id = _served(cijoe, be_opts)
    uri = device["uri"]
    nqueues = 2
    both = 2 * nqueues

    baseline = _queues_on(cijoe, homi_id, uri)

    for i in (1, 2):
        _bg(cijoe, f"two-{i}", _perf([uri], be_opts["be"], homi_id, 25, nqueues))

    came, _, used = _wait_for(
        cijoe, homi_id, lambda _c, u: u.get(uri, 0) >= baseline + both, 40
    )
    if not came:
        cijoe.run("cat /tmp/homi-two-1.out /tmp/homi-two-2.out")
    assert came, f"both clients never held queues together; saw {used.get(uri)}"
    assert used[uri] == baseline + both

    went, _, used = _wait_for(
        cijoe, homi_id, lambda _c, u: u.get(uri, 0) <= baseline, 60
    )
    assert went, f"queues were not released; {used.get(uri)} against {baseline}"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_one_client_costs_one_connection_for_every_controller(
    cijoe, device, be_opts, cli_args
):
    """A client of several controllers is one connection, counted on each of them"""

    homi_id = _served(cijoe, be_opts)

    _alone(cijoe, homi_id)

    # In index order, which is what the requests name; the server holds every
    # 'pcie' device, which _held_uris() filters
    _, before = _status(cijoe, homi_id)
    uris = [c["uri"] for c in (before.get("controllers") or [])]
    if len(uris) < 2:
        pytest.skip(reason="One connection covering several needs several")

    held = list(range(min(3, len(uris))))
    init = "".join(
        _py_msg(_OP_INIT_CONNECTION, i) + "s.sendall(bytes(m));" + _PY_RECV
        for i in held
    )

    _hold(cijoe, homi_id, "held", 30, setup=init)

    # The held connection and the one asking
    came, connections, _ = _wait_for(cijoe, homi_id, lambda c, _u: c == 2, 30)
    assert came, f"{connections} connections for one client of {len(held)} controllers"

    # The client connects before it initialises each controller, so poll for those
    for _ in range(30):
        _, doc = _status(cijoe, homi_id)
        gains = [
            (_ctrlr_of(doc, uri) or {}).get("connections", 0)
            - (_ctrlr_of(before, uri) or {}).get("connections", 0)
            for uri in uris
        ]
        if all(gained == (1 if i in held else 0) for i, gained in enumerate(gains)):
            break
        sleep(1)

    for i, (uri, gained) in enumerate(zip(uris, gains)):
        assert gained == (1 if i in held else 0), (
            f"{uri} {'was' if i in held else 'was not'} initialised on the held"
            f" connection, and its client count moved by {gained}"
        )

    cijoe.run("pkill -f '[s]ocket.AF_UNIX' || true")


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_a_client_holding_queues_on_several_controllers_gives_them_all_back(
    cijoe, device, be_opts, cli_args
):
    """
    A killed client's queues go back to every controller that lent one

    Killed, since a client exiting tidily hands its queues back itself.
    """

    homi_id = _served(cijoe, be_opts)

    _alone(cijoe, homi_id)

    _, before = _status(cijoe, homi_id)
    uris = [c["uri"] for c in (before.get("controllers") or [])]
    if len(uris) < 2:
        pytest.skip(reason="Queues on several controllers needs several")

    held = [uris[i] for i in range(2)]
    was = {c["uri"]: c["nsq_used"] for c in (before.get("controllers") or [])}

    # u.alloc_ioqpair.depth, past the allocation it replies with
    depth_at = _MSG_HDR_NBYTES + 32
    steps = "".join(
        _py_msg(_OP_INIT_CONNECTION, i)
        + "s.sendall(bytes(m));"
        + _PY_RECV
        + _py_msg(_OP_ALLOC_IOQPAIR, i)
        + f"m[{depth_at}:{depth_at} + 2]=struct.pack('<H', 8);"
        + "s.sendall(bytes(m));"
        + _PY_RECV
        for i in range(2)
    )

    _hold(cijoe, homi_id, "queues", 60, setup=steps)

    took, _, used = _wait_for(
        cijoe, homi_id, lambda _c, u: all(u.get(x, 0) > was.get(x, 0) for x in held), 30
    )
    assert took, f"one connection did not take a queue on each of {held}; saw {used}"

    cijoe.run("pkill -f '[s]ocket.AF_UNIX' || true")

    gave, _, used = _wait_for(
        cijoe,
        homi_id,
        lambda _c, u: all(u.get(x, 0) == was.get(x, 0) for x in held),
        60,
    )
    assert gave, f"queues on {held} were not all given back; {used} against {was}"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_a_client_allocates_a_buffer_per_queue_entry(cijoe, device, be_opts, cli_args):
    """
    One connection holds more buffers than one per tag at qublk's default depth

    That is 64, which the fixed table this replaced refused past.
    """

    homi_id = _served(cijoe, be_opts)

    wanted = 96
    got = _alloc_buffers(cijoe, homi_id, wanted)

    assert got == wanted, f"the server allocated {got} of {wanted} buffers"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_buffers_are_bounded_by_the_heap(cijoe, device, be_opts, cli_args):
    """
    Small buffers are bounded by the heap, not by a table of the 2 MiB pages they share

    160 buffers of 1 MiB take 80 such pages; a table of 64 refused qublk's fifth
    device on a heap with room for ten.
    """

    homi_id = _served(cijoe, be_opts)

    # homi gives clients 64 MiB of heap per controller it serves
    wanted = 160
    if 64 * len(cijoe_config_get_all_devices(["pcie"])) < 2 * wanted:
        pytest.skip(reason="The server's heap is too small to tell the table from it")

    got = _alloc_buffers(cijoe, homi_id, wanted, nbytes=1024 * 1024)

    assert got == wanted, f"the server allocated {got} of {wanted} buffers of 1 MiB"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_a_client_that_stops_reading_holds_up_nobody(cijoe, device, be_opts, cli_args):
    """
    A client that sends requests and never reads the replies does not block others

    Its receive buffer is shrunk so it fills after a few dozen replies, while
    its own send buffer never does: the client keeps the connection open,
    which is what keeps a server waiting to write stuck.
    """

    homi_id = _served(cijoe, be_opts)
    uri = device["uri"]

    _hold(
        cijoe,
        homi_id,
        "deaf",
        25,
        setup=_py_msg(_OP_STATUS),
        then="[s.sendall(bytes(m)) for _ in range(400)];",
        rcvbuf=2048,
    )

    sleep(2)

    started = time()
    err, doc = _status(cijoe, homi_id)
    elapsed = time() - started

    assert not err, "status did not answer while a client was not reading"
    assert _ctrlr_of(doc, uri), f"{uri} is not among the controllers the server holds"
    assert elapsed < 10, f"status took {elapsed:.1f}s while a client was not reading"

    cijoe.run("pkill -f '[s]ocket.AF_UNIX' || true")
    for _ in range(30):
        sleep(1)
        err, _ = _status(cijoe, homi_id)
        if not err:
            break
    else:
        pytest.fail("status stopped answering after the deaf client ended")


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_a_stranger_version_is_refused(cijoe, device, be_opts, cli_args):
    """A message of an unknown protocol version is refused with -EPROTO"""

    homi_id = _served(cijoe, be_opts)

    status = _speak(cijoe, homi_id, _OP_STATUS, version=0xBADF00D)
    assert status == -71, f"expected -EPROTO for a stranger's version, got {status}"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_an_unknown_request_is_refused(cijoe, device, be_opts, cli_args):
    """An unknown opcode is refused with -ENOSYS rather than misread"""

    homi_id = _served(cijoe, be_opts)

    status = _speak(cijoe, homi_id, 99)
    assert status == -38, f"expected -ENOSYS for an unknown opcode, got {status}"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_asking_for_resources_before_initialising_is_refused(
    cijoe, device, be_opts, cli_args
):
    """
    A connection that has not initialised cannot take a queue or memory

    Either reply is a heap offset, which such a connection has no mapping for.
    """

    homi_id = _served(cijoe, be_opts)

    # A depth at offset 32 of the body and a byte count at 0, so each request is
    # one the server would otherwise honour
    status = _speak(cijoe, homi_id, _OP_ALLOC_IOQPAIR, payload_at=(32, 8))
    assert status == -107, f"expected -ENOTCONN for a queue before init, got {status}"

    status = _speak(cijoe, homi_id, _OP_ALLOC_BUF, payload_at=(0, 4096))
    assert status == -107, f"expected -ENOTCONN for memory before init, got {status}"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_handing_back_what_was_never_held_is_refused(cijoe, device, be_opts, cli_args):
    """A client can only hand back what it holds, and only name controllers there are"""

    homi_id = _served(cijoe, be_opts)
    uri = device["uri"]

    status = _speak(cijoe, homi_id, _OP_FREE_IOQPAIR, payload_at=(0, 4095))
    assert status == -2, f"expected -ENOENT for a queue never held, got {status}"

    status = _speak(cijoe, homi_id, _OP_FREE_BUF, payload_at=(8, 0xDEAD))
    assert status == -2, f"expected -ENOENT for memory never held, got {status}"

    status = _speak(cijoe, homi_id, _OP_STATUS, index=4096)
    assert (
        status == -34
    ), f"expected -ERANGE for a controller past the last, got {status}"

    _, doc = _status(cijoe, homi_id)
    assert _ctrlr_of(
        doc, uri
    ), "the server stopped holding the controller after a refusal"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_a_stalled_client_does_not_stop_a_working_one(cijoe, device, be_opts, cli_args):
    """A client attaches, takes a queue and reads while another connection stalls"""

    homi_id = _served(cijoe, be_opts)
    stall = 30

    _stall(cijoe, homi_id, stall)

    sleep(2)

    # Bounded, since a server serialising its clients does not answer slowly but
    # not at all, and an unbounded wait is a stuck job rather than a red test
    started = time()
    err, _ = _lblk_read(cijoe, device, be_opts, homi_id, timeout=stall // 3)
    elapsed = time() - started

    assert not err, f"a client gave up after {elapsed:.1f}s against a {stall}s stall"
    assert elapsed < (
        stall / 2
    ), f"the read took {elapsed:.1f}s against a {stall}s stall"
