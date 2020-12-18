.. _sec-backends-spdk:

SPDK
====

**xNVMe** provides a kernel-bypassing backend implemented using `SPDK
<http://www.spdk.io/>`_. SPDK is built and embedded in the main static
**xNVMe** library ``libxnvme.a``.

Compiling and linking your code with **xNVMe**
----------------------------------------------

To compile e.g. the following ``hello.c`` which uses **xNVMe**:

.. literalinclude:: hello.c
   :language: c

Then invoke compiled with the following linker flags:

.. literalinclude:: 100_compile.cmd
   :language: bash

.. note:: You do not need to link with SPDK/DPDK/liburing, as these are bundled
  with **xNVMe**. However, do take note of the linker flags surrounding
  ``-lxnvme``, these are required as SPDK makes use of
  ``__attribute__((constructor))``. Without the linker flags, none of SPDK
  transports will work, as **ctors** will be "linked-out", and **xNVMe** will
  give you errors such as **device not found**.

Running this:

.. literalinclude:: 110_run.cmd
   :language: bash

Should yield output similar to:

.. literalinclude:: 110_run.out
   :language: bash

Note that the device identifier is hardcoded in the examples. You can use
``xnvme enum``, to enumerate your devices and their associated identifiers.

Unbinding devices and setting up memory
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

By running the command below **8GB** of hugepages will be configured and the
device detached from the Kernel NVMe driver:

.. literalinclude:: 010_xnvme_driver.cmd
   :language: bash

The ``xnvme-driver`` script is a merge of the **SPDK** ``setup.sh`` script and
its dependencies.

The command above should produce output similar to:

.. literalinclude:: 010_xnvme_driver.out
   :language: bash

If anything else that the above is output from ``setup.sh``, for example::

  0000:01:00.0 (1d1d 2807): nvme -> uio_generic

Or::

  Current user memlock limit: 16 MB

  This is the maximum amount of memory you will be
  able to use with DPDK and VFIO if run as current user.
  To change this, please adjust limits.conf memlock limit for current user.

  ## WARNING: memlock limit is less than 64MB
  ## DPDK with VFIO may not be able to initialize if run as current user.

Then consult the section :ref:`sec-backends-spdk-vfio`.

Re-binding devices
~~~~~~~~~~~~~~~~~~

Run the following:

.. literalinclude:: 120_xnvme_driver_reset.cmd
   :language: bash

Should output similar to:

.. literalinclude:: 120_xnvme_driver_reset.out
   :language: bash

.. _sec-backends-spdk-identifiers:

Device Identifiers
~~~~~~~~~~~~~~~~~~

Since devices are no longer available in ``/dev``, then the PCI ids are used,
such as ``pci:0000:03:00.0?nsid=1``, e.g. using the CLI:

.. literalinclude:: 115_xnvme_info.cmd
   :language: bash

And using the API it would be similar to::

  ...
  struct xnvme_dev *dev = xnvme_dev_open("pci:0000:01:00.0?nsid=1");
  ...

.. _sec-backends-spdk-vfio:

Enabling ``VFIO`` without limits
--------------------------------

If ``nvme`` is rebound to ``uio_generic``, and not ``vfio``, then VT-d is
probably not supported or disabled. In either case try these two steps:

1) Verify that your CPU supports VT-d and that it is enabled in BIOS.

2) Enable your kernel by providing the kernel option `intel_iommu=on`.  If you
   have a non-Intel CPU then consult documentation on enabling VT-d / IOMMU for
   your CPU.

3) Increase limits, open ``/etc/security/limits.conf`` and add:

::

  *    soft memlock unlimited
  *    hard memlock unlimited
  root soft memlock unlimited
  root hard memlock unlimited

Once you have gone through these steps, then this command:

.. literalinclude:: 200_dmesg.cmd
   :language: bash

Should output:

.. literalinclude:: 200_dmesg.out
   :language: bash

And this this command:

.. literalinclude:: 300_find.cmd
   :language: bash

Should have output similar to:

.. literalinclude:: 300_find.out
   :language: bash

And user-space driver setup:

.. literalinclude:: 400_xnvme_driver.cmd
   :language: bash

Should rebind the device to ``vfio-pci``, eg.:

.. literalinclude:: 400_xnvme_driver.out
   :language: bash

Inspecting and manually changing memory available to ``SPDK`` aka ``HUGEPAGES``
-------------------------------------------------------------------------------

The `SPDK` setup script provides `HUGEMEM` and `NRHUGE` environment variables
to control the amount of memory available via `HUGEPAGES`. However, if you want
to manually change or just inspect the `HUGEPAGE` config the have a look below.

Inspect the system configuration by running::

  grep . /sys/devices/system/node/node0/hugepages/hugepages-2048kB/*

If you have not yet run the setup script, then it will most likely output::

  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/free_hugepages:0
  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages:0
  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/surplus_hugepages:0

And after running the setup script it should output::

  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/free_hugepages:1024
  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages:1024
  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/surplus_hugepages:0

This tells that `1024` hugepages, each of size `2048kB` are available, that is,
a total of two gigabytes can be used.

One way of increasing memory available to `SPDK` is by increasing the number of
`2048Kb` hugepages. E.g. increase from two to eight gigabytes by increasing
`nr_hugespages` to `4096`::

  echo "4096" > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages

After doing this, then inspecting the configuration should output::

  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/free_hugepages:4096
  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages:4096
  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/surplus_hugepages:0
