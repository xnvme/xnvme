#!/usr/bin/env python3
"""
Check QEMU development environment readiness for xNVMe guest workflows.

Checks:
- qemu-system-x86_64 binary available
- /dev/kvm accessible
"""
import os
import shutil
import subprocess
import sys


def is_custom_qemu(qemu_bin):
    """Check if the QEMU binary is the custom xNVMe fork (supports KVS, mdts=0)."""

    try:
        result = subprocess.run(
            [qemu_bin, "-device", "nvme-ns,help"],
            capture_output=True,
            text=True,
            timeout=5,
        )
        return "kv=" in (result.stdout + result.stderr)
    except (subprocess.TimeoutExpired, FileNotFoundError, OSError):
        return False


def main():
    errors = []

    print("Checking environment...")

    # Check qemu-system-x86_64
    qemu = shutil.which("qemu-system-x86_64")
    if qemu:
        if is_custom_qemu(qemu):
            print(f"  qemu-system-x86_64: OK (xNVMe fork at {qemu})")
        else:
            print(f"  qemu-system-x86_64: WRONG BUILD ({qemu})")
            errors.append(
                "  Found qemu-system-x86_64 but it is upstream QEMU, not the xNVMe fork.\n"
                "  The xNVMe fork is required for KVS emulation and mdts=0 support.\n"
                "  Either:\n"
                "    a) Run inside the xNVMe Docker environment: make docker\n"
                "    b) Build the xNVMe QEMU fork: https://github.com/SamsungDS/qemu.git (branch: for-xnvme)"
            )
    else:
        print("  qemu-system-x86_64: MISSING")
        errors.append(
            "  qemu-system-x86_64 not found.\n"
            "  Either:\n"
            "    a) Run inside the xNVMe Docker environment: make docker\n"
            "    b) Build the xNVMe QEMU fork: https://github.com/SamsungDS/qemu.git (branch: for-xnvme)"
        )

    # Check KVM — not fatal, QEMU works without it (just slower)
    if os.path.exists("/dev/kvm"):
        if os.access("/dev/kvm", os.R_OK | os.W_OK):
            print("  /dev/kvm: OK")
        else:
            print("  /dev/kvm: NO ACCESS (guests will run without KVM acceleration)")
            print("    Fix: sudo usermod -aG kvm $USER")
    else:
        print("  /dev/kvm: MISSING (guests will run without KVM acceleration)")
        print(
            "    Fix: enable KVM in your kernel / hypervisor (e.g. nested virtualization)"
        )

    if errors:
        print()
        for err in errors:
            print(err)
            print()
        sys.exit(1)

    print()


if __name__ == "__main__":
    main()
