# Clone the xNVMe repos
git clone https://github.com/xnvme/xnvme.git
cd xnvme

# Install toolchain and libraries on macOS (14)
sudo ./xnvme/toolbox/pkgs/macos-14.sh

# configure xNVMe
meson setup builddir

# build xNVMe
meson compile -C builddir

# install xNVMe
sudo meson install -C builddir

# uninstall xNVMe
# cd builddir && meson --internal uninstall