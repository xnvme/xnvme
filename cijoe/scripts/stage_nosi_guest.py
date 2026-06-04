#!/usr/bin/env python3
"""
Stage a nosi guest disk-image
=============================

Pulls ``[guest_image].url`` (gzip blob, fetched with anonymous ghcr auth when
applicable), decompresses sparsely, converts to qcow2, and resizes; lands it at
``system-imaging.images.<image>.disk.path`` where ``guest_initialize`` finds it.
No-op when ``[guest_image]`` is absent.

Retargetable: False (writes to the cijoe initiator's local filesystem).
"""
import errno
import gzip
import json
import logging as log
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request
from argparse import ArgumentParser
from pathlib import Path


def add_args(parser: ArgumentParser):
    pass


def ghcr_anon_auth(url):
    """Mint an anonymous pull-scoped bearer header for ghcr.io blob URLs.

    Returns {} for non-ghcr URLs. Public ghcr blobs still 401 without a token.
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


def main(args, cijoe):
    url = cijoe.getconf("guest_image.url", None)
    if not url:
        log.info("config has no [guest_image].url; nothing to stage")
        return 0

    # Resolve the system-imaging disk.path that guest_initialize copies to boot.img.
    guest_name = cijoe.getconf("qemu.default_guest", None)
    image_name = (
        cijoe.getconf(f"qemu.guests.{guest_name}.system_image_name", None)
        if guest_name
        else None
    )
    image_name = image_name or cijoe.getconf("qemu.default_systemimage", None)
    if not image_name:
        log.error("qemu.default_systemimage is not set; cannot resolve disk.path")
        return errno.EINVAL

    disk = cijoe.getconf(f"system-imaging.images.{image_name}.disk", None)
    if not disk:
        log.error(f"system-imaging.images.{image_name}.disk is not set")
        return errno.EINVAL

    dst = Path(disk["path"])
    dst.parent.mkdir(parents=True, exist_ok=True)
    log.info(f"staging {url} -> {dst}")

    # Workdir on dst's filesystem: the container's /tmp overlay is too small for
    # the multi-GB raw decompression; dst.parent is the workflow-staged /mnt.
    with tempfile.TemporaryDirectory(dir=dst.parent) as workdir:
        blob = Path(workdir) / "guest.img.gz"

        # Retry on transient ghcr throttle/reset. Truncated bodies don't raise
        # from urllib but surface in gzip as EOFError, so fetch and decompress
        # share the loop. Sparse decompression: most blocks are zero.
        raw = Path(workdir) / "guest.img"
        block_size = 1 << 20
        zero_block = b"\0" * block_size
        last = None
        for attempt in range(1, 6):
            try:
                fetch_blob(url, blob)
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
                last = None
                break
            except (urllib.error.URLError, OSError, EOFError) as exc:
                last = exc
                log.warning(f"download attempt {attempt}/5 failed ({exc}); retrying")
                time.sleep(5 * attempt)
        if last is not None:
            raise last

        log.info(f"pulled {blob.stat().st_size} bytes")
        # Drop the .gz immediately so convert sees only raw + qcow2.
        blob.unlink(missing_ok=True)

        err, _ = cijoe.run_local(f"qemu-img convert -f raw {raw} -O qcow2 {dst}")
        raw.unlink(missing_ok=True)
        if err:
            log.error(f"qemu-img convert failed: err({err})")
            return err

    # Headroom for the in-guest xNVMe build; cloud-init growpart grows the rootfs.
    err, _ = cijoe.run_local(f"qemu-img resize {dst} +8G")
    if err:
        return err
    err, _ = cijoe.run_local(f"qemu-img info {dst}")
    return err
