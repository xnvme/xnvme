.. _sec-backends:

==========
 Backends
==========

Below is a list of backends and the system interfaces and libraries
they encapsulate. These backends implement the :ref:`sec-backends-intf`
within the **xNVMe** library, offering a unified interface through the
**xNVMe** :ref:`sec-api`. This enables you to switch between system interfaces,
libraries, and drivers at runtime without altering your application logic.

While **xNVMe** abstracts away most differences, it's still important to
understand the specifics related to your platform, system interfaces, and
supporting libraries. For each backend we will cover the following topics:
System configuration, device identification, and backend instrumentation.

System Configuration
   Some backends or I/O interfaces require you to configure your system in
   a certain way to use them. Examples include using a specific OS kernel or
   running a script to attach or detach devices.

Device Identification
   The schema for identifying devices differ across platforms and
   interfaces. Varying from file device handles, e.g. ``/dev/nvme0n1``, to
   PCI device handles, e.g. ``0000:02:00.0``, and NVMe/TCP endpoints, e.g.
   ``172.10.10.10:4420``.

Backend Instrumentation
   On a given platform multiple interface may be available. At runtime,
   the **xNVMe** library makes a decision on which interface to use. You
   can overrule this decision, however, to do so, you need to know what the
   available options are, what they are named, and what they offer.

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
   * - io_uring passthru
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
   * - Driverkit (MacVFN)
     - no
     - no
     - no
     - **yes**
     - no
     - no
   * - I/O Control Ports
     - no
     - no
     - **yes**
     - no
     - no
     - no
   * - I/O Ring
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

.. list-table:: Synchronous I/O and Admin Commands
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
   * - NVMe Driver ioctl()
     - **yes**
     - **yes**
     - **yes**
     - no
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
