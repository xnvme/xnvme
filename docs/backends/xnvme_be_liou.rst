.. _sec-backends-liou:

Linux/io_uring
==============

The Linux/io_uring backend, ``be:liou``, uses ``io_uring`` to provide an
asynchronous interface, over which NVMe command opcodes with equivalent
``io_uring`` opcodes are shipped. Currently, this includes read and write.

Everything else is mapped to the Linux NVMe Driver IOCTLs via ``be:lioc``.

Note on Errors
--------------

Some commands may be issued through ``ioctl`` that are invalid. While most
commands will still be issued to the drive and the error relayed back to the
user, some violations may be picked up by the kernel cause the ``ioctl`` to
fail with ``-1`` and ``errno`` set to ``Invalid Argument``.

TODO: note on mapping ``io_uring`` opcodes to req.-cpl.

In this case, the backend will transform such errors into the NVMe equivalent.
See the documentation on :ref:`xnvme_req <sec-c-apis-xnvme-struct-xnvme_req>`.
