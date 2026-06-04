#!/usr/bin/env python3
"""
Inventory the local QEMU environment against what xNVMe's guest workflows need:

- qemu-system-x86_64 >= 10.1 (for the vfio-user-pci device transport)
- the vfio-user-pci device, used to attach the external vfu_kvssd binary
  (https://github.com/safl/vfio-user-kvssd) that supplies the NVMe KV
  command set
- /dev/kvm for guest acceleration (optional; guests still run without it)

Reports detected versions and capabilities. Exits non-zero only when a hard
requirement is missing.
"""
import os
import re
import shutil
import subprocess
import sys

MIN_QEMU_VERSION = (10, 1)


def qemu_version(qemu_bin):
    """Return (major, minor) parsed from `qemu_bin --version`, or None."""

    try:
        result = subprocess.run(
            [qemu_bin, "--version"], capture_output=True, text=True, timeout=5
        )
    except (subprocess.TimeoutExpired, FileNotFoundError, OSError):
        return None
    match = re.search(r"QEMU emulator version (\d+)\.(\d+)", result.stdout)
    return (int(match.group(1)), int(match.group(2))) if match else None


def qemu_has(qemu_bin, args, needle):
    """Return True when 'needle' appears in the output of 'qemu_bin args'."""

    try:
        result = subprocess.run(
            [qemu_bin, *args], capture_output=True, text=True, timeout=5
        )
        return needle in (result.stdout + result.stderr)
    except (subprocess.TimeoutExpired, FileNotFoundError, OSError):
        return False


def main():
    errors = []

    print("Checking QEMU environment for xNVMe guest workflows...")
    print()

    qemu = shutil.which("qemu-system-x86_64")
    if not qemu:
        print("qemu-system-x86_64: MISSING")
        errors.append(
            "qemu-system-x86_64 not found. Need upstream QEMU >= %d.%d.\n"
            "  a) Run inside the nosi Docker environment: make docker\n"
            "  b) Build upstream QEMU from source" % MIN_QEMU_VERSION
        )
    else:
        version = qemu_version(qemu)
        version_str = "%d.%d" % version if version else "unknown"
        print(f"qemu-system-x86_64: {qemu}")
        print(f"  version: {version_str} (need >= %d.%d)" % MIN_QEMU_VERSION)
        if version and version < MIN_QEMU_VERSION:
            errors.append(
                f"QEMU {version_str} is older than the required "
                "%d.%d." % MIN_QEMU_VERSION
            )

        has_vfio_user = qemu_has(qemu, ["-device", "help"], "vfio-user-pci")
        print(f"  vfio-user-pci device: {'yes' if has_vfio_user else 'NO'}")
        if not has_vfio_user:
            errors.append(
                "QEMU is missing vfio-user-pci. xNVMe attaches the external "
                "vfu_kvssd device over vfio-user to supply NVMe KV."
            )

    print()
    if os.path.exists("/dev/kvm"):
        if os.access("/dev/kvm", os.R_OK | os.W_OK):
            print("/dev/kvm: OK (guests will run with KVM acceleration)")
        else:
            print("/dev/kvm: NO ACCESS (guests will run without KVM acceleration)")
            print("  fix: sudo usermod -aG kvm $USER")
    else:
        print("/dev/kvm: missing (guests will run without KVM acceleration)")

    print()
    print("vfu_kvssd: fetched at run time from")
    print("  https://github.com/safl/vfio-user-kvssd/releases")
    print("  when a cijoe config defines a [kvssd] table.")

    if errors:
        print()
        for err in errors:
            print(err)
            print()
        sys.exit(1)


if __name__ == "__main__":
    main()
