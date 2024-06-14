.. _sec-backends-common:

Common Backend Impl (CBI)
=========================

The common backend implementations are implementations for re-use by other
backends. For example, instead of the Linux, FreeBSD, and macOS backends all
implement the same POSIX asynchronous I/O interface, then it is instead provided
here, and the can all include it in their support for async. interfaces.

Similarly, then **xNVMe** provide several software-only implementations of the
**xNVMe** API, such as the **thrpool** which implements the asynchronous parts
of the **xNVMe** API by using a thread-pool.

This composability grants convenience during development and interesting
experiments when mixing abstractions and system interfaces. In addition to
providin common implementations, interface emulation, then **xNVMe** also
provide simple device emulation via the **ramdisk** backend implementation.

Instrumentation
---------------

* Memory Allocators

  - ``posix```, Use **libc** ``malloc()/free()`` with ``sysconf()`` for
    alignment, add link to CBI

* Asynchronous Interfaces

  -  ``nil``, **xNVMe** null-IO, does nothing but complete submitted commands,
     for experimentation only to evaluate **xNVMe** encapsulation overhead
  - ``emu``, ..
  - ``thrpool``, ..
  - ``posix``, ..

* Synchronous Interfaces

  - ``psync``, Use pread()/write() for synchronous I/O



.. toctree::
   :maxdepth: 2
   :hidden:

   psync.rst
   aio.rst
   emu.rst
   thrpool.rst
   nil.rst
   ramdisk.rst
