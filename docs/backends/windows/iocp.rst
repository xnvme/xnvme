.. _sec-backends-windows-async-iocp:

Async I/O via ``iocp``
~~~~~~~~~~~~~~~~~~~~~~

When AIO is available then the NVMe NVM Commands for **read** and **write** are
sent over the Windows IOCP interface. Doing so improves command-throughput at
higher queue-depths when compared to sending the command via the NVMe driver
ioctl().

One can explicitly tell **xNVMe** to utilize ``iocp`` for async I/O by
encoding it in the device identifier, like so:

.. literalinclude:: xnvme_win_io_async_read_iocp.cmd
   :language: bash

Yielding the output:

.. literalinclude:: xnvme_win_io_async_read_iocp.out
   :language: bash