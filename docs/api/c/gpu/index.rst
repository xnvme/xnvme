.. _sec-api-c-gpu:

GPU-Resident Queue API
######################

The :ref:`sec-backends-upcie-cuda` backend supports GPU-resident NVMe queue pairs via this API.
A GPU-resident queue pair lives entirely in CUDA device memory and can be
passed as a kernel argument so that CUDA threads submit and reap NVMe commands
without any host involvement after launch.

.. note::
  This API is experimental and may change without notice.

API Overview
------------

.. list-table::
   :header-rows: 1
   :widths: 35 20 45

   * - Symbol
     - Where
     - Description
   * - ``xnvme_cuda_queue_create()``
     - Host (C/C++)
     - Allocate a GPU-resident queue pair and register it with the NVMe controller
   * - ``xnvme_cuda_queue_destroy()``
     - Host (C/C++)
     - Deregister the queue pair and free device memory
   * - ``xnvme_cuda_cmd_io()``
     - Device (CUDA kernel)
     - Submit one NVMe command per thread and reap its completion cooperatively

``struct xnvme_cuda_queue`` is an opaque type — its internals are not part of the
public API. ``xnvme_cuda_queue_create()`` allocates and initializes the queue
pair in CUDA device memory, so the returned pointer can be passed directly to a
kernel without any additional copy. This is in contrast to the command array, which
is prepared on the host and must be copied to device memory with
``cudaMemcpy()`` before the kernel launch.

Host-Side Setup
---------------

Open a device on the ``upcie-cuda`` backend, allocate a data buffer, and
prepare one ``xnvme_spec_cmd`` per thread on the host. PRP entries require
physical addresses, so use :ref:`sec-api-c-xnvme_buf-func-xnvme_buf_vtophys`
to resolve them before copying the commands to device memory and launching the
kernel:

.. literalinclude:: ../../../../examples/xnvme_cuda.cu
   :language: cuda
   :start-after: // [host]
   :end-before: // [host]

Device-Side Dispatch
--------------------

Inside a CUDA kernel, use ``xnvme_cuda_cmd_io()`` to submit one NVMe command
per active thread. The commands are fully prepared on the host, so the kernel
only needs to index into the command array and call the helper.

.. literalinclude:: ../../../../examples/xnvme_cuda.cu
   :language: cuda
   :start-after: // [kernel]
   :end-before: // [kernel]

The queue depth passed to ``xnvme_cuda_queue_create()`` must equal the CUDA
block dimension. The ``batch_size`` argument to ``xnvme_cuda_cmd_io()`` controls
how many threads actively submit commands and can be anything up to the queue
depth. Threads with ``tid >= batch_size`` participate in the required barriers
but do not submit or reap a command.


.. toctree::
   :hidden:
   :glob:

   xnvme_*
