.. _sec-backends-spdk:

SPDK
====

**xNVMe** provides a kernel-bypassing backend implemented using `Intel SPDK
<http://www.spdk.io/>`_. The following contains a couple of notes on building
SPDK for use with **xNVMe**.

Building **SPDK**
-----------------

With **xNVMe** a make-target is provided which clones the required version
(**v19.07**) of SPDK via git-subodule, configures and builds it. To use it,
then go to the root of the **xNVMe** repository and run ``make deps-spdk``.

Build **xNVMe** with **SPDK** support
-------------------------------------

With **SPDK** in place, configure the **xNVMe** build with::

  ./configure --enable-be-spdk

Linking your source with **libxnvme** and **SPDK**
--------------------------------------------------

Invoke like so:

.. literalinclude:: 000_compile.cmd
   :language: bash

You do not need to specify the SPDK libraries, since xNVMe have them bundled.

The above compiles the example from the quick-start guide, note that the code
has a hardcoded device identifier, you must change this to match the **SPDK**
identifier.

Unbinding devices and setting up memory
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

By running the command below `8GB` of hugepages will be configured and the
device detached from the Kernel NVMe driver:

.. literalinclude:: 110_xnvme_driver.cmd
   :language: bash

The ``xnvme-driver`` script is a merge of the **SPDK** ``setup.sh`` script and
its dependencies.

The command above should produce output similar to:

.. literalinclude:: 110_xnvme_driver.out
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

Then consult the notes on **Enabling ``VFIO`` without Limits**.

Re-binding devices
~~~~~~~~~~~~~~~~~~

Run the following:

.. literalinclude:: 120_xnvme_driver_reset.cmd
   :language: bash

Should output similar to:

.. literalinclude:: 120_xnvme_driver_reset.out
   :language: bash


Device Identifiers
~~~~~~~~~~~~~~~~~~

Since devices are no longer available in ``/dev``, then the PCI ids are used,
such as ``pci://0000:03:00.0/1``, e.g. using the CLI:

.. literalinclude:: xnvme_info.cmd
   :language: bash

And using the API it would be similar to::

  ...
  struct xnvme_dev *dev = xnvme_dev_open("pci://0000:01:00.0/1");
  ...


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

Inspecting and manually changing memory avaiable to `SPDK` aka `HUGEPAGES`
--------------------------------------------------------------------------

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
