.. _sec-backends-linux:

Linux
=====

The Linux backend communicates with the Linux NVMe driver via IOCTLs. The
backend relies on the driver-provided passthru interface to submit admin,
IO, and arbitrary user-defined commands.

The backend has four different asynchronous implementations:

* ``thr``, wraps around the synchronous interface providing async. behavior
* ``aio``, Linux Asynchronous IO.
* ``iou``, the efficient Linux IO interface, io_uring.
* ``nil``, xNVMe null-IO, does nothing but complete submitted commands, for
  experimentation only

By default the Linux backend will use ``thr`` as it has the broadest
command-support. When interested only in read/write commands, one can use the
``io_uring``, if it supported, otherwise, it will fall back to ``libaio``.

To explicitly select an async. implementation, then if you are using device
``/dev/nvme0n1``, add ``?async=impl`` option to the path, e.g.::

  # Use the thr implemention or fail
  xnvme info /dev/nvme0n1?async=thr

  # Use the libaio implemention or fail
  xnvme info /dev/nvme0n1?async=aio

  # Use the liburing/io_uring implemention or fail
  xnvme info /dev/nvme0n1?async=iou

  # Use the nil implemention or fail
  xnvme info /dev/nvme0n1?async=nil

The ``nil`` backend is entirely for debugging and measuring the IO-layer, all
the ``nil`` async. implementation does is queue up commands and when polled for
completion they are returned with success.

If you want both control and performance then use the ``SPDK`` backend.

Note on Errors
--------------

Some commands may be issued through ``ioctl`` that are invalid. While most
commands will still be issued to the drive and the error relayed back to the
user, some violations may be picked up by the kernel cause the ``ioctl`` to
fail with ``-1`` and ``errno`` set to ``Invalid Argument``.

In this case, the backend will transform such errors into the NVMe equivalent.
See the documentation on :ref:`xnvme_cmd_ctx <sec-c-api-xnvme-struct-xnvme_cmd_ctx>`.
