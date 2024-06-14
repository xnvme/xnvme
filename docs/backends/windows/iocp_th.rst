.. _sec-backends-windows-async-iocp_th:

Async I/O via ``iocp_th``
~~~~~~~~~~~~~~~~~~~~~~~~~

Similar to ``iocp`` interface, only difference is separate poller is used to
fetch the completed IOs.

One can explicitly tell **xNVMe** to utilize ``iocp_th`` for async I/O by
encoding it in the device identifier, like so:

.. literalinclude:: xnvme_win_io_async_read_iocp_th.cmd
   :language: bash

Yielding the output:

.. literalinclude:: xnvme_win_io_async_read_iocp_th.out
   :language: bash
