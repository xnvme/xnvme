.. _sec-tutorials-devs-linux:

Development Environment
=======================

**xNVMe** is developed and tested in `xnvme-devenv`_'s host container: a
container with **QEMU**, **CIJOE**, the vfio-user device servers and the rigs,
the PCIe topologies a guest boots with. The guest runs **xNVMe**, built from
your tree, against emulated NVMe devices. The **CI** runs its emulated jobs in
the same container, so a job and your own run boot the same guest on the same
rig.

What you need on your machine:

* Linux on x86-64 with **KVM**, that is ``/dev/kvm``
* Docker or Podman
* Room on disk: the staged guest images are some 7 GiB each, and the guests
  and their NVMe backing images take tens of GiB more

Guests
------

The guest to pick depends on what you work on. Debian 13 is kept
upstream-pure, a stock kernel with in-tree drivers only, so plain NVMe work
runs on a kernel users run, without bleeding-edge surprises.
The xnvme-devenv Ubuntu 26.10 guest carries what upstream does not have yet:
a patched kernel, safl/linux 7.3.0-xnvme2, and drivers built for it, matched
to the device servers in the host container of the same release.

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Work on
     - ``DEVENV_GUEST``
     - ``RIG``
   * - NVMe/PCIe, NVMe/TCP
     - ``debian-trixie-iommu_disabled``, ``debian-trixie-iommu_enabled`` or
       ``freebsd-14``
     - the default, ``storage-node-emulated homi-queues``
   * - NVMe/PCIe on a drive of your machine
     - ``debian-trixie-iommu_enabled``, ``freebsd-14`` or ``windows-11``
     - none; ``PASSTHRU=<bdf>`` names the drive
   * - NVMe/RDMA
     - ``ubuntu-2610-rocm``
     - ``rdma-node-emulated``

A rig is named for the machine it stands in for and where its devices come
from. ``RIG`` takes a list of rig names or files, each one more **CIJOE**
config layered over the one before, so a later one changes a detail of an
earlier. A name is looked up in ``cijoe/rigs/``, then in the host container's
rigs; ``homi-queues``, in ``cijoe/rigs/``, gives one controller of
``storage-node-emulated`` the queue count the homi tests need.

``PASSTHRU=<bdf>`` passes the drive at that address on your machine through to
the guest with vfio-pci, which takes it from the host: the guest boots on
``storage-node-passthru``, and the tests use the ``[[devices]]`` of
``cijoe/rigs/passthru-<debian|freebsd|windows>.toml``, which name the drive as
the guest sees it. The host needs its IOMMU on, and a drive it has mounted is
refused. The **CI**'s MAAS jobs and benchmarks boot their guests this way::

  make devenv-up DEVENV_GUEST=debian-trixie-iommu_enabled PASSTHRU=0000:02:00.0

.. _sec-tutorials-devs-linux-reproduce-ci:

Developing in a guest
---------------------

The ``devenv-*`` make targets keep one host container running, with this tree
mounted, so a guest boots once and stays up while you change, rebuild and test
against it::

  make devenv-up DEVENV_GUEST=debian-trixie-iommu_disabled
  make devenv-sync
  make devenv-test TESTS='-k test_verify tests/logic/test_xnvme_tests_ioworker.py'
  make devenv-down

* ``devenv-up`` starts the container, unless it is running, then the guest on
  its rig, and provisions this tree into it: dependencies, a clean build, the
  Python bindings and fio. ``DEVENV_GUEST`` is one of the guests ``make
  guest-select`` lists, and defaults to the last selected; ``RIG`` defaults to
  the storage rig. The other targets use the rig the guest booted with, unless
  given ``RIG``.

* ``devenv-sync`` updates the guest's source with this tree and rebuilds only
  what changed, in the build directory the provision left, then installs; a
  change to one source file takes seconds. A file deleted from the tree stays
  in the guest; ``devenv-provision`` starts over from a clean build.

* ``devenv-test`` runs the tests ``TESTS`` selects, as pytest arguments, or
  without it the guest's test task, as ``guest-test`` does, with
  ``TRANSPORT`` and ``TEST_STEPS`` passed through. ``TESTS`` replaces the
  task's own selection, so add what it leaves out for the guest, such as
  ``-k "not fabrics and not libvfn"`` on a guest without an IOMMU. ``TESTS``
  is for the Linux guests; on FreeBSD, run the guest's test task.

* ``devenv-docgen`` generates the documentation in the guest, as the
  ``docgen`` job does, from the source the guest has, so run ``devenv-sync``
  first. It installs xNVMe's documentation tooling in the container before it
  starts.

* ``devenv-shell`` opens a shell in the host container, and ``devenv-ssh`` an
  SSH session in the guest, as root, on the port the container publishes the
  guest's SSH on.

* ``devenv-down`` stops the guest and removes the container.

The container's home is ``DEVENV_HOME`` (default ``.devenv`` in this tree),
which keeps **CIJOE** and the guest across ``devenv-down``. The staged guest
images are in ``DEVENV_CACHE`` (default ``~/.cache/xnvme-devenv``), which every
checkout's environment shares, so point it at a disk that has the room.
``DEVENV_ENGINE=podman`` runs it with Podman instead of Docker, and
``DEVENV_IMAGE`` picks another host container, by default the one the **CI**
jobs use. Each checkout gets an environment of its own, named for its
directory (``DEVENV_NAME``), so several, such as one per worktree, run side by
side on one machine.

The ``guest-*`` targets the ``devenv-*`` ones run can also be run by hand in
the container, ``make devenv-shell``, for instance ``make verify-guest`` for a
whole provision and test run as a **CI** job does it, ``make docgen-guest``
for the documentation, or ``make guest-bench BENCH=latency`` (or ``bdev``) on a
guest with a drive passed through, for a benchmark as the benchmark jobs run
it.

Testing on a machine of your own
--------------------------------

To build and test **xNVMe** on a machine rather than in a guest, such as a
lab machine with real NVMe drives, describe it in a **CIJOE** config: copy the
one in ``cijoe/configs/`` that is closest, and change its SSH login, its
``[[devices]]`` and its paths. Then build the source archives and run the
provisioning and a test task against it, as the ``verify-physical`` job
does with its macOS machine::

  make gen-artifacts ALLOW_DIRTY=1
  cd cijoe
  cijoe tasks/provision-using-tgz.yaml --config mymachine.toml \
    --config configs/fio.toml --config configs/xnvme.toml
  cijoe tasks/test-debian-trixie-iommu_enabled-pcie.yaml --config mymachine.toml \
    --config configs/fio.toml --config configs/xnvme.toml

``provision-using-tgz.yaml`` copies the archives over, then builds and
installs **xNVMe** on the machine; it boots nothing, so it needs neither the
host container nor a rig.

.. _xnvme-devenv: https://github.com/xnvme/xnvme-devenv
