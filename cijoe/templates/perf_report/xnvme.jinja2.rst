What is xNVMe?
--------------

**xNVMe** is a low-level storage abstraction layer. It is designed for
convenient use of NVMe devices, however, not limited to these. At its core
**xNVMe** provides an API operating on **commands**, that is, their
construction, submission, and completion. This can be in a synchronous / blocking
manner, or asynchronously / non-blocking via **queues**.

This **foundational** abstraction based on **commands**, encapsulates the
command-transport and semantics usually tied to storage interfaces.

That is, with **xNVMe** there is no assumption that the commands are
read/write commands.

By decomposing the storage interface in this manner, then any API such as the
blocking ``preadv/pwritev`` system calls, the asynchronous
``io_uring``/``io_uring_cmd`` system interface, or the rich **NVMe** commands
and IO queue pairs, can be represented.

The API provided by **xNVMe** has implementations on top of:
``libaio``, ``POSIX aio``, ``io_uring``, ``io_uring_cmd``, **SPDK
NVMe driver**, **libvfn NVMe driver**, OS ioctl() interfaces, thread pools, and
async. emulation.

As such, **xNVMe** provides the encapsulation of the storage interfaces of
operating system provided abstractions, user space NVMe drivers, storage system
interfaces, and storage system APIs.
