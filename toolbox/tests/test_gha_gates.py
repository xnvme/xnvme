import importlib.util
import json
import subprocess
import sys
from pathlib import Path

import pytest
import yaml

ROOT = Path(__file__).parents[2]
SCRIPT = ROOT / "toolbox" / "gha_gates.py"
spec = importlib.util.spec_from_file_location("gha_gates", SCRIPT)
gates = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gates)

RULES = gates.rules(ROOT / ".github" / "labeler.yml")
VERIFY = (ROOT / ".github" / "workflows" / "verify.yml").read_text()
LABELER = (ROOT / ".github" / "workflows" / "labeler.yml").read_text()
COUNT = len(gates.GATES)


@pytest.mark.parametrize(
    "glob, path, matches",
    [
        ("lib/upcie/**", "lib/upcie/xnvme_be_upcie_dev.c", True),
        ("lib/upcie/**", "lib/upcie/sub/dir/file.c", True),
        ("lib/upcie/**", "lib/upcie", False),
        ("lib/xnvme_be_linux*", "lib/xnvme_be_linux_nvme.c", True),
        ("lib/xnvme_be_linux*", "lib/sub/xnvme_be_linux.c", False),
        ("**/*cuda*", "tools/xnvmeperf/xnvmeperf_cuda.cu", True),
        ("**/*cuda*", "xnvmeperf_cuda.cu", True),
        ("**/*cuda*", "lib/cudafree/file.c", False),
        ("include/**", ".github/include/x", False),
        ("meson.build", "meson.build", True),
        ("meson.build", "lib/meson.build", False),
        ("*.md", "README.md", True),
        ("*.md", "docs/x.md", False),
    ],
)
def test_glob_matches_as_minimatch_does(glob, path, matches):
    assert bool(gates.glob_regex(glob).match(path)) == matches


@pytest.mark.parametrize(
    "glob",
    ["a**b", "lib/**.c", "lib/xnvme_be_{linux,spdk}*", "lib/[ab].c", "!lib/x"],
)
def test_a_glob_minimatch_reads_otherwise_is_an_error(glob):
    with pytest.raises(ValueError):
        gates.glob_regex(glob)


@pytest.mark.parametrize(
    "files, expected",
    [
        (["docs/index.rst"], ["gate:docgen"]),
        (["README.md"], ["gate:docgen"]),
        (["toolbox/gha_gates.py"], ["gate:emulated"]),
        (["tests/xnvme_tests_ioworker.c"], ["gate:emulated"]),
        (["examples/xnvme_hello.c"], ["gate:emulated"]),
        ([".github/workflows/verify-emulated.yml"], ["gate:emulated"]),
        ([".github/workflows/verify.yml"], ["gate:emulated", "gate:release"]),
        (["Makefile"], ["gate:emulated", "gate:release"]),
        (["CHANGELOG.rst"], ["gate:release"]),
        (["toolbox/xnvme_release.py"], ["gate:emulated", "gate:release"]),
        (["subprojects/spdk.wrap"], ["gate:emulated", "gate:release"]),
        (["toolbox/pkgs/debian-trixie.sh"], ["gate:emulated"]),
        (["meson.build"], ["gate:emulated", "gate:release"]),
        (
            ["lib/backend/upcie/xnvme_be_upcie_dev.c"],
            ["gate:emulated", "gate:hardware", "gate:perf"],
        ),
        (
            ["lib/backend/upcie/xnvme_be_upcie_cuda_gpu.c"],
            ["gate:emulated", "gate:hardware", "gate:gpu", "gate:perf"],
        ),
        (
            ["lib/backend/liburing/xnvme_be_liburing_dmabuf.c"],
            ["gate:emulated", "gate:hardware", "gate:gpu"],
        ),
        (["lib/backend/spdk/xnvme_be_spdk_dev.c"], ["gate:emulated", "gate:rdma"]),
        (
            ["tools/xnvmeperf/xnvmeperf.c", "docs/index.rst"],
            ["gate:emulated", "gate:perf", "gate:docgen"],
        ),
    ],
)
def test_labels_for_the_repository_rules(files, expected):
    assert gates.labels(RULES, files) == expected


def test_an_unsupported_rule_is_an_error(tmp_path):
    rules = tmp_path / "labeler.yml"
    rules.write_text("x:\n- head-branch: ['^feat']\n")

    with pytest.raises(ValueError):
        gates.rules(rules)


@pytest.mark.parametrize(
    "action, attempt, counts",
    [
        ("ready_for_review", "1", True),
        ("opened", "1", True),
        ("reopened", "1", False),
        ("ready_for_review", "2", False),
        ("synchronize", "1", False),
        ("labeled", "1", False),
    ],
)
def test_only_the_first_attempt_of_the_labelers_run_counts_the_rules(
    action, attempt, counts
):
    assert gates.counts_rules(action, attempt) == counts


def test_the_rules_are_counted_on_the_labelers_actions():
    actions = ", ".join(gates.LABELER_ACTIONS)

    assert f"types: [{actions}]" in LABELER


@pytest.mark.parametrize(
    "labels, expected",
    [
        ([], set()),
        (["gate:gpu", "documentation"], {"gpu"}),
        (["Gate:Hardware"], {"hardware"}),
        (["gate:emulated", "gate:docgen"], {"emulated", "docgen"}),
        (["gate:all"], set(gates.GATES)),
        (["gate:ALL", "gate:gpu"], set(gates.GATES)),
        (["gpu", "hardware"], set()),
    ],
)
def test_the_labels_open_their_gates(labels, expected):
    opened = gates.opened(labels)

    assert set(opened) == set(gates.GATES)
    assert {gate for gate, is_open in opened.items() if is_open} == expected


def test_every_gate_has_a_rule_an_output_and_its_jobs_in_verify():
    assert sorted(RULES) == sorted(f"gate:{gate}" for gate in gates.GATES)
    for gate, jobs in gates.GATES.items():
        assert f"      {gate}: ${{{{ steps.read.outputs.{gate} }}}}" in VERIFY
        for job in jobs:
            start = VERIFY.index(f"\n  {job}:\n")
            end = VERIFY.index("\n\n", start)
            assert f"needs.gates.outputs.{gate} == 'true'" in VERIFY[start:end]
            assert "docs_prose_only" not in VERIFY[start:end]


NEEDS = {
    "gates": {"result": "success", "outputs": {"emulated": "true", "gpu": "true"}},
    "verify-nvme-pcie-emulated": {"result": "success"},
    "verify-nvme-tcp-emulated": {"result": "failure"},
    "verify-nvme-pcie-hardware": {"result": "success"},
    "verify-gpu-emulated": {"result": "skipped"},
    "verify-nvme-rdma-emulated": {"result": "skipped"},
    "perf": {"result": "success"},
    "docgen-full": {"result": "skipped"},
    "build": {"result": "success"},
}


def test_summary_counts_the_gates_that_passed():
    out = gates.summary(NEEDS)

    assert out.splitlines()[-1] == f"2/{COUNT} gates passed: hardware, perf"
    assert (
        "| gate:emulated | verify-nvme-pcie-emulated, verify-nvme-tcp-emulated "
        "| failure |" in out
    )


def test_summary_tells_an_open_gate_not_run_from_one_not_opened():
    out = gates.summary(NEEDS)

    assert "| gate:gpu | verify-gpu-emulated | open, not run |" in out
    assert "| gate:rdma | verify-nvme-rdma-emulated | not opened |" in out


def test_summary_with_no_gate_opened():
    results = {
        job: {"result": "skipped"} for jobs in gates.GATES.values() for job in jobs
    }

    assert gates.summary(results).splitlines()[-1] == f"0/{COUNT} gates passed: none"


JOBS = [
    {"name": "gates", "conclusion": "success"},
    {"name": "verify-nvme-pcie-emulated / (debian, test_os)", "conclusion": "success"},
    {"name": "verify-nvme-tcp-emulated / (debian, test_tcp)", "conclusion": "success"},
    {"name": "verify-nvme-pcie-hardware / passthru (debian)", "conclusion": "success"},
    {"name": "verify-nvme-pcie-hardware / physical (macos)", "conclusion": "failure"},
    {"name": "verify-gpu-emulated", "conclusion": "skipped"},
    {
        "name": "verify-nvme-rdma-emulated / verify-nvme-rdma-emulated",
        "conclusion": "success",
    },
    {"name": "perf / latency (linux)", "conclusion": "success"},
    {"name": "perf / bdev_xnvme", "conclusion": "cancelled"},
    {"name": "docgen-full / docgen-full (debian)", "conclusion": "success"},
    {"name": "ci-result", "conclusion": "failure"},
]


def test_a_gate_gives_the_worst_of_its_called_jobs():
    results = gates.results_from_jobs(JOBS)

    assert results["verify-nvme-pcie-hardware"] == {"result": "failure"}
    assert results["verify-gpu-emulated"] == {"result": "skipped"}
    assert results["verify-nvme-rdma-emulated"] == {"result": "success"}
    assert results["perf"] == {"result": "cancelled"}
    assert results["ci-result"] == {"result": "failure"}


def log_of(summary):
    stamped = [f"2026-10-30T12:00:00.0000000Z {line}" for line in summary.splitlines()]
    return "\n".join(["2026-10-30T11:59:59.0000000Z ##[group]Run x"] + stamped)


def test_the_report_takes_the_table_ci_result_printed():
    table = gates.summary(NEEDS)

    body = gates.report(JOBS, log_of(table), "0123456789abcdef", "2", "https://x")

    lines = body.splitlines()
    assert lines[0] == gates.REPORT_MARKER
    assert lines[1].startswith("**ci-result: failure** at `012345678`")
    assert body.endswith(table)


def test_the_report_takes_no_line_summary_would_not_write():
    table = gates.summary(NEEDS).replace("| open, not run |", "| @everyone |")

    body = gates.report(JOBS, log_of(table), "0" * 40, "1", "https://x")

    assert "@everyone" not in body
    assert body.splitlines()[-1] == f"3/{COUNT} gates passed: emulated, rdma, docgen"


def test_no_report_for_a_run_without_ci_result():
    draft = [{"name": "ci-result", "conclusion": "skipped"}]

    assert gates.report(draft, "", "0" * 40, "1", "u") is None
    assert gates.report([], "", "0" * 40, "1", "u") is None


def test_only_the_gated_jobs_and_ci_result_need_gates():
    """Re-running 'gates' re-runs what needs it, so that must be all it re-runs"""

    content = yaml.safe_load(VERIFY)
    gated = {job for jobs in gates.GATES.values() for job in jobs}

    needing = {
        name
        for name, job in content["jobs"].items()
        if "gates" in (job.get("needs") or [])
    }
    assert needing == gated | {"ci-result"}


def test_jobs_lists_every_gated_job():
    """The 'publish-release' workflow requires what 'jobs' lists to have passed"""

    listed = subprocess.run(
        [sys.executable, str(SCRIPT), "jobs"],
        capture_output=True,
        text=True,
        check=True,
    ).stdout.split()

    assert listed == [job for jobs in gates.GATES.values() for job in jobs]
    assert "release-checklist" in listed


def needs(**results):
    """A 'needs' context of jobs with 'results', the fixed jobs a pull-request
    must pass among them unless given"""

    jobs = {job: "success" for job in gates.REQUIRED} | results
    return {job: {"result": result} for job, result in jobs.items()}


def test_a_pull_request_passes_with_its_gates_closed():
    assert gates.verdict(needs(perf="skipped"), "pull_request", "") == []


def test_a_failed_or_cancelled_job_fails_any_run():
    results = needs(perf="failure", build="cancelled", matrices="skipped")

    for event in ("pull_request", "workflow_dispatch"):
        assert gates.verdict(results, event, "") == ["Did not succeed: perf build"]


def test_a_pull_request_requires_its_fixed_jobs():
    results = needs(**{"docgen-shallow": "skipped"})

    assert gates.verdict(results, "pull_request", "") == [
        "docgen-shallow is required and was skipped"
    ]
    assert gates.verdict(results, "workflow_dispatch", "") == []


def test_a_release_requires_every_job_to_have_passed():
    results = needs(perf="skipped", build="success")

    assert gates.verdict(results, "pull_request", "1.2.3") == [
        "v1.2.3 is released only when every job passed; did not: perf"
    ]
    assert gates.verdict(needs(perf="success"), "pull_request", "1.2.3") == []


def test_verdict_prints_the_results_and_fails_with_why():
    run = subprocess.run(
        [sys.executable, str(SCRIPT), "verdict", "--event", "pull_request"]
        + ["--version", "1.2.3"],
        input=json.dumps(needs(perf="skipped")),
        capture_output=True,
        text=True,
    )

    assert run.returncode == 1
    assert "perf: skipped" in run.stdout.splitlines()
    assert run.stdout.splitlines()[-1] == (
        "::error::v1.2.3 is released only when every job passed; did not: perf"
    )
