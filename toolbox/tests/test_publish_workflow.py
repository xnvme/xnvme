import hashlib
import importlib.util
import json
import os
import re
import shutil
import subprocess
import sys
import tarfile
from pathlib import Path

import pytest
import yaml

ROOT = Path(__file__).parents[2]
WORKFLOWS = ROOT / ".github" / "workflows"
GATES_SPEC = importlib.util.spec_from_file_location(
    "gha_gates", ROOT / "toolbox" / "gha_gates.py"
)
gha_gates = importlib.util.module_from_spec(GATES_SPEC)
GATES_SPEC.loader.exec_module(gha_gates)

# The values of the expressions the steps run below take their env from
CONTEXTS = {
    "${{ github.token }}": "token",
    "${{ github.repository }}": "xnvme/xnvme",
    "${{ github.event_name }}": "pull_request",
    "${{ github.event.action }}": "synchronize",
    "${{ github.run_attempt }}": "1",
    "${{ github.event.pull_request.number }}": "1",
    "${{ steps.check.outputs.version }}": "1.2.3",
    "${{ fromJSON(steps.check.outputs.runs || '{}')['verify.yml'] }}": "42",
}

# A gh that downloads, for --name, the files the upload-artifact step of that
# name in verify.yml uploads, as STUB_ARTIFACTS lists them, and for --pattern
# '*-reports', one artifact of reports
GH = f"""#!{sys.executable}
import json, os, sys
from pathlib import Path

args = sys.argv[1:]
assert args[:2] == ["run", "download"], args
opts = dict(zip(args[3::2], args[4::2]))
directory = Path(opts["--dir"])
if "--name" in opts:
    files = json.loads(os.environ["STUB_ARTIFACTS"]).get(opts["--name"])
    if files is None:
        sys.exit(f"no artifact {{opts['--name']}}")
    for name in files:
        directory.mkdir(parents=True, exist_ok=True)
        (directory / name).write_text(opts["--name"] + "/" + name)
elif opts.get("--pattern") == "*-reports":
    (directory / "verify-reports").mkdir(parents=True)
    (directory / "verify-reports" / "report.txt").write_text("report")
else:
    sys.exit(f"no artifact matches {{opts.get('--pattern')}}")
"""


def workflow(name):
    return yaml.safe_load((WORKFLOWS / name).read_text())


def steps(workflow_, job):
    return workflow(workflow_)["jobs"][job]["steps"]


def step_named(workflow_, job, name):
    (found,) = [step for step in steps(workflow_, job) if step.get("name") == name]
    return found


def run_step(workflow_, job, name, cwd, contexts=(), **extra):
    """Run the step 'name' of 'job' in 'cwd' as GitHub runs a bash step, with
    the env of the job and of the step, each value taken from CONTEXTS and
    'contexts' by its expression, so a variable the step lacks stays unset;
    'extra' adds to the environment, and 'python3' and the stub 'gh' come
    first on PATH"""

    values = CONTEXTS | dict(contexts)
    declared = workflow(workflow_)["jobs"][job].get("env", {})
    declared = declared | step_named(workflow_, job, name).get("env", {})
    stubs = cwd / "stub-bin"
    stubs.mkdir(exist_ok=True)
    if not (stubs / "python3").exists():
        (stubs / "python3").symlink_to(sys.executable)
        (stubs / "gh").write_text(GH)
        (stubs / "gh").chmod(0o755)
    output = cwd / "github-output"
    output.write_text("")
    env = {
        "PATH": f"{stubs}:{os.environ['PATH']}",
        "GITHUB_OUTPUT": str(output),
        "RUNNER_TEMP": str(cwd),
    }
    env |= {key: values[value] for key, value in declared.items()}

    return subprocess.run(
        ["bash", "--noprofile", "--norc", "-eo", "pipefail", "-c"]
        + [step_named(workflow_, job, name)["run"]],
        cwd=cwd,
        env=env | extra,
        capture_output=True,
        text=True,
    )


def uploads():
    """{artifact: [file names]} of the upload-artifact steps of verify.yml, the
    name expanded for each variant of the job's matrix"""

    found = {}
    for job in workflow("verify.yml")["jobs"].values():
        variants = job.get("strategy", {}).get("matrix", {}).get("variant", [None])
        for step in job.get("steps", []):
            if not step.get("uses", "").startswith("actions/upload-artifact@"):
                continue
            for variant in variants:
                name = re.sub(
                    r"\$\{\{ matrix\.variant == '([^']+)' && '([^']*)' \|\| '' \}\}",
                    lambda m: m.group(2) if variant == m.group(1) else "",
                    step["with"]["name"],
                )
                found[name] = [Path(path).name for path in step["with"]["path"].split()]

    return found


def test_every_push_to_main_builds_the_documentation_of_main():
    found = [
        step
        for step in steps("publish-docs.yml", "docs")
        if "--ref main -f job=docgen" in step.get("run", "")
    ]

    assert len(found) == 1
    assert "if" not in found[0]


def action_steps(workflow, job, action):
    """The steps of 'job' that use the 'action' of gha-project-auto-release"""

    uses = f"refenv/gha-project-auto-release/{action}@v1"
    return [step for step in steps(workflow, job) if step.get("uses") == uses]


def test_a_release_builds_the_documentation_of_its_tag():
    found = action_steps("publish-release.yml", "release", "release")

    assert len(found) == 1
    assert found[0]["with"]["dispatch"] == "verify.yml job=docgen"
    # The token of the job starts the run
    release = workflow("publish-release.yml")["jobs"]["release"]
    assert release["permissions"]["actions"] == "write"


def test_the_release_has_the_title_and_notes_the_checklist_names(tmp_path):
    (action,) = action_steps("publish-release.yml", "release", "release")
    checklist = (ROOT / "docs" / "contributing" / "release-checklist.rst").read_text()

    assert action["with"]["title"] == "Release Notes: {tag}"
    assert "titled ``Release Notes: vX.Y.Z``" in checklist

    # The notes are what xnvme_release.py writes for the subject of the bump
    (tmp_path / "toolbox").mkdir()
    (tmp_path / "toolbox" / "xnvme_release.py").write_text(
        'import sys\nprint("notes of", sys.argv[1])\n'
    )
    git = ["git", "-c", "user.name=Test", "-c", "user.email=test@example.com"]
    subprocess.run(git + ["init", "-q"], cwd=tmp_path, check=True)
    subprocess.run(
        git + ["commit", "-q", "--allow-empty", "-m", "ver: bump to v1.2.3"],
        cwd=tmp_path,
        check=True,
    )
    notes = "Check the files carrying the version, and write the release notes"

    run = run_step("publish-release.yml", "release", notes, tmp_path)

    assert run.returncode == 0, run.stderr
    assert (tmp_path / "notes.md").read_text() == "notes of ver: bump to v1.2.3\n"

    # xnvme_release.py fails when a file carrying the version lacks the bump
    (tmp_path / "toolbox" / "xnvme_release.py").write_text("raise SystemExit(1)\n")

    assert run_step("publish-release.yml", "release", notes, tmp_path).returncode


def test_a_release_requires_ci_result_and_every_gated_job():
    content = yaml.safe_load((WORKFLOWS / "publish-release.yml").read_text())
    assert content["jobs"]["release"]["if"] == (
        "startsWith(github.event.head_commit.message, 'ver: bump to v')"
    )

    listing = next(
        s for s in steps("publish-release.yml", "release") if s.get("id") == "jobs"
    )
    assert "echo 'verify.yml: ci-result'" in listing["run"]
    assert (
        "python3 toolbox/gha_gates.py jobs | sed 's/^/verify.yml: /'" in listing["run"]
    )

    (check,) = action_steps("publish-release.yml", "release", "check")
    # The steps after it read its outputs by this id
    assert check["id"] == "check"
    assert check["with"]["workflows"] == "verify.yml"
    assert check["with"]["jobs"] == "${{ steps.jobs.outputs.jobs }}"


def test_a_failed_check_adds_no_template_error():
    """When check@v1 fails, its outputs are empty, and GitHub still evaluates
    the env of the steps after it; fromJSON('') would add a template error"""

    runs = [
        step["env"]["RUN"]
        for step in steps("publish-release.yml", "release")
        if "RUN" in step.get("env", {})
    ]

    assert set(runs) == {
        "${{ fromJSON(steps.check.outputs.runs || '{}')['verify.yml'] }}"
    }


def test_the_release_checklist_requires_the_tag_in_the_archives_of_a_release():
    (check,) = [
        step
        for step in steps("verify.yml", "release-checklist")
        if "--archive" in step.get("run", "")
    ]
    archives = [line for line in check["run"].splitlines() if "--archive" in line]

    assert check["env"]["VERSION"] == "${{ needs.gates.outputs.version }}"
    assert len(archives) == 2
    assert all(line.endswith("${VERSION:+--release} || status=1") for line in archives)
    assert "--archive src/xnvme-src.tar.gz --subprojects upcie,ufuse " in archives[0]
    assert "--archive fat/xnvme-src.tar.gz --subprojects upcie,ufuse,spdk " in (
        archives[1]
    )
    downloaded = {
        step["with"]["name"]: step["with"]["path"]
        for step in steps("verify.yml", "release-checklist")
        if step.get("uses", "").startswith("actions/download-artifact@")
    }
    assert downloaded == {
        "xnvme-src-archive": "src",
        "xnvme-src-archive-with-subprojects": "fat",
    }


def test_a_release_opens_every_gate(tmp_path):
    (tmp_path / "base" / "toolbox").mkdir(parents=True)
    shutil.copy(ROOT / "toolbox" / "gha_gates.py", tmp_path / "base" / "toolbox")
    read = "Read the labels, and those the changed files call for"
    version = "${{ steps.ready.outputs.version }}"

    for released, expected in (("1.2.3", "true"), ("", "false")):
        contexts = {version: released, "${{ github.event_name }}": "push"}
        run = run_step("verify.yml", "gates", read, tmp_path, contexts)

        assert run.returncode == 0, run.stderr
        assert (tmp_path / "github-output").read_text().splitlines() == [
            f"{gate}={expected}" for gate in gha_gates.GATES
        ]


def test_the_gates_output_the_version_a_pull_request_releases():
    gates = workflow("verify.yml")["jobs"]["gates"]
    (ready,) = [step for step in gates["steps"] if step.get("id") == "ready"]

    assert gates["outputs"]["version"] == "${{ steps.ready.outputs.version }}"
    assert ready["if"] == "github.event_name == 'pull_request'"
    assert ready["uses"] == "refenv/gha-project-auto-release/ready@v1"


def test_ci_result_needs_every_other_job():
    jobs = workflow("verify.yml")["jobs"]

    assert set(jobs["ci-result"]["needs"]) == set(jobs) - {"ci-result"}


def test_ci_result_fails_a_release_unless_every_job_passed(tmp_path):
    (tmp_path / "toolbox").mkdir()
    shutil.copy(ROOT / "toolbox" / "gha_gates.py", tmp_path / "toolbox")
    needs = {job: {"result": "success"} for job in gha_gates.REQUIRED}
    needs["perf"] = {"result": "skipped"}
    check = "Check the results of the jobs"

    for released, returncode in (("1.2.3", 1), ("", 0)):
        contexts = {
            "${{ toJSON(needs) }}": json.dumps(needs),
            "${{ needs.gates.outputs.version }}": released,
        }
        run = run_step("verify.yml", "ci-result", check, tmp_path, contexts)

        assert run.returncode == returncode, run.stdout + run.stderr


def test_a_missing_contributor_fails_only_a_release(tmp_path):
    (tmp_path / "toolbox").mkdir()
    script = tmp_path / "toolbox" / "xnvme_release.py"
    check = "Check that CONTRIBUTORS.md names the authors since the previous release"

    script.write_text('print("A Person")\n')
    for released, returncode, annotation in (
        ("1.2.3", 1, "::error::"),
        ("", 0, "::warning::"),
    ):
        contexts = {"${{ needs.gates.outputs.version }}": released}
        run = run_step("verify.yml", "release-checklist", check, tmp_path, contexts)

        assert run.returncode == returncode
        assert "A Person" in run.stdout
        assert annotation in run.stdout

    # CONTRIBUTORS.md names every author
    script.write_text("")
    for released in ("1.2.3", ""):
        contexts = {"${{ needs.gates.outputs.version }}": released}
        run = run_step("verify.yml", "release-checklist", check, tmp_path, contexts)

        assert run.returncode == 0
        assert "::" not in run.stdout

    # A script that fails, as without a tag of a release, is no list of authors
    script.write_text("raise SystemExit(1)\n")
    for released in ("1.2.3", ""):
        contexts = {"${{ needs.gates.outputs.version }}": released}
        run = run_step("verify.yml", "release-checklist", check, tmp_path, contexts)

        assert run.returncode == 1


def test_the_release_checklist_fails_on_either_archive(tmp_path):
    (tmp_path / "toolbox").mkdir()
    calls = tmp_path / "calls"
    (tmp_path / "toolbox" / "xnvme_release.py").write_text(
        "import os, sys\n"
        f"with open({str(calls)!r}, 'a') as fd:\n"
        "    fd.write(' '.join(sys.argv[1:]) + '\\n')\n"
        "sys.exit(1 if os.environ['STUB_FAIL'] in sys.argv[2] else 0)\n"
    )
    check = "Check the source archives"

    for released, fail, returncode in (
        ("1.2.3", "src/", 1),
        ("1.2.3", "fat/", 1),
        ("", "src/", 1),
        ("1.2.3", "none", 0),
        ("", "none", 0),
    ):
        calls.write_text("")
        contexts = {"${{ needs.gates.outputs.version }}": released}
        run = run_step(
            "verify.yml",
            "release-checklist",
            check,
            tmp_path,
            contexts,
            STUB_FAIL=fail,
        )
        called = calls.read_text().splitlines()

        assert run.returncode == returncode
        assert len(called) == 2
        assert all(line.endswith("--release") == bool(released) for line in called)


def test_the_release_checklist_checks_the_head_with_its_history():
    """The release checklist reads the tags and the authors since the previous
    release, and the source archive was made from the head of the
    pull-request"""

    (checkout,) = [
        step
        for step in steps("verify.yml", "release-checklist")
        if step.get("uses", "").startswith("actions/checkout@")
    ]

    assert checkout["with"]["ref"] == (
        "${{ github.event.pull_request.head.sha || github.sha }}"
    )
    assert checkout["with"]["fetch-depth"] == 0


def test_the_ready_action_reads_the_head_of_the_pull_request():
    """The ready action reads the subject of HEAD, which on the merge commit
    GitHub checks out for a pull-request by default is not the bump"""

    grab = step_named("verify.yml", "gates", "Grab the head of the pull-request")
    names = [step.get("name") for step in steps("verify.yml", "gates")]

    assert grab["if"] == "github.event_name == 'pull_request'"
    assert grab["with"]["ref"] == "${{ github.event.pull_request.head.sha }}"
    assert names.index(grab["name"]) < names.index(
        "Check whether the pull-request releases a version"
    )


def test_publish_release_lists_ci_result_and_every_gated_job(tmp_path):
    (tmp_path / "toolbox").mkdir()
    shutil.copy(ROOT / "toolbox" / "gha_gates.py", tmp_path / "toolbox")
    listing = "List the jobs that must have passed"

    run = run_step("publish-release.yml", "release", listing, tmp_path)

    assert run.returncode == 0, run.stderr
    gated = [job for jobs in gha_gates.GATES.values() for job in jobs]
    assert (tmp_path / "github-output").read_text().splitlines() == (
        ["jobs<<EOF", "verify.yml: ci-result"]
        + [f"verify.yml: {job}" for job in gated]
        + ["EOF"]
    )


def test_a_release_attaches_the_archives_and_reports_of_the_run(tmp_path):
    collect = "Collect the release assets from the pull-request's run"
    run = run_step(
        "publish-release.yml",
        "release",
        collect,
        tmp_path,
        STUB_ARTIFACTS=json.dumps(uploads()),
    )
    assets = tmp_path / "assets"

    assert run.returncode == 0, run.stderr
    archives = ["xnvme-1.2.3.tar.gz", "xnvme-fat-1.2.3.tar.gz"]
    archives += ["xnvme-py-sdist-v1.2.3.tar.gz"]
    assert sorted(p.name for p in assets.iterdir()) == sorted(
        archives
        + [f"{name}.sha256sum" for name in archives]
        + ["test-results-v1.2.3.tar.gz"]
    )
    for name in archives:
        digest = hashlib.sha256((assets / name).read_bytes()).hexdigest()
        assert (assets / f"{name}.sha256sum").read_text() == f"{digest}  {name}\n"
    # The stub writes the name of the artifact and the file into each file
    assert (assets / "xnvme-1.2.3.tar.gz").read_text() == (
        "xnvme-src-archive/xnvme-src.tar.gz"
    )
    assert (assets / "xnvme-py-sdist-v1.2.3.tar.gz").read_text() == (
        "xnvme-py-sdist/xnvme-py-sdist.tar.gz"
    )
    assert (assets / "xnvme-fat-1.2.3.tar.gz").read_text() == (
        "xnvme-src-archive-with-subprojects/xnvme-src.tar.gz"
    )
    with tarfile.open(assets / "test-results-v1.2.3.tar.gz") as tar:
        assert "reports/verify-reports/report.txt" in tar.getnames()


def test_the_release_takes_the_version_notes_and_assets():
    release = workflow("publish-release.yml")
    (action,) = action_steps("publish-release.yml", "release", "release")

    assert release["concurrency"]["cancel-in-progress"] is False
    assert action["with"]["version"] == "${{ steps.check.outputs.version }}"
    assert action["with"]["notes-file"] == "${{ runner.temp }}/notes.md"
    assert action["with"]["assets"] == "assets/*"
    notes = "Check the files carrying the version, and write the release notes"
    assert (
        '> "$RUNNER_TEMP/notes.md"'
        in step_named("publish-release.yml", "release", notes)["run"]
    )


def test_the_actions_read_the_version_as_xnvme_release_does():
    spec = importlib.util.spec_from_file_location(
        "xnvme_release", ROOT / "toolbox" / "xnvme_release.py"
    )
    release = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(release)
    path, pattern = release.VERSIONED[0]

    found = action_steps("publish-release.yml", "release", "check")
    found += action_steps("verify.yml", "gates", "ready")
    assert len(found) == 2
    for step in found:
        assert step["with"]["version-file"] == path
        assert step["with"]["version-pattern"] == pattern
        # CHANGELOG.rst is not the Markdown the actions read; xnvme_release.py
        # checks it and writes the notes
        assert step["with"]["changelog"] == ""


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
    step = step_named("verify.yml", "ci-result", "Check the results of the jobs")
    assert step["env"]["EVENT"] == "${{ github.event_name }}"
    assert '--event "$EVENT"' in step["run"]
    docgen = {"docgen-full": {"result": "success"}}
    docgen |= {job: {"result": "skipped"} for job in gha_gates.REQUIRED}
    assert gha_gates.verdict(docgen, "workflow_dispatch", "") == []


def test_the_source_archive_is_made_from_the_head_of_the_pull_request():
    """vcs_tag.py --dist reads the subject of HEAD, which on the merge commit
    GitHub checks out for a pull-request by default is not the bump"""

    (checkout,) = [
        step
        for step in steps("verify.yml", "source-archive")
        if step.get("uses", "").startswith("actions/checkout@")
    ]

    assert checkout["with"]["ref"] == (
        "${{ github.event.pull_request.head.sha || github.sha }}"
    )
