# Clone the xNVMe repos
git clone https://github.com/xnvme/xnvme.git
cd xnvme

# Install toolchain and libraries on FreeBSD (13)
sudo ./xnvme/toolbox/pkgs/freebsd-13.sh

# configure xNVMe and build meson subprojects(SPDK)
meson setup builddir

# build xNVMe
meson compile -C builddir

# install xNVMe
sudo meson install -C builddir

# uninstall xNVMe
# cd builddir && meson --internal uninstall
