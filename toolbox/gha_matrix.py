#!/usr/bin/env python3
"""
Expand a GitHub Actions job-matrix into one entry per cijoe test-step

Reads a YAML list of matrix entries on stdin and, for each of them, locates the
cijoe workflow it would run. The steps of that workflow are split in two: those
named 'test_*' and the rest, which are treated as preparation. One matrix entry is
emitted per test-step, with a 'steps' key holding the preparation steps followed by
that single test-step, such that the test-steps of a workflow run as separate jobs.

Emits 'matrix=<json>' on stdout, for consumption via GITHUB_OUTPUT::

    python3 toolbox/gha_matrix.py <<< "$BASE_MATRIX" >> "$GITHUB_OUTPUT"

The workflow of an entry is 'test-{os}-{ver}-{transport}.yaml', falling back to
'test-{os}-{ver}.yaml' for the entries whose 'ver' already carries the transport.
"""

import json
import sys
from pathlib import Path

import yaml

WORKFLOWS = Path("cijoe/workflows")
TEST_PREFIX = "test_"


def workflow_path(entry):
    """Returns the path to the cijoe workflow which the given entry runs"""

    stem = f"test-{entry['os']}-{entry['ver']}"
    candidates = [
        WORKFLOWS / f"{stem}-{entry['transport']}.yaml",
        WORKFLOWS / f"{stem}.yaml",
    ]

    for candidate in candidates:
        if candidate.is_file():
            return candidate

    raise SystemExit(f"no workflow for entry({entry}), tried({candidates})")


def expand(entry):
    """Returns a list of matrix entries, one per test-step of the entry's workflow"""

    workflow = yaml.safe_load(workflow_path(entry).read_text())
    names = [step["name"] for step in workflow.get("steps", [])]

    tests = [name for name in names if name.startswith(TEST_PREFIX)]

    # Without test-steps there is nothing to split on; run the workflow as it is
    if not tests:
        return [entry_with(entry, "", "")]

    # Keep the order the workflow declares, dropping only the other test-steps. Taking
    # the non-test steps as a prefix instead would move any teardown step ahead of the
    # test it is meant to run after.
    return [
        entry_with(
            entry,
            test,
            " ".join(
                name
                for name in names
                if name == test or not name.startswith(TEST_PREFIX)
            ),
        )
        for test in tests
    ]


def entry_with(entry, test, steps):
    """Returns the given entry, extended with its 'id', 'test' and 'steps'"""

    # 'id' identifies the entry, both as the job-name and when naming outputs and
    # artifacts. It has to carry the transport as well: without it two workflows for the
    # same guest collide whenever they happen to share a test-step name.
    return {
        "id": "-".join(
            part
            for part in (entry["os"], entry["ver"], entry["transport"], test)
            if part
        ),
        **entry,
        "test": test,
        "steps": steps,
    }


def main():
    entries = yaml.safe_load(sys.stdin)

    expanded = [item for entry in entries for item in expand(entry)]

    print("matrix=" + json.dumps({"include": expanded}))


if __name__ == "__main__":
    main()
