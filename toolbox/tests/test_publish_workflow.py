from pathlib import Path

import pytest
import yaml

ROOT = Path(__file__).parents[2]
WORKFLOWS = ROOT / ".github" / "workflows"


def steps(workflow, job):
    content = yaml.safe_load((WORKFLOWS / workflow).read_text())
    return content["jobs"][job]["steps"]


def test_every_push_to_main_builds_the_documentation_of_main():
    found = [
        step
        for step in steps("publish-docs.yml", "docs")
        if "--ref main -f job=docgen" in step.get("run", "")
    ]

    assert len(found) == 1
    assert "if" not in found[0]


def test_a_release_builds_the_documentation_of_its_tag():
    found = [
        step
        for step in steps("publish-release.yml", "release")
        if '--ref "v$VERSION" -f job=docgen' in step.get("run", "")
    ]

    assert len(found) == 1
    assert found[0]["if"] == "steps.version.outputs.version"


@pytest.mark.parametrize("workflow", ["publish-docs.yml", "publish-release.yml"])
def test_publishing_deploys_no_documentation_itself(workflow):
    text = (WORKFLOWS / workflow).read_text()

    assert "DOCS_PAT" not in text
    assert "xnvme-docs-dest" not in text


def test_both_publish_on_a_push_to_main():
    for workflow in ("publish-docs.yml", "publish-release.yml"):
        content = yaml.safe_load((WORKFLOWS / workflow).read_text())
        # PyYAML reads the key 'on' as True
        assert content[True] == {"push": {"branches": ["main"]}}


def test_the_docgen_deploy_retries_a_rejected_push():
    push = [
        step
        for step in steps("docgen-full.yml", "docgen-full")
        if step.get("name") == "Push site-changes"
    ]

    assert len(push) == 1
    assert "git pull --rebase" in push[0]["run"]


def test_a_docgen_run_runs_only_what_the_documentation_needs():
    """'publish-docs' starts 'verify' with job=docgen after every merge; main
    runs nothing else again"""

    jobs = yaml.safe_load((WORKFLOWS / "verify.yml").read_text())["jobs"]

    only = (
        "needs.changes-to-docs.outputs.docs_prose_only != 'true' && "
        "(github.event_name == 'pull_request' || github.event.inputs.job == 'all')"
    )
    for name in ("ffi-rust", "verify-makefile", "verify-packaging"):
        assert jobs[name]["if"] == only, name
    assert jobs["source-format-check"]["if"] == (
        "github.event.pull_request.draft != true && github.event.inputs.job != 'docgen'"
    )
    assert "source-format-check" not in jobs["docgen-full"]["needs"]

    # ci-result requires the source checks of a pull-request only, or every
    # docgen run, which skips them, would fail
    step = next(
        step
        for step in jobs["ci-result"]["steps"]
        if step.get("name") == "Check the results of the jobs"
    )
    assert step["env"]["EVENT"] == "${{ github.event_name }}"
    check = step["run"]
    guard = check.index('if [ "$EVENT" = "pull_request" ]; then')
    assert guard < check.index("for job in source-format-check docgen-shallow")
