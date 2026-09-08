#!/usr/bin/env python3
"""
Run xnvmeperf over a set of shapes, in one or more modes, and record it
=======================================================================

A shape says which of the configured devices take part and which CPUs drive
them. The simple form is a slice of the device list and a CPU list::

    [[perf.shapes]]
    name = "four-on-one-core"
    ndevs = 4
    cpulist = "1,3"        # two hyperthreads of one core, per hwloc-calc
    taskset = "1"          # optional: CPU the launch is pinned to

``taskset`` pins the thread that opens the devices and allocates the queues,
so their memory lands on that CPU's NUMA node. A shape can also run several
such groups at once, one process each, for a two-socket host where a single
process cannot be local to both::

    [[perf.shapes]]
    name = "one-process-per-node"
    groups = [
      { offset = 0, ndevs = 4, cpulist = "1,3", taskset = "1" },
      { offset = 4, ndevs = 4, cpulist = "2,4", taskset = "2" },
    ]

A mode is one way of driving the devices. Currently:

* direct: CPU-initiated I/O into host memory, one process opening the
  devices itself (``xnvmeperf run ... --be upcie``).
* served: the same I/O, but the devices are held open by a ``homi`` server
  started for the mode on every configured device, and xnvmeperf runs as its
  client (``xnvmeperf run ... --be upcie --homi-id N``).

Every repetition writes one JSON file under <output>/perf/ with, per group,
the per-device rows, the total, the version line from the help footer of the
xnvmeperf that ran, the title line of its results block, and a
``numastat -p`` sample taken halfway through, so where the memory was is
recorded rather than assumed. A summary table goes to
<output>/artifacts/perf_summary.md.

Config Arguments
----------------

perf.shapes: list of tables as above; from the devices config, since both
             follow from the host's topology
perf.xnvmeperf: the xnvmeperf to run (default: xnvmeperf from PATH); a path
                into a build tree compares one build against another on the
                same host, since a build-tree binary finds its own libxnvme
perf.modes: list of modes to run (default: ["direct"])
perf.be: backend for both modes (default: upcie)
perf.homi_id: identifier the served mode's server shares under (default: 42)
perf.iosize, perf.qdepth, perf.runtime, perf.iopattern: passed through
perf.repetitions: runs per shape and mode (default: 3)
perf.retries: how many times a repetition whose xnvmeperf exits non-zero is
              run again before the step fails (default: 0); the attempts are
              recorded with the result, so a flaky abort is visible, not hidden
devices: list of tables with 'pci_addr' (the BDF), from the devices config

Retargetable: True
------------------
"""
import json
import logging as log
import re
from pathlib import Path

TITLE_RE = re.compile(
    r"^\s*xnvmeperf\S*\s+(?P<title>.*)\(elapsed:\s*(?P<elapsed>[\d.]+)s\)"
)
ROW_RE = re.compile(
    r"^\s*(?P<name>\S+)\s+(?:(?P<cpus>[\d,\-]+)\s+)?"
    r"(?P<iops>[\d.]+)\s+(?P<mibps>[\d.]+)\s+(?P<failed>\d+)\s*$"
)
GROUP_MARK = "== perf group "


def parse_results(output):
    """Return (title, rows, total) from one xnvmeperf results block."""

    title, rows, total = None, [], None
    for line in output.splitlines():
        m = TITLE_RE.match(line)
        if m:
            title = m.group("title").strip()
            continue
        m = ROW_RE.match(line)
        if not m:
            continue
        row = {
            "iops": float(m.group("iops")),
            "mibps": float(m.group("mibps")),
            "failed": int(m.group("failed")),
        }
        if m.group("name") == "Total:":
            total = row
        else:
            row["device"] = m.group("name")
            row["cpus"] = m.group("cpus")
            rows.append(row)
    return title, rows, total


def groups_of(shape):
    """The groups a shape consists of; a shape without 'groups' is one group."""

    if "groups" in shape:
        return shape["groups"]
    return [
        {k: shape[k] for k in ("offset", "ndevs", "cpulist", "taskset") if k in shape}
    ]


def xnvmeperf_command(mode, group, bdfs, cijoe):
    """The invocation for one group in 'mode', or None when the mode is unknown."""

    offset = int(group.get("offset", 0))
    devices = " ".join(bdfs[offset : offset + int(group["ndevs"])])
    options = (
        f" --iopattern {cijoe.getconf('perf.iopattern', 'randread')}"
        f" --iosize {cijoe.getconf('perf.iosize', 512)}"
        f" --qdepth {cijoe.getconf('perf.qdepth', 128)}"
        f" --runtime {cijoe.getconf('perf.runtime', 10)}"
        f" --cpulist {group['cpulist']}"
    )
    xnvmeperf = cijoe.getconf("perf.xnvmeperf", "xnvmeperf")
    if mode == "served":
        options += f" --homi-id {cijoe.getconf('perf.homi_id', 42)}"
    if mode in ("direct", "served"):
        command = f"{xnvmeperf} run {devices} --be {cijoe.getconf('perf.be', 'upcie')}{options}"
    else:
        return None
    if group.get("taskset"):
        command = f"taskset -c {group['taskset']} {command}"
    return command


def run_groups(cijoe, commands, runtime):
    """
    Run the group commands at once and return their outputs, plus a numastat
    sample of the xnvmeperf processes taken halfway through the run
    """

    # Leave a core behind when xnvmeperf aborts; it lands where the shell runs
    lines = ["ulimit -c unlimited", "out=$(mktemp -d)"]
    for idx, command in enumerate(commands):
        lines.append(f"{command} > $out/{idx} 2>&1 &")
    lines.append(
        f"sleep {max(1, runtime // 2)}; numastat -p xnvmeperf > $out/numastat 2>&1"
    )
    lines.append("wait")
    for idx in range(len(commands)):
        lines.append(f'echo "{GROUP_MARK}{idx}"; cat $out/{idx}')
    lines.append(f'echo "{GROUP_MARK}numastat"; cat $out/numastat; rm -rf $out')

    err, state = cijoe.run("\n".join(lines))
    if err:
        return err, None, None

    parts = state.output().split(GROUP_MARK)[1:]
    outputs = {}
    for part in parts:
        key, _, body = part.partition("\n")
        outputs[key.strip()] = body
    numastat = outputs.pop("numastat", "")
    return 0, [outputs.get(str(idx), "") for idx in range(len(commands))], numastat


def mode_enter(mode, bdfs, cijoe):
    """Start what a mode needs on the host; for 'served', a homi holding every device."""

    if mode != "served":
        return 0
    homi_id = cijoe.getconf("perf.homi_id", 42)
    be = cijoe.getconf("perf.be", "upcie")
    err, _ = cijoe.run(
        f"nohup homi start {' '.join(bdfs)} --be {be} --homi-id {homi_id}"
        f" > /var/tmp/homi-{homi_id}.log 2>&1 < /dev/null &"
    )
    if err:
        log.error(f"starting homi failed: err({err})")
        return err
    # status exits non-zero until the server is up and its devices are ready
    err, _ = cijoe.run(
        f"for i in $(seq 1 120); do homi status --homi-id {homi_id} > /dev/null 2>&1 && exit 0;"
        " sleep 1; done; exit 1"
    )
    if err:
        log.error(f"homi did not come up; see /var/tmp/homi-{homi_id}.log on the host")
        cijoe.run(f"cat /var/tmp/homi-{homi_id}.log")
        return err
    cijoe.run(f"homi status --homi-id {homi_id}")
    return 0


def mode_leave(mode, cijoe):
    """Stop what mode_enter started."""

    if mode != "served":
        return
    cijoe.run("pkill -INT -x homi")
    cijoe.run(
        "for i in $(seq 1 30); do pgrep -x homi > /dev/null || exit 0; sleep 1; done; pkill -x homi"
    )


def main(args, cijoe):
    devices = cijoe.getconf("devices", [])
    bdfs = [d["pci_addr"] for d in devices if "pci_addr" in d]
    if not bdfs:
        log.error("no devices with 'pci_addr' in config")
        return 1

    shapes = cijoe.getconf("perf.shapes", [])
    if not shapes:
        log.error("no perf.shapes in config; see the docstring for what one looks like")
        return 1
    for shape in shapes:
        for group in groups_of(shape):
            if "ndevs" not in group or "cpulist" not in group:
                log.error(
                    f"shape '{shape.get('name')}': a group needs 'ndevs' and 'cpulist'"
                )
                return 1
            if int(group.get("offset", 0)) + int(group["ndevs"]) > len(bdfs):
                log.error(
                    f"shape '{shape.get('name')}': a group reaches past the {len(bdfs)} devices"
                )
                return 1

    modes = cijoe.getconf("perf.modes", ["direct"])
    repetitions = int(cijoe.getconf("perf.repetitions", 3))
    retries = int(cijoe.getconf("perf.retries", 0))
    runtime = int(cijoe.getconf("perf.runtime", 10))
    results_dir = Path(args.output) / "perf"
    results_dir.mkdir(parents=True, exist_ok=True)

    # The help footer has carried the version line on every build; --version
    # only on recent ones
    err, state = cijoe.run(f"{cijoe.getconf('perf.xnvmeperf', 'xnvmeperf')} --help")
    footer = [line for line in state.output().splitlines() if "ver:" in line]
    version = footer[-1].strip() if not err and footer else "(unknown)"
    log.info(version)

    summary = []
    for mode in modes:
        err = mode_enter(mode, bdfs, cijoe)
        if err:
            return err
        for shape in shapes:
            name = shape["name"]
            groups = groups_of(shape)
            commands = [xnvmeperf_command(mode, g, bdfs, cijoe) for g in groups]
            if None in commands:
                log.error(f"unknown mode '{mode}'")
                mode_leave(mode, cijoe)
                return 1
            for rep in range(repetitions):
                label = f"{name} {mode} rep {rep}"
                # An aborted xnvmeperf leaves the shell's exit status at zero,
                # so a block that does not parse counts as a failed attempt too
                for attempt in range(1, retries + 2):
                    err, outputs, numastat = run_groups(cijoe, commands, runtime)
                    if err:
                        log.warning(
                            f"{label}: xnvmeperf failed on attempt {attempt}: err({err})"
                        )
                        continue
                    results = []
                    for group, command, output in zip(groups, commands, outputs):
                        title, rows, total = parse_results(output)
                        if total is None or len(rows) != int(group["ndevs"]):
                            log.warning(
                                f"{label}: no results block on attempt {attempt}:\n{output}"
                            )
                            err = 1
                            break
                        results.append(
                            {
                                "group": group,
                                "command": command,
                                "title": title,
                                "devices": rows,
                                "total": total,
                            }
                        )
                    if not err:
                        break
                if err:
                    log.error(f"{label}: xnvmeperf failed {attempt} time(s)")
                    mode_leave(mode, cijoe)
                    return err
                failed_ios = sum(r["total"]["failed"] for r in results)
                if failed_ios:
                    log.error(f"{label}: {failed_ios} failed I/Os")
                    mode_leave(mode, cijoe)
                    return 1

                iops = sum(r["total"]["iops"] for r in results)
                mibps = sum(r["total"]["mibps"] for r in results)
                result = {
                    "shape": name,
                    "mode": mode,
                    "rep": rep,
                    "attempts": attempt,
                    "version": version,
                    "groups": results,
                    "iops": iops,
                    "mibps": mibps,
                    "numastat": numastat,
                }
                path = results_dir / f"{name}-{mode}-rep{rep}.json"
                path.write_text(json.dumps(result, indent=2))
                ndevs = sum(int(g["ndevs"]) for g in groups)
                cpus = " + ".join(g["cpulist"] for g in groups)
                summary.append((name, ndevs, cpus, mode, rep, iops, mibps))
                log.info(f"{label}: {iops:.0f} IOPS, {mibps:.0f} MiB/s")
        mode_leave(mode, cijoe)

    artifacts = Path(args.output) / "artifacts"
    artifacts.mkdir(parents=True, exist_ok=True)
    lines = [
        f"# xnvmeperf: {version}",
        "",
        "| shape | devices | cpulist | mode | rep | IOPS | MiB/s |",
        "|-------|--------:|---------|------|----:|-----:|------:|",
    ]
    lines += [
        f"| {n} | {d} | {c} | {m} | {r} | {i:.0f} | {b:.0f} |"
        for n, d, c, m, r, i, b in summary
    ]
    (artifacts / "perf_summary.md").write_text("\n".join(lines) + "\n")
    return 0
