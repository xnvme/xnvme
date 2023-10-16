"""
Linux Custom Kernel as Debian Package
=====================================

There are a myriad of ways to build and install a custom Linux kernel. This
worklet builds it as a Debian package. The generated .deb packages are stored
in cijoe.output_path.

Retargetable: False
-------------------

It is intended to be run "locally" since, currently the collection of the generated
.debs are not retrieved via cijoe.get(), doing so would make it retargetable.

Worklet arguments
-----------------

with.localversion
"""
from pathlib import Path


def main(args, cijoe, step):
    """Configure, build and collect the build-artifacts"""

    repos = Path(cijoe.config.options["linux"]["repository"]["path"]).resolve()
    err, _ = cijoe.run(f"[ -d {repos} ]")
    if err:
        return err

    localversion = step.get("with", {}).get("localversion", "custom")
    run_local = step.get("with", {}).get("run_local", True)
    run = cijoe.run_local if run_local else cijoe.run

    commands = [
        "[ -f .config ] && rm .config || true",
        "yes " " | make olddefconfig",
        "./scripts/config --disable CONFIG_DEBUG_INFO",
        "./scripts/config --disable SYSTEM_TRUSTED_KEYS",
        "./scripts/config --disable SYSTEM_REVOCATION_KEYS",
        "./scripts/config --disable CONFIG_BLK_CGROUP",
        "./scripts/config --disable CONFIG_BLK_WBT_MQ",
        "./scripts/config --disable CONFIG_RETPOLINE",
        "./scripts/config --disable CONFIG_PAGE_TABLE_ISOLATION",
        "./scripts/config --enable CONFIG_HZ_1000"
        "./scripts/config --set-val 1000 CONFIG_HZ",
        f"yes '' | make -j$(nproc) bindeb-pkg LOCALVERSION={localversion}",
        f"mkdir -p {cijoe.output_path}/artifacts/linux",
        f"mv ../*.deb {cijoe.output_path}/artifacts/linux",
        f"mv ../*.changes {cijoe.output_path}/artifacts/linux",
        f"mv ../*.buildinfo {cijoe.output_path}/artifacts/linux",
    ]
    for cmd in commands:
        err, _ = run(cmd, cwd=str(repos))
        if err:
            return err

    return 0
