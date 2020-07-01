=====================================================
 Cross-platform libraries and tools for NVMe devices
=====================================================

**xNVMe** provides the means to program and interact with NVMe devices from
user space.

The foundation of **xNVMe** is ``libxnvme``, a user space library for working
with NVMe devices. It provides a **C API** for memory management, that is, for
allocating physical / DMA transferable memory when needed. An NVMe command
interface allowing you to submit and complete NVMe commands in a synchronous as
well as an asynchronous manner.

Convenience functions are built-in for common operations and retrieving
parameters essential for I/O applications. Such as your maximum data transfer
size and introducing the concept of a device geometry.

The foundation is as such fairly low-level, sitting right on top of a NVMe
driver. The actual NVMe driver being used by ``libxnvme`` is re-targetable and
can be any one of the GNU/Linux Kernel NVMe driver via ``libaio``, ``IOCTLs``,
and ``io_uring``, the SPDK NVMe driver, or your own custom NVMe driver.

As such, ``libxnvme`` provides a unifying ``C API`` for NVMe tool builders,
application developers, and for anyone wanting their host-defined software to
run on more than a single platform.

On top of ``libxnvme`` a suite of tools and libraries are provided, that is, a
command-line interface for managing your NVMe device named ``xnvme``, and
``lblk`` for commands specific to NVM Namespaces.

For the Zoned Command Set a command-line interface designed specifically for
convenient management of Zoned NVMe devices named ``zoned`` and a library for
direct application integration of Zoned NVMe device named ``libznd``.

To evaluate the performance of abstractions introduced by ``xNVMe`` a Fio IO
engine is provided, supporting **conventional** NVMe devices, as well as
**Zoned** devices.

Jump right into the :ref:`sec-quick-start` and with the basics in place you can
explore the :ref:`sec-c-apis` and the :ref:`sec-tools`.

Contents:

.. toctree::
   :maxdepth: 2
   :includehidden:

   quick_start/index.rst
   tools/index.rst
   capis/index.rst
   examples/index.rst
   tutorial/index.rst
   backends/index.rst
   building/index.rst

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
