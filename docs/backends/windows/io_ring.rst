.. _sec-backends-windows-async-io_uring:

Async I/O via ``io_ring``
~~~~~~~~~~~~~~~~~~~~~~~~~

**xNVMe** utilizes the Windows **io_ring** interface, its support for
feature-probing the **io_ring** interface and the **io_ring** opcodes:

When available, then **xNVMe** can send the **io_ring** specific request using
**IORING_HANDLE_REF** and **IORING_BUFFER_REF** structure for **read** and
**write** via Windows **io_ring** interface. Doing so improves command-throughput
at all io-depths when compared to sending the command via NVMe Driver IOCTLs.

One can explicitly tell **xNVMe** to utilize ``io_ring`` for async I/O by
encoding it in the device identifier, like so:

.. literalinclude:: xnvme_win_io_async_read_io_ring.cmd
   :language: bash

Yielding the output:

.. literalinclude:: xnvme_win_io_async_read_io_ring.out
   :language: bash
