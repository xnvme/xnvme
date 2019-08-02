.. _sec-building:

============================
 Building xNVMe from source
============================

**xNVMe** builds and runs on Linux, FreeBSD, and Windows. The latter is not
publicly supported, however, have a look at :ref:`sec-building-windows` for
more information.

.. include:: clone.rst

If you want to change the build-configuration, then have a look at the
following :ref:`sec-building-config`, if you are seeing build errors, then jump
to the :ref:`sec-building-toolchain` section describing packages to install on
different Linux distributions and on FreeBSD.

There you will also find notes on customizing the toolchain and
cross-compilation.

.. _sec-building-config:

Configuration
=============

A ``configure`` script is provided to configure the build of **xNVMe**. You can
inspect configuration options by invoking::

  ./configure --help

* Boolean options are enabled by prefixing ``--enable-<OPTION>``
* Boolean options are disabled by prefixing ``--disable-<OPTION>``
* A couple of examples:

  - Enable debug with: ``--enable-debug``

  - Disable the SPDK backend with: ``--disable-be-spdk``

The configure-script will enable backends relevant to the system
platform determined by the environment variable ``OSTYPE``.

On Linux, the configure script enables the backends ``be:lioc``, ``be:liou``,
and ``be:spdk``.
On FreeBSD, the configure script enables the backends ``be:fioc`` and
``be:spdk``.

**xNVMe** provides third-party libraries via submodules, it builds these and
embeds them in the **xNVMe** static libraries and executables. If you want to
link with your version of these libraries then you can overwrite the respective
include and library paths. See ``./configure --help`` for details.

.. _sec-building-toolchain:

Toolchain
=========

The toolchain (compiler, archiver, and linker) used for building **xNVMe**
must support **C11**, **pthreads**:

* CMake (>= 3.9, For **xNVMe**)
* glibc (>= 2.28, for **io_uring**)
* libaio-dev (>=0.3, For **xNVMe** and **SPDK**)
* libnuma-dev (>=2, For **SPDK**)
* libssl-dev (>=1.1, For **SPDK**)
* make (gmake)
* uuid-dev (>=2.3, For **SPDK**)

The preferred toolchain is **gcc** and the following sections describe how to
install it and required libraries on FreeBSD and a set of popular Linux
Distributions.

If you which to use a different toolchain then see the
:ref:`sec-building-custom`, on how to instrument the build-system using a
compiler other than **gcc**.

Arch Linux 20200306
-------------------

Install the following packages via ``pacman``:

.. literalinclude:: ../../scripts/pkgs/arch-20200306.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/arch-20200306.sh
   :language: bash
   :lines: 8-

CentOS 7
--------

Install the following packages via ``yum``:

.. literalinclude:: ../../scripts/pkgs/centos-7.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/centos-7.sh
   :language: bash
   :lines: 8-

Debian 11 (Bullseye)
--------------------

Install the following packages via ``apt-get``:

.. literalinclude:: ../../scripts/pkgs/debian-11.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/debian-11.sh
   :language: bash
   :lines: 17-

Debian 10 (Buster)
------------------

Install the following packages via ``apt-get``:

.. literalinclude:: ../../scripts/pkgs/debian-10.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/debian-10.sh
   :language: bash
   :lines: 17-

Debian 9 (Stretch)
------------------

Install the following packages via ``apt-get``:

.. literalinclude:: ../../scripts/pkgs/debian-9.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/debian-9.sh
   :language: bash
   :lines: 17-

Freebsd 12
----------

Ensure that you have kernel source in ``/usr/src``, then install the following
packages via ``pkg``:

.. literalinclude:: ../../scripts/pkgs/freebsd-12.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/freebsd-12.sh
   :language: bash
   :lines: 8-

Ubuntu 20.04 (Focal)
--------------------

Install the following packages via ``apt-get``:

.. literalinclude:: ../../scripts/pkgs/ubuntu-2004.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/ubuntu-2004.sh
   :language: bash
   :lines: 17-

Ubuntu 18.04 (Bionic)
---------------------

Install the following packages via ``apt-get``:

.. literalinclude:: ../../scripts/pkgs/ubuntu-1804.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/ubuntu-1804.sh
   :language: bash
   :lines: 17-

Ubuntu 16.04 (Xenial)
---------------------

Install the following packages via ``apt-get``:

.. literalinclude:: ../../scripts/pkgs/ubuntu-1604.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/ubuntu-1604.sh
   :language: bash
   :lines: 17-

Alpine Linux 3.11.3
-------------------

Install the following packages via ``apk``:

.. literalinclude:: ../../scripts/pkgs/alpine-3.11.3.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/alpine-3.11.3.sh
   :language: bash
   :lines: 8-

Known Build Issues
~~~~~~~~~~~~~~~~~~

There are a couple of known issues when building **xNVMe** on Alpine Linux,
these are due to issues with the musl standard library which is not entirely
compatible with **GLIBC** / **BSD**.

When building on Alpine with ``./configure --enable-be-spdk``, currently fails
with due to re-definition of ``STAILQ_*`` macros.

And, enabling **be:liou**, ``./configure --enable-be-liou``, currently fails
with due to missing definition in **musl**, leading to this::

  include/liburing.h:195:17: error: unknown type name 'loff_t'; did you mean
  'off_t'?


.. _sec-building-custom:

Custom Toolchain
----------------

To use a compiler other than **gcc**, then:

1) Set the ``CC`` and ``CXX`` environment variable for the ``./configure`` script
2) Pass ``CC``, and ``CXX`` as arguments to ``make``

For example, compiling **xNVMe** on a system where the default compiler is not
**gcc**:

.. code-block:: bash

  CC=gcc CXX=g++ ./configure <YOUR_OPTIONS_HERE>
  make CC=gcc CXX=g++
  make install CC=gcc CXX=g++

Recent versions of **icc**, **clang**, and **pgi** should at be able to satisfy
the **C11** and **pthreads** requirements. However, it will most likely require
a bit of fiddling.

.. note:: The **icc** works well after you bought a license and installed it
  correctly. There is a free option with Intel System Suite 2019.

.. note:: The **pgi** compiler has some issues linking with **SPDK/DPDK** due
  to unstable **ABI** for **RTE** it seems.

The path of least fiddling around is to just install the toolchain and
libraries as described in the sections below.

.. _sec-building-crosscompiling:

Cross-compiling for ARM on x86
------------------------------

In case you do not have the build-tools available on your ARM target, then you
can cross-compile g by parsing ``CC`` parameter to make e.g.:

.. code-block:: bash

  CC=aarch64-linux-gnu-gcc-9 ./configure <config_options>
  make CC=aarch64-linux-gnu-gcc-9

Then transfer and unpack ``xnvme0.tar.gz`` from the ``build`` directory to your
ARM machine.

.. note:: This is currently not supported with the SPDK backend

.. _sec-building-windows:

Windows
-------

Windows is not supported in public domain. However, if you want to roll your
support into **xNVMe**, then you could follow the pointers below.

C11 support is quite poor with most compilers on Windows except for Intel ICC
and the GCC port `TDM-GCC <http://tdm-gcc.tdragon.net/>`_.

A backend implementation for Windows could utilize an IO path, sending
read/write IO to the block device and then wrap all other NVMe commands around
the NVMe driver IOCTL interface.

Such as the one provided by the Open-Source NVMe Driver for Windows:

* https://svn.openfabrics.org/svnrepo/nvmewin/

Or use the IOCTL interface of the built-in NVMe driver:

* https://docs.microsoft.com/en-us/windows/win32/fileio/working-with-nvme-devices

A bit of care has been taken in **xNVMe**, e.g. buffer-allocation, to support /
run on Windows. So, you "only" have to worry about implementing the
command-transport and async interface.
