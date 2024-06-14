.. _sec-backends:

==========
 Backends
==========

Below is a list of backends and the system interfaces and libraries
they encapsulate. These backends implement the :ref:`sec-backends-intf`
within the **xNVMe** library, offering a unified interface through the
**xNVMe** :ref:`sec-api`. This enables you to switch between system interfaces,
libraries, and drivers at runtime without altering your application logic.

While **xNVMe** abstracts these differences, it's still important to understand
specifics related to your platform, system interfaces, and supporting libraries.
Each section covers the following topics.

Device identification
   The schema for identifying devices differ across platforms and interfaces,
   such as naming conventions for NVMe device files can be as different
   as ``/dev/nvme0n1``, ``/dev/ng0n1``, ``/dev/nvme0ns1``, ``disk4``, ``\
   \. \PhysicalDevice2`` and PCI device handles for user-space devices
   ``0000:02:00.0`` and NVMe/TCP endpoints ``172.10.10.10:4420``.
   
System configuration
   This is mostly conserned with the setup and use of OS kernel bypassing
   interfaces such as the user-space NVMe drivers provided by ``SPDK/NVMe``
   and ``libvfn``

Backend instrumentation
   On a given platform multiple interface can be available, the **xNVMe**
   library makes a runtime decision on which interface to use when talking to
   your device. You can overrule the library decision, however, to do so, you
   need to know what the available options are, what they are named and what
   they offer.

The valid combinations of interfaces and backends are listed below:

.. list-table:: Asynchronous I/O
   :header-rows: 1
   :widths: 20 20 20 20 10 10 10

   * - 
     - :ref:`sec-backends-linux`
     - :ref:`sec-backends-freebsd`
     - :ref:`sec-backends-windows`
     - :ref:`sec-backends-macos`
     - :ref:`sec-backends-spdk`
     - :ref:`sec-backends-libvfn`
   * - io_uring
     - **yes**
     - no
     - no
     - no
     - no
     - no
   * - io_uring command (ucmd)
     - **yes**
     - no
     - no
     - no
     - no
     - no
   * - libaio
     - **yes**
     - no
     - no
     - no
     - no
     - no
   * - kqueue
     - no
     - **yes**
     - no
     - no
     - no
     - no
   * - POSIX aio
     - **yes**
     - **yes**
     - no
     - **yes**
     - no
     - no
   * - NVMe Driver (vfio-pci)
     - no
     - no
     - no
     - no
     - **yes**
     - **yes**
   * - NVMe Driver (uio-generic)
     - no
     - no
     - no
     - no
     - **yes**
     - no
   * - I/O Control Ports (iocp)
     - no
     - no
     - **yes**
     - no
     - no
     - no
   * - I/O Ring (io_ring)
     - no
     - no
     - **yes**
     - no
     - no
     - no
   * - emu
     - **yes**
     - **yes**
     - **yes**
     - **yes**
     - **yes**
     - **yes**
   * - thrpool
     - **yes**
     - **yes**
     - **yes**
     - **yes**
     - **yes**
     - **yes**
   * - nil
     - **yes**
     - **yes**
     - **yes**
     - **yes**
     - **yes**
     - **yes**

.. list-table:: Synchronous I/O and Administrative Commands
   :header-rows: 1
   :widths: 20 20 20 20 10 10 10

   * - 
     - :ref:`sec-backends-linux`
     - :ref:`sec-backends-freebsd`
     - :ref:`sec-backends-windows`
     - :ref:`sec-backends-macos`
     - :ref:`sec-backends-spdk`
     - :ref:`sec-backends-libvfn`
   * - psync
     - **yes**
     - **yes**
     - **yes**
     - **yes**
     - no
     - no
   * - Block-Layer ioctl()
     - **yes**
     - no
     - **yes**
     - no
     - no
     - no
   * - NVMe/Driver ioctl()
     - **yes**
     - **yes**
     - **yes**
     - no
     - no
     - no

.. list-table:: Memory interfaces
   :header-rows: 1
   :widths: 20 20 20 20 10 10 10

   * - 
     - :ref:`sec-backends-linux`
     - :ref:`sec-backends-freebsd`
     - :ref:`sec-backends-windows`
     - :ref:`sec-backends-macos`
     - :ref:`sec-backends-spdk`
     - :ref:`sec-backends-libvfn`
   * - libc
     - **yes**
     - **yes**
     - **yes**
     - **yes**
     - **yes**
     - **yes**
   * - hugepages
     - **yes**
     - no
     - no
     - no
     - no
     - no


.. toctree::
   :maxdepth: 2
   :hidden:

   linux/index.rst
   freebsd/index.rst
   macos/index.rst
   windows/index.rst
   spdk/index.rst
   libvfn/index.rst
   common/index.rst
   common/interface/index.rst
