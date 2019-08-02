.. _sec-backends-liou:

Linux/io_uring
==============

The ``XNVME_BE_LIOU`` backend uses synchronous ``ioctl`` calls to have the
kernel perform NVMe command and ``io_uring`` for:

* ``xnvme_cmd_read``
* ``xnvme_cmd_write``
* And for everything else it can ship via ``io_uring`` opcodes

Note on Errors
--------------

Some commands may be issued through ``ioctl`` that are invalid. While most
commands will still be issued to the drive and the error relayed back to the
user, some violations may be picked up by the kernel cause the ``ioctl`` to
fail with ``-1`` and ``errno`` set to ``Invalid Argument``.

TODO: note on mapping ``io_uring`` opcodes to req.-cpl.

In this case, the backend will transform such errors into the NVMe equivalent.
See the documentation on :ref:`xnvme_req <sec-c-apis-xnvme-struct-xnvme_req>`.
