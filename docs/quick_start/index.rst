.. _sec-quick-start:

=============
 Quick Start
=============

.. include:: ../building/clone.rst

If the above causes any issues, then have a look at :ref:`sec-building`. Or,
you can grab a binary release of **xNVMe** from the GitHUB release page.

API: Hello NVMe Device
======================

This `"hello-world"` example prints out device information of the NVMe
device at ``/dev/nvme0n1``.

To use **xNVMe** include the ``libxnvme.h`` header in your C/C++ source:

.. literalinclude:: hello.c
   :language: c

Compile and link
~~~~~~~~~~~~~~~~

.. literalinclude:: hello_00.cmd
   :language: bash

.. note:: You do not need to link with SPDK/DPDK/liburing, as these are bundled
  with **xNVMe**. However, do take note of the linker flags surrounding
  ``-lxnvme``, these are required as SPDK makes use of
  ``__attribute__((constructor))``. Without the linker flags, none of SPDK
  transports will work, as **ctors** will be "linked-out", and **xNVMe** will
  give you errors such as **device not found**.

Run!
~~~~

.. literalinclude:: hello_01.cmd
   :language: bash

.. literalinclude:: hello_01.out
   :language: bash

CLI: Hello Device
=================

Most of the C API is wrapped in a suite of command-line interface (CLI) tools.
The equivalent of the above example is readily available from the
:ref:`sec-tools-xnvme` command:

.. literalinclude:: quick_start_00.cmd
   :language: bash

Which should output information similar to:

.. literalinclude:: quick_start_00.out
   :language: bash

With the basics in place, have a look at the :ref:`sec-examples`, follow the
:ref:`sec-tutorials`, and dive deeper into the :ref:`sec-c-apis` and experiment
with the :ref:`sec-tools`.
