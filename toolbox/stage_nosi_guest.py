#!/usr/bin/env python3
"""
Stage a nosi guest disk-image for the xNVMe guest workflows.

nosi publishes its headless guest disk-images as OCI artifacts on ghcr. The nosi
container ships 'oras', so this just pulls the artifact with it, decompresses the
disk blob, and converts it to the qcow2 file that cijoe's 'qemu.guest_initialize'
expects at 'system-imaging.images.<image>.disk.path' -- guest_initialize then
copies it to the guest boot.img unchanged.

Usage:
    stage_nosi_guest.py <guest-config.toml> <system_imaging.toml>

The guest config provides a [guest_image] table:

    [guest_image]
    registry = "ghcr.io"            # optional, default ghcr.io
    repository = "safl/nosi/debian-13-headless"
    tag = "latest"                  # used when 'digest' is absent
    # digest = "sha256:..."         # optional pin (preferred over 'tag')

and either 'qemu.guests.<default_guest>.system_image_name' or
'qemu.default_systemimage' names the system-imaging entry whose 'disk.path' is
the destination.
"""
import gzip
import os
import subprocess
import sys
import tempfile
import time
import tomllib
from pathlib import Path


def expand_home(value):
    """Expand the cijoe '{{ local.env.HOME }}' template in a config string."""

    return value.replace("{{ local.env.HOME }}", os.environ["HOME"])


def disk_path(cfg, imaging):
    """Resolve the system-imaging disk.path destination for the guest."""

    qemu = cfg.get("qemu", {})
    default_guest = qemu.get("default_guest")
    image_name = qemu.get("guests", {}).get(default_guest, {}).get("system_image_name")
    image_name = image_name or qemu.get("default_systemimage")
    disk = imaging["system-imaging"]["images"][image_name]["disk"]
    return Path(expand_home(disk["path"]))


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <guest-config.toml> <system_imaging.toml>")
        return 1

    with open(sys.argv[1], "rb") as cfg_file:
        cfg = tomllib.load(cfg_file)
    with open(sys.argv[2], "rb") as imaging_file:
        imaging = tomllib.load(imaging_file)

    guest_image = cfg.get("guest_image")
    if not guest_image:
        print("config has no [guest_image] table; nothing to stage")
        return 0

    registry = guest_image.get("registry", "ghcr.io")
    repository = guest_image["repository"]
    digest = guest_image.get("digest")
    ref = f"@{digest}" if digest else f":{guest_image.get('tag', 'latest')}"
    image_ref = f"{registry}/{repository}{ref}"

    dst = disk_path(cfg, imaging)
    dst.parent.mkdir(parents=True, exist_ok=True)
    print(f"staging {image_ref} -> {dst}")

    with tempfile.TemporaryDirectory() as workdir:
        # Retry: parallel verify/docgen jobs pull this multi-GB blob anonymously
        # at the same time and ghcr intermittently throttles/resets the transfer.
        last = None
        for attempt in range(1, 6):
            try:
                subprocess.run(["oras", "pull", image_ref, "-o", workdir], check=True)
                last = None
                break
            except subprocess.CalledProcessError as exc:
                last = exc
                print(f"oras pull attempt {attempt}/5 failed; retrying")
                time.sleep(5 * attempt)
        if last is not None:
            raise last

        files = [path for path in Path(workdir).rglob("*") if path.is_file()]
        if not files:
            raise RuntimeError(f"oras pull produced no files for {image_ref}")
        blob = max(files, key=lambda path: path.stat().st_size)
        print(f"  pulled {blob.name} ({blob.stat().st_size} bytes)")
        # Drop the sidecar layers (.sha256, report, metadata) before decompressing
        # so we free their space and only carry the disk blob forward.
        for path in files:
            if path != blob:
                path.unlink(missing_ok=True)

        # nosi disk blobs are gzip-compressed raw images; decompress then convert.
        # Two disk-pressure mitigations on a GHA runner:
        #   * Write the raw sparsely (most of a fresh disk image is zeros), so
        #     the on-disk footprint is the used capacity, not the virtual size.
        #   * Delete the .gz immediately after decompression so the convert
        #     stage sees only raw + qcow2, not all three at once.
        if blob.suffix == ".gz":
            raw = Path(workdir) / blob.stem
            block_size = 1 << 20
            zero_block = b"\0" * block_size
            with gzip.open(blob, "rb") as src, open(raw, "wb") as out:
                while True:
                    buf = src.read(block_size)
                    if not buf:
                        break
                    if len(buf) == block_size and buf == zero_block:
                        out.seek(block_size, 1)
                    else:
                        out.write(buf)
                out.truncate()
            blob.unlink(missing_ok=True)
            convert_in = ["-f", "raw", str(raw)]
        else:
            raw = None
            convert_in = [str(blob)]

        subprocess.run(
            ["qemu-img", "convert", *convert_in, "-O", "qcow2", str(dst)],
            check=True,
        )
        if raw is not None:
            raw.unlink(missing_ok=True)

    # Headroom for the in-guest xNVMe build; cloud-init growpart grows the rootfs.
    subprocess.run(["qemu-img", "resize", str(dst), "+8G"], check=True)
    subprocess.run(["qemu-img", "info", str(dst)], check=True)

    return 0


if __name__ == "__main__":
    sys.exit(main())
