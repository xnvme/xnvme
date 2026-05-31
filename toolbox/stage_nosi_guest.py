#!/usr/bin/env python3
"""
Stage a nosi guest disk-image for the xNVMe guest workflows.

The guest config carries a direct URL to the .img.gz blob:

    [guest_image]
    url = "https://ghcr.io/v2/safl/nosi/debian-13-headless/blobs/sha256:<layer-digest>"

The URL is fetched with stdlib urllib -- no oras CLI required. If the host is
ghcr.io we mint an anonymous pull-scoped bearer token first (public blobs still
require auth) and send it as Authorization; for other hosts the request goes
straight through. The blob is assumed to be gzip-compressed (nosi convention),
decompressed sparsely, and converted to the qcow2 destination given by
'system-imaging.images.<image>.disk.path' -- guest_initialize then copies that
qcow2 to the guest boot.img unchanged.

Usage:
    stage_nosi_guest.py <guest-config.toml> <system_imaging.toml>
"""
import gzip
import json
import os
import subprocess
import sys
import tempfile
import time
import tomllib
import urllib.error
import urllib.parse
import urllib.request
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


def ghcr_anon_auth(url):
    """Mint an anonymous pull-scoped bearer header for ghcr.io blob URLs.

    Returns {} for non-ghcr URLs (handled by the caller as 'no auth needed').
    Public blobs on ghcr still 401 without a token; the token endpoint itself
    is unauthenticated. Scope is derived from the URL's repository segment.
    """

    parsed = urllib.parse.urlparse(url)
    if parsed.hostname != "ghcr.io":
        return {}
    # Path is "/v2/<repository>/blobs/<digest>"; repository may have slashes.
    parts = parsed.path.split("/")
    blob_idx = parts.index("blobs")
    repository = "/".join(parts[2:blob_idx])
    token_url = (
        f"https://ghcr.io/token?service=ghcr.io&scope=repository:{repository}:pull"
    )
    with urllib.request.urlopen(token_url) as resp:
        token = json.load(resp)["token"]
    return {"Authorization": f"Bearer {token}"}


def fetch_blob(url, dst_path):
    """Stream a URL to dst_path, with anonymous ghcr auth when needed."""

    headers = ghcr_anon_auth(url)
    req = urllib.request.Request(url, headers=headers)
    with urllib.request.urlopen(req) as resp, open(dst_path, "wb") as out:
        while True:
            chunk = resp.read(1 << 20)
            if not chunk:
                break
            out.write(chunk)


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
    url = guest_image.get("url")
    if not url:
        print("config has no [guest_image].url; nothing to stage")
        return 0

    dst = disk_path(cfg, imaging)
    dst.parent.mkdir(parents=True, exist_ok=True)
    print(f"staging {url} -> {dst}")

    with tempfile.TemporaryDirectory() as workdir:
        blob = Path(workdir) / "guest.img.gz"

        # Retry: parallel verify/docgen jobs pull this multi-GB blob anonymously
        # at the same time and ghcr intermittently throttles/resets the transfer.
        last = None
        for attempt in range(1, 6):
            try:
                fetch_blob(url, blob)
                last = None
                break
            except (urllib.error.URLError, OSError) as exc:
                last = exc
                print(f"download attempt {attempt}/5 failed ({exc}); retrying")
                time.sleep(5 * attempt)
        if last is not None:
            raise last

        print(f"  pulled {blob.stat().st_size} bytes")

        # nosi disk blobs are gzip-compressed raw images; decompress then convert.
        # Two disk-pressure mitigations on a GHA runner:
        #   * Write the raw sparsely (most of a fresh disk image is zeros), so
        #     the on-disk footprint is the used capacity, not the virtual size.
        #   * Delete the .gz immediately after decompression so the convert
        #     stage sees only raw + qcow2, not all three at once.
        raw = Path(workdir) / "guest.img"
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

        subprocess.run(
            ["qemu-img", "convert", "-f", "raw", str(raw), "-O", "qcow2", str(dst)],
            check=True,
        )
        raw.unlink(missing_ok=True)

    # Headroom for the in-guest xNVMe build; cloud-init growpart grows the rootfs.
    subprocess.run(["qemu-img", "resize", str(dst), "+8G"], check=True)
    subprocess.run(["qemu-img", "info", str(dst)], check=True)

    return 0


if __name__ == "__main__":
    sys.exit(main())
