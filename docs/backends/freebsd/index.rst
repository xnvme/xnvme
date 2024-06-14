.. _sec-backends-freebsd:

FreeBSD
=======

.. _sec-backends-freebsd-identifiers:

Device Identifiers
------------------

FreeBSD expose NVMe devices via the following device-files:

* ``/dev/nvme{ctrlr_id}``, NVMe Controller device node
* ``/dev/nvme{ctrlr_id}ns{ns_id}``, NVMe Namespace as device node
* ``/dev/nda{num}``, NVMe Namespace as storage device node
* ``/dev/nvd{num}``, NVMe Namespace as disk drive

For details see the following man-pages::

  nvme, nda, nvd, pci, disk, and nvmecontrol

The FreeBSD backend uses synchronous ``ioctl`` calls to have the kernel perform
NVMe commands.

.. _sec-backends-freebsd-configuration:

System Configuration
--------------------

... notes on io_uring support kernel versions on mainline kernel.

.. _sec-backends-freebsd-instrumentation:

Instrumentation
---------------

The backend identifier for the Linux backend is: ``linux``. The Linux backend
encapsulate the interfaces below, some of these are **mix-ins** from the
**Common-Backend Implementations** for these, links point to descriptions in the
CBI section. For the Linux-specific, descriptions are available in subsection.

* Memory Allocators

  - ``posix```, Use **libc** ``malloc()/free()`` with ``sysconf()`` for
    alignment, add link to CBI

* Asynchronous Interfaces

  - ``kqueue``, add link to section here
  - ``posix``, add link to CBI
  - ``emu``, add link to CBI
  - ``thrpool``, add link to CBI
  - ``nil``, add link to CBI

* Synchronous Interfaces

  - ``nvme``, Use FreeBSD NVMe Driver ioctl() for synchronous I/O
  - ``psync``, Use pread()/write() for synchronous I/O

* Device Interfaces

  - ``linux``, Use Linux file/dev handles and enumerate NVMe devices

By default the Linux backend will use ``emu`` for async. emulation wrapping
around the synchronous Linux ``nvme`` driver ioctl-interface. This is done as
it has the broadest command-support. For effiency, one can tap into using the
``io_uring`` and ``io_uring_cmd``.

.. toctree::
   :maxdepth: 2
   :hidden:

   ioctl.rst
