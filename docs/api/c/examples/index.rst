.. _sec-api-c-examples:

===================
 API Code Examples
===================

This section includes three examples of using the **xNVMe** library. These are
gradually increasing in complexity, and are meant to complement the description
of the :ref:`sec-api-c-core` API.

All of these examples can be run even without having an NVMe device. If you give
the argument ``1GB`` on the command-line, **xNVMe** will recognize that you want
to use the ``ramdisk`` backend.

In the source repository, you will find more usage examples in the ``examples``
folder.

.. _sec-api-c-examples-hello:

Hello World
===========

In this example we open a device with the uri provided on the command line. This
is done by using the default ``xnvme_opts``, but this struct is where you would
specify the desired backend, sync/async/admin command interface, etc.

.. literalinclude:: ../../../../examples/xnvme_hello.c
   :language: c

.. _sec-api-c-examples-sync:

Synchronous I/O
===============

In this example we open a device and send a single synchronous read command.
Note, when doing synchronous IO, there is no need for a command-queue.
Instead, you get the command-context directly from the device with ``xnvme_cmd_ctx_from_dev()``.
The example is intended to be easily extended by adding a for-loop and submitting multiple commands.

.. literalinclude:: ../../../../examples/xnvme_single_sync.c
   :language: c

.. _sec-api-c-examples-async:

Asynchronous I/O
================

The typical flow for doing asynchronous IO can be reduced to the following:

- Open the device with ``xnvme_dev_open()``
- Initialize the command queue with ``xnvme_queue_init()``
- Set the queue callback with ``xnvme_queue_set_cb()``
- Get a command context from the queue with ``xnvme_queue_get_cmd_ctx()``
- Submit an I/O command, such as with ``xnvme_nvm_read()`` (see :ref:`sec-api-c-xnvme_nvm` for more options)
- Get completion with ``xnvme_queue_poke()``
- Terminate the command queue with ``xnvme_queue_term()``

In this example, we open a device and send a single asynchronous read command.
The process follows these steps:

1. **Initialize the Command Queue**: The ``xnvme_queue``, referred to as the
   command queue, is initialized with a number of command contexts equal to
   its capacity.
2. **Submit a Command**: To submit a command, you obtain a command context from
   the command queue.
3. **Process Completed Commands**: When processing completed commands, you
   poke the command queue, and a callback function is called for each command
   context. At the end of the callback function, you should return the command
   context to the command queue.

The example is intended to be easily extended by adding a for-loop and
submitting multiple commands.

.. literalinclude:: ../../../../examples/xnvme_single_async.c
   :language: c
