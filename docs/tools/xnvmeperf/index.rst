.. _sec-tools-xnvmeperf:

xnvmeperf
#########

**xnvmeperf** is a multi-threaded async I/O benchmark for NVMe devices. It
runs a time-bounded async I/O loop and reports per-device and aggregate IOPS
and throughput. A ``verify`` subcommand checks data integrity by writing a known
per-LBA pattern and reading it back.

When built with CUDA support (``XNVME_BE_UPCIE_CUDA_ENABLED``), two additional
subcommands are available: ``cuda-run`` and ``cuda-verify``. These run NVMe I/O
directly from CUDA kernels via the :ref:`sec-backends-upcie-cuda` backend, bypassing
the host I/O path entirely.

.. literalinclude:: xnvmeperf_usage.out
   :language: bash

``run`` — Benchmark
===================

Runs async I/O against one or more NVMe devices for a fixed duration. Devices
are listed as trailing positional arguments. One thread is spawned per CPU in
``--cpumask``, e.g. ``0x3`` spawns two threads on CPUs 0 and 1. If there are
more devices than CPUs, devices are split evenly so each thread owns a slice and
runs one async job per owned device. If there are more CPUs than devices,
devices are assigned round-robin so multiple threads drive the same device
concurrently.

.. literalinclude:: xnvmeperf_run_usage.out
   :language: bash

Example — sequential read on a single device::

   xnvmeperf run --iopattern read --qdepth 32 --iosize 4096 \
       --runtime 10 --cpumask 0x1 /dev/nvme0n1

Example — random write across two devices on two CPUs::

   xnvmeperf run --iopattern randwrite --qdepth 64 --iosize 4096 \
       --runtime 10 --cpumask 0x3 /dev/nvme0n1 /dev/nvme1n1

``verify`` — Data integrity check
==================================

Writes ``--count`` sequential I/Os with a per-LBA stamp pattern, then reads
them back and compares against the expected data. Reports the number of
mismatches and I/O errors per device.

.. literalinclude:: xnvmeperf_verify_usage.out
   :language: bash

Example::

   xnvmeperf verify --iosize 4096 --count 256 /dev/nvme0n1

``cuda-run`` — GPU benchmark
============================

.. seealso::

   :ref:`sec-backends-upcie-cuda`
      Backend setup, system configuration, and memory architecture.

   :ref:`sec-api-c-gpu`
      The ``libxnvme_cuda`` API used to create queues and dispatch commands
      from CUDA kernels.

Requires the ``upcie-cuda`` backend. All queues across all devices are driven
by a single CUDA kernel: each CUDA block owns one NVMe queue and each thread
within the block owns one queue slot, so ``--qdepth`` threads submit and reap
commands in lock-step. The grid has ``ndevs × --queues`` blocks in total.

Both ``--qdepth`` and ``--iosize`` must be powers of 2. Supported patterns are
``read``, ``write``, ``randread``, and ``randwrite``.

.. literalinclude:: xnvmeperf_cuda_run_usage.out
   :language: bash

Example — sequential read, four queues of depth 32 on two devices::

   xnvmeperf cuda-run --iopattern read --queues 4 --qdepth 32 --iosize 4096 \
       --runtime 10 --be upcie-cuda 0000:01:00.0 0000:02:00.0

Example — random write, single queue::

   xnvmeperf cuda-run --iopattern randwrite --qdepth 64 --iosize 4096 \
       --runtime 10 --be upcie-cuda 0000:01:00.0

``cuda-verify`` — GPU data integrity check
==========================================

Uses the same queue topology as ``cuda-run``. For each queue slot the kernel
writes a unique LBA-stamped pattern, then reads it back on the host and
compares against the expected data. This is intended to confirm that
``cuda-run`` results reflect correct I/O rather than silent data corruption.

.. literalinclude:: xnvmeperf_cuda_verify_usage.out
   :language: bash

Example::

   xnvmeperf cuda-verify --iosize 4096 --qdepth 32 --be upcie-cuda 0000:01:00.0
