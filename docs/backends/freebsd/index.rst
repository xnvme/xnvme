.. _sec-backends-freebsd:

FreeBSD
=======

.. _sec-backends-freebsd-configuration:

System Configuration
--------------------

To use the FreeBSD backend you need to be running FreeBSD. xNVMe is likely to
work on multiple different versions of FreeBSD, however to ensure compatibility
we recommend using a version listed in the :ref:`sec-toolchain` section.

.. _sec-backends-freebsd-identification:

Device Identification
---------------------

FreeBSD exposes NVMe devices as files using the following naming schemes:

``/dev/nvme{ctrlr_id}``
   **NVMe** Controller device node

``/dev/nvme{ctrlr_id}ns{ns_id}``
   **NVMe** Namespace as device node

``/dev/nda{num}``
   **NVMe** Namespace as storage device node

``/dev/nvd{num}``
   **NVMe** Namespace as disk drive

For details see the man-pages for ``nvme``, ``nda``, ``nvd``, ``pci``, ``disk``,
and ``nvmecontrol``.

.. _sec-backends-freebsd-instrumentation:

Backend Instrumentation
-----------------------

The backend identifier for the FreeBSD backend is: ``fbsd``. The FreeBSD backend
encapsulates the interfaces below. Some of these are **mix-ins** from the
**Common-Backend Implementations**. For these, links point to descriptions in
the CBI section. For the FreeBSD-specific interfaces, descriptions are available in the
corresponding subsections.

* Memory Allocators

  - ``posix```, Use **libc** ``malloc()/free()`` with ``sysconf()`` for
    alignment, add link to CBI

* Asynchronous Interfaces

  - ``kqueue``, Use kqueue based aio for Asynchronous I/O
  - ``posix``, add link to CBI
  - ``emu``, add link to CBI
  - ``thrpool``, add link to CBI
  - ``nil``, add link to CBI

* Synchronous Interfaces

  - ``nvme``, Use FreeBSD NVMe Driver ioctl() for synchronous I/O
  - ``psync``, Use pread()/write() for synchronous I/O

* Admin Interfaces

  - ``nvme``, Use FreeBSD NVMe Driver ioctl() for admin commands

* Device Interfaces

  - ``fbsd``, Use FreeBSD file/dev handles and enumerate NVMe devices

.. toctree::
   :maxdepth: 2
   :hidden:

   ioctl.rst
