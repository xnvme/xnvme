.. _sec-backends-linux-async-io_uring:

Async. I/O via ``io_uring``
===========================

**xNVMe** support **io_uring** it does so by mapping the NVMe commands read
and write from the NVM command-set to **io_uring** opcodes:

* ``IORING_OP_READ`` / ``IORING_OP_WRITE``

  - Mapped when using ``xnvme_cmd_pass(..)`` with payload as contigous / fixed buffers 

* ``IORING_OP_READV`` / ``IORING_OP_WRITEV``

  - Mapped when using ``xnvme_cmd_passv(...)`` with payload as iovec

Passthru
--------

If you are looking to do command-passthru, that is, send arbitrary user-defined
NVMe commands via the Linux operating system NVMe-driver, then ``io_uring`` is
not the interface to use. Rather, you can do:

* ``opts = {.sync="nvme", .async="io_uring_cmd"}``
* ``opts = {.sync="nvme", .async="emu"}``
* ``opts = {.sync="nvme", .async="thrpool"}``

Or use a user-space driver such as provided via SPDK/NVMe and libvfn.