.. _sec-backends-linux:

Linux
=====

...

Device Identifiers
------------------

Linux exposes storage devices as device files using the following naming schemes.

``/dev/nvme{ctrlr_num}``
   **NVMe** Controllers as character device
  
``/dev/nvme{ctrlr_num}n{namespace_num}``
   **NVMe** Namespace as block device (Command-Sets: NVM, ZNS)
  
``/dev/ng{ctrlr_num}n{namespace_num}``
   **NVMe** Namespace as characater device (Command-sets: Key-Value, ZNS-nonpo2, etc.)
  
``/dev/sd{a-z}``
   Block device representing multiple devivces: **SCSI**, **SATA**,
   **USB**-Sticks, etc.
  
``/dev/hd{a-z}``
   **IDE** Drives (Harddisk, CD/DVD Drives)

System Configuration
--------------------

... notes on io_uring support kernel versions on mainline kernel.


Instrumentation
---------------

The backend identifier for the Linux backend is: ``linux``. The Linux backend
encapsulate the interfaces below, some of these are **mix-ins** from the
**Common-Backend Implementations** for these, links point to descriptions in the
CBI section. For the Linux-specific, descriptions are available in subsection.

* Memory Allocators

  - ``posix```, Use **libc** ``malloc()/free()`` with ``sysconf()`` for
    alignment, add link to CBI
  - ``hugepage``, Allocate buffers using hugepages via ``mmap``` on hugetlbfs

* Asynchronous Interfaces

  - ``libaio``, Linux Asynchronous IO.
  - ``io_uring``, the efficient Linux IO interface, io_uring.
  - ``io_uring_cmd``, the efficient Linux IO interface, io_uring.
  - ``emu``, add link to CBI
  - ``posix``, add link to CBI
  - ``thrpool``, add link to CBI
  - ``nil``, add link to CBI

* Synchronous Interfaces

  - ``nvme``, Use Linux NVMe Driver ioctl() for synchronous I/O
  - ``psync``, Use pread()/write() for synchronous I/O
  - ``block``, Use Linux Block Layer ioctl() and pread()/pwrite() for I/O

* Admin Interfaces

  - ``block``, Use Linux Block Layer ioctl() and sysfs for admin commands

* Device Interfaces

  - ``linux``, Use Linux file/dev handles and enumerate NVMe devices

By default the Linux backend will use ``emu`` for async. emulation wrapping
around the synchronous Linux ``nvme`` driver ioctl-interface. This is done as
it has the broadest command-support. For effiency, one can tap into using the
``io_uring`` and ``io_uring_cmd``.

Examples
~~~~~~~~

Thus, when using to get a handle to device, then pass the path to the device in
question, you can experiment by using the ``xnvme`` cli-tool. Such as:

.. code-block:: bash

    # Command
    xnvme info /dev/sda

    # Output
    xnvme_dev:
      xnvme_ident:
        uri: '/dev/sda'
        dtype: 0x3
        nsid: 0x1
        csi: 0x1f
        subnqn: ''
      xnvme_be:
        admin: {id: 'block'}
        sync: {id: 'block'}
        async: {id: 'emu'}
        attr: {name: 'linux'}
      xnvme_opts:
        be: 'linux'
        mem: 'posix'
        dev: 'FIX-ID-VS-MIXIN-NAME'
        admin: 'block'
        sync: 'block'
        async: 'emu'
      xnvme_geo:
        type: XNVME_GEO_CONVENTIONAL
        npugrp: 1
        npunit: 1
        nzone: 1
        nsect: 15669248
        nbytes: 512
        nbytes_oob: 0
        tbytes: 8022654976
        mdts_nbytes: 262144
        lba_nbytes: 512
        lba_extended: 0
        ssw: 9
        pi_type: 0
        pi_loc: 0
        pi_format: 0

.. toctree::
   :maxdepth: 2
   :hidden:

   ioctl.rst
   block.rst
   libaio.rst
   io_uring.rst
   io_uring_cmd.rst
