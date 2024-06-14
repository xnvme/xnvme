.. _sec-api-c-core:

Core
####

Here are the main concept and mechanics with pointers to the relevant parts of
the API.

Device Interaction
  Device interaction spans multiple modules, starting with the device
  representation by the opaque type :ref:`sec-api-c-xnvme_dev`.

  To query a system for devices, use
  the :ref:`sec-api-c-xnvme_dev-func-xnvme_enumerate` function, which
  invokes callback functions for each discovered device, providing the
  opaque handle to the device. Another way to obtain device handles is
  through direct calls to :ref:`sec-api-c-xnvme_dev-func-xnvme_dev_open`,
  where the essential structure is the option struct
  (:ref:`sec-api-c-xnvme_opts-struct-xnvme_opts`).

  Once a handle is available, the device geometry module
  (:ref:`sec-api-c-xnvme_geo`) is quite useful for querying constraints and
  capabilities. For details, see :ref:`sec-api-c-xnvme_geo-struct-xnvme_geo`.
  Notably, the properties ``mdts_nbytes`` and ``lba_nbytes`` are essential
  for constructing and submitting valid commands, as they represent a useful
  controller property and a useful NVM namespace property, respectively.

  Lastly, the :ref:`sec-api-c-xnvme_ident-struct-xnvme_ident` structure,
  retrievable via :ref:`sec-api-c-xnvme_dev-func-xnvme_dev_get_ident`, contains,
  among other things, information such as the command-set-identifier, which
  tells you which NVMe command-set is available.

Commands
  Commands are encapsulated in the :ref:`sec-api-c-xnvme_cmd` module
  and :ref:`sec-api-c-xnvme_cmd-struct-xnvme_cmd_ctx`. The ``xnvme_cmd_ctx``,
  referred to as command-context, encapsulates both the command to be processed
  and the completion result once it is done. The essential command submission
  functions include:

  - :ref:`sec-api-c-xnvme_cmd-func-xnvme_cmd_pass`
  - :ref:`sec-api-c-xnvme_cmd-func-xnvme_cmd_pass_iov`
  - :ref:`sec-api-c-xnvme_cmd-func-xnvme_cmd_pass_admin`

  A code example is provided using these primitives
  for :ref:`sec-api-c-examples-sync` and via the :ref:`sec-api-c-xnvme_queue`
  module for :ref:`sec-api-c-examples-async`.

Buffers
  The :ref:`sec-api-c-xnvme_buf` module provides a ``malloc``-like interface for
  allocating I/O capable buffers and virtual memory.

This section describes the core **xNVMe** API, which includes functions (and
headers) that will most likely be part of every program using the **xNVMe**
library.

.. toctree::
   :hidden:
   :glob:

   xnvme_*
