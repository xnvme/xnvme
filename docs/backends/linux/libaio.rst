.. _sec-backends-linux-async-libaio:

Async I/O via ``libaio``
~~~~~~~~~~~~~~~~~~~~~~~~

When AIO is available then the NVMe NVM Commands for **read** and **write**
are sent over the Linux AIO interface. Doing so improves command-throughput
at higher queue-depths when compared to sending the command over via the NVMe
driver ioctl().

One can explicitly tell **xNVMe** to utilize ``libaio`` for async I/O by
encoding it in the device identifier, like so:

.. literalinclude:: xnvme_io_async_read_libaio.cmd
   :language: bash

Yielding the output:

.. literalinclude:: xnvme_io_async_read_libaio.out
   :language: bash
   :lines: 1-12