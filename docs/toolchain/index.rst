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
of the backends provided by :ref:`sec-backends-linux-async-io_uring`
and :ref:`sec-backends-linux-async-io_uring_cmd`.

The following sections describe how to install the recommended toolchain for
each platform and the libraries necessary for all backends on various popular
operating systems, including Linux distributions, FreeBSD, macOS, and Windows.

.. toctree::
   :maxdepth: 2
   :includehidden:

   linux/index.rst
   freebsd/index.rst
   macos/index.rst
   windows/index.rst