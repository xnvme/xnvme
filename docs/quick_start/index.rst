.. _sec-quick-start:

=============
 Quick Start
=============

Go ahead and install **xNVMe**::

  apt install xnvme

If you would like to build **xNVMe** yourself, then have a look at
:ref:`sec-building`.

API: Hello NVMe Device
======================

This "hello-world" example prints out device information of an Zoned Storage
Device. Include the ``libxnvme.h`` header in your C/C++ source:

.. literalinclude:: hello.c
   :language: c

Compile it
~~~~~~~~~~

.. literalinclude:: hello_00.cmd
   :language: bash

Run it
~~~~~~

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
