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
available on your system and you do not want to install then, then xNVMe will
simply build without io_uring-support.

Since software dependencies means a lot of for those building a given project,
then xNVMe provides an exhaustive overview of what software xNVMe makes use of,
and for which purposes. Along with the documentation of software-dependencies,
then the setup of a system with those packages are provided in
docker-containers, and in the remainder of this section, then description on
installing xNVMe and co. is provided.

The preferred toolchain is **gcc** and the following sections describe how to
install it and required libraries on a set of popular Linux Distributions,
FreeBSD, MacOS, and Windows. If you wish to use a different toolchain then see
the :ref:`sec-building-custom-toolchain`, on how to instrument the build-system
using a compiler other than **gcc**.

In the following sections, the then the system package-manager is used whenever
possible to install the toolchain and libraries. However, on some Linux
distribution there are not recent enough versions. To circurvent that, then the
packages are installed via the Python package-manager. In some cases even a
recent enough version of Python is not available, to bootstrap it, then Python
is built and installed from source.

.. note:: When installing packages via the Python package-manager (``python3 -m
   pip install``), then packages should be installed system-wide. This is
   ensure that the installed packages behave as though they were installed
   using the system package-manager.

Packages managers: apk, pkg, dnf, yum, pacman, apt, aptitude, apt-get, pkg,
choco, brew.

Alpine Linux
------------

From the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/alpine-latest.sh
   :language: bash
   :lines: 8-

The commands above will install the following packages via the system
package-manager (``apk``):

.. literalinclude:: ../../toolbox/pkgs/alpine-latest.txt
   :language: bash

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/alpine-latest-build.sh
   :language: bash
   :lines: 2-

.. note:: There are issues with SPDK/DPDK due to incompatibilities with the
   standard library provided by ``musl libc``. Additionally, the
   ``libexecinfo-dev`` package is no longer available on Alpine. Pull-requests
   fixing this is most welcome, until then, disable support for the SPDK NVMe
   driver as the ``meson setup`` command above.

.. note:: libvfn also relies on ``libexecinfo-dev`` which is currently not
   available for Alpine Linux. Thus, it is also disabled.

Arch Linux
----------

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/archlinux-latest.sh
   :language: bash
   :lines: 8-

The commands above will install the following packages via the system
package-manager (``pacman``):

.. literalinclude:: ../../toolbox/pkgs/archlinux-latest.txt
   :language: bash

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/archlinux-latest-build.sh
   :language: bash
   :lines: 2-

.. note:: The build is configured to install with ``--prefix=/usr`` this is
   intentional such the the ``pkg-config`` files end up in the default search
   path on the system. If you do not want this, then remove ``--prefix=/usr``
   and adjust your ``$PKG_CONFIG_PATH`` accordingly.

CentOS Stream9
--------------

Install the following packages via ``dnf``:

.. literalinclude:: ../../toolbox/pkgs/centos-stream9.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/centos-stream9.sh
   :language: bash
   :lines: 8-

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-

CentOS Stream8
--------------

Install the following packages via ``dnf``:

.. literalinclude:: ../../toolbox/pkgs/centos-stream8.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/centos-stream8.sh
   :language: bash
   :lines: 8-

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/centos-stream8-build.sh
   :language: bash
   :lines: 2-

.. note:: The build is configured to install with ``--prefix=/usr`` this is
   intentional such the the ``pkg-config`` files end up in the default search
   path on the system. If you do not want this, then remove ``--prefix=/usr``
   and adjust your ``$PKG_CONFIG_PATH`` accordingly.


CentOS 7
--------

Install the following packages via ``yum``:

.. literalinclude:: ../../toolbox/pkgs/centos-centos7.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/centos-centos7.sh
   :language: bash
   :lines: 8-

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/centos-centos7-build.sh
   :language: bash
   :lines: 2-

.. note:: The build is configured to install with ``--prefix=/usr`` this is
   intentional such the the ``pkg-config`` files end up in the default search
   path on the system. If you do not want this, then remove ``--prefix=/usr``
   and adjust your ``$PKG_CONFIG_PATH`` accordingly.

Debian 12 (Bookworm)
--------------------

Install the following packages via ``apt-get`` and ``aptitude``:

.. literalinclude:: ../../toolbox/pkgs/debian-bookworm.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/debian-bookworm.sh
   :language: bash
   :lines: 17-

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-

Debian 11 (Bullseye)
--------------------

Install the following packages via ``apt-get``, ``aptitude`` and ``pip3``:

.. literalinclude:: ../../toolbox/pkgs/debian-bullseye.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/debian-bullseye.sh
   :language: bash
   :lines: 17-

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-

Debian 10 (Buster)
------------------

Install the following packages via ``apt-get``, ``aptitude`` and ``pip3``:

.. literalinclude:: ../../toolbox/pkgs/debian-buster.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/debian-buster.sh
   :language: bash
   :lines: 17-

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-

Fedora (36)
-----------

From the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/fedora-36.sh
   :language: bash
   :lines: 8-

The above will install the packages seen below via the system package-manager
``dnf``:

.. literalinclude:: ../../toolbox/pkgs/fedora-36.txt
   :language: bash

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


Fedora (35)
-----------

From the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/fedora-35.sh
   :language: bash
   :lines: 8-

The above will install the packages seen below via the system package-manager
``dnf``:

.. literalinclude:: ../../toolbox/pkgs/fedora-35.txt
   :language: bash

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


Fedora (34)
-----------

From the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/fedora-34.sh
   :language: bash
   :lines: 8-

The above will install the packages seen below via the system package-manager
``dnf``:

.. literalinclude:: ../../toolbox/pkgs/fedora-34.txt
   :language: bash

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


Freebsd 13
----------

Ensure that you have kernel source in ``/usr/src``, then install the following
packages via ``pkg``:

.. literalinclude:: ../../toolbox/pkgs/freebsd-13.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/freebsd-13.sh
   :language: bash
   :lines: 8-

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-

Gentoo
------

Install the following packages using ``emerge``:

.. literalinclude:: ../../toolbox/pkgs/gentoo-latest.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/gentoo-latest.sh
   :language: bash
   :lines: 9-

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/gentoo-latest-build.sh
   :language: bash
   :lines: 2-

.. note:: In case you get: ``error adding symbols: DSO missing from command
   line``, during compilation, then add ``-ltinfo -lnurces`` to ``LDFLAGS`` as
   it is done in the commands above.

.. note:: The build is configured to install with ``--prefix=/usr`` this is
   intentional such the the ``pkg-config`` files end up in the default search
   path on the system. If you do not want this, then remove ``--prefix=/usr``
   and adjust your ``$PKG_CONFIG_PATH`` accordingly.

macOS
-----

Install the following packages using Homebrew_ (``brew``):

.. literalinclude:: ../../toolbox/pkgs/macos-12.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/macos-12.sh
   :language: bash
   :lines: 3-

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/toolbox/pkgs/macos-12-build.sh
   :language: bash
   :lines: 2-

openSUSE Tumbleweed
-------------------

Install the following packages via ``zypper``:

.. literalinclude:: ../../toolbox/pkgs/opensuse-tumbleweed-latest.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/opensuse-tumbleweed-latest.sh
   :language: bash
   :lines: 8-

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-

openSUSE Leap (15.4)
--------------------

Install the following packages via ``zypper``:

.. literalinclude:: ../../toolbox/pkgs/opensuse-leap-15.4.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/opensuse-leap-15.4.sh
   :language: bash
   :lines: 8-

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-

openSUSE Leap (15.3)
--------------------

Install the following packages via ``zypper``:

.. literalinclude:: ../../toolbox/pkgs/opensuse-leap-15.3.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/opensuse-leap-15.3.sh
   :language: bash
   :lines: 8-

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-

Ubuntu 22.04 (Jammy)
--------------------

Install the following packages via ``apt-get`` and ``pip3``:

.. literalinclude:: ../../toolbox/pkgs/ubuntu-jammy.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/ubuntu-jammy.sh
   :language: bash
   :lines: 17-

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-

Ubuntu 20.04 (Focal)
--------------------

Install the following packages via ``apt-get`` and ``pip3``:

.. literalinclude:: ../../toolbox/pkgs/ubuntu-focal.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/ubuntu-focal.sh
   :language: bash
   :lines: 17-

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-

Ubuntu 18.04 (Bionic)
---------------------

Install the following packages via ``apt-get`` and ``pip3``:

.. literalinclude:: ../../toolbox/pkgs/ubuntu-bionic.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../toolbox/pkgs/ubuntu-bionic.sh
   :language: bash
   :lines: 17-

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-

Windows
-------

You can build **xNVMe** using the MSVC toolchain, however, some of the features
are lost when doing so, specifcally the **xNVMe** fio io-engine. Thus, the
instructions provided here utilizes **msys2**.

The toolchain setup is a bit involved, it **bootstraps** by installing the
Chocolatey package manager by opening an eleveted PowerShell and running::

  Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

Then continues with installing a couple of tools via Chocolatey, then the
actual compiler toolchain via the msys2. A script stays more than a thousand
words, so please consult the ``toolbox`` scripts::

  toolbox/pkgs/windows-2019.bat
  toolbox/pkgs/windows-2022.bat

For details.

To utilize the scripts, then invoke the script in an elevated command-prompt
(``cmd.exe`` as Administrator)::

  cd toolbox\pkgs
  windows-2019.bat

.. note:: in case you see .dll loader-errors, then check that the environment
   variable ``PATH`` contains the various library locations of the the
   toolchain.

