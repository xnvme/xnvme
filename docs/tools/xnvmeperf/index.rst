.. _sec-tools-xnvmeperf:

xnvmeperf
#########

**xnvmeperf** is a multi-threaded async I/O benchmark for NVMe devices. It
runs a time-bounded async I/O loop and reports per-device and aggregate IOPS
and throughput. A ``verify`` subcommand is also provided to check data integrity
by writing a known per-LBA pattern and reading it back.

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
