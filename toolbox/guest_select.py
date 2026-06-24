#!/usr/bin/env python3
"""
Interactive guest selection for xNVMe CIJOE workflows.

Discovers available guests by scanning cijoe/workflows/test-*.yaml and
verifying a matching cijoe/configs/{guest}.toml exists. Presents an
interactive numbered menu on TTY, reads from stdin when piped.

Workflow names carry the transport as the trailing segment, e.g.
'test-debian-trixie-iommu_enabled-pcie.yaml' targets the guest
'debian-trixie-iommu_enabled'. The trailing transport segment is dropped
to find the matching config, and multiple workflows backed by the same
config are deduped so each guest appears once.

Writes the selected guest name to cijoe/current.guest.
"""
import glob
import os
import sys

KNOWN_TRANSPORTS = ("pcie", "tcp", "rdma")
KNOWN_PROVIDERS = ("spdk", "linux")


def _workflow_to_guest(stem):
    """Strip the trailing transport and optional provider segment."""

    parts = stem.rsplit("-", 1)
    if len(parts) == 2 and parts[1] in KNOWN_PROVIDERS:
        stem = parts[0]
    parts = stem.rsplit("-", 1)
    if len(parts) == 2 and parts[1] in KNOWN_TRANSPORTS:
        return parts[0]
    return stem


def discover_guests(cijoe_dir):
    """Find guests that have both a test workflow and a config file."""

    workflow_pattern = os.path.join(cijoe_dir, "workflows", "test-*.yaml")
    configs_dir = os.path.join(cijoe_dir, "configs")

    seen = set()
    guests = []
    for wf in sorted(glob.glob(workflow_pattern)):
        basename = os.path.basename(wf)
        stem = basename.removeprefix("test-").removesuffix(".yaml")

        # Skip ramdisk entries — they aren't qemu guests
        if "ramdisk" in stem:
            continue

        guest = _workflow_to_guest(stem)
        if guest in seen:
            continue

        config_path = os.path.join(configs_dir, f"{guest}.toml")
        if os.path.isfile(config_path):
            guests.append(guest)
            seen.add(guest)

    return guests


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <cijoe-dir>", file=sys.stderr)
        sys.exit(1)

    cijoe_dir = sys.argv[1]
    guests = discover_guests(cijoe_dir)

    if not guests:
        print(
            "ERROR: No guests found. Check cijoe/workflows/test-*.yaml and configs/.",
            file=sys.stderr,
        )
        sys.exit(1)

    guest_file = os.path.join(cijoe_dir, "current.guest")

    if sys.stdin.isatty():
        # Interactive mode
        print("Available guests:")
        for i, guest in enumerate(guests, 1):
            print(f"  {i}) {guest}")
        print()

        while True:
            try:
                choice = input("Select guest: ")
            except (EOFError, KeyboardInterrupt):
                print()
                sys.exit(1)

            try:
                idx = int(choice)
                if 1 <= idx <= len(guests):
                    selected = guests[idx - 1]
                    break
                print(f"Please enter a number between 1 and {len(guests)}")
            except ValueError:
                # Allow typing the guest name directly
                if choice.strip() in guests:
                    selected = choice.strip()
                    break
                print(f"Invalid choice: {choice}")
    else:
        # Piped / non-interactive mode. Accept either a canonical guest name
        # or a workflow-stem-style input (e.g. 'debian-trixie-iommu_enabled-tcp-spdk');
        # the trailing transport segment is stripped to find the canonical guest.
        selected = sys.stdin.readline().strip()
        if selected not in guests:
            stripped = _workflow_to_guest(selected)
            if stripped in guests:
                selected = stripped
            else:
                print(f"ERROR: Unknown guest '{selected}'", file=sys.stderr)
                print(f"Available: {', '.join(guests)}", file=sys.stderr)
                sys.exit(1)

    with open(guest_file, "w") as f:
        f.write(selected + "\n")

    print(f"Selected: {selected}")


if __name__ == "__main__":
    main()
