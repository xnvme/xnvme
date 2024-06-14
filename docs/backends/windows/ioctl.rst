.. _sec-backends-windows-sync-ioctl:

NVMe Driver and IOCTLs
----------------------

The default for **xNVMe** is to communicate with devices via the operating
system NVMe driver IOCTLs, specifically on Windows the following are used:

* ``IOCTL_STORAGE_QUERY_PROPERTY``
* ``IOCTL_STORAGE_SET_PROPERTY``
* ``IOCTL_STORAGE_REINITIALIZE_MEDIA``
* ``IOCTL_SCSI_PASS_THROUGH_DIRECT``

You can check that this interface is behaving as expected by running:

.. literalinclude:: xnvme_win_info_default.cmd
   :language: bash

Which should yield output equivalent to:

.. literalinclude:: xnvme_win_info_default.out
   :language: bash
   :lines: 1-12

This tells you that **xNVMe** can communicate with the given device identifier
and it informs you that it utilizes **nvme_ioctl** for synchronous command
execution and it uses **iocp** for asynchronous command execution. This method
can be used for raw devices via **\\.\PhysicalDrive<disk number>** device path.

Below mentioned commands are currently supported by **xNVMe** using IOCTL path:

* ``Admin Commands``
   * ``Get Log Page``
   * ``Identify``
   * ``Get Feature``
   * ``Format NVM``

* ``I/O Commands``
   * ``Read``
   * ``Write``

Note on Errors
~~~~~~~~~~~~~~

Some commands may be issued through ``ioctl`` that are invalid. While most
commands will still be issued to the drive and the error relayed back to the
user, some violations may be picked up by the kernel cause the ``ioctl`` to
fail with ``-1`` and ``GetLastError()`` set to ``Invalid Argument``.

In this case, the backend will transform such errors into the NVMe equivalent.
See the documentation on :ref:`xnvme_cmd_ctx <sec-api-c-xnvme_cmd-struct-xnvme_cmd_ctx>`.