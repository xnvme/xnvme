.. _sec-contributing-packaging:

Packaging
=========

A couple of notes/pointers on the packaging of **xNVMe** for Linux
distributions, FreeBSD, Windows, Python, and Rust. For the most recent changes,
see the :xref-github-xnvme-issues:`GitHUB Issue Tracker<389>`.

* Alpine Linux (``apk``)

  - See: https://pkgs.alpinelinux.org/packages?name=xnvme

* :ref:`sec-contributing-packaging-aur`

  - See: https://aur.archlinux.org/packages/xnvme

* :ref:`sec-contributing-packaging-brew`

  - See: https://formulae.brew.sh/formula/xnvme

* Debian Linux (``.deb`` / ``apt``)

  - See: https://salsa.debian.org/safl/xnvme

  - This is a work-in-progress

* Fedora / OpenSUSE, CentOS / Rocky / RHEL (``rpm``)

  - See: https://copr.fedorainfracloud.org/coprs/karlowich/xNVMe/

  - ``toolbox/xnvme-dist.spec.in`` packages the release's source archive
    without subprojects, ``xnvme-X.Y.Z.tar.gz``, with ``%meson`` and the
    system's dependencies. ``make rpm-dist`` builds it, and CI's ``rpm-dist``
    job builds it from the archive of its run, in ``fedora:44`` with only the
    spec's ``BuildRequires`` installed

  - ``make rpm-dev`` is not packaging: it builds an RPM of your checkout,
    installing to ``/usr/local``, for putting on a test box

* FreeBSD (``ports``)

  - See: https://ports.freebsd.org/cgi/ports.cgi?query=xnvme 

* Windows (``choco``)

  - Currently under exploration
  
.. toctree::
   :hidden:

   aur/index.rst
   brew/index.rst
   windows/index.rst
