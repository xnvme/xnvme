#!/usr/bin/env python3
"""
The gates of the 'verify' workflow: the labels that open them, and what they gave

'labels' is run by the 'gates' job on the files a pull-request changes, read
from stdin one per line, and prints the labels they call for, one per line. It
reads the same rules, .github/labeler.yml of the base branch, that the
'labeler' workflow adds labels by, so a run started as the pull-request leaves
draft has the jobs those labels add without waiting for the labeler to have
run. Only that run counts the rules, on its first attempt; by any later run or
attempt the labeler has run, and the labels alone decide, so a maintainer
removing one closes its gate rather than having the rules open it again. Only
the 'changed-files' rule 'any-glob-to-any-file' is supported, with '*', '**'
and '?' in its globs, as that is all .github/labeler.yml uses; anything else
is an error rather than a label silently not given.

'open' is run by the 'gates' job on the labels, read from stdin one per line,
and prints for each gate whether it is open, as 'name=true' or 'name=false',
the lines GITHUB_OUTPUT takes. 'gate:all' opens every gate.

'summary' is run by 'ci-result' on the results of the jobs, the 'needs' context
as JSON on stdin, and prints a table of the gates and what each gave, ending in
a line with how many of them passed.

'report' is run by the 'gates-report' workflow on the jobs of a finished run
attempt, as the API lists them, a JSON list of objects with 'name' and
'conclusion' on stdin, and the log of its 'ci-result' job, and prints the
comment it keeps on the pull-request: what 'ci-result' gave, and its table.
"""

import argparse
import json
import re
import sys
from pathlib import Path

# The pull-request event actions whose run counts the rules: those the 'labeler'
# workflow runs on, as it may not have added the labels yet
LABELER_ACTIONS = ("opened", "ready_for_review")

# The first line of the comment 'gates-report' keeps, by which it finds it again
REPORT_MARKER = "<!-- gates-report -->"

# The jobs each gate opens, by the name of its label after 'gate:'
GATES = {
    "emulated": ["verify-nvme-pcie-emulated", "verify-nvme-tcp-emulated"],
    "hardware": ["verify-nvme-pcie-hardware"],
    "gpu": ["verify-gpu-emulated"],
    "rdma": ["verify-nvme-rdma-emulated"],
    "perf": ["perf"],
    "docgen": ["docgen-full"],
}

# Job results, the worst first; a gate gives the worst of its jobs'
WORST = ["failure", "cancelled", "timed_out", "action_required", "success", "skipped"]

# What summary() writes in the result column, and so all report() accepts there
SHOWN = WORST + ["open, not run", "not opened", "missing", "pending"]


def glob_regex(glob):
    """A regex matching what the minimatch glob matches, as actions/labeler uses it"""

    if re.search(r"[{}\[\]()!+@]", glob):
        raise ValueError(f"{glob}: only '*', '**' and '?' are supported")

    out = ""
    i = 0
    while i < len(glob):
        if glob.startswith("**", i):
            starts = i == 0 or glob[i - 1] == "/"
            ends = i + 2 == len(glob) or glob[i + 2] == "/"
            if not (starts and ends):
                raise ValueError(f"{glob}: '**' must be a whole path segment")
            if i + 2 < len(glob):
                out += "(?:.*/)?"
                i += 3
            else:
                out += ".*"
                i += 2
        elif glob[i] == "*":
            out += "[^/]*"
            i += 1
        elif glob[i] == "?":
            out += "[^/]"
            i += 1
        else:
            out += re.escape(glob[i])
            i += 1

    return re.compile(f"^{out}$")


def rules(path):
    """The rules of 'path' as {label: [glob, ...]}"""

    import yaml

    content = yaml.safe_load(Path(path).read_text()) or {}

    out = {}
    for label, matchers in content.items():
        globs = []
        for matcher in matchers:
            if set(matcher) != {"changed-files"}:
                raise ValueError(
                    f"{label}: only 'changed-files' is supported, got {matcher}"
                )
            for option in matcher["changed-files"]:
                if set(option) != {"any-glob-to-any-file"}:
                    raise ValueError(
                        f"{label}: only 'any-glob-to-any-file' is supported, got {option}"
                    )
                globs += option["any-glob-to-any-file"]
        for glob in globs:
            glob_regex(glob)
        out[label] = globs

    return out


def labels(rules, files):
    """The labels whose globs match any of 'files', in the order of 'rules'"""

    found = []
    for label, globs in rules.items():
        regexes = [glob_regex(glob) for glob in globs]
        if any(regex.match(path) for path in files for regex in regexes):
            found.append(label)

    return found


def counts_rules(action, attempt):
    """Whether a run of the pull-request event 'action', on its 'attempt', counts
    what the rules call for besides the labels the pull-request carries"""

    return action in LABELER_ACTIONS and int(attempt) == 1


def opened(labels):
    """{gate: bool} for the 'labels' a pull-request carries; GitHub matches
    label names ignoring case, and so does this"""

    names = {label.strip().casefold() for label in labels}
    every = "gate:all" in names

    return {gate: every or f"gate:{gate}" in names for gate in GATES}


def worst(results):
    """The worst of 'results', job results as the 'needs' context has them"""

    return sorted(results, key=lambda r: WORST.index(r) if r in WORST else 0)[0]


def summary(results):
    """A markdown table of the gates and what their jobs gave, by 'results' as the
    'needs' context has them, and a last line with how many gates passed. A
    skipped job of a gate that 'gates' opened is shown as open but not run, as
    when the jobs it needs failed or the change is to documentation only"""

    opens = results.get("gates", {}).get("outputs", {})

    lines = ["| Gate | Jobs | Result |", "|---|---|---|"]
    passed = []
    for gate, jobs in GATES.items():
        result = worst([results.get(job, {}).get("result", "missing") for job in jobs])
        if result == "success":
            passed.append(gate)
        shown = result
        if result == "skipped":
            shown = "open, not run" if opens.get(gate) == "true" else "not opened"
        lines.append(f"| gate:{gate} | {', '.join(jobs)} | {shown} |")

    names = ", ".join(passed) if passed else "none"
    lines.append("")
    lines.append(f"{len(passed)}/{len(GATES)} gates passed: {names}")

    return "\n".join(lines)


def results_from_jobs(jobs):
    """The 'needs'-shaped results of the gated jobs and 'ci-result' from the jobs
    of a run as the API lists them. A called workflow's jobs are named
    '<caller> / <job>', or just '<caller>' when skipped; the result of the caller
    is the worst of its jobs."""

    results = {}
    for caller in [job for jobs in GATES.values() for job in jobs] + ["ci-result"]:
        found = [
            job.get("conclusion") or "pending"
            for job in jobs
            if job["name"] == caller or job["name"].startswith(f"{caller} / ")
        ]
        if found:
            results[caller] = {"result": worst(found)}

    return results


def table_from_log(log):
    """The table and the last line summary() printed, taken from the log of
    'ci-result', or None when the log does not have all of it. Only lines of the
    exact form summary() writes are taken, as the log is the pull-request's"""

    gate = "|".join(GATES)
    job = r"[a-z0-9-]+(?:, [a-z0-9-]+)*"
    shown = "|".join(re.escape(s) for s in SHOWN)
    row = re.compile(rf"^\| gate:({gate}) \| {job} \| (?:{shown}) \|$")
    last = re.compile(
        rf"^\d+/{len(GATES)} gates passed: (?:none|(?:{gate})(?:, (?:{gate}))*)$"
    )

    rows, final = {}, None
    for line in log.splitlines():
        line = re.sub(r"^\S+Z ", "", line.rstrip())
        if match := row.match(line):
            rows[match.group(1)] = line
        elif last.match(line):
            final = line

    if set(rows) != set(GATES) or final is None:
        return None

    head = ["| Gate | Jobs | Result |", "|---|---|---|"]
    return "\n".join(head + [rows[gate] for gate in GATES] + ["", final])


def report(jobs, log, sha, attempt, url):
    """The comment 'gates-report' keeps on the pull-request, or None when the run
    gave no 'ci-result', as for a draft"""

    results = results_from_jobs(jobs)
    verdict = results.get("ci-result", {}).get("result")
    if verdict in (None, "skipped"):
        return None

    table = table_from_log(log) or summary(results)
    lines = [
        REPORT_MARKER,
        f"**ci-result: {verdict}** at `{sha[:9]}`, [run attempt {attempt}]({url})",
        "",
        table,
    ]

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    labels_parser = sub.add_parser(
        "labels", help="The labels the changed files call for"
    )
    labels_parser.add_argument(
        "--rules", default=".github/labeler.yml", help="The labeler rules"
    )
    labels_parser.add_argument(
        "--action", required=True, help="The pull-request event's action"
    )
    labels_parser.add_argument(
        "--attempt", required=True, help="The attempt of the run"
    )
    sub.add_parser("open", help="Which gates the labels open")
    sub.add_parser("summary", help="The gates and what they gave")
    report_parser = sub.add_parser("report", help="The pull-request comment")
    report_parser.add_argument("--sha", required=True, help="The run's head")
    report_parser.add_argument("--attempt", required=True, help="The run attempt")
    report_parser.add_argument("--url", required=True, help="The run's page")
    report_parser.add_argument(
        "--log", required=True, help="The log of the run's 'ci-result' job"
    )
    args = parser.parse_args()

    if args.command == "labels":
        if not counts_rules(args.action, args.attempt):
            return 0
        files = [line.strip() for line in sys.stdin if line.strip()]
        for label in labels(rules(args.rules), files):
            print(label)
    elif args.command == "open":
        for gate, is_open in opened(sys.stdin).items():
            print(f"{gate}={'true' if is_open else 'false'}")
    elif args.command == "summary":
        print(summary(json.load(sys.stdin)))
    else:
        log = Path(args.log).read_text(errors="replace")
        body = report(json.load(sys.stdin), log, args.sha, args.attempt, args.url)
        if body is None:
            return 1
        print(body)

    return 0


if __name__ == "__main__":
    sys.exit(main())
