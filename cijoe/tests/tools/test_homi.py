import pytest
import yaml

from ..conftest import get_shm_id, xnvme_parametrize


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

    # The CLI appends its own error line on failure, which is not part of the
    # document; drop it rather than teaching the test to parse both
    lines = [ln for ln in output.split("\n") if not ln.startswith("# ERR")]
    doc = yaml.safe_load("\n".join(lines))

    assert doc is not None, f"status emitted no document; output was: {output!r}"

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

        # The admin queue is excluded, so a controller with no I/O queue pair
        # in use reports zero rather than wrapping
        assert ctrlr["nsq_used"] >= 0
        assert ctrlr["ncq_used"] == ctrlr["nsq_used"]

        # Totals are omitted when the controller did not report them
        if "nsq_total" in ctrlr:
            assert ctrlr["nsq_used"] <= ctrlr["nsq_total"]
            assert ctrlr["ncq_used"] <= ctrlr["ncq_total"]


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_status_does_not_disturb_election(cijoe, device, be_opts, cli_args):
    """
    Probing must not take the role-election lock.

    A probe that took it, even briefly, would make a process electing at that
    instant demote itself to secondary and then wait for a primary that never
    arrives. Hammering the probe against a live primary catches a regression
    to that: the primary must still hold the role afterwards.
    """

    _require_upcie(cijoe)

    shm_id = get_shm_id()
    if not shm_id:
        pytest.skip(reason="Requires a multi-process primary; pass --shm_id")
    if not be_opts["be"].startswith("upcie"):
        pytest.skip(
            reason="status reads uPCIe's shared segment; other backends are opaque to it"
        )

    err, _ = cijoe.run(
        f"for i in $(seq 1 200); do homi status --shm_id {shm_id} >/dev/null || exit 1; done"
    )
    assert not err, "status stopped reporting the primary while being polled"

    err, doc = _status(cijoe, shm_id)
    assert not err
    assert doc["primary_running"] is True
