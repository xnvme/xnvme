.. _sec-tools-xnvme:

xnvme
#####

``xnvme`` is the entry point to the **xNVMe** command line. Run it without
arguments for the commands to start with and the groups the rest are in:

.. literalinclude:: xnvme_overview.cmd
   :language: bash

.. literalinclude:: xnvme_overview.out
   :language: bash

Run a group to list its commands, and append ``--help`` to any command for its
arguments. Commands such as ``perf``, ``homi`` and ``qublk``
are separate programs, which ``xnvme`` looks up on its exec path,
the install directory unless ``XNVME_EXEC_PATH`` names others;
``xnvme env library`` shows it as ``exec_path``, and ``xnvme --exec-path``
prints it alone, for scripts.

``xnvme env`` says, as YAML, whether the host is ready for the userspace
drivers: the IOMMU groups, whether each kernel module is available, built in
and loaded, the hugepages of each size and NUMA node, where hugetlbfs is
mounted, memlock, the device nodes, the NVMe controllers by driver, and the
queues of the kernel ``nvme`` driver, each with a ``fix`` where something is
missing. For the kernel driver that is ``poll_queues``: polled I/O through
``io_uring`` needs it above zero, which takes a module option and a reload of
``nvme``, and a reload drops every NVMe device on the host, so ``xnvme`` says
how rather than doing it. It then prints the library: the
version, the build configuration and the backends. ``xnvme env system`` and
``xnvme env library`` print one of the two. It changes nothing.
The fixes name the ``hugepages`` and ``iommu`` tools, installed with
``pipx install hugepages iommu``.

``xnvme list`` shows the NVMe controller functions on the PCI bus, found with
``xnvme_scan()``, as YAML: the driver each is on and its IOMMU group, read from
sysfs without opening anything. ``xnvme driver --be <backend> <bdf>`` moves a
controller to the driver the backend needs, and prints the outcome as YAML,
each controller with the driver it is on, whether that changed, and why not.
A backend driving the controller itself, ``spdk``, ``libvfn`` or ``upcie``,
needs a userspace driver: on Linux ``vfio-pci``, or ``uio_pci_generic``
without an IOMMU, and ``vfio-pci`` alone for ``libvfn``; on FreeBSD,
``nic_uio``. Every other backend needs the kernel ``nvme`` driver, so
``--be io_uring`` returns a controller to it. Only the named controller is
rebound, and ``--all`` takes every one not in use. Bare ``xnvme driver``
prints ``xnvme list`` and then ``xnvme env``. One whose
namespaces are mounted, held by device-mapper, md or LVM, used as swap or open
in a process is refused, as is one sharing its IOMMU group with a device on
another driver; one whose namespace carries a filesystem or partition table is
refused unless ``--force``. The binding is done by the platform, through
``xnvme_driver_for_be()``, ``xnvme_driver_attach()`` and
``xnvme_driver_detach()`` in libxnvme, and is not
supported on macOS and Windows. Nothing is recorded on the side: what is
shown is what sysfs says, and whether a controller is in use is checked when
it is to move, which ``--dry-run`` does without moving it.

``xnvme cmd-admin``, ``cmd-io`` and ``cmd-iov`` send a raw command, and take
the options of ``nvme admin-passthru`` and ``nvme io-passthru``, the same long
names and the same letters, so a command line moves between the two by
changing the program and the subcommand. The device is any **xNVMe** URI, so
the same command reaches a controller on a userspace driver, which
``nvme-cli`` cannot:

.. code-block:: bash

   nvme  admin-passthru /dev/nvme0    --opcode=0x06 --cdw10=1 --data-len=4096 -r
   xnvme cmd-admin      /dev/nvme0n1  --opcode=0x06 --cdw10=1 --data-len=4096 -r
   xnvme cmd-admin      0000:03:00.0  --opcode=0x06 --cdw10=1 --data-len=4096 -r --be upcie

The options choosing the path, ``--be``, ``--admin``, ``--sync``, ``--async``,
``--dev-nsid`` and ``--homi-id``, are long options only. ``cmd-iov`` sends the
data in ``--vec-cnt`` equal segments. ``--timeout`` and ``--output-format json``
are refused rather than ignored.

The admin commands with their output decoded, Identify, the log pages,
features, format, sanitize, FDP and the resets, are ``xnvme test adm``, which
the tests run through every backend. For a controller on the kernel driver,
``nvme-cli`` is the tool for these; ``xnvme cmd-admin`` sends any of them raw,
on any backend.

The installed test programs are listed and run the same way:

.. literalinclude:: xnvme_test.cmd
   :language: bash

.. literalinclude:: xnvme_test.out
   :language: bash

``xnvme --help`` prints the same overview.

The bash completion, installed with ``xnvme``, asks it what to complete:
``xnvme --list-cmds`` prints the commands and the installed programs,
``xnvme --list-cmds=<group>`` those of a group such as ``test``, and each
program prints its subcommands with ``--list-cmds``. It therefore completes what
is installed, and the man pages, ``man xnvme`` and ``man xnvme-<command>``, are
generated from the same ``--help`` output shown here.

Commands
========

.. toctree::

   list/index
   info/index
   enum/index
   env/index
   driver/index
   cmd/index
