.. _sec-tools-fio:

fio IO Engine
=============

Provided with **xNVMe** is an external fio_ IO engine. There are a couple of
things which needs a bit of explanation when using the **xNVMe** fio IO engine:

* How to locate the external engine
* How to tell fio_ to use it
* How to select Operating System IO interface
* How to utilize a User space driver

The :ref:`sec-tools-fio-external` section describes the first two and section
:ref:`sec-tools-fio-instrumentation` describes the latter. Expanding on this
are a couple of :ref:`sec-tools-fio-examples`:

* :ref:`sec-tools-fio-examples-verification` (``xnvme-verify.fio``)
* :ref:`sec-tools-fio-examples-comparison` (``xnvme-compare.fio``)
* :ref:`sec-tools-fio-examples-zoned` (``xnvme-zoned.fio``)

One of the above scripts are also used in the
:ref:`sec-tools-fio-instrumentation` section to reduce the number of fio_
arguments in the command-examples.

.. _sec-tools-fio-external:

External Engine
---------------

Assuming that you have built and installed **xNVMe** as described in the
:ref:`sec-getting-started` section, then the **xNVMe** IO engine is available
as a shared library on your system. You can locate it using ``pkg-config``,
e.g. by running:

.. literalinclude:: 100_pkg-config-libdir.cmd
   :language: bash

Which on a Debian Linux system would output:

.. literalinclude:: 100_pkg-config-libdir.out
   :language: bash

The absolute path in this case is:

.. literalinclude:: 110_pkg-config-libdir.out
   :language: bash
   :lines: 1

Pass this path to fio_ with an ``external:`` prefix, e.g.::

  --ioengine=external:/usr/local/lib/x86_64-linux-gnu/libxnvme-fio-engine.so

Usage examples are provided in the following
:ref:`sec-tools-fio-instrumentation` section in the form fio-script in
:ref:`sec-tools-fio-examples`.

.. _sec-tools-fio-instrumentation:

Instrumentation
---------------

When nothing is selected, then the **xNVMe** library will use the first backend
implementation capable of obtaining a handle on the device given with the
``--filename`` argument.

This can look like so:

.. literalinclude:: 200_fio_compare_os.cmd
   :language: bash

However, commonly one wants to select a specific asynchronous interface to use,
the following section will show how to do that using operating system
device-handles and interfaces and following that an example of telling the IO
engine to use a User space driver.

Operating System
~~~~~~~~~~~~~~~~

The asynchronous IO interface is selected explicitly via the IO engine option
``--xnvme_async``. Use it to e.g. use ``io_uring``:

.. literalinclude:: 210_fio_compare_os.cmd
   :language: bash

Or, ``libaio``:

.. literalinclude:: 220_fio_compare_os.cmd
   :language: bash

Or, ``thrpool``:

.. literalinclude:: 230_fio_compare_os.cmd
   :language: bash

The ``thrpool`` is not true asynchronous IO interface, rather, it is a fallback
implementation using a pool of threads on top of a synchronous IO interface,
such as ``pread()/pwrite()``, or as in the example above, the NVMe driver
``ioctl()``.

User-space driver
~~~~~~~~~~~~~~~~~

To utilize the a User space NVMe driver, then the NVMe devices must first be
unbound from operating system NVMe-driver and associated with ``vfio-pci`` or
``uio-generic`` and identify the device using ``pcie`` identfier and
namespace-id, this can be done by running:

.. literalinclude:: 300_prep.cmd
   :language: bash

Which should output something similar to:

.. literalinclude:: 300_prep.out
   :language: bash

From the output above use the ``pcie`` identifier in ``uri``. In the above,
there are two namespace, the ``csi`` value tells you the
command-set-identifier, in this case we want to use the device with ``csi=0x0``
which is NVM the other is ZNS. Note that, when passing the ``pcie`` identifer
then the ``:`` needs to be escaped, here is an example of using it:

.. literalinclude:: 310_fio_compare_uspace.cmd
   :language: bash

See details on the parameters of the ``.fio`` file in the following section.

.. _sec-tools-fio-examples:

Example Scripts
---------------

Assuming that you have built and installed **xNVMe** as described in the
:ref:`sec-getting-started` section, then the fio scripts are installed on your
system and you can use ``pkg-config`` to locate them:

.. literalinclude:: 110_pkg-config-datadir.cmd
   :language: bash

Yielding the path:

.. literalinclude:: 110_pkg-config-datadir.out
   :language: bash

Which should contain:

.. literalinclude:: 120_pkg-config-datadir.out
   :language: bash

The scripts are provided in the following sections for reference.

.. _sec-tools-fio-examples-verification:

Verification (xnvme-verify.fio)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This fio-script is used for verifying **xNVMe** under load.

.. literalinclude:: ../../../tools/fio-engine/xnvme-verify.fio
   :language: bash


.. _sec-tools-fio-examples-comparison:

Comparison (xnvme-compare.fio)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This fio-script can be used for comparing the performance of **xNVMe** to other
means of shipping IO to your device. E.g. **xNVMe/uring vs uring**.

.. literalinclude:: ../../../tools/fio-engine/xnvme-compare.fio
   :language: bash


.. _sec-tools-fio-examples-zoned:

Zoned Command Set (xnvme-zoned.fio)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This fio-script provides the basics for running on an NVMe device with the
Zoned Command Set enabled.

.. literalinclude:: ../../../tools/fio-engine/xnvme-zoned.fio
   :language: bash

Build Notes
-----------

The ``fio`` **xNVMe** IO engine is enabled by default, whether or not it is
built is controlled via the meson-option: ``with-fio``. When enabled, then
Meson will fetch fio into 

Caveats
-------

This section provides a couple of notes and insights to double check in case
you are facing unexpected issues.

Threading vs Forking
~~~~~~~~~~~~~~~~~~~~

* Must be used with ``--thread=1``

The above option is required as forking for parallel job execution is not
supported; aka threading is required.

Multi-device support
~~~~~~~~~~~~~~~~~~~~

* Multiple devices are supported, however, they all either have to be
  operating-system managed or all utilize a User space driver.

Details follow.

1) ``iomem_{alloc/free}`` introduces a limitation with regards to multiple
   devices. Specifically, the devices opened must use backends which share
   memory allocators. E.g. using ``be:linux`` + ``be:posix`` is fine, using
   ``be:linux`` + ``be:spdk`` is not.
   This is due to the fio 'io_mem_*' helpers are not tied to devices, as such,
   it is required that all devices opened use compatible buffer-allocators.
   Currently, the implementation does dot check for this unsupported use-case,
   and will thus lead to a runtime error.

2) The implementation assumes that ``thread_data.o.nr_files`` is available and
   that instances of ``fio_file.fileno`` are valued ``[0,
   thread_data.o.nr_files -1]``.
   This is to pre-allocate file-wrapping-structures, xnvme_fioe_fwrap, at I/O
   engine initialization time and to reference file-wrapping with constant-time
   lookup.

3) The ``_open()`` and ``_close()`` functions do not implement the "real"
   device/file opening, this is done in ``_init()`` and torn down in
   ``_cleanup()`` as the io-engine needs device handles ready for
   ``iomem_{alloc/free}``.

NVMe devices formatted with extended-LBA
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* These are currently not supported.

Details follow.

To support extended-lba initial work has been done in xNVMe, however, further
work is probably need for this to trickle up to the fio I/O engine, also, in
the io-engine ``>> ssw`` are used which does not account for extended LBA.

.. _fio: https://github.com/axboe/fio
.. _Meson: https://mesonbuild.com/
.. _Meson-build-options: https://mesonbuild.com/Build-options.html
