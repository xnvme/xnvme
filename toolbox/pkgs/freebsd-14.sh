#!/bin/tcsh
# Query the linker version
ld -v || true

# Query the (g)libc version
ldd --version || true

# Upgrade existing packages to avoid library version mismatches
if ( "`printenv XNVME_PKGS_NO_UPGRADE`" != "1" ) then
  pkg upgrade -qy
endif

# Install packages via the system package-manager (pkg)
pkg install -qy \
 autoconf \
 automake \
 bash \
 cunit \
 devel/py-Jinja2 \
 devel/py-pipx \
 devel/py-pyelftools \
 devel/py-tabulate \
 findutils \
 git \
 gmake \
 isa-l \
 libtool \
 libuuid \
 meson \
 nasm \
 ncurses \
 openssl \
 patch \
 pkgconf \
 python3 \
 wget

