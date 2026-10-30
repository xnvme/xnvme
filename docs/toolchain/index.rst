.. _sec-toolchain:

Toolchain
=========

The toolchain (**compiler**, archiver, and linker) used for building **xNVMe**
must support **C11** and **pthreads**. Additionally, then the following tools
must be available on the system:

* Python (>=3.7)
* meson (>=0.58) and a matching version of ninja

**xNVMe** leverages available libraries and interfaces and will "gracefully
degrade" when a specific library is not present.

For example, if **liburing** is not available on your system and you do
not wish to install it, then **xNVMe** will build without the functionality
of the backends provided by :ref:`sec-platforms-linux-async-io_uring`
and :ref:`sec-platforms-linux-async-io_uring_cmd`.

The following sections describe how to install the recommended toolchain for
each platform and the libraries necessary for all backends on various popular
operating systems, including Linux distributions, FreeBSD, macOS, and Windows.

The package installation scripts in ``toolbox/pkgs`` upgrade the installed
system packages before installing the dependencies, on the platforms whose
package manager is ``apt`` (Debian, Ubuntu) or ``pkg`` (FreeBSD). Where that
upgrade is unwanted, for example on a machine with held driver packages whose
userspace libraries must not move, set ``XNVME_PKGS_NO_UPGRADE=1``. The
scripts then refresh the package index and install the dependencies, but skip
the upgrade. Installing can still upgrade an installed package that a
dependency requires a newer version of, such as the C library that a
development package is built against, but packages outside the dependencies
of **xNVMe** are left alone. Note that ``sudo`` drops the environment, so pass
the variable through it:

.. code-block:: bash

   sudo XNVME_PKGS_NO_UPGRADE=1 ./xnvme/toolbox/pkgs/debian-trixie.sh

The scripts for the other package managers do not run a system upgrade, so
the variable makes no difference there. The exception is Arch Linux, where the
script always runs ``pacman -Syyu``, since installing packages against a
refreshed package database without upgrading is a partial upgrade, which Arch
does not support.

.. toctree::
   :maxdepth: 2
   :includehidden:

   linux/index.rst
   freebsd/index.rst
   macos/index.rst
   windows/index.rst