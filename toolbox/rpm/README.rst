Packaging: Fedora / Redhat
==========================

These are a couple of notes when attempting to produce the ``xnvme.spec`` file
with the goal of generating RPM packages. This was done on a Fedora 36
workstation installation, with a user named ``odus``.  The following tools
installed::

  dnf install \
   fedpkg \
   git \
   rpmdevtools \
   vim

A couple of other packages were also installed, just forgot which...

The user ``odus`` added to the ``mock``::

  usermod -a -G mock odus

This is done to enable allow::

  fedpkg ... mockbuild

Grab the xNVMe git-repos, checkout the ``packaging/rpm`` branch and generate a
source-archive::

  mkdir -p $HOME/git
  pushd $HOME/git
  git clone https://github.com/OpenMPDK/xNVMe.git xnvme
  cd xnvme
  git checkout packaging/rpm
  make gen-src-archive
  popd

The ``packaging/rpm`` branch is currently needed as a couple of changes were
needed for the **mockbuild** to succeed. With the above in place, then continue
with the following to generate the **RPM** packages::

  mkdir $HOME/xnvme-rpm-build
  cd $HOME/xnvme-rpm-build
  cp $HOME/git/xnvme/builddir/meson-dist/xnvme-0.3.0.tar.gz .
  cp $HOME/git/xnvme/toolbox/rpm/xnvme.spec .

  rpmlint xnvme.spec
  fedpkg --release f36 mockbuild

Reading material
----------------

* https://docs.fedoraproject.org/en-US/package-maintainers/Packaging_Tutorial_GNU_Hello/
* https://docs.fedoraproject.org/en-US/packaging-guidelines/Meson/
