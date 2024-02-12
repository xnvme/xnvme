.. _sec-contributing-packaging-aur:

Arch Linux (AUR - ``pacman``)
-----------------------------

To update the pkg::

  # Clone the repository
  git clone https://aur.archlinux.org/xnvme.git
  cd xnvme

Edit ``PKGBUILD`` -- update ``pkgver`` and ``sha256sums`` then::

  makepkg --printsrcinfo > .SRCINFO
  git add PKGBUILD
  git add .SRCINFO
  git commit -s -m "bump to <major.minor.patch>"
  git push origin
