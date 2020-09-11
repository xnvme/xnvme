.. _sec-backends-linux:

Linux
=====

The Linux backend communicates with the Linux NVMe driver via IOCTLs. The
backend relies on the driver-provided passthrough interface to submit admin,
IO, and arbitrary user-defined commands.

The backend supports three different asynchronous implementations:

* ``libaio``
* ``io_uring``
* ``nil``

By default the backend will use ``io_uring`` if it supported, otherwise, it
will fall back to ``libaio``. To explicitly select a backend, then if you are
using device ``/dev/nvme0n1``, then append the ``?async=impl`` option to the
path, e.g.::

  # Use the libaio implemention or fail
  xnvme info /dev/nvme0n1?async=aio

  # Use the liburing/io_uring implemention or fail
  xnvme info /dev/nvme0n1?async=iou

  # Use the nil implemention or fail
  xnvme info /dev/nvme0n1?async=nil

The ``nil`` backend is entirely for debugging and measuring the IO-layer, all
the ``nil`` async. implementation does is queue up commands and when polled for
completion they are returned with success.

Take note that only ``read`` and ``write`` commands work with the Linux
async.-implementations. If you need to submit other commands, then either use
the synchronous commands or use the ``SPDK`` backend.

Note on Errors
--------------

Some commands may be issued through ``ioctl`` that are invalid. While most
commands will still be issued to the drive and the error relayed back to the
user, some violations may be picked up by the kernel cause the ``ioctl`` to
fail with ``-1`` and ``errno`` set to ``Invalid Argument``.

In this case, the backend will transform such errors into the NVMe equivalent.
See the documentation on :ref:`xnvme_req <sec-c-apis-xnvme-struct-xnvme_req>`.
