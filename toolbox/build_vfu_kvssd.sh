#!/usr/bin/env bash
#
# TEMPORARY: build the vfu_kvssd vfio-user KV device from source for CI.
#
# The released binary lags the device source while the NVMe emulation is being
# stabilised (FLR/reset handling, tracing, SPDK-probe fixes). Building from a
# pinned commit lets xNVMe CI iterate on the device without cutting a release
# each time. Once the device is stable, drop this and pin [kvssd].url to a
# release instead.
#
# Usage: build_vfu_kvssd.sh [dest-binary-path]
#   dest defaults to $HOME/guests/vfu_kvssd, matching the [kvssd].binary path in
#   the debian-trixie cijoe configs, so xnvme_guest_start_nvme.py finds it and
#   skips the release download.
#
set -euo pipefail

DEST="${1:-$HOME/guests/vfu_kvssd}"
REPO="${VFU_KVSSD_REPO:-https://github.com/safl/vfio-user-kvssd}"
REF="${VFU_KVSSD_REF:-52cb0e1c578252c704bcc84eb1d05483578acb67}"
ZIG_VERSION="${ZIG_VERSION:-0.16.0}"

# The nosi image ships an older zig; build.zig needs the 0.16 module API.
# Install 0.16 to /opt/zig when the available zig is missing or wrong-series.
zig_bin="$(command -v zig || true)"
if [ -z "$zig_bin" ] || ! zig version 2>/dev/null | grep -q "^${ZIG_VERSION%.*}\."; then
	curl -fsSL \
		"https://ziglang.org/download/${ZIG_VERSION}/zig-x86_64-linux-${ZIG_VERSION}.tar.xz" \
		-o /tmp/zig.tar.xz
	mkdir -p /opt/zig
	tar -C /opt/zig --strip-components=1 -xf /tmp/zig.tar.xz
	zig_bin=/opt/zig/zig
fi

# Fetch the pinned commit (shallow) with its submodules (libvfio-user, json-c).
src=/tmp/vfio-user-kvssd
rm -rf "$src"
git init -q "$src"
git -C "$src" remote add origin "$REPO"
git -C "$src" fetch -q --depth 1 origin "$REF"
git -C "$src" checkout -q FETCH_HEAD
git -C "$src" submodule update --init --recursive

# Native build (runs in the CI container alongside qemu); ReleaseSafe keeps
# assertions on for the debug phase.
(cd "$src" && "$zig_bin" build -Doptimize=ReleaseSafe)

mkdir -p "$(dirname "$DEST")"
cp "$src/zig-out/bin/vfu_kvssd" "$DEST"
chmod +x "$DEST"
"$DEST" --version || true
echo "built vfu_kvssd ($REF) -> $DEST"
