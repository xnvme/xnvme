.. _sec-tools-fio:

fio xNVMe IO Engine
===================

Provided with **xNVMe** is an external fio_ IO engine.

Caveat
------

1. For "raw" device-files only
   - That is, ``/dev/nvme0n1``, ``/dev/nullb0`` and **NOT** file-system files
2. Must be used with ``--thread=1``

Usage
-----

Given that you have installed **xNVMe** then the IO engine should be available
as a shared library.

The library prefix, and suffix vary based on distribution and platform,
however, on a Debian-like distribution it is available as
``libxnvme-fio-engine.so``. Location of the engine is also dist specific, but
would end up in e.g. ``/usr/lib/``.

Make sure you adjust it accordingly when using the fio-examples below.

Also, when, using the **SPDK** backend do::

  # Unbind devices from Linux Kernel NVMe Driver
  xnvme-driver

  # Run fio using "be:spdk"
  fio xnvme-compare-nvme.fio --section=xnvme_spdk

Aslo, when, using any of the non **SPDK** backends make sure that your devices
are attached to the Kernel NVMe driver::

  # Unbind devices from Linux Kernel NVMe Driver
  xnvme-driver reset

  # Run fio using "be:spdk"
  fio xnvme-compare-nvme.fio --section=xnvme_spdk

See details on the parameters of the ``.fio`` file in the following section.

fio xNVMe IO Engine verification
--------------------------------

This fio-script is used for verifying **xNVMe** under load.

.. literalinclude:: ../../../tools/fio-engine/xnvme-verify.fio
   :language: bash

fio xNVMe IO Engine on NVMe Device
----------------------------------

This fio-script can be used for comparing the performance of **xNVMe** to other
means of shipping IO to your device. E.g. **xNVMe/uring vs uring**.

.. literalinclude:: ../../../tools/fio-engine/xnvme-compare.fio
   :language: bash

fio xNVMe IO Engine on NVMe Device with Zoned Command Set
---------------------------------------------------------

This fio-script provides the basics for running on an NVMe device with the
Zoned Command Set enabled.

.. literalinclude:: ../../../tools/fio-engine/xnvme-zoned.fio
   :language: bash


Build Notes
-----------

The ``fio`` **xNVMe** IO engine is enabled by default via the autoconfiguration
produced by::

  make
  sudo make install-deb

If, for some reason you are manually instrumenting CMake / configuration, then
the options to enable/disable the IO engine are::

  --enable-tools-fio
  --disable-tools-fio

.. _fio: https://github.com/axboe/fio
