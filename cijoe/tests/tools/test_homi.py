import re
from time import sleep, time

import pytest
import yaml

from ..conftest import (
    CPlaneServer,
    cijoe_config_get_all_devices,
    get_homi_id,
    xnvme_parametrize,
)

# Selection is '-k' against the test id, so the cases taking no device carry
# 'upcie' in their name to run beside their parametrized siblings.


def _require_upcie(cijoe):
    """
    Skip unless this build has the uPCIe backend.

    Inspection reads the state the uPCIe runtime keeps, so 'homi status' exists
    only where that backend is built, which is Linux and only when it was
    enabled. Asking the library what it has beats deciding from the platform:
    a Linux build configured with -Dbe_upcie=false is in the same position, and
    an errno cannot be compared portably.
    """

    err, state = cijoe.run("xnvme library-info")
    assert not err, "could not ask the library what backends it has"

    if "name: 'upcie'" not in state.output():
        pytest.skip(reason="Requires the uPCIe backend; this build does not have it")


def _status(cijoe, homi_id):
    """Run 'homi status' and return its exit code and parsed document"""

    err, state = cijoe.run(f"homi status --homi-id {homi_id}")
    output = state.output()
    lines = output.split("\n")

    # A debug build logs before the document starts, and the CLI appends its
    # own error line after it; neither is part of the document
    start = next((i for i, ln in enumerate(lines) if ln.startswith("homi_id:")), None)
    assert start is not None, f"status emitted no document; output was: {output!r}"

    body = [ln for ln in lines[start:] if not ln.startswith("# ERR")]
    doc = yaml.safe_load("\n".join(body))

    assert doc is not None, f"status document did not parse; output was: {output!r}"

    return err, doc


def test_upcie_status_refuses_when_absent(cijoe):
    """
    A build that cannot inspect says so, rather than reporting nothing found.

    This is the inverse of what every other testcase here skips on, and it is
    the case the other platforms actually run. 'nothing is running' and 'this
    build cannot tell you' are different answers, and a caller gating on the
    exit status has to be able to distinguish them.
    """

    err, state = cijoe.run("xnvme library-info")
    assert not err, "could not ask the library what backends it has"

    if "name: 'upcie'" in state.output():
        pytest.skip(
            reason="This build has uPCIe; the refusal is what a build without it does"
        )

    err, state = cijoe.run("homi status --homi-id 4242")

    assert err, "status exits zero on a build that cannot inspect"
    assert "requires the uPCIe backend" in state.output()

    # A refusal, not an empty document, which would read as 'no server'
    assert "homi_id:" not in state.output()


def test_upcie_status_without_server(cijoe):
    """An id no server claimed reports as absent, and says so in its exit code"""

    _require_upcie(cijoe)

    # Well above what the suite hands out, so it cannot collide
    homi_id = 4242

    err, doc = _status(cijoe, homi_id)

    assert err, "status exits zero with no server running"
    assert doc["homi_id"] == homi_id
    assert doc["server_running"] is False
    assert doc["ready"] is False
    assert doc["controllers"] == []


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_status_with_server(cijoe, device, be_opts, cli_args):
    """A held runtime reports what it holds, and exits zero once it is ready"""

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(
            reason="status reads what the uPCIe runtime holds; others are opaque"
        )

    err, doc = _status(cijoe, homi_id)

    assert not err, "status exits non-zero while a ready server holds the runtime"
    assert doc["server_running"] is True
    assert doc["ready"] is True

    assert doc["connections"] >= 1

    assert doc["controllers"], "a running server reports no controllers"
    for ctrlr in doc["controllers"]:
        assert ctrlr["readable"] is True
        assert ctrlr["initialized"] is True

        # The admin queue is excluded by subtraction, so an unsigned wrap
        # would surface as a huge count rather than a negative one
        assert ctrlr["nsq_used"] < 65536, "queue count looks like an unsigned wrap"
        assert ctrlr["ncq_used"] == ctrlr["nsq_used"]

        if "nsq_total" in ctrlr:
            assert ctrlr["nsq_used"] <= ctrlr["nsq_total"]
            assert ctrlr["ncq_used"] <= ctrlr["ncq_total"]


def _ctrlr_of(doc, uri):
    """The entry for `uri` in a status document, or None"""

    for ctrlr in doc.get("controllers") or []:
        if ctrlr["uri"] == uri:
            return ctrlr
    return None


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_queue_count_tracks_a_client(cijoe, device, be_opts, cli_args):
    """
    The reported queue count follows what a client allocates.

    A stale or double-counted value still looks plausible, so what pins this is
    the count moving with `--nqueues` rather than merely being in range: a
    client asking for N takes exactly N, and gives them all back when it exits.

    N and no more, because this client submits asynchronously. The sync queue
    pair is allocated on first synchronous I/O and this never does any; admin
    carries its own PRP scratch, so asking the controller about itself at open
    no longer drags one in.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(
            reason="status reads what the uPCIe runtime holds; others are opaque"
        )

    uri = device["uri"]
    nqueues = 2

    _, doc = _status(cijoe, homi_id)
    before = _ctrlr_of(doc, uri)
    assert before, f"{uri} is not among the controllers the server holds"
    baseline = before["nsq_used"]

    # 'setsid' and the closed stdin for the same reason CPlaneServer.start uses
    # them: backgrounding alone leaves this holding the transport's channel when
    # the target is remote, and the client never runs at all
    cijoe.run(
        f"setsid xnvmeperf run {uri} --be {be_opts['be']} --homi-id {homi_id}"
        f" --iopattern randread --iosize 4096 --qdepth 8 --nqueues {nqueues}"
        f" --runtime 20 --cpulist 0 < /dev/null > /tmp/qcount.out 2>&1 &"
    )

    peak = baseline
    for _ in range(30):
        sleep(1)
        _, doc = _status(cijoe, homi_id)
        ctrlr = _ctrlr_of(doc, uri)
        if ctrlr and ctrlr["nsq_used"] > baseline:
            peak = ctrlr["nsq_used"]
            break
    else:
        cijoe.run("cat /tmp/qcount.out")
        pytest.fail("the client never showed up in the queue count")

    assert peak == baseline + nqueues, f"expected {baseline} + {nqueues}, got {peak}"
    assert ctrlr["ncq_used"] == ctrlr["nsq_used"]

    for _ in range(60):
        sleep(1)
        _, doc = _status(cijoe, homi_id)
        ctrlr = _ctrlr_of(doc, uri)
        if ctrlr and ctrlr["nsq_used"] <= baseline:
            break
    else:
        still = ctrlr["nsq_used"] if ctrlr else "the controller is gone"
        pytest.fail(f"queues were not released; still {still}, expected {baseline}")


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_admin_costs_no_ioqueue(cijoe, device, be_opts, cli_args):
    """
    Asking the controller about itself takes no I/O queue.

    Admin is a single queue and it is shared, whereas an I/O queue is dedicated
    to the client holding it and is what the server has fewest of. A client
    that only issues admin commands should cost none.

    The count is sampled while such clients run, not after: a borrowed queue is
    handed back on exit and leaves nothing behind to find, so an after-the-fact
    reading passes whether or not one was taken.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(
            reason="status reads what the uPCIe runtime holds; other backends are opaque"
        )

    uri = device["uri"]

    _, doc = _status(cijoe, homi_id)
    before = _ctrlr_of(doc, uri)
    assert before, f"{uri} is not among the controllers the server holds"
    baseline = before["nsq_used"]

    # 'setsid' and the closed stdin for the same reason CPlaneServer.start uses
    # them, and a marker file rather than a wait, since the loop is backgrounded
    # on the target rather than here
    cijoe.run("rm -f /tmp/admin_loop.done")
    cijoe.run(
        "setsid sh -c 'for i in $(seq 60); do"
        f" xnvme info {uri} --be {be_opts['be']} --homi-id {homi_id} >/dev/null 2>&1;"
        " done; touch /tmp/admin_loop.done'"
        " < /dev/null > /tmp/admin_loop.out 2>&1 &"
    )

    peak = baseline
    for _ in range(90):
        _, doc = _status(cijoe, homi_id)
        ctrlr = _ctrlr_of(doc, uri)
        if ctrlr:
            peak = max(peak, ctrlr["nsq_used"])
        err, _ = cijoe.run("test -f /tmp/admin_loop.done")
        if not err:
            break
        sleep(0.2)
    else:
        cijoe.run("cat /tmp/admin_loop.out")
        pytest.fail("the admin clients never finished")

    assert peak == baseline, (
        f"an admin-only client took an I/O queue: {baseline} -> {peak}."
        " Admin carries its own PRP scratch and should borrow no queue"
    )


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_totals_come_from_the_controller(cijoe, device, be_opts, cli_args):
    """
    The totals are read per controller rather than assumed.

    A constant would satisfy every other assertion here, so this checks the
    reported ceiling against what the controller answers for itself.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(
            reason="status reads what the uPCIe runtime holds; others are opaque"
        )

    _, doc = _status(cijoe, homi_id)
    ctrlr = _ctrlr_of(doc, device["uri"])
    assert ctrlr, f"{device['uri']} is not among the controllers the server holds"

    if "nsq_total" not in ctrlr:
        pytest.skip(reason="the controller did not report its queue allocation")

    err, state = cijoe.run(f"xnvme feature-get {cli_args} --fid 0x7 --sel 0")
    assert not err, "could not ask the controller for Number of Queues"

    # NSQA and NCQA are zero-based; the totals are those plus one
    m = re.search(r"nsqa:\s*(\d+),\s*ncqa:\s*(\d+)", state.output())
    assert m, f"no nqueues feature in output: {state.output()[-400:]}"

    assert ctrlr["nsq_total"] == int(m.group(1)) + 1
    assert ctrlr["ncq_total"] == int(m.group(2)) + 1


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_status_does_not_disturb_election(cijoe, device, be_opts, cli_args):
    """
    Probing must not disturb the role-election lock.

    This hammers the probe against a lock that is already held, so what it
    guards is that probing neither steals nor breaks a held role. The sharper
    hazard, a probe taking the lock during the window where it is still free
    and demoting a concurrent elector, needs a probe racing a starting server
    and is not covered here.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(
            reason="status reads what the uPCIe runtime holds; others are opaque"
        )

    # Wrapped in 'sh -c' so it stays a simple command: a configuration that
    # sets 'cijoe.run.env' has the transport prefix every command with
    # assignments, and those are a syntax error ahead of a compound one
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
    A server asked to stop does stop, and does so quickly.

    Closing a controller walks the queue-id bitmap to reap what is still
    allocated. A bound the loop counter cannot represent makes that walk
    endless, and the server then outlives its signal: every later testcase
    still passes, and the run instead stalls at session teardown, far from
    the change that caused it. Bounding it here names it.

    This leaves no server behind on purpose; the device fixture starts a
    fresh one for whichever testcase runs next.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(
            reason="status reads what the uPCIe runtime holds; others are opaque"
        )

    assert CPlaneServer.is_running(cijoe), "no server to stop"

    # Generous next to a teardown of about a second
    budget = 15

    start = time()
    CPlaneServer.stop(cijoe)
    elapsed = time() - start

    assert elapsed < budget, f"server took {elapsed:.1f}s to terminate"

    _, doc = _status(cijoe, homi_id)
    assert doc["server_running"] is False, "status still reports a live server"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_runtime_is_reachable_by_its_name(cijoe, device, be_opts, cli_args):
    """
    A runtime is one socket, named for the identifier clients pass.

    That name is the whole rendezvous: a client derives it from --homi-id and
    finds the server or does not. A rename that misses it leaves clients
    unable to attach while everything else looks healthy.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="Only uPCIe serves clients over a socket")

    err, _ = cijoe.run(f"test -S /tmp/xnvme-homi-{homi_id}.sock")
    assert not err, "a running server has no socket at the name clients use"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_a_killed_server_leaves_nothing_to_clean_up(cijoe, device, be_opts, cli_args):
    """
    A server that dies takes its rendezvous with it.

    The arrangement this replaced left a segment that outlived its creator and
    read exactly like a live runtime, so telling debris from a server was
    something status had to do. A socket cannot be left behind in that state:
    the process holding it is the only thing that answers on it.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="Only uPCIe serves clients over a socket")

    assert CPlaneServer.is_running(cijoe), "no server to kill"

    # SIGKILL rather than SIGTERM: the point is a server that had no chance
    # to clean up after itself
    cijoe.run("pkill -9 -x homi")
    CPlaneServer.forget()

    for _ in range(30):
        sleep(1)
        if not CPlaneServer.is_running(cijoe):
            break
    else:
        pytest.fail("the server survived SIGKILL")

    err, doc = _status(cijoe, homi_id)

    assert err, "status exits zero with no server running"
    assert doc["server_running"] is False
    assert doc["ready"] is False
    assert doc["controllers"] == []


def _held_uris():
    """
    Every controller the server was started over

    Plain block devices only. The client used here reads 4 KiB at an
    arbitrary offset, which a zoned controller refuses outright and which does
    not divide a controller formatted with protection information in an
    extended LBA, where the block carries its metadata inline. The shapes being
    covered are about which controller a client reaches, not about what it
    does once it is there.

    Taken from the configuration rather than from what the server reports,
    because reporting is itself served over the socket: a server that cannot
    serve a controller does not list it either, and a test that enumerated
    from status would quietly shrink to the controllers that still work.
    """

    return [
        d["uri"]
        for d in cijoe_config_get_all_devices(["pcie"])
        if "nvm" in (d.get("labels") or [])
        and not any(lbl.startswith("pi") for lbl in (d.get("labels") or []))
    ]


def _used(cijoe, homi_id):
    """Queues in use per controller, as the server reports them"""

    _, doc = _status(cijoe, homi_id)

    return {c["uri"]: c["nsq_used"] for c in (doc.get("controllers") or [])}


def _counts(cijoe, homi_id):
    """
    What the server says it is serving: connections, queues per controller

    Both come from one status call, so they cannot describe different moments.
    The client count is what says control plane happened; queues in use cannot,
    since they are the controller's, so a client that bypassed the server and
    opened the controller itself still shows up in them.
    """

    _, doc = _status(cijoe, homi_id)

    return (
        doc.get("connections", 0),
        {c["uri"]: c["nsq_used"] for c in (doc.get("controllers") or [])},
    )


def _wait_for(cijoe, homi_id, predicate, timeout):
    """Poll the server's account of itself until it says what is wanted"""

    connections, used = _counts(cijoe, homi_id)
    for _ in range(timeout):
        if predicate(connections, used):
            return True, connections, used
        sleep(1)
        connections, used = _counts(cijoe, homi_id)

    return False, connections, used


def _stop_clients(cijoe, uris):
    """Kill the clients started for these controllers"""

    for uri in uris:
        cijoe.run(f"pkill -f '[x]nvmeperf run {uri} ' || true")


def _start_client(cijoe, uri, be, homi_id):
    """
    Start a client that holds a queue until it is killed

    'setsid' and the closed stdin for the same reason CPlaneServer.start uses
    them: backgrounding alone leaves this holding the transport's channel when
    the target is remote, and the client never runs at all.

    The runtime is long enough that the client never leaves of its own accord.
    A client that exits on a timer makes its departure a race against whatever
    the caller does next, and what is being timed here is the server's account
    of clients arriving and leaving.
    """

    tag = uri.replace(":", "-").replace(".", "-")

    return cijoe.run(
        f"setsid xnvmeperf run {uri} --be {be} --homi-id {homi_id}"
        f" --iopattern randread --iosize 4096 --qdepth 8 --nqueues 1"
        f" --runtime 3600 --cpulist 0 < /dev/null > /tmp/perm-{tag}.out 2>&1 &"
    )


def _wait_until(cijoe, homi_id, predicate, timeout):
    """Poll the server's own account of itself until it says what is wanted"""

    used = {}
    for _ in range(timeout):
        used = _used(cijoe, homi_id)
        if predicate(used):
            return True, used
        sleep(1)

    return False, used


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_io_lands_on_the_controller_it_was_asked_for(cijoe, device, be_opts, cli_args):
    """
    A client holding several controllers submits each one's I/O on that one.

    A connected client builds every controller it opens from what the server
    published. Where that description comes from whichever controller the
    client happened to open first, every device's I/O is submitted on that one:
    the commands complete, the throughput looks plausible, and the rest of the
    controllers sit idle.

    So this asserts on where the queues appear rather than on whether the I/O
    succeeded. The server counts them per controller, and a client submitting
    to the wrong one cannot make that count look right.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(
            reason="status reads what the uPCIe runtime holds; others are opaque"
        )

    uris = _held_uris()
    if len(uris) < 2:
        pytest.skip(reason="Requires two controllers to tell one from the other")

    pair = uris[:2]

    runtime = 20
    rc_path = "/tmp/cplane-spread.rc"
    cijoe.run(f"rm -f {rc_path}")

    cijoe.run(
        "setsid sh -c 'xnvmeperf run " + " ".join(pair) + f" --be {be_opts['be']}"
        f" --homi-id {homi_id} --iopattern randread --iosize 4096 --qdepth 8"
        f" --nqueues 1 --runtime {runtime} --cpulist 0"
        f"; echo $? > {rc_path}' < /dev/null > /tmp/spread.out 2>&1 &"
    )

    # Two failures look alike in the counts, so tell them apart: a client that
    # never reached the server leaves every controller at zero, which says
    # nothing about placement
    connections, used = _wait_until(
        cijoe, homi_id, lambda u: any(u.get(uri, 0) > 0 for uri in pair), 30
    )
    if not connections:
        cijoe.run("cat /tmp/spread.out")
    assert connections, f"the client never took a queue from the server; saw {used}"

    ok, used = _wait_until(
        cijoe, homi_id, lambda u: all(u.get(uri, 0) > 0 for uri in pair), 20
    )

    assert ok, (
        f"a client holding {pair} put queues on {used}. Every controller it"
        " holds should carry its own, rather than one carrying all of them"
    )

    # Leave nothing running: a client still holding a queue moves the counts
    # the next testcase reads
    for _ in range(runtime + 30):
        err, _ = cijoe.run(f"test -f {rc_path}")
        if not err:
            break
        sleep(1)
    else:
        cijoe.run("cat /tmp/spread.out")
        pytest.fail(f"the client of {pair} never finished")


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_clients_come_and_go_across_controllers(cijoe, device, be_opts, cli_args):
    """
    Clients reach the controller they asked for, and give it back

    A server holds several controllers and serves each of them, so the shapes
    worth covering are the ones that tell those apart: one controller at a
    time, a subset at once, and every one at once. Each shape is the same three
    steps. Ask the server what it holds, start a client per controller and ask
    again, then kill them and ask a third time.

    What is asserted is the server's own account of itself, not whether the
    client exited zero. A client that cannot reach the server opens the
    controller directly and succeeds at its I/O, so exit status says nothing
    about whether control plane happened; a client appearing in the server's
    count, and a queue appearing on the controller that was asked for and on no
    other, is what says it.

    This is the path that regressed: a server that served only its first
    controller left clients of the rest to open those directly, closing them
    out from under it.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="Control plane over the socket is uPCIe's")

    be = be_opts["be"]
    uris = _held_uris()
    assert uris, "a running server reports no controllers"

    shapes = [[uri] for uri in uris]
    if len(uris) > 2:
        shapes.append(uris[: len(uris) // 2])
    if len(uris) > 1:
        shapes.append(uris)

    # Start from a server nobody else is talking to. Anything an earlier case
    # left connected would be counted into the baseline and then leave during
    # the wait below, which reads exactly like the new client never arriving.
    # Clients are not the only holders: the stall cases park a python process
    # on a socket, so both are cleared. The bracket stops pkill matching the
    # command line it is running under. With nothing else connected the only
    # connection is the one asking, so the count is one.
    cijoe.run("pkill -f '[x]nvmeperf run ' || true")
    cijoe.run("pkill -f '[s]ocket.AF_UNIX' || true")
    alone, connections, _ = _wait_for(cijoe, homi_id, lambda c, _u: c == 1, 60)
    assert alone, (
        f"the server still reports {connections} connections with no clients"
        " running; something outside this testcase is holding one"
    )

    try:
        for shape in shapes:
            connections, used = _counts(cijoe, homi_id)

            for uri in shape:
                _start_client(cijoe, uri, be, homi_id)

            came, now_attached, now_used = _wait_for(
                cijoe,
                homi_id,
                lambda a, _u, want=connections + len(shape): a == want,
                30,
            )
            assert came, (
                f"the server reports {now_attached} clients for {sorted(shape)},"
                f" expected {connections + len(shape)}. A client that cannot reach"
                " the server opens the controller itself and its I/O still"
                " succeeds, so this count is what says control plane happened"
            )

            for uri in uris:
                gained = now_used.get(uri, 0) - used.get(uri, 0)
                if uri in shape:
                    assert gained > 0, (
                        f"{uri} was asked for but gained no queue; the client"
                        " reached the server without reaching this controller"
                    )
                else:
                    assert gained == 0, (
                        f"{uri} gained a queue while only {sorted(shape)} were"
                        " asked for; clients are reaching the wrong controller"
                    )

            _stop_clients(cijoe, shape)

            went, now_attached, now_used = _wait_for(
                cijoe,
                homi_id,
                lambda a, u, wa=connections, wu=used: a == wa and u == wu,
                60,
            )
            assert went, (
                f"the server did not take {sorted(shape)} back; it reports"
                f" {now_attached} clients and {now_used}, expected {connections}"
                f" and {used}"
            )
    finally:
        _stop_clients(cijoe, uris)


def _held_devices():
    """The controllers of _held_uris(), with the namespace to address them by"""

    return [
        d
        for d in cijoe_config_get_all_devices(["pcie"])
        if "nvm" in (d.get("labels") or [])
        and not any(lbl.startswith("pi") for lbl in (d.get("labels") or []))
    ]


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_a_queue_survives_a_release_on_another_controller(
    cijoe, device, be_opts, cli_args
):
    """
    A client giving its queue back does not take somebody else's with it.

    Queue identifiers are handed out per controller, so a server holding
    several has the same identifier live on each of them at once. A server that
    looks its queues up by identifier alone finds whichever was recorded first,
    and releasing one client's queue then hands another client's submission and
    completion queues back to the heap while that client is still submitting on
    them. The memory is reused by whoever asks next, and the client that lost
    it goes on writing commands nobody fetches.

    So this holds a queue on one controller while clients come and go on
    another, and asserts the first client finishes its run. Its exit is what
    says the queue was still its own: the server's queue counts stay right
    either way, since each is decremented against the client that asked.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="Serving queues over the socket is uPCIe's")

    devices = _held_devices()
    if len(devices) < 2:
        pytest.skip(reason="Requires two controllers to release on the other one")

    holder, visitor = devices[0], devices[1]
    be = be_opts["be"]
    runtime = 20
    rc_path = "/tmp/cplane-holder.rc"

    # Other testcases leave background clients running, so watch for this one
    # by the status it records rather than for any client of its own name.
    cijoe.run(f"rm -f {rc_path}")

    base = _used(cijoe, homi_id)

    cijoe.run(
        f"setsid sh -c 'xnvmeperf run {holder['uri']} --be {be} --homi-id {homi_id}"
        f" --iopattern randread --iosize 4096 --qdepth 8 --nqueues 1"
        f" --runtime {runtime} --cpulist 0; echo $? > {rc_path}'"
        " < /dev/null > /tmp/cplane-holder.out 2>&1 &"
    )

    took, used = _wait_until(
        cijoe,
        homi_id,
        lambda u: u.get(holder["uri"], 0) > base.get(holder["uri"], 0),
        30,
    )
    assert took, (
        f"the client never took a queue on {holder['uri']}; saw {used} against a"
        f" baseline of {base}. Nothing is being covered until it holds one"
    )

    # Each of these takes a queue on the other controller and gives it back,
    # which is the release that must not reach the queue held above.
    for _ in range(5):
        err, _ = cijoe.run(
            f"lblk read {visitor['uri']} --dev-nsid {visitor['nsid']}"
            f" --slba 0x0 --nlb 0 --be {be} --admin {be} --sync {be}"
            f" --homi-id {homi_id}"
        )
        assert not err, f"a client of {visitor['uri']} could not read"

    for _ in range(runtime + 30):
        err, _ = cijoe.run(f"test -f {rc_path}")
        if not err:
            break
        sleep(1)
    else:
        cijoe.run("cat /tmp/cplane-holder.out")
        pytest.fail(
            f"the client holding a queue on {holder['uri']} never finished."
            " Its queue was released by a client of another controller and the"
            " memory behind it handed to somebody else"
        )

    err, state = cijoe.run(f"cat {rc_path}")
    assert not err, "the client recorded no exit status"
    assert state.output().strip() == "0", (
        f"the client holding a queue on {holder['uri']} exited"
        f" {state.output().strip()} while clients came and went on"
        f" {visitor['uri']}"
    )


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_a_client_outlives_the_server_shutting_down(cijoe, device, be_opts, cli_args):
    """
    A client whose server shuts down mid-run still terminates.

    A server asked to stop deletes the queues its clients hold before it goes,
    so a client mid-I/O is left with commands that will never complete. A
    client with no way out of waiting for them spins forever: it looks healthy
    while the run lasts and turns up afterwards as a hung process, pinning the
    heap it still maps so the server's hugepages are never returned.

    What is asserted is that the client exits, not what its I/O did: the run
    was cut short on purpose, and the client is expected to notice the socket
    closing and give up on what was in flight within the command timeout.

    This stops the server on purpose; the device fixture starts a fresh one
    for whichever testcase runs next.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="Serving queues over the socket is uPCIe's")

    uri = device["uri"]
    be = be_opts["be"]
    runtime = 15
    rc_path = "/tmp/cplane-shutdown.rc"

    # Other testcases leave background clients running, so watch for this one
    # by the status it records rather than for any client of its own name.
    cijoe.run(f"rm -f {rc_path}")

    base = _used(cijoe, homi_id)

    cijoe.run(
        f"setsid sh -c 'xnvmeperf run {uri} --be {be} --homi-id {homi_id}"
        f" --iopattern randread --iosize 4096 --qdepth 8 --nqueues 1"
        f" --runtime {runtime} --cpulist 0; echo $? > {rc_path}'"
        " < /dev/null > /tmp/cplane-shutdown.out 2>&1 &"
    )

    took, used = _wait_until(
        cijoe, homi_id, lambda u: u.get(uri, 0) > base.get(uri, 0), 30
    )
    assert took, (
        f"the client never took a queue on {uri}; saw {used} against a"
        f" baseline of {base}. Nothing is being covered until it holds one"
    )

    CPlaneServer.stop(cijoe)

    # The runtime it was asked for, the command timeout it is allowed for
    # noticing, and slack for a loaded host
    for _ in range(runtime + 45):
        err, _ = cijoe.run(f"test -f {rc_path}")
        if not err:
            break
        sleep(1)
    else:
        cijoe.run("cat /tmp/cplane-shutdown.out")
        cijoe.run("pgrep -a -x xnvmeperf")
        pytest.fail(
            f"the client of {uri} never terminated after its server shut down."
            " Its queue was deleted with the server and it is waiting for"
            " completions that cannot come"
        )

    err, state = cijoe.run(f"cat {rc_path}")
    assert not err, "the client recorded no exit status"
    assert state.output().strip() == "0", (
        f"the client of {uri} exited {state.output().strip()} after its server"
        " shut down; giving up on what was in flight is not an error"
    )


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_the_two_spellings_name_one_identifier(cijoe, device, be_opts, cli_args):
    """
    A client passing --shm_id reaches a server started with --homi-id.

    The older spelling named this before there was a control plane, and
    instrumentation that predates the newer one keeps passing it, so the two
    have to be one identifier rather than two that happen to agree. Crossing
    them is what says so: the server was started with --homi-id, which is the
    only spelling homi accepts, and a client reaches it by the older name.

    A client that cannot reach the server opens the controller itself and its
    I/O still succeeds, so what is asserted is the server's own count of the
    queues it handed out, not the exit status of the client.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="Serving queues over the socket is uPCIe's")

    uri = device["uri"]
    base = _used(cijoe, homi_id)

    err, _ = cijoe.run(
        f"lblk read {uri} --dev-nsid {device['nsid']} --slba 0x0 --nlb 0"
        f" --be {be_opts['be']} --admin {be_opts['be']} --sync {be_opts['be']}"
        f" --shm_id {homi_id}"
    )
    assert not err, f"a client of {uri} could not read with --shm_id"

    # The queue is given back when that client exits, so the count returning to
    # its baseline is the observation; that it ever rose is what --shm_id did
    returned, used = _wait_until(
        cijoe, homi_id, lambda u: u.get(uri, 0) == base.get(uri, 0), 30
    )
    assert returned, f"queues were not released; server reports {used}, expected {base}"

    # homi itself takes only --homi-id; the crossing being tested is a client
    # using the older spelling against a server that never accepted it.
    err, state = cijoe.run(f"homi status --homi-id {homi_id}")
    assert not err, "status did not answer for the identifier the client used"
    assert f"homi_id: {homi_id}" in state.output(), (
        "status answered for a different identifier than the one asked for;"
        f" output was {state.output()!r}"
    )


def _await_connected(cijoe, out, timeout=15):
    """
    Wait until a backgrounded protocol client says it reached the server.

    Every client here is a `python3 -c` script started with `&`, so the shell
    returns zero for launching it whether or not it ran. A script that dies on
    the way, for a syntax error or a socket that is not there, leaves a server
    that is simply never spoken to, and a server nobody speaks to answers
    promptly and holds nothing. Which is to say it looks exactly like the thing
    these testcases are trying to prove.

    So each client prints a line once it is connected, and nothing is asserted
    until that line is there. It has been wrong twice; the marker is cheaper
    than reading the assertion and believing it.

    Matched whole-line, which is not fussiness. A traceback quotes the source
    line it died on, and that line is the one carrying the print, so a
    substring match finds the marker in the wreckage of a client that never
    connected and reports it as connected.
    """

    for _ in range(timeout):
        err, _ = cijoe.run(f"grep -qx connected {out}")
        if not err:
            return True
        sleep(1)

    _, state = cijoe.run(f"cat {out}")
    pytest.fail(
        f"a protocol client never reached the server; {out} holds"
        f" {state.output()!r}. Nothing below this asserts anything until it"
        " does, since a server nobody spoke to passes every one of these"
    )


def _stall_a_connection(cijoe, homi_id, seconds):
    """
    Connect to the server's socket, send part of a message, and hold it open.

    Half a message is the interesting shape: the socket is readable, so a server
    that waits for the rest of one is stuck until this lets go. What is sent is
    a byte, which is neither a message nor a disconnect.
    """

    sock = f"/tmp/xnvme-homi-{homi_id}.sock"

    return cijoe.run(
        'setsid python3 -c "'
        "import socket,time;"
        f"s=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM);s.connect('{sock}');"
        "print('connected',flush=True);"
        f"s.send(b'x');time.sleep({seconds})"
        f'" < /dev/null > /tmp/stalled.out 2>&1 &'
    )


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_a_stalled_client_holds_up_nobody(cijoe, device, be_opts, cli_args):
    """
    A connection that sends half a message does not stop the server.

    This is the shape that costs nothing to produce by accident: a process
    descheduled between two writes leaves a socket readable with less than a
    message on it. A server that reads by waiting for the rest is held there,
    and every other client waits with it. So the assertion is that the server
    keeps answering while one connection sits half-spoken, and that it is still
    answering once that connection has gone away.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="Serving over the socket is uPCIe's")

    uri = device["uri"]

    err, _ = _stall_a_connection(cijoe, homi_id, 20)
    assert not err, "could not open a connection to stall"
    _await_connected(cijoe, "/tmp/stalled.out")

    # Long enough that a server waiting for the rest of the message is still
    # waiting, and short enough to be well inside the stall
    sleep(2)

    started = time()
    err, doc = _status(cijoe, homi_id)
    elapsed = time() - started

    assert not err, "status did not answer while a connection sat half-spoken"
    assert _ctrlr_of(doc, uri), f"{uri} is not among the controllers the server holds"
    assert elapsed < 10, (
        f"status took {elapsed:.1f}s while one connection was stalled, which is"
        " what being blocked behind it looks like"
    )

    # And the stall resolving leaves nothing behind
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
    Two clients of one controller are both served, and both let go of.

    Every other case here drives one client, so nothing said what happens when
    two are connections at once: whether the second is served at all, whether the
    count adds up across them, and whether disconnecting one takes the other's
    queues with it. The count reaching both allocations and then returning to
    its baseline answers all three.

    What this does not say is anything about concurrency. Requests are short, so
    both clients end up holding queues whether the server answers them side by
    side or one after the other; separating those needs a client that asks for
    something slow while another asks for something quick.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(
            reason="status reads what the uPCIe runtime holds; others are opaque"
        )

    uri = device["uri"]
    nqueues = 2

    _, doc = _status(cijoe, homi_id)
    before = _ctrlr_of(doc, uri)
    assert before, f"{uri} is not among the controllers the server holds"
    baseline = before["nsq_used"]

    for i in (1, 2):
        cijoe.run(
            f"setsid xnvmeperf run {uri} --be {be_opts['be']} --homi-id {homi_id}"
            f" --iopattern randread --iosize 4096 --qdepth 8 --nqueues {nqueues}"
            f" --runtime 25 --cpulist 0 < /dev/null > /tmp/two_{i}.out 2>&1 &"
        )

    peak = baseline
    for _ in range(40):
        sleep(1)
        _, doc = _status(cijoe, homi_id)
        ctrlr = _ctrlr_of(doc, uri)
        if ctrlr:
            peak = max(peak, ctrlr["nsq_used"])
        if peak >= baseline + (2 * nqueues):
            break
    else:
        cijoe.run("cat /tmp/two_1.out /tmp/two_2.out")
        pytest.fail(
            f"both clients were never outstanding together; the count peaked at"
            f" {peak} against a baseline of {baseline}, and two clients asking"
            f" for {nqueues} each should reach {baseline + 2 * nqueues}"
        )

    assert peak == baseline + (2 * nqueues), (
        f"expected {baseline} + {2 * nqueues} with both clients holding queues,"
        f" got {peak}"
    )

    for _ in range(60):
        sleep(1)
        _, doc = _status(cijoe, homi_id)
        ctrlr = _ctrlr_of(doc, uri)
        if ctrlr and ctrlr["nsq_used"] <= baseline:
            break
    else:
        still = ctrlr["nsq_used"] if ctrlr else "the controller is gone"
        pytest.fail(f"queues were not released; still {still}, expected {baseline}")


# The wire format, as nvme_cplane.h lays it out. Duplicating it here is the
# point: a change to the message that forgets to bump NVME_CPLANE_VERSION shows
# up as these cases failing rather than as two builds quietly disagreeing.
_MSG_NBYTES = 104
_MSG_HDR_NBYTES = 24  # op, version, status, nfds, index, reserved
_MSG_VERSION = 4
_OP_INIT_CONNECTION = 1
_OP_ALLOC_IOQPAIR = 2
_OP_FREE_IOQPAIR = 3
_OP_ALLOC_BUF = 5
_OP_FREE_BUF = 6
_OP_STATUS = 7


def _speak(cijoe, homi_id, op, version=_MSG_VERSION, payload_at=None, index=0):
    """
    Send one message to the server's socket and return the reply's status.

    Built here rather than driven through a tool, because a tool cannot ask for
    something it knows is wrong, and refusals are the half of the protocol no
    tool exercises.

    `payload_at` is an offset into the message's union and a 32-bit value to put
    there, so a caller says where in the request body it is writing without
    also having to know how long the header is.
    """

    sock = f"/tmp/xnvme-homi-{homi_id}.sock"

    at = None if payload_at is None else _MSG_HDR_NBYTES + payload_at[0]
    poke = (
        ""
        if payload_at is None
        else (
            f"m[{at}:{at} + 4] =" f" __import__('struct').pack('<I', {payload_at[1]});"
        )
    )

    err, state = cijoe.run(
        'python3 -c "'
        "import socket,struct;"
        f"m=bytearray({_MSG_NBYTES});"
        f"m[0:24]=struct.pack('<IIiIII', {op}, {version}, 0, 0, {index}, 0);"
        f"{poke}"
        f"s=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM);s.connect('{sock}');"
        "s.sendall(bytes(m));"
        f"r=b''" + ";"
        f"[r := r + s.recv({_MSG_NBYTES} - len(r)) for _ in range(8) if len(r) < {_MSG_NBYTES}];"
        "print('status', struct.unpack('<i', r[8:12])[0])"
        '"'
    )

    assert (
        not err
    ), f"the server did not answer a request it should refuse: {state.output()!r}"

    m = re.search(r"status (-?\d+)", state.output())
    assert m, f"no status in the reply; output was {state.output()!r}"

    return int(m.group(1))


def _hold_a_connection(cijoe, homi_id, indices, seconds):
    """
    Initialise several controllers over one connection, and hold it open.

    Spoken directly rather than driven through a tool, because no tool opens
    more than one controller in a process, and one connection covering several
    is the thing being asserted.

    The descriptors the replies carry are dropped: recv() without a control
    buffer discards them, and nothing here has any use for a heap it will not
    map. The server has recorded the controller as initialised by then, which
    is what the count is read from.
    """

    sock = f"/tmp/xnvme-homi-{homi_id}.sock"
    init = ";".join(
        f"m=bytearray({_MSG_NBYTES});"
        f"m[0:24]=struct.pack('<IIiIII', {_OP_INIT_CONNECTION}, {_MSG_VERSION}, 0, 0, {i}, 0);"
        "s.sendall(bytes(m));"
        f"r=b''"
        f";[r := r + s.recv({_MSG_NBYTES} - len(r)) for _ in range(8) if len(r) < {_MSG_NBYTES}]"
        for i in indices
    )

    return cijoe.run(
        'setsid python3 -c "'
        "import socket,struct,time;"
        f"s=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM);s.connect('{sock}');"
        f"{init};"
        "print('connected',flush=True);"
        f"time.sleep({seconds})"
        f'" < /dev/null > /tmp/held.out 2>&1 &'
    )


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_one_client_costs_one_connection_for_every_controller(
    cijoe, device, be_opts, cli_args
):
    """
    A client using several controllers is one connection, not several.

    This is what naming the controller in the message buys, and it is not
    visible from a client: the arrangement it replaced gave each controller its
    own socket, so a process using three of them cost the server three
    connections, three accepts and three lots of bookkeeping. Only the server's
    own count can tell the two apart.

    The per-controller count is asserted alongside it, because the runtime-wide
    number alone would also be satisfied by a server that had stopped
    distinguishing controllers at all.

    Which controller sits at which index comes from the server rather than from
    the configuration. A server is started over every device labelled `pcie`,
    and the plain-block subset the other cases here use is a filter over that,
    so the two orders agree only by accident. Using the server's own list is
    also no weakness: it is an index-to-name map, and the property being
    asserted is the counts, which a server cannot satisfy by misreporting the
    order.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="Serving over the socket is uPCIe's")

    # Anything else connected would be counted into the baseline and could
    # leave during the wait, which reads exactly like the client never arriving
    cijoe.run("pkill -f '[x]nvmeperf run ' || true")
    cijoe.run("pkill -f '[s]ocket.AF_UNIX' || true")
    alone, connections, _ = _wait_for(cijoe, homi_id, lambda c, _u: c == 1, 60)
    assert alone, (
        f"the server still reports {connections} connections with no clients"
        " running; something outside this testcase is holding one"
    )

    _, before = _status(cijoe, homi_id)

    # Index order, because status reports one controller per index and walks
    # the range from zero, which is the same range the requests below name
    uris = [c["uri"] for c in (before.get("controllers") or [])]
    if len(uris) < 2:
        pytest.skip(reason="One connection covering several needs several")

    held = list(range(min(3, len(uris))))

    err, _ = _hold_a_connection(cijoe, homi_id, held, 30)
    assert not err, "could not open a connection to hold"
    _await_connected(cijoe, "/tmp/held.out")

    came, _now, _used = _wait_for(cijoe, homi_id, lambda c, _u: c == 2, 30)

    _, doc = _status(cijoe, homi_id)
    assert came, (
        f"the server reports {doc['connections']} connections for one client"
        f" using {len(held)} controllers; one connection plus the one asking is"
        " two, and anything more is a connection per controller"
    )

    for i, uri in enumerate(uris):
        entry = _ctrlr_of(doc, uri)
        was = (_ctrlr_of(before, uri) or {}).get("connections", 0)
        gained = (entry or {}).get("connections", 0) - was

        if i in held:
            assert gained == 1, (
                f"{uri} was initialised on the held connection but its client"
                f" count moved by {gained}"
            )
        else:
            assert gained == 0, (
                f"{uri} was not asked for but its client count moved by"
                f" {gained}; the per-controller count is reporting the"
                " runtime's clients rather than its own"
            )

    cijoe.run("pkill -f '[s]ocket.AF_UNIX' || true")


def _hold_queues_on(cijoe, homi_id, indices, depth, seconds):
    """
    Take an I/O queue on each of several controllers over one connection.

    No tool reaches this. A tool opens one controller per process, so the
    queues one process holds are all on the same controller and giving them
    back is one controller's business. Here they are not, which is the case the
    server has to take apart controller by controller.

    The descriptors that arrive with initialising are dropped: recv() without a
    control buffer discards them, and a client that will not map the heap has
    no use for it. The queue is real regardless, which is what the counts then
    show.
    """

    sock = f"/tmp/xnvme-homi-{homi_id}.sock"
    depth_at = (
        _MSG_HDR_NBYTES + 32
    )  # u.queue.depth, past the allocation it replies with

    steps = ";".join(
        f"m=bytearray({_MSG_NBYTES});"
        f"m[0:24]=struct.pack('<IIiIII', {op}, {_MSG_VERSION}, 0, 0, {i}, 0);"
        + (f"m[{depth_at}:{depth_at} + 2]=struct.pack('<H', {depth});" if poke else "")
        + "s.sendall(bytes(m));"
        f"r=b''"
        f";[r := r + s.recv({_MSG_NBYTES} - len(r)) for _ in range(8) if len(r) < {_MSG_NBYTES}]"
        for i in indices
        for op, poke in ((_OP_INIT_CONNECTION, False), (_OP_ALLOC_IOQPAIR, True))
    )

    return cijoe.run(
        'setsid python3 -c "'
        "import socket,struct,time;"
        f"s=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM);s.connect('{sock}');"
        f"{steps};"
        "print('connected',flush=True);"
        f"time.sleep({seconds})"
        f'" < /dev/null > /tmp/queues.out 2>&1 &'
    )


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_a_client_holding_queues_on_several_controllers_gives_them_all_back(
    cijoe, device, be_opts, cli_args
):
    """
    Queues go back to every controller that lent one, not just the first.

    A connection reaches every controller a server holds, so the queues behind
    one can be spread across several, and each has to be told separately: two
    threads deleting on one admin queue is the thing the arrangement is built
    to avoid, and one thread deleting on two controllers is how that would
    happen by accident. Whichever finishes last is what frees the connection.

    The client is killed rather than closed, because a client that exits
    tidily hands its queues back itself and never exercises this. What is
    asserted is that both controllers come back to where they started, since a
    server that released only the controller it happened to look at first
    leaves the other holding a queue for a process that no longer exists.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="Serving over the socket is uPCIe's")

    # Anything else holding queues moves the counts under this
    cijoe.run("pkill -f '[x]nvmeperf run ' || true")
    cijoe.run("pkill -f '[s]ocket.AF_UNIX' || true")
    alone, connections, _ = _wait_for(cijoe, homi_id, lambda c, _u: c == 1, 60)
    assert alone, (
        f"the server still reports {connections} connections with no clients"
        " running; something outside this testcase is holding one"
    )

    _, before = _status(cijoe, homi_id)
    uris = [c["uri"] for c in (before.get("controllers") or [])]
    if len(uris) < 2:
        pytest.skip(reason="Queues on several controllers needs several")

    held = list(range(min(2, len(uris))))
    was = {c["uri"]: c["nsq_used"] for c in (before.get("controllers") or [])}

    err, _ = _hold_queues_on(cijoe, homi_id, held, 8, 60)
    assert not err, "could not open a connection to take queues on"
    _await_connected(cijoe, "/tmp/queues.out")

    took, _c, used = _wait_for(
        cijoe,
        homi_id,
        lambda _c, u, w=was, h=held, n=uris: all(
            u.get(n[i], 0) > w.get(n[i], 0) for i in h
        ),
        30,
    )
    assert took, (
        f"one connection did not take a queue on each of {[uris[i] for i in held]};"
        f" the server reports {used} against {was}"
    )

    cijoe.run("pkill -f '[s]ocket.AF_UNIX' || true")

    gave, _c, used = _wait_for(
        cijoe,
        homi_id,
        lambda _c, u, w=was, h=held, n=uris: all(
            u.get(n[i], 0) == w.get(n[i], 0) for i in h
        ),
        60,
    )
    assert gave, (
        f"the server did not take back every queue the connection held; it"
        f" reports {used} against {was}. A server that released only one"
        " controller leaves the rest holding queues for a dead process"
    )


def _alloc_buffers(cijoe, homi_id, count, nbytes=4096):
    """
    Ask for `count` DMA buffers on one connection, and report how many landed.

    Every request goes out before any reply is read, which keeps this to two
    exchanges rather than `count` of them. That is safe at this size: the
    replies are 104 bytes each and the socket buffer holds far more, so the
    server is never blocked writing them, which is the thing a client is not
    allowed to do to it.
    """

    sock = f"/tmp/xnvme-homi-{homi_id}.sock"
    total = count * _MSG_NBYTES

    err, state = cijoe.run(
        'python3 -c "'
        "import socket,struct;"
        f"s=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM);s.connect('{sock}');"
        f"i=bytearray({_MSG_NBYTES});"
        f"i[0:24]=struct.pack('<IIiIII', {_OP_INIT_CONNECTION}, {_MSG_VERSION}, 0, 0, 0, 0);"
        f"s.sendall(bytes(i));r=b'';"
        f"[r := r + s.recv({_MSG_NBYTES} - len(r)) for _ in range(8) if len(r) < {_MSG_NBYTES}];"
        f"a=bytearray({_MSG_NBYTES});"
        f"a[0:24]=struct.pack('<IIiIII', {_OP_ALLOC_BUF}, {_MSG_VERSION}, 0, 0, 0, 0);"
        f"a[24:32]=struct.pack('<Q', {nbytes});"
        f"[s.sendall(bytes(a)) for _ in range({count})];b=b'';"
        f"[b := b + s.recv({total} - len(b)) for _ in range(4000) if len(b) < {total}];"
        f"print('allocated', sum(1 for k in range({count}) "
        f"if struct.unpack('<i', b[k * {_MSG_NBYTES} + 8:k * {_MSG_NBYTES} + 12])[0] == 0))"
        '"'
    )
    assert not err, f"the allocating client failed: {state.output()!r}"

    m = re.search(r"allocated (\d+)", state.output())
    assert m, f"no count in the output; got {state.output()!r}"

    return int(m.group(1))


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_a_client_allocates_a_buffer_per_queue_entry(cijoe, device, be_opts, cli_args):
    """
    A client is not held to a fixed number of DMA buffers.

    How many a client wants follows its own configuration rather than anything
    the server can predict: qublk takes one per tag at startup, which is its
    queue count times its queue depth, and is 64 with the depth it defaults to.
    A server with a fixed table refuses the moment that table is smaller,
    before the client has done any I/O at all.

    So the count asked for here is past both the table this replaced and
    qublk's default. What limits it is the heap, which says so by refusing an
    allocation rather than by refusing to record one.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="Serving over the socket is uPCIe's")

    wanted = 96
    got = _alloc_buffers(cijoe, homi_id, wanted)

    assert got == wanted, (
        f"the server allocated {got} of {wanted} buffers on one connection;"
        " a client asking for one per queue entry is refused before it has"
        " submitted anything"
    )


def _ask_without_listening(cijoe, homi_id, count, seconds):
    """
    Send requests on one connection, never read the replies, and hold it open.

    The connection has to survive, because a server stuck writing to a peer
    comes unstuck the moment that peer's socket closes. So this must neither
    fail nor exit while the assertion is being made.

    Which is why the receive buffer is shrunk rather than the request count
    raised. A default buffer takes thousands of replies to fill, and by the
    time it is full the server has stopped reading, so the sender blocks too
    and the pair deadlocks. A small one is full after a few dozen, which the
    sender reaches without ever filling its own, so it goes on to sleep and
    holds the connection exactly as it is.
    """

    sock = f"/tmp/xnvme-homi-{homi_id}.sock"

    return cijoe.run(
        'setsid python3 -c "'
        "import socket,struct,time;"
        f"m=bytearray({_MSG_NBYTES});"
        f"m[0:24]=struct.pack('<IIiIII', {_OP_STATUS}, {_MSG_VERSION}, 0, 0, 0, 0);"
        "s=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM);"
        "s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 2048);"
        f"s.connect('{sock}');"
        "print('connected',flush=True);"
        f"[s.sendall(bytes(m)) for _ in range({count})];"
        f"time.sleep({seconds})"
        f'" < /dev/null > /tmp/deaf.out 2>&1 &'
    )


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_a_client_that_stops_reading_holds_up_nobody(cijoe, device, be_opts, cli_args):
    """
    A client that asks and never listens does not stop the server.

    The counterpart to the stalled client, and the harder half. A stalled
    client is not ready to be read; this one is always ready, and what fills up
    is the direction the server writes in. A server that waits for room is held
    by whichever client stopped reading, and every other client waits with it,
    which is worse than the arrangement this replaced: there a thread per
    connection meant such a client blocked only itself.

    Sending is what has to be bounded, so the assertion is that another client
    is answered promptly while this one is pushing requests it will never
    collect.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="Serving over the socket is uPCIe's")

    uri = device["uri"]

    # Enough to outrun a shrunk receive buffer several times over, and far
    # short of what it would take to fill this end's own
    err, _ = _ask_without_listening(cijoe, homi_id, 400, 25)
    assert not err, "could not open a connection to pipeline on"
    _await_connected(cijoe, "/tmp/deaf.out")

    sleep(2)

    started = time()
    err, doc = _status(cijoe, homi_id)
    elapsed = time() - started

    assert not err, "status did not answer while a client was not reading"
    assert _ctrlr_of(doc, uri), f"{uri} is not among the controllers the server holds"
    assert elapsed < 10, (
        f"status took {elapsed:.1f}s while one client was not reading its"
        " replies, which is what waiting on that client looks like"
    )

    # And the server is still there once it has let that client go
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
    """
    A peer speaking a version nobody knows is turned away, not served.

    The record describes queue memory whose layout comes from this library, so a
    client built against another version cannot be trusted to read it. Refusing
    is the only safe answer, and it has to be the server's answer rather than
    something the client checks about itself.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="Serving over the socket is uPCIe's")

    status = _speak(cijoe, homi_id, _OP_STATUS, version=0xBADF00D)
    assert status == -71, f"expected -EPROTO for a stranger's version, got {status}"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_an_unknown_request_is_refused(cijoe, device, be_opts, cli_args):
    """
    An opcode the server does not know is refused rather than misread.

    A server that fell through to some default would act on a message it did not
    understand; what it does instead is say so, which is also what lets the
    protocol gain an operation without the older side guessing.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="Serving over the socket is uPCIe's")

    status = _speak(cijoe, homi_id, 99)
    assert status == -38, f"expected -ENOSYS for an unknown opcode, got {status}"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_asking_for_resources_before_initialising_is_refused(
    cijoe, device, be_opts, cli_args
):
    """
    A connection that has not initialised cannot take a queue or memory.

    Both replies are an offset into the heap, and a connection that never asked
    for the descriptors has no mapping of the heap to resolve one against. A
    server that answered anyway would carve a real queue off the controller, or
    a 2 MiB payload granule, for a client that cannot use either.

    No tool can ask for this: the backend always initialises before it allocates,
    so the refusal is only reachable by speaking the protocol directly.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="Serving over the socket is uPCIe's")

    # Queue depth at offset 32 of the body, and a byte count at 0, so each
    # request is one the server would otherwise honour.
    status = _speak(cijoe, homi_id, _OP_ALLOC_IOQPAIR, payload_at=(32, 8))
    assert status == -107, f"expected -ENOTCONN for a queue before init, got {status}"

    status = _speak(cijoe, homi_id, _OP_ALLOC_BUF, payload_at=(0, 4096))
    assert status == -107, f"expected -ENOTCONN for memory before init, got {status}"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_handing_back_what_was_never_held_is_refused(cijoe, device, be_opts, cli_args):
    """
    A client can only give back what it was given.

    The server frees a queue by identifier, and identifiers are handed out per
    controller, so a client naming one it never held is either confused or
    trying to release somebody else's. Either way the answer is no, and this is
    the case that says the check is on what the client holds rather than on
    whether the identifier exists.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="Serving over the socket is uPCIe's")

    uri = device["uri"]

    status = _speak(cijoe, homi_id, _OP_FREE_IOQPAIR, payload_at=(0, 4095))
    assert status == -2, f"expected -ENOENT for a queue never held, got {status}"

    status = _speak(cijoe, homi_id, _OP_FREE_BUF, payload_at=(0, 0xDEAD))
    assert status == -2, f"expected -ENOENT for memory never held, got {status}"

    # A controller nobody holds, which is what naming one in the message
    # rather than in a socket name makes possible to ask for at all.
    status = _speak(cijoe, homi_id, _OP_STATUS, index=4096)
    assert (
        status == -34
    ), f"expected -ERANGE for a controller past the last, got {status}"

    # The refusals are answers, not damage: the server is still serving
    _, doc = _status(cijoe, homi_id)
    assert _ctrlr_of(
        doc, uri
    ), "the server stopped holding the controller after a refusal"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_a_stalled_client_does_not_stop_a_working_one(cijoe, device, be_opts, cli_args):
    """
    Real work on a controller proceeds while another connection sits stalled.

    This is the case that separates the designs. A server that reads its clients
    one after another, however it is threaded, cannot get to this client until
    the stalled one finishes being read, so the read below waits out the stall
    and the timing says so. Serving each connection on its own thread is what
    makes the stall cost only the client that caused it.

    A real client rather than a status probe, because attaching, allocating a
    queue and submitting I/O is the whole path a client takes, and a stall that
    only delayed inspection would be a much smaller claim.
    """

    _require_upcie(cijoe)

    homi_id = get_homi_id()
    if not homi_id:
        pytest.skip(reason="Requires a control-plane server; pass --homi-id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(reason="Serving over the socket is uPCIe's")

    uri = device["uri"]
    stall = 30

    err, _ = _stall_a_connection(cijoe, homi_id, stall)
    assert not err, "could not open a connection to stall"
    _await_connected(cijoe, "/tmp/stalled.out")

    # Inside the stall, so a server that has to finish reading that connection
    # first has not finished
    sleep(2)

    # Under its own timeout, because a server that serialises its clients does
    # not answer this slowly, it does not answer at all: the client blocks until
    # the stall ends. Left unbounded that is a stuck job reporting nothing rather
    # than a red test saying what happened.
    started = time()
    err, _ = cijoe.run(
        f"timeout {stall // 3} lblk read {uri} --dev-nsid {device['nsid']}"
        f" --slba 0x0 --nlb 0 --be {be_opts['be']} --admin {be_opts['be']}"
        f" --sync {be_opts['be']} --homi-id {homi_id}"
    )
    elapsed = time() - started

    assert not err, (
        f"a client could not work while another connection was stalled; it gave"
        f" up after {elapsed:.1f}s against a {stall}s stall, which is what"
        " waiting behind that connection looks like"
    )
    assert elapsed < (stall / 2), (
        f"the read took {elapsed:.1f}s against a {stall}s stall, which is what"
        " waiting for the stalled connection to be read looks like"
    )
