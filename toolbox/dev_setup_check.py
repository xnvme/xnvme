#!/usr/bin/env python3
"""
Check QEMU development environment readiness for xNVMe guest workflows.

Checks:
- qemu-system-x86_64 binary available
- it can provide the NVMe KV command set: either natively (the SamsungDS fork,
  'nvme-ns,kv=on') or via an external vfu_kvssd device over vfio-user (upstream
  QEMU >= 10.1, 'vfio-user-pci')
- /dev/kvm accessible
"""
import os
import shutil
import subprocess
import sys


def qemu_has(qemu_bin, args, needle):
    """Return True when 'needle' appears in the output of 'qemu_bin args'."""

    try:
        result = subprocess.run(
            [qemu_bin, *args],
            capture_output=True,
            text=True,
            timeout=5,
        )
        return needle in (result.stdout + result.stderr)
    except (subprocess.TimeoutExpired, FileNotFoundError, OSError):
        return False


def main():
    errors = []

    print("Checking environment...")

    # Check qemu-system-x86_64
    qemu = shutil.which("qemu-system-x86_64")
    if qemu:
        if qemu_has(qemu, ["-device", "nvme-ns,help"], "kv="):
            print(f"  qemu-system-x86_64: OK (native KV, xNVMe fork at {qemu})")
        elif qemu_has(qemu, ["-device", "help"], "vfio-user-pci"):
            print(f"  qemu-system-x86_64: OK (upstream + vfio-user at {qemu})")
        else:
            print(f"  qemu-system-x86_64: NO KV SUPPORT ({qemu})")
            errors.append(
                "  Found qemu-system-x86_64 but it can neither emulate NVMe KV\n"
                "  natively (no 'nvme-ns,kv=on') nor attach an external vfu_kvssd\n"
                "  device (no 'vfio-user-pci'; needs upstream QEMU >= 10.1).\n"
                "  Either:\n"
                "    a) Run inside the nosi Docker environment (QEMU >= 10.1)\n"
                "    b) Build the xNVMe QEMU fork: https://github.com/SamsungDS/qemu.git (branch: for-xnvme)"
            )
    else:
        print("  qemu-system-x86_64: MISSING")
        errors.append(
            "  qemu-system-x86_64 not found.\n"
            "  Either:\n"
            "    a) Run inside the nosi Docker environment (QEMU >= 10.1)\n"
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
