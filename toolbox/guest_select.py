#!/usr/bin/env python3
"""
Interactive guest selection for xNVMe CIJOE workflows.

Discovers available guests by scanning cijoe/workflows/test-*.yaml and
verifying a matching cijoe/configs/{guest}.toml exists. Presents an
interactive numbered menu on TTY, reads from stdin when piped.

Writes the selected guest name to cijoe/current.guest.
"""
import glob
import os
import sys


def discover_guests(cijoe_dir):
    """Find guests that have both a test workflow and a config file."""

    workflow_pattern = os.path.join(cijoe_dir, "workflows", "test-*.yaml")
    configs_dir = os.path.join(cijoe_dir, "configs")

    guests = []
    for wf in sorted(glob.glob(workflow_pattern)):
        basename = os.path.basename(wf)
        # test-debian-trixie.yaml -> debian-trixie
        guest = basename.removeprefix("test-").removesuffix(".yaml")

        # Skip ramdisk entries — they aren't qemu guests
        if "ramdisk" in guest:
            continue

        config_path = os.path.join(configs_dir, f"{guest}.toml")
        if os.path.isfile(config_path):
            guests.append(guest)

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
        # Piped / non-interactive mode
        selected = sys.stdin.readline().strip()
        if selected not in guests:
            print(f"ERROR: Unknown guest '{selected}'", file=sys.stderr)
            print(f"Available: {', '.join(guests)}", file=sys.stderr)
            sys.exit(1)

    with open(guest_file, "w") as f:
        f.write(selected + "\n")

    print(f"Selected: {selected}")


if __name__ == "__main__":
    main()
