Standardized I/O
================

For the tasks of write/reading in a portable manner, then the Portable
Operating System Interface (POSIX), and the X/Open Portability Group
(XPG/XOPEN) have produced standard for complying operating systems to follow.

Writing and reading comes in different forms.

* Stateful
  - ``read``/``write``
  - ISO/IEC 9945-1:1990 ("POSIX.1")

* Stateless
  - ``pread``/``pwrite``
  - X/Open Portability  Guide Issue 4, Version 2 ("XPG4.2")

* Vectorized
  - ``readv``/``writev``
  - X/Open Portability  Guide Issue 4, Version 2 ("XPG4.2")

* Vectorized and Stateless
  - ``preadv``/``pwritev``
  - No standards body for these system calls

In practice, then even for the system-call interfaces which are standardized,
then their implementations on systems differentiate on behavior and even also
in function signature. These syscalls are only as portable to the extend that
the POSIX and X/GROUP members implement them.

POSIX Asynchronous I/O (aio)
----------------------------

The POSIX approach to asynchronous I/O, provides a definition for invoking
stateless I/O, that is, the equivalent of the ``pread()``/``pwrite()``
syscalls, in an asynchronous manner.

In the design of POSIX ``aio``, nothing prevents support for ``preadv`` and
``pwritev``,

  ``LIO_READ``, ``LIO_WRITE``, ``LIO_NOP``

This set of opcodes could be extended with ``LIO_PREADV``, ``LIO_PWRITEV``,
however, this never happened. Being POSIX, the members of the group would have
to get together and agree on the value for a given opcode, otherwise, one OS
might use different values and then the purpose of POSIX ends.

There are other practical limitations to POSIX aio, namely how the asynchronous
behavior is implemented. This is often referred to as whether a given OS
supports **true** async. behavior, or whether the async. behavior is emulated
with threads.

The notion of **true** async. behavior, would be that the invocation of an
asynchronous operation would block due to operating infrastructure, or whether
it would be sent with minimal OS intervention to a device queue.

In case of async-emulation, a common pattern of a thread-pool is utilized,
either implemented in user-space, specifically in libc, or whether the pool of
threads are running in kernel-space.

Linux AIO (libaio)
------------------

...

Linux io_uring (liburing)
-------------------------

...

Windows Overlapped
------------------

...

Commonly Used but non-standardized features
-------------------------------------------

* Device enumeration
* Semantics and utilization of Direct-IO
* Driver and OS subsystem interfaces ``ioctl()``
