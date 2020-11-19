.. _sec-tutorials-library:

Library Usage Notes
===================

Introducing how to use the **xNVMe** libraries and C APIs.

Backends
--------

...

Command Interfaces
------------------

...

Return Values and Status Codes
------------------------------

The ``xnvme_cmd`` functions signal errors using the standard ``errno``
thread-local variable and a return value of ``-1``. To gain more insights into
what went wrong in a command, use the optional ``xnvme_cmd_ctx`` parameter that all
``xnvme_cmd`` functions take.  When supplied, the ``xnvme_cmd_ctx`` will contain
the NVMe status code in the ``status`` field.

**Note** that while the :ref:`spdk <sec-backends-spdk>` backend has a
one-to-one correspondence between what is in the ``xnvme_cmd_ctx`` and what is
reported by the device, the :ref:`Linux <sec-backends-linux>`, backend will
transform certain errors from the kernel into their NVMe equivalents.

This is because certain ``ioctl`` calls may return ``-1`` and ``Invalid
Argument`` without actually sending a command to the device. To align the
backends with each other, such an error will be transformed into the NVMe
equivalent, if possible. For example, ``Invalid Argument`` will be transformed
into the NVMe status code ``Invalid Field in Command`` and set that in the
given ``xnvme_cmd_ctx``.

TODO: add references to the C API

Memory Management
-----------------

When using the ``xnvme_cmd`` option ``XNVME_CMD_SGL``, then use these functions
to manage the Scather / Gather Lists.

Asynchronous Interface
----------------------

When using the ``xnvme_cmd`` option ``XNVME_CMD_ASYNC``, then use these
functions to steer the asynchronous mechanics.

Pretty-Printing
---------------

Every structure has a pretty-printer function. Even the opaque ones, e.g.
:cpp:class:`xnvme_dev` has the printer function :c:func:`xnvme_dev_pr()`.  By
convention they are named the same as the structure plus a `_pr` postfix.

The printers all take an `opts` parameter which controls how the structure
should be printed, the current default is `YAML`_ representation.

.. _YAML: https://yaml.org/
