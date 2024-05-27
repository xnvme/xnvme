.. _sec-building-toolchain:

Toolchain
=========

The toolchain (compiler, archiver, and linker) used for building **xNVMe**
must support **C11**, **pthreads** and on the system the following tools must
be available:

* Python (>=3.7)
* meson (>=0.58) and matching version of ninja
* make (gmake)
* gcc/mingw/clang

Along with libraries:

* glibc (>= 2.28, for **io_uring/liburing**)
* libaio-dev (>=0.3, For **xNVMe** and **SPDK**)
* libnuma-dev (>=2, For **SPDK**)
* libssl-dev (>=1.1, For **SPDK**)
* liburing (>=2.2, for **xNVMe**)
* uuid-dev (>=2.3, For **SPDK**)

xNVMe makes use of libraries and interfaces when available and will "gracefully
degrade" when a given library is not available. For example, if liburing is not
available on your system and you do not want to install it, then xNVMe will
simply build without io_uring-support.

The preferred toolchain is **gcc** and the following sections describe how to
install it and required libraries on a set of popular Linux Distributions,
FreeBSD, MacOS, and Windows.

In the following sections, the system package-manager is used whenever possible
to install the toolchain and libraries. However, on some Linux distribution
there are not recent enough versions. To circumvent that, then the packages are
installed via the Python package-manager. In some cases even a recent enough
version of Python is not available, to bootstrap it, then Python is built and
installed from source.

.. note:: When installing packages via the Python package-manager (``python3 -m
   pip install``), then packages should be installed system-wide. This is
   ensure that the installed packages behave as though they were installed
   using the system package-manager.

Package-managers: apk, pkg, dnf, yum, pacman, apt, aptitude, apt-get, pkg,
choco, brew.

Alpine Linux (latest)
---------------------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/alpine-latest.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/alpine-latest.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-alpine-latest:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/alpine-latest-build.sh
   :language: bash
   :lines: 2-


.. note::
   There are issues with SPDK/DPDK due to incompatibilities with the standard
   library provided by ``musl libc``. Additionally, the ``libexecinfo-dev``
   package is no longer available on Alpine.
   Additionally, libvfn also relies on ``libexecinfo-dev`` which is currently
   not available for Alpine Linux. Thus, it is also disabled.
   Pull-request fixing these issues are most welcome, until then, disable
   libvfn and spdk on Alpine.



Arch Linux (latest)
-------------------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/archlinux-latest.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/archlinux-latest.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-archlinux-latest:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/archlinux-latest-build.sh
   :language: bash
   :lines: 2-


.. note::
   The build is configured to install with ``--prefix=/usr`` this is
   intentional such the the ``pkg-config`` files end up in the default search
   path on the system. If you do not want this, then remove ``--prefix=/usr``
   and adjust your ``$PKG_CONFIG_PATH`` accordingly.



Oracle Linux 9 (9)
------------------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/oraclelinux-9.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/oraclelinux-9.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-oraclelinux-9:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/oraclelinux-9-build.sh
   :language: bash
   :lines: 2-



Rocky Linux 9.2 (9.2)
---------------------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/rockylinux-9.2.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/rockylinux-9.2.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-rockylinux-9.2:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/rockylinux-9.2-build.sh
   :language: bash
   :lines: 2-



CentOS Stream 9 (stream9)
-------------------------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/centos-stream9.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/centos-stream9.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-centos-stream9:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/centos-stream9-build.sh
   :language: bash
   :lines: 2-



Debian Testing (trixie)
-----------------------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/debian-trixie.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/debian-trixie.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-debian-trixie:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-



Debian Stable (bookworm)
------------------------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/debian-bookworm.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/debian-bookworm.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-debian-bookworm:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-



Debian Oldstable (bullseye)
---------------------------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/debian-bullseye.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/debian-bullseye.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-debian-bullseye:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-



Fedora (38)
-----------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/fedora-38.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/fedora-38.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-fedora-38:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/fedora-38-build.sh
   :language: bash
   :lines: 2-



Fedora (37)
-----------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/fedora-37.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/fedora-37.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-fedora-37:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/fedora-37-build.sh
   :language: bash
   :lines: 2-



Fedora (36)
-----------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/fedora-36.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/fedora-36.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-fedora-36:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/fedora-36-build.sh
   :language: bash
   :lines: 2-



Ubuntu Latest (lunar)
---------------------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/ubuntu-lunar.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/ubuntu-lunar.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-ubuntu-lunar:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


.. note::
   All tools and libraries are available via system package-manager.


Ubuntu LTS (jammy)
------------------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/ubuntu-jammy.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/ubuntu-jammy.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-ubuntu-jammy:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


.. note::
   Installing liburing from source and meson + ninja via pip


Ubuntu LTS (focal)
------------------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/ubuntu-focal.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/ubuntu-focal.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-ubuntu-focal:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


.. note::
   Installing liburing from source and meson + ninja via pip


Gentoo (latest)
---------------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/gentoo-latest.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/gentoo-latest.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-gentoo-latest:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/gentoo-latest-build.sh
   :language: bash
   :lines: 2-


.. note::
   In case you get ``error adding symbols: DSO missing from command line``,
   during compilation, then add ``-ltinfo -lnurces`` to ``LDFLAGS`` as it is
   done in the commands above.
   The build is configured to install with ``--prefix=/usr`` this is
   intentional such the the ``pkg-config`` files end up in the default search
   path on the system. If you do not want this, then remove ``--prefix=/usr``
   and adjust your ``$PKG_CONFIG_PATH`` accordingly.



openSUSE (tumbleweed-latest)
----------------------------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/opensuse-tumbleweed-latest.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/opensuse-tumbleweed-latest.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-opensuse-tumbleweed-latest:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


.. note::
   All tools and libraries are available via system package-manager.


FreeBSD (13)
------------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/freebsd-13.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/freebsd-13.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-freebsd-13:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


.. note::
   Interfaces; libaio, liburing, and libvfn are not supported on FreeBSD.


macOS (13)
----------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/macos-13.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/macos-13.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-macos-13:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


.. note::
   Interfaces; libaio, liburing, libvfn, and SPDK are not supported on macOS.


macOS (12)
----------

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/macos-12.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/macos-12.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-macos-12:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/macos-12-build.sh
   :language: bash
   :lines: 2-


.. note::
   Interfaces; libaio, liburing, libvfn, and SPDK are not supported on macOS.


Windows (2022)
--------------


Install the required toolchain and libraries, with sufficient system privileges
(e.g. as eleveted command prompt), by executing the commands below. You can
run this on administrator command prompt of the **xNVMe** by invoking::

  call xnvme\toolbox\pkgs\windows-2022.bat

Or, run the commands contained within the script manually:

.. literalinclude:: ../../toolbox/pkgs/windows-2022.bat
   :language: batch



Then go ahead and configure, build and install using helper batch script ``build.bat``:

.. literalinclude:: build_windows.rst
   :language: bash
   :lines: 2-


.. note::
   In case you see ``.dll`` loader-errors, then check that the environment
   variable ``PATH`` contains the various library locations of the toolchain.
   Interfaces; libaio, liburing, libvfn, and SPDK are not supported on
   Windows.





