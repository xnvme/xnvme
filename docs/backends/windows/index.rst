.. _sec-backends-windows:

Windows
=======

The Windows backend communicates with the Windows NVMe driver StorNVMe.sys via
IOCTLs. The backend relies on the NVMe pass-through through IOCTLs to submit
admin, IO and arbitrary user-defined commands.

System Configuration
--------------------

To use the Windows backend you need to be running Windows. xNVMe is likely
to work on multiple different versions of Windows, however Windows 10, or
later, is recommended as it has all the features which **xNVMe** utilizes.
Alternatively, to ensure compatibility, you can use the version listed in
the :ref:`sec-toolchain` section.

Device Identifiers
------------------

Windows exposes NVMe devices as files using the following naming schemes:

``\\.\PhysicalDrive{ctrlr_id}``
   **NVMe** Controller device node


Instrumentation
---------------

The backend identifier for the Windows backend is: ``windows``. The Windows backend
encapsulates the interfaces below. Some of these are **mix-ins** from the
**Common-Backend Implementations**. For these, links point to descriptions in
the CBI section. For the Windows-specific interfaces, descriptions are available in the
corresponding subsections.

* Memory Allocators

  - ``windows```, Use Windows memory allocator

* Asynchronous Interfaces

  - ``iocp``, Use Windows readfile/writefile with overlapped(iocp) for Asynchronous I/O
  - ``iocp_th``, Use Windows readfile/writefile with thread based overlapped(iocp) for Asynchronous I/O
  - ``io_ring``, Use Windows io_ring for Asynchronous I/O
  - ``emu``, add link to CBI
  - ``thrpool``, add link to CBI

* Synchronous Interfaces

  - ``nvme``, Use Windows NVMe Driver ioctl() for synchronous I/O
  - ``file``, Use Windows File System APIs and readfile()/writefile() for I/O

* Admin Interfaces

  - ``nvme``, Use Windows NVMe Driver ioctl() for admin commands
  - ``block``, Use Windows NVMe Driver ioctl() for admin commands
  - ``file``, Use Windows File System APIs for admin commands

* Device Interfaces

  - ``windows``, Use Windows file/dev handles and enumerate NVMe devices


.. toctree::
   :maxdepth: 2
   :hidden:

   files.rst
   ioctl.rst
   iocp.rst
   iocp_th.rst
   io_ring.rst
