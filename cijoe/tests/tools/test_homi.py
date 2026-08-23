import re
from time import sleep, time

import pytest
import yaml

from ..conftest import MprocPrimary, get_shm_id, xnvme_parametrize


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


def _status(cijoe, shm_id):
    """Run 'homi status' and return its exit code and parsed document"""

    err, state = cijoe.run(f"homi status --shm_id {shm_id}")
    output = state.output()
    lines = output.split("\n")

    # A debug build logs to stdout before the document starts, so find where the
    # document begins rather than assuming it is the first line. The CLI also
    # appends its own error line on failure, which is not part of it either.
    start = next((i for i, ln in enumerate(lines) if ln.startswith("shm_id:")), None)
    assert start is not None, f"status emitted no document; output was: {output!r}"

    body = [ln for ln in lines[start:] if not ln.startswith("# ERR")]
    doc = yaml.safe_load("\n".join(body))

    assert doc is not None, f"status document did not parse; output was: {output!r}"

    return err, doc


def test_status_without_primary(cijoe):
    """An id no primary claimed reports as absent, and says so in its exit code"""

    _require_upcie(cijoe)

    # Well above what the suite hands out, so it cannot collide with a primary
    # started for another testcase
    shm_id = 4242

    err, doc = _status(cijoe, shm_id)

    assert err, "status exits zero with no primary running"
    assert doc["shm_id"] == shm_id
    assert doc["primary_running"] is False
    assert doc["ready"] is False
    assert doc["stale_segment"] is False
    assert doc["controllers"] == []


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_status_with_primary(cijoe, device, be_opts, cli_args):
    """A held runtime reports what it holds, and exits zero once it is ready"""

    _require_upcie(cijoe)

    shm_id = get_shm_id()
    if not shm_id:
        pytest.skip(reason="Requires a multi-process primary; pass --shm_id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(
            reason="status reads uPCIe's shared segment; other backends are opaque to it"
        )

    err, doc = _status(cijoe, shm_id)

    assert not err, "status exits non-zero while a ready primary holds the runtime"
    assert doc["primary_running"] is True
    assert doc["ready"] is True

    # The primary counts itself, and this process attached to run the testcase
    assert doc["attached"] >= 1

    assert doc["controllers"], "a running primary reports no controllers"
    for ctrlr in doc["controllers"]:
        assert ctrlr["readable"] is True
        assert ctrlr["initialized"] is True

        # The admin queue is excluded by subtraction, so an unsigned wrap
        # would surface as a huge count rather than a negative one
        assert ctrlr["nsq_used"] < 65536, "queue count looks like an unsigned wrap"
        assert ctrlr["ncq_used"] == ctrlr["nsq_used"]

        # Totals are omitted when the controller did not report them
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
def test_queue_count_tracks_a_secondary(cijoe, device, be_opts, cli_args):
    """
    The reported queue count follows what a secondary allocates.

    A stale or double-counted value still looks plausible, so what pins this is
    the count moving with `--nqueues` rather than merely being in range: a
    secondary asking for N takes N I/O queues plus one for its sync queue pair,
    and gives them all back when it exits.
    """

    _require_upcie(cijoe)

    shm_id = get_shm_id()
    if not shm_id:
        pytest.skip(reason="Requires a multi-process primary; pass --shm_id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(
            reason="status reads uPCIe's shared segment; other backends are opaque to it"
        )

    uri = device["uri"]
    nqueues = 2

    _, doc = _status(cijoe, shm_id)
    before = _ctrlr_of(doc, uri)
    assert before, f"{uri} is not among the controllers the primary holds"
    baseline = before["nsq_used"]

    cijoe.run(
        f"nohup xnvmeperf run {uri} --be {be_opts['be']} --shm_id {shm_id}"
        f" --iopattern randread --iosize 4096 --qdepth 8 --nqueues {nqueues}"
        f" --runtime 20 --cpulist 0 > /tmp/qcount.out 2>&1 &"
    )

    # The secondary takes a second or two to attach and create its queues
    peak = baseline
    for _ in range(30):
        sleep(1)
        _, doc = _status(cijoe, shm_id)
        ctrlr = _ctrlr_of(doc, uri)
        if ctrlr and ctrlr["nsq_used"] > baseline:
            peak = ctrlr["nsq_used"]
            break
    else:
        cijoe.run("cat /tmp/qcount.out")
        pytest.fail("the secondary never showed up in the queue count")

    assert (
        peak == baseline + nqueues + 1
    ), f"expected {baseline} + {nqueues} + 1 for the sync pair, got {peak}"
    assert ctrlr["ncq_used"] == ctrlr["nsq_used"]

    # And released again once it exits
    for _ in range(60):
        sleep(1)
        _, doc = _status(cijoe, shm_id)
        ctrlr = _ctrlr_of(doc, uri)
        if ctrlr and ctrlr["nsq_used"] == baseline:
            break
    else:
        still = ctrlr["nsq_used"] if ctrlr else "the controller is gone"
        pytest.fail(f"queues were not released; still {still}, expected {baseline}")


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_totals_come_from_the_controller(cijoe, device, be_opts, cli_args):
    """
    The totals are read per controller rather than assumed.

    A constant would satisfy every other assertion here, so this checks the
    reported ceiling against what the controller answers for itself.
    """

    _require_upcie(cijoe)

    shm_id = get_shm_id()
    if not shm_id:
        pytest.skip(reason="Requires a multi-process primary; pass --shm_id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(
            reason="status reads uPCIe's shared segment; other backends are opaque to it"
        )

    _, doc = _status(cijoe, shm_id)
    ctrlr = _ctrlr_of(doc, device["uri"])
    assert ctrlr, f"{device['uri']} is not among the controllers the primary holds"

    if "nsq_total" not in ctrlr:
        pytest.skip(reason="the controller did not report its queue allocation")

    err, state = cijoe.run(f"xnvme feature-get {cli_args} --fid 0x7 --sel 0")
    assert not err, "could not ask the controller for Number of Queues"

    # The tool prints the raw feature: NSQA and NCQA are zero-based, and the
    # status totals are those plus one
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
    and demoting a concurrent elector, needs a probe racing a starting primary
    and is not covered here.
    """

    _require_upcie(cijoe)

    shm_id = get_shm_id()
    if not shm_id:
        pytest.skip(reason="Requires a multi-process primary; pass --shm_id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(
            reason="status reads uPCIe's shared segment; other backends are opaque to it"
        )

    # Wrapped in 'sh -c' so it stays a simple command: a configuration that
    # sets 'cijoe.run.env' has the transport prefix every command with
    # assignments, and those are a syntax error ahead of a compound one
    err, _ = cijoe.run(
        "sh -c 'for i in $(seq 1 200); do homi status"
        f" --shm_id {shm_id} >/dev/null || exit 1; done'"
    )
    assert not err, "status stopped reporting the primary while being polled"

    err, doc = _status(cijoe, shm_id)
    assert not err
    assert doc["primary_running"] is True


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_primary_terminates_promptly(cijoe, device, be_opts, cli_args):
    """
    A primary asked to stop does stop, and does so quickly.

    Closing a controller walks the queue-id bitmap to reap what is still
    allocated. A bound the loop counter cannot represent makes that walk
    endless, and the primary then outlives its signal: every later testcase
    still passes, and the run instead stalls at session teardown, far from
    the change that caused it. Bounding it here names it.

    This leaves no primary behind on purpose; the device fixture starts a
    fresh one for whichever testcase runs next.
    """

    _require_upcie(cijoe)

    shm_id = get_shm_id()
    if not shm_id:
        pytest.skip(reason="Requires a multi-process primary; pass --shm_id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(
            reason="status reads uPCIe's shared segment; other backends are opaque to it"
        )

    assert MprocPrimary.is_running(cijoe), "no primary to stop"

    # Generous next to a teardown that takes about a second, and still far
    # below the point where a stuck primary would look like a slow one
    budget = 15

    start = time()
    MprocPrimary.stop(cijoe)
    elapsed = time() - start

    assert elapsed < budget, f"primary took {elapsed:.1f}s to terminate"

    _, doc = _status(cijoe, shm_id)
    assert doc["primary_running"] is False, "status still reports a live primary"


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_runtime_objects_are_named_consistently(cijoe, device, be_opts, cli_args):
    """
    The four objects a runtime creates follow one naming scheme.

    Documented names are what an operator greps for and what a stale-state
    cleanup deletes, so a rename that misses one of the four leaves something
    behind under a name nothing looks for. The BDF-keyed pair also has to
    survive being both a path and a POSIX shm name, which is why its key
    carries no ':' or '.'.
    """

    _require_upcie(cijoe)

    shm_id = get_shm_id()
    if not shm_id:
        pytest.skip(reason="Requires a multi-process primary; pass --shm_id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(
            reason="status reads uPCIe's shared segment; other backends are opaque to it"
        )

    key = re.sub(r"[:./]", "-", device["uri"])

    for path in [
        f"/tmp/xnvme-upcie-lock-{shm_id}",
        f"/dev/shm/xnvme-upcie-shm-{shm_id}",
        f"/tmp/xnvme-upcie-lock-{key}",
        f"/dev/shm/xnvme-upcie-shm-{key}",
    ]:
        err, _ = cijoe.run(f"test -e {path}")
        assert not err, f"a running primary has no {path}"
