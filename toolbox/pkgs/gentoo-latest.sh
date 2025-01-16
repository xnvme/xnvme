#!/usr/bin/env bash
# Query the linker version
ld -v || true

# Query the (g)libc version
ldd --version || true

echo ""
echo "build requires linking against ncurses AND tinfo, run the following before compilation:"
echo "export LDFLAGS=\"-ltinfo -lncurses\""

emerge-webrsync
emerge \
 app-arch/libarchive \
 bash \
 dev-build/meson \
 dev-lang/nasm \
 dev-libs/isa-l \
 dev-libs/libaio \
 dev-libs/openssl \
 dev-python/pip \
 dev-python/pyelftools \
 dev-util/cunit \
 dev-util/pkgconf \
 dev-vcs/git \
 findutils \
 make \
 patch \
 sys-libs/liburing \
 sys-libs/ncurses \
 sys-process/numactl

#
# Clone, build and install libvfn
#
# Assumptions:
#
# - Dependencies for building libvfn are met (system packages etc.)
# - Commands are executed with sufficient privileges (sudo/root)
#
git clone https://github.com/SamsungDS/libvfn.git toolbox/third-party/libvfn/repository

pushd toolbox/third-party/libvfn/repository
git checkout v5.1.0
meson setup builddir -Dlibnvme="disabled" -Ddocs="disabled" --buildtype=release --prefix=/usr
meson compile -C builddir
meson install -C builddir
popd

# Install packages via the Python package-manager (pip)
python3 -m pip install --break-system-packages \
 pipx

