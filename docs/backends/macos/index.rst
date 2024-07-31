.. _sec-backends-macos:

macOS
=====

.. _sec-backends-macos-configuration:

System Configuration
--------------------

To use the macOS backend you need to be running macOS. xNVMe is likely to
work on multiple different versions of macOS, however to ensure compatibility
we recommend choosing among the versions listed in the :ref:`sec-toolchain`
section.

.. _sec-backends-macos-identification:

Device Identifiers
------------------

macOS exposes NVMe devices as files using the following naming schemes:

``/dev/disk{ctrlr_id}``
   **NVMe** Controller device node

.. _sec-backends-macos-instrumentation:

Instrumentation
---------------

The backend identifier for the macOS backend is: ``macos``. The macOS backend
encapsulates the interfaces below. Some of these are **mix-ins** from the
**Common-Backend Implementations**. For these, links point to descriptions in
the CBI section. For the macOS-specific interfaces, descriptions are available in the
corresponding subsections.

* Memory Allocators

  - ``posix```, Use **libc** ``malloc()/free()`` with ``sysconf()`` for
    alignment, add link to CBI

* Asynchronous Interfaces

  - ``emu``, add link to CBI
  - ``posix``, add link to CBI
  - ``thrpool``, add link to CBI

* Synchronous Interfaces

  - ``macos``, Use pread()/write() for synchronous I/O

* Admin Interfaces

  - ``macos``, Use macOS NVMe SMART Interface admin commands

* Device Interfaces

  - ``macos``, Use macOS file/dev handles and enumerate NVMe devices