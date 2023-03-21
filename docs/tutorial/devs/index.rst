.. _sec-tutorials-devs:

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

* Custom Linux kernel

  * Built from source
  * Installed via ``.deb``

* Custom Qemu

  * Built from source
  * Latest NVMe emulation features

* xNVMe

  * Built from source
  * Testing package

.. _sec-tutorials-devs-pm:

Physical Machine
----------------

It is assumes that you have a physisical machine with NVMe storage available,
and that you can utilize it destructively. By desctructively, what is meant, is
that any data can and will be lost.

Most machines will do for common development and testing, throughout an machine
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


.. _sec-tutorials-devs-os:

Operating System
----------------

Here, the latest **stable** Debian Linux, currently **bullseye** is used
throughout.

* Install it

  * Un-select desktop environment

  * Select OpenSSH

* Add user ``odus`` with password as you see fit
* Partition using the entire device

* Setup networking

And make sure it is accessible from your "main" development machine::

  ssh-copy-id odus@box01

Log into the machine::

  ssh odus@box01

And then install a couple of things::

  # Switch to root
  su -
  apt-get -qy update
  apt-get -qy install \
    git \
    htop \
    screen \
    sudo

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

Have a look at the :ref:`sec-gs-system-config-userspace-config` section for the
details on this.

.. _sec-tutorials-devs-homedir:

Homedir
-------

Create a directory structure in the ``$HOME`` directory::

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

.. _sec-tutorials-devs-cijoe:

CIJOE
-----

Setup ``python3`` and ``pipx``::

  sudo apt-get -qy install python3-pip python3-venv
  sudo python3 -m pip install pipx
  sudo python3 -m pipx ensurepath

Then install **cijoe** in a ``pipx`` virtual environment::

  pipx install cijoe --include-deps
  pipx inject cijoe cijoe-pkg-qemu
  pipx inject cijoe cijoe-pkg-linux

Then logout and back in to reload the environment, the
addition of ``pipx`` and the ``cijoe`` into ``$PATH``.

Do a trial-run::

  # Create a workdir
  mkdir -p ~/workdirs/cijoe
  cd ~/workdirs/cijoe

  # Create a default configuration and workflow
  cijoe --example core

In case everything is fine, then it will execute silently.

You can increase the information-level with ``-l``
argument, the more times you provide the higher the level.
Try running it with three, that is debug-level::

  cijoe -lll

In the ``cwd`` then a ``cijoe-output`` is produced, this
directory holds all information about what was executed.
Have a look at the generated report at
``cijoe-output/report.html``.

.. _sec-tutorials-devs-screen:

Screen + http.server
--------------------

Regardless of whether your **devbox** is physical/virtual/local/remote or some
combination thereof. Then having access to misc. files, and specifically, to
things like **cijoe** output / reports. Is very convenient.

With minimal fuss, then this is achievable with a combinaion of ``screen`` and
Python::

  cd ~/workdirs
  screen -d -m python3 -m http.server

The above starts a webserver, serving the content of the ``cwd`` where
``python3`` is executed and served up over ``tcp/http`` on port **8000**.

The ``screen -d -m`` part, creates a screen-session and detaches from it. Thus,
it continues executing even if you disconnect.

You can see the running screen-sessions with::

  screen -list

And attach to them using their ``<name>``::

  screen -r <name>

.. _sec-tutorials-devs-customkernel:

Linux Kernel
------------

Here are the steps to run the **cijoe** workflow, compiling a custom kernel as
a ``.deb`` package::

  # Create a workdir for the workflow
  mkdir -p ~/workdirs/linux
  cd ~/workdirs/linux

  # Grab the cijoe-example for linux
  cijoe --example linux

  # Run it with log-level debug (-lll)
  cijoe -lll

The above will fail as the required dependencies for building the kernel are
not available. Thus, to satisfy those, install::

  sudo apt-get -qy install \
    bc \
    bison \
    debhelper \
    flex \
    git \
    libelf-dev \
    libssl-dev \
    rsync

Then re-run the command above. It should now succeed, after which you can
collect the artifacts of interest::

  cp -r cijoe-output/artifacts/linux ~/artifacts/

You can install them by running::

  sudo dpkg -i ~/artifacts/linux/*.deb

Qemu
----

Checkout qemu::

  cd ~/git
  git clone https://github.com/OpenMPDK/qemu --recursive
  cd qemu
  git checkout for-xnvme
  git submodule update --init --recursive

Create a work-directory::

  mkdir -p ~/workdirs/qemu
  cd ~/workdirs/qemu

Run the **cijoe** qemu workflow::

  # Grab the config and workflow example for qemu
  cijoe --example qemu

  # Run it with log-level debug (-lll)
  cijoe -lll

Similarly, to how the built failed previously, then it will
fail here as well, and for the same reason; missing
packages. Thus, install the following to fix it::

  # Dependencies to build qemu
  sudo python3 -m pip install meson ninja
  sudo apt-get -qy install \
    libattr1-dev \
    libcap-ng-dev \
    libglib2.0-dev \
    libpixman-1-dev \
    libslirp-dev \
    pkg-config

  # Dependencies for cloud-init
  sudo apt-get -qy install \
    cloud-image-utils

With the packages installed, go back and run the **cijoe** workflow. Have a
look at the report, it describes what it does, that is, build and install qemu,
spin up a vm using a cloud-init-enabled Debian image, ssh into it.

.. tip::
   In case you get errors such as::

     Could not access KVM kernel module: No such file or directory
     qemu-system-x86_64: failed to initialize kvm: No such file or directory

   Then this is usually a symptom of virtualization being
   disabled in the BIOS of the physical machine. Have a look
   at ``dmesg`` it might proide messages supporting this.

xNVMe
-----

clone, build, and install
~~~~~~~~~~~~~~~~~~~~~~~~~

Clone **xNVMe** and checkout the ``next`` branch::

  cd ~/git
  git clone https://github.com/OpenMPDK/xNVMe.git xnvme
  cd xnvme
  git checkout next

Install dependencies::

  sudo ./toolbox/pkgs/debian-bullseye.sh

Additionally for development, then ``clang`` and ``clang-format`` are needed.
Unfortunately, in versions more recent than what is provided in the Debian
Bullseye repositories.

Install and setup clang by::

  mkdir ~/workdirs/clang
  cd ~/workdirs/clang

  # Some additional packages
  sudo apt-get -qy install \
    gnupg \
    lsb-release \
    software-properties-common \
    wget

  wget https://apt.llvm.org/llvm.sh
  chmod +x llvm.sh
  sudo ./llvm.sh 14

  sudo apt-get install clang-format-14
  sudo update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-14 14

Build and install **xNVMe**::

  cd ~/git/xnvme
  make
  sudo make install

Check that it is functional::

  sudo xnvme enum

This should yield output similar to::

  xnvmec_enumeration:
  - {uri: '/dev/nvme0n1', dtype: 0x2, nsid: 0x1, csi: 0x0, subnqn: ''}

Artifacts
~~~~~~~~~

Produce a set of **artifacts**::

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


cijoe-pkg-xnvme
~~~~~~~~~~~~~~~

Goto the **xNVMe** toolbox::

  cd ~/git/xnvme/toolbox/

Have a look around here.

Then build and install the **xNVMe** **cijoe** package::

  cd ~/git/xnvme/toolbox/cijoe-pkg-xnvme

  # Build the package
  make

  # Add it to the cijoe pipx-environment
  pipx inject cijoe dist/cijoe-pkg-xnvme-*.tar.gz

xNVMe: Workflows
----------------

Create a workdir with workflows from the repository::

  cp -r ~/git/xnvme/toolbox/workflow ~/workdirs/xnvme
  cd ~/workdirs/xnvme/

Provision a qemu-guest
~~~~~~~~~~~~~~~~~~~~~~

Setup a virtual machine with **xNVMe** installed, and a bunch of NVMe devices configured::

  cijoe -c configs/debian-bullseye.config -w provision.workflow

.. tip::
   It will likely fail with the error::

     /bin/sh: 1: /opt/qemu/bin/qemu-system-x86_64: not found

   This is because the default configuration is for running on Github. Thus,
   adjust the file ``configs/debian-bullseye.config`` such that qemu is
   pointing to ``$HOME``.
