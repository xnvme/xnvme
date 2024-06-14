.. _sec-backends-linux-nvme-ioctl:

Sync. I/O via ``nvme`` ioctl()
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The default for **xNVMe** is to communicate with devices via the operating
system NVMe driver IOCTLs, specifically on Linux the following are used:

* ``NVME_IOCTL_ID``
* ``NVME_IOCTL_IO_CMD``
* ``NVME_IOCTL_ADMIN_CMD``
* ``NVME_IOCTL_IO64_CMD``
* ``NVME_IOCTL_ADMIN64_CMD``

In case the ``*64_CMD`` IOCTLs are not available then **xNVMe** falls back to
using the non-64bit equivalents. The 64 vs 32 completion result mostly affect
commands such as Zone Append. You can check that this interface is behaving as
expected by running:

.. literalinclude:: xnvme_info_default.cmd
   :language: bash

Which you yield output equivalent to:

.. literalinclude:: xnvme_info_default.out
   :language: bash
   :lines: 1-10

This tells you that **xNVMe** can communicate with the given device identifier
and it informs you that it utilizes **nvme_ioctl** for synchronous command
execution and it uses **thr** for asynchronous command execution.  Since IOCTLs
are inherently synchronous then **xNVMe** mimics asynchronous behavior over
IOCTLs to support the asynchronous primitives provided by the **xNVMe** API.