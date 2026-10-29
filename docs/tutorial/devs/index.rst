.. _sec-tutorials-devs-linux:

Linux Dev. Environment
======================

This shows the manual steps of setting up a physical machine for **xNVMe** test
and development. This includes using devices attached to the machine, as well
as using the physical machine to setup virtual machines with advanced
NVMe-device emulation.

Advanced NVMe device emulation covers command-set enchancements such as
Flexible Data-Placement (FDP), Zoned Namespaces (ZNS), and Key-Value SSDs (KV).

The documentation is provided as a means to setup development environment with
minimal fuss and maximum foss :)

* **xNVMe**

  * Built from source
  * Testing package

* CIJOE

  * Scripts and utilities for development, testing, and maintenance

* Custom Qemu

  * Built from source
  * Latest NVMe emulation features

* Custom Linux kernel

  * Built from source
  * Installed via ``.deb``

.. _sec-tutorials-devs-linux-pm:

Physical Machine
----------------

It is assumed that you have a physisical machine with NVMe storage available,
and that you can utilize it destructively. That is, that any data can and will
be lost when using the NVMe devices for testing.

Qemu is utilized extensively during testing to verify NVMe functionality of
NVM, ZNS, KV, FDP, etc. that is NVMe features not readiliy available in HW
is emulated via qemu. Thus, a physical machine is attractive reduce build and
runtime.

It is of course possible to do nested-virtualization, however, it is often
impractical as I/O get orders of magnitudes slower which less than ideal during
build.

Most machines will do for common development and testing, throughout a machine
with the following traits are used:

* An x86 64-bit CPU

* At least 8GB of memory

* A SATA SSD, 100GB will suffice, used for the

  * Operating system
  * User data (``$HOME``), repositories, workdirs, etc.

* One or more NVMe SSDs

  * Utilized for testing

This physical machine will be referred to as ``box01``.

.. tip::
   Since, the devices utilized for testing a primarily NVMe devices, then it is
   convenient to have the operating system and user-data installed on a
   separate non-NVMe device, quite simply since SATA pops up as ``/dev/sd*``
   and NVMe as ``/dev/nvme*`` / ``/dev/ng*``. It avoids one accidentally wiping
   the OS device when the devices are entirely separate.


.. _sec-tutorials-devs-linux-os:

Operating System
----------------

Here, the latest **stable** Debian Linux, currently **bookworm** is used
throughout.

* Install it

  * Un-select desktop environment

  * Select OpenSSH

* Add user ``odus`` with password as you see fit
* Partition using the entire device

* Setup networking

And make sure it is accessible from your "main" development machine:

.. code-block:: bash

  ssh-copy-id odus@box01

Log into the machine:

.. code-block:: bash

  ssh odus@box01

And then install a couple of things:

.. code-block:: bash

  # Switch to root
  su -
  apt-get -qy update && apt-get -qy install \
    git \
    htop \
    screen \
    pipx \
    sudo \
    vim

  # Make sure that cli-tools installed with pipx are available
  pipx ensurepath

  # Add odus to sudoers (required to do various things as non-root)
  usermod -aG sudo odus

  # Add odus to kvm (required to run qemu as non-root)
  usermod -aG kvm odus

  # switch back out of root
  exit

  # log out of odus
  exit

.. tip::
   Log out and back in again, to refresh credentials

Additionally, in order to prepare the system for user-space NVMe drivers, then
vfio/iommu should be enabled along with a couple of user-limit tweaks.

Have a look at the :ref:`sec-backends-spdk-config` section for the details on
this.

.. _sec-tutorials-devs-linux-homedir:

Homedir
-------

Regardless of whether you are using the **box** directly as ``root``, or using the
``odus`` user, then setup the ``$HOME`` directory like so:

.. code-block:: bash

  mkdir $HOME/{artifacts,git,workdirs,guests,images}

The directories are used for the following:

**git**
  A place to store source-repositories, usually these are git repositories for
  projects like: xnvme, fio, spdk, linux, and qemu.

**workdirs**
  A place for auxilary files, when executing **cijoe** workflows, or doing
  misc. experiments and exploration.

**artifacts**
  A place to store intermediate artifacts during development. Such as adhoc
  Linux kernel ``.deb`` packages, source-archives etc.

**guests**
  A place where boot-images, pid-files, cloud-seeds and other files related to
  qemu guests live.

**images**
  A place to store VM "boot-images", such as cloud-init enabled images.

.. _sec-tutorials-devs-linux-screen:

Screen + http.server
--------------------

Regardless of whether your **devbox** is physical/virtual/local/remote or some
combination thereof. Then having access to misc. files, and specifically, to
things like **cijoe** output / reports. Is very convenient.

With minimal fuss, then this is achievable with a combinaion of ``screen`` and
Python:

.. code-block:: bash

  cd ~/workdirs
  screen -d -m python3 -m http.server

The above starts a webserver, serving the content of the ``cwd`` where
``python3`` is executed and served up over ``tcp/http`` on port **8000**.

The ``screen -d -m`` part, creates a screen-session and detaches from it. Thus,
it continues executing even if you disconnect.

You can see the running screen-sessions with:

.. code-block:: bash

  screen -list

And attach to them using their ``<name>``:

.. code-block:: bash

  screen -r <name>

.. _sec-tutorials-devs-linux-cijoe:

xNVMe
-----

Clone, build, and install **xNVMe**:

.. code-block:: bash

  cd ~/git
  git clone https://github.com/OpenMPDK/xNVMe.git xnvme
  cd xnvme

Install prerequisites:

.. code-block:: bash

  sudo ./toolbox/pkgs/debian-bookworm.sh

Build and install **xNVMe**:

.. code-block:: bash

  cd ~/git/xnvme
  make
  sudo make install

Check that it is functional:

.. code-block:: bash

  sudo xnvme enum

This should yield output similar to:

.. code-block:: bash

  xnvme_cli_enumeration:
  - {uri: '/dev/nvme0n1', dtype: 0x2, nsid: 0x1, csi: 0x0, subnqn: ''}

cijoe
-----

Setup by running the following in the root of the **xNVMe** repository:

.. code-block:: bash

  cd ~/git/xnvme
  make guest-env

This will:

1. Install **cijoe** along with dependencies for Linux, qemu, and fio
2. Check that QEMU and KVM are available
3. Present an interactive menu to select a guest (e.g. ``debian-trixie``,
   ``freebsd-14``)

The selected guest is stored in ``cijoe/current.guest`` and used by all
subsequent ``make guest-*`` targets.

Then logout and back in to reload the environment, the addition of ``pipx`` and
the ``cijoe`` into ``$PATH``.

.. note::
   Make sure that ``pytest`` is not installed system-wide by running ``which
   pytest``. In case it says ``/usr/bin/pytest``, then it is not using the
   ``pytest`` provided with the **CIJOE** module. Thus, uninstall it using
   ``apt-get remove python3-pytest``.

Artifacts
~~~~~~~~~

Produce a set of **artifacts**:

.. code-block:: bash

  cd ~/git/xnvme
  make clobber gen-artifacts

  # Keep them handy if need be
  cp -r /tmp/artifacts ~/artifacts/xnvme

.. warning::
   The ``make clobber`` removes any unstaged changes and removes subprojects.
   This is done to ensure an entirely "clean" repository. Thus, make sure that
   you have commit your changes.
   The ``make clobber`` is required for ``make gen-artifacts``, as it will
   otherwise include side-effects from previous builds.

.. note::
   The artifacts produces by ``make gen-artifacts`` are output to
   ``/tmp/artifacts``. There are **cijoe** workflows, expecting to be available
   at that location, specifically the **provision** workflow.

.. _sec-tutorials-devs-linux-customkernel:

Linux Kernel
------------

Install prerequisites:

.. code-block:: bash

  # Essentials for building the kernel
  sudo apt-get -qy install \
    bc \
    bison \
    build-essential \
    debhelper \
    flex \
    git \
    libelf-dev \
    libssl-dev \
    pahole \
    rsync

  # A couple of extra libraries and tools
  sudo apt-get -qy install \
    libncurses-dev \
    linux-cpupower \
    python3-cpuinfo

.. note::
   ``libnurses-dev`` is needed for ``make menuconfig``. ``linux-cpupower``
   provides a cli-tool ``cpupower`` that let's you control the Linux CPU
   governor, useful for performance evaluation.

Then run the **cijoe** workflow, compiling a custom kernel as a ``.deb``
package:

.. code-block:: bash

  # Create a workdir for the workflow
  mkdir -p ~/workdirs/linux
  cd ~/workdirs/linux

  # Grab the cijoe-example for linux
  cijoe --example linux

  # Run it with logging (-l)
  cijoe -l

Then re-run the command above. It should now succeed, after which you can
collect the artifacts of interest:

.. code-block:: bash

  cp -r cijoe-output/artifacts/linux ~/artifacts/

You can install them by running:

.. code-block:: bash

  sudo dpkg -i ~/artifacts/linux/*.deb

.. _sec-tutorials-devs-linux-qemu:

Qemu
----

Install prerequisites:

.. code-block:: bash

  # Packages for building qemu
  sudo apt-get -qy install \
    meson \
    libattr1-dev \
    libcap-ng-dev \
    libglib2.0-dev \
    libpixman-1-dev \
    libslirp-dev \
    pkg-config

  # Packages for cloud-init
  sudo apt-get -qy install \
    cloud-image-utils

Checkout qemu:

.. code-block:: bash

  cd ~/git
  git clone https://github.com/qemu/qemu.git --recursive
  cd qemu
  git checkout v10.2.1
  git submodule update --init --recursive

Create a work-directory:

.. code-block:: bash

  mkdir -p ~/workdirs/qemu
  cd ~/workdirs/qemu

Run the **cijoe** qemu workflow:

.. code-block:: bash

  # Grab the config and workflow example for qemu
  cijoe --example qemu

  # Run it with log-level debug (-l)
  cijoe -l

With the packages installed, go back and run the **cijoe** workflow. Have a look
at the report, it describes what it does, that is, build and install qemu, spin
up a vm using a cloud-init-enabled Debian image, ssh into it.

.. tip::
   In case you get errors such as::

     Could not access KVM kernel module: No such file or directory
     qemu-system-x86_64: failed to initialize kvm: No such file or directory

   Then this is usually a symptom of virtualization being
   disabled in the BIOS of the physical machine. Have a look
   at ``dmesg`` it might proide messages supporting this.

Setup qemu-guest / virtual machine for testing
----------------------------------------------

Now that you have qemu built and installed. You can use it to emulate NVMe
devices in the guest for testing. The **xNVMe** ``Makefile`` has a bunch of
helper-targets to do this. That is, spinning up the guest, synchronizing your
**xNVMe** git repository changes into the qemu-guest, building, installing, and
running tests.

The ``make guest-env`` target will present an interactive menu to select the
guest OS (e.g. ``debian-trixie``, ``freebsd-14``). The same workflow applies
regardless of which guest you choose:

.. code-block:: bash

  # One-time setup: install CIJOE and select a guest
  make guest-env

  # Build source artifacts and run the full test cycle
  make gen-artifacts ALLOW_DIRTY=1 && make verify-guest

  # Or run stages individually:
  make guest-start          # start the guest
  make guest-provision   # sync source, build, install
  make guest-test        # run the test suite

To switch to a different guest, run ``make guest-select``.

Generate documentation
~~~~~~~~~~~~~~~~~~~~~~

You can reuse the qemu-guest to generate the documentation. See the
:ref:`sec-tutorials-devs-linux-reproduce-ci` section for using
``make docgen-guest`` inside the Docker container.



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
  without it the guest's test workflow, as ``guest-test`` does, with
  ``TRANSPORT`` and ``TEST_STEPS`` passed through. ``TESTS`` replaces the
  workflow's own selection, so add what it leaves out for the guest, such as
  ``-k "not fabrics and not libvfn"`` on a guest without an IOMMU. ``TESTS``
  is for the Linux guests; on FreeBSD, run the guest's test workflow.

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

Running pytest from the repository
----------------------------------

Invoke pytest providing a configuration file and an output directory for
artifacts and captured output:

.. code-block:: bash

  pytest \
    --config configs/debian-trixie.toml \
    --output /tmp/somewhere \
   tests

The ``--config`` is needed to inform pytest about the environment you are
running in such as which devices it can use for testing. The information is
utilized by pytest to, among other things, do parametrization for **xNVMe** backend
configurations etc.

Provision a qemu-guest
~~~~~~~~~~~~~~~~~~~~~~

Setup a virtual machine with **xNVMe** installed, and a bunch of NVMe devices
configured:

.. code-block:: bash

  cijoe -c configs/debian-trixie.toml provision.yaml

.. tip::
   It will likely fail with the error::

     /bin/sh: 1: /opt/qemu/bin/qemu-system-x86_64: not found

   This is because the default configuration is for running on Github. Thus,
   adjust the file ``configs/debian-trixie.toml`` such that qemu is
   pointing to ``$HOME``.

Remote dev
----------

Assuming your primary device for development is something like a Chromebook/
Macbook, something light-weight and great for reading mail... but now you want
to fire up your editor and do some development.

Or, your primary system is simply separate from the dev-box for a myriad of
reasons. Then, have a look at the existing **cijoe** configuration files in
``cijoe/configs/*.toml``, copy one that mathes your intended system, e.g.
use the configuration file for the qemu-guest as a starting point for a
configuration file for another physical machine:

.. code-block:: bash

  cp configs/debian-trixie.toml ~/.config/cijoe/cijoe-config.toml

Open up ``~/.config/cijoe/cijoe-config.toml`` and adjust it to your physical
machine. That is, change the ssh-login information, change the list of devices,
paths to binaries etc. Once you have done that, then go ahead and run:

.. code-block:: bash

  # Sync source, build, and install xNVMe on the remote end
  make guest-provision

  # Run the test suite
  make guest-test

You can do any of the ``make guest-*`` things like the above.