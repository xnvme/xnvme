.. _sec-getting-started:

=================
 Getting Started
=================

Provide an overview text here

.. _sec-building:

Building xNVMe
==============

**xNVMe** builds and runs on Linux, FreeBSD, and Windows. The latter is not
publicly supported, however, if you are interested in Windows support then have
a look at the :ref:`sec-building-windows` section for more information.

.. include:: clone.rst

If you want to change the build-configuration, then have a look at the
following :ref:`sec-building-config`, if you are seeing build errors, then jump
to the :ref:`sec-building-toolchain` section describing packages to install on
different Linux distributions and on FreeBSD.

There you will also find notes on customizing the toolchain and
cross-compilation.

Building an xNVMe Program
=========================

Example code
------------

This `"hello-world"` example prints out device information of the NVMe
device at ``/dev/nvme0n1``.

To use **xNVMe** include the ``libxnvme.h`` header in your C/C++ source:

.. literalinclude:: hello.c
   :language: c

Compile and link
----------------

.. literalinclude:: hello_00.cmd
   :language: bash

.. note:: You do not need to link with SPDK/DPDK/liburing, as these are bundled
  with **xNVMe**. However, do take note of the linker flags surrounding
  ``-lxnvme``, these are required as SPDK makes use of
  ``__attribute__((constructor))``. Without the linker flags, none of the SPDK
  transports will work, as **ctors** will be "linked-out", and **xNVMe** will
  give you errors such as **device not found**.

Also, xNVMe provides three different libraries, two static, and a shared
library as well. Here is what the different libraries are intended for:

* ``libxnvme.a``, this is static library version of **xNVMe** and it comes with
  **batteries included**, that is, all the third-party libraries are bundled
  within the static library ``libxnvme.a``. Thus you only need to link with
  **xNVMe**, as described above, and need not worry about linking with SPDK,
  liburing etc.
* ``libxnvme-static.a``, this is the same as ``libxnvme.a`` except it does
  **not** come with **batteries included**, so you have to manually link with
  SPDK, liburing, etc.
* ``libxnvme-shared.so``, this is the shared library version of **xNVMe**, this
  also does **not** come with **batteries included**, so when linking with, or
  dynamically loading, the shared library version of **xNVMe** then you also
  have to link or load the third-party dependencies.

Using ``libxnvme.a`` is the preferred way to consume **xNVMe** as it comes with
the correct version of the various third-party libraries and provides for
a simpler link-target.

Run!
----

.. literalinclude:: hello_01.cmd
   :language: bash

.. literalinclude:: hello_01.out
   :language: bash

CLI: Hello Device
=================

Most of the C API is wrapped in a suite of command-line interface (CLI) tools.
The equivalent of the above example is readily available from the
:ref:`sec-tools-xnvme` command:

.. literalinclude:: quick_start_00.cmd
   :language: bash

Which should output information similar to:

.. literalinclude:: quick_start_00.out
   :language: bash

With the basics in place, have a look at the :ref:`sec-examples`, follow the
:ref:`sec-tutorials`, and dive deeper into the :ref:`sec-c-api` and experiment
with the :ref:`sec-tools`.

Build Errors
------------

If you are getting errors while attempting to configure and build **xNVMe**
then it is likely due to one of the following:

**git submodules**

* You did not clone with ``--recursive`` or are for other reasons missing the
  submodules. Either clone again with the ``--recursive`` flag, or update
  submodules: ``git submodule update --init --recursive``.

* You used the **"Download Source"** link on GitHUB, this does not work as it
  does not provide all the third-party repositories, only the xNVMe
  source-tree.

In case you cannot use git submodules then a source-archive is provided with
each xNVMe release, you can download it from the `GitHUB Release page
<https://github.com/OpenMPDK/xNVMe/releases>`_ release page. It contains the
xNVMe source code along with all the third-party dependencies, namely: SPDK,
liburing, libnvme, and fio.

**missing dependencies / toolchain**

* You are missing dependencies, see the :ref:`sec-building-toolchain` for
  installing these on FreeBSD and a handful of different Linux Distributions

The :ref:`sec-building-toolchain` section describes preferred ways of
installing libraries and tools. For example, on Ubuntu 18.04 it is preferred to
install ``meson`` via ``pip`` since the version in the package registry is too
old for SPDK, thus if installed via the package manager then you will
experience build errors as the xNVMe build system starts building SPDK.

Once you have the full source of xNVMe, third-party library dependencies, and
setup the toolchain then run the following to ensure that the xNVMe repository
is clean from any artifacts left behind by previous build failures::

  make clobber

And then go back to the :ref:`sec-building` and follow the steps there.

Known Build Issues
------------------

If the above did not sort out your build-issues, then you might be facing one
of the following known build-issues. If these do not apply to you, then please
post an issue on GitHUB describing your build environment and output from the
failed build.

When building **xNVMe** on **Alpine Linux** you might encounter some issues due
to musl standard library not being entirely compatible with **GLIBC** /
**BSD**.

The SPDK backend does not build on due to re-definition of ``STAILQ_*``
macros. As a work-around, then disable the SPDK backend::

  ./configure --disable-be-spdk

The Linux backend support for ``io_uring`` fails on **Alpine Linux** due to a
missing definition in **musl** leading to this error message::

  include/liburing.h:195:17: error: unknown type name 'loff_t'; did you mean
  'off_t'?

As a work-around, then disable ``io_uring`` support::

  ./configure --disable-be-linux-iou

See more details on changing the default build-configuration of **xNVMe** in
the section :ref:`sec-building-config`.

.. _sec-building-toolchain:

Toolchain
=========

The toolchain (compiler, archiver, and linker) used for building **xNVMe**
must support **C11**, **pthreads** and on the system the following libraries
and tools must be available:

* CMake (>= 3.9, For **xNVMe**)
* glibc (>= 2.28, for **io_uring/liburing**)
* libaio-dev (>=0.3, For **xNVMe** and **SPDK**)
* libnuma-dev (>=2, For **SPDK**)
* libssl-dev (>=1.1, For **SPDK**)
* make (gmake)
* meson (>=0.48, for **SPDK**)
* ninja (>=, for **SPDK**)
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

.. literalinclude:: ../../scripts/pkgs/archlinux:20200306.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/archlinux:20200306.sh
   :language: bash
   :lines: 8-

CentOS 7
--------

Install the following packages via ``yum``:

.. literalinclude:: ../../scripts/pkgs/centos:centos7.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/centos:centos7.sh
   :language: bash
   :lines: 8-

Debian 11 (Bullseye)
--------------------

Install the following packages via ``apt-get``:

.. literalinclude:: ../../scripts/pkgs/debian:bullseye.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/debian:bullseye.sh
   :language: bash
   :lines: 17-

Debian 10 (Buster)
------------------

Install the following packages via ``apt-get``:

.. literalinclude:: ../../scripts/pkgs/debian:buster.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/debian:buster.sh
   :language: bash
   :lines: 17-

Debian 9 (Stretch)
------------------

Install the following packages via ``apt-get``:

.. literalinclude:: ../../scripts/pkgs/debian:stretch.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/debian:stretch.sh
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

.. literalinclude:: ../../scripts/pkgs/ubuntu:focal.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/ubuntu:focal.sh
   :language: bash
   :lines: 17-

Ubuntu 18.04 (Bionic)
---------------------

Install the following packages via ``apt-get``:

.. literalinclude:: ../../scripts/pkgs/ubuntu:bionic.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/ubuntu:bionic.sh
   :language: bash
   :lines: 17-

Ubuntu 16.04 (Xenial)
---------------------

Install the following packages via ``apt-get``:

.. literalinclude:: ../../scripts/pkgs/ubuntu:xenial.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/ubuntu:xenial.sh
   :language: bash
   :lines: 17-

Alpine Linux 3.11.3
-------------------

Install the following packages via ``apk``:

.. literalinclude:: ../../scripts/pkgs/alpine:3.12.0.txt
   :language: bash

For example, from the root of the **xNVMe** source repository, do:

.. literalinclude:: ../../scripts/pkgs/alpine:3.12.0.sh
   :language: bash
   :lines: 8-

.. _sec-building-custom:

Customizing the Build
=====================

.. _sec-building-config:

Custom Configuration
--------------------

The **xNVMe** build system configures itself, so you do not have to run
configure script yourself. However, if you for example want to build **xNVMe**
with debugging enabled or disable a specific feature, then this section
describes how to accomplish that.

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

On Linux, the configure script enables the SPDK backend ``be:spdk`` and the
Linux native backend ``be:linux``, with all features enabled, that is, support
for ``libaio``, ``io_uring``, ``nil-io``, and ``thr-io``.

On FreeBSD, the configure script enables the SPDK backend ``be:spdk`` and the
FreeBSD native backend ``be:fbsd``. For more information about these so-called
library backends, see the :ref:`sec-backends` section.

**xNVMe** provides third-party libraries via submodules, it builds these and
embeds them in the **xNVMe** static libraries and executables. If you want to
link with your version of these libraries then you can overwrite the respective
include and library paths. See ``./configure --help`` for details.

.. _sec-building-custom-toolchain:

Non-GCC Toolchain
-----------------

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
