.. _sec-ci-runners-hetzner:

On Hetzner
==========

Qemu guests on MaaS from Hetzner **AX42**, using vfio-passthru for one of the
two NVMe SSDs on the host. Simultaneous Multithreading (SMT) disabled and six
cores pinned for the guest machine, two left for Linux and the GitHub runner.

Hostname naming convention for the guest:

* ``xnvme-ci-hetzner-bookworm-amd64-XX``

Runner naming convention:

* ``xnvme-ci-hetzner-bookworm-amd64-XX-runner-YY``

Installation of the bare-metal instances are done via the rescue system, which
you enable with the Hetzner Robot Console. To assist with the setup, scripts are
provided making this less of an effort.

System Properties
-----------------

With the following hardware properties:

.. list-table:: Hardware Specs. for Hetzner **AX42** MaaS.
   :widths: 30 15 15 40
   :header-rows: 1

   * - CPU
     - Memory
     - Motherboard
     - NVMe Devices
   * - AMD Ryzen 7 PRO 8700GE
     - 2x 32GB M324R4GA3BB0-CQKOD
     - AsRockRack B665D4U-1L
     - - 2x Samsung 980 PRO 512MB

And the following system software setup and configuration:

* One NVMe SSD has Debian Bookworm installed

  - System packages needed for CI, qemu, etc.
  - Docker Community Edition installed
  - GitHub Runner **unpacked**, but not **not** registered
  - BDF: ``0000:01:00.0``

* The ``vfio-pci`` Kernel module is loaded at boot

* Second NVMe SSD is bound to ``vfio-pci`` at boot using ``devbind`` script

  - BDF: ``0000:02:00.0``

* Members of the ``kvm`` group can use ``vfio``

* A ``ghr`` user is available for the GitHub Runner to execute as

  - ``ghr`` is a member of ``kvm``, thus, can run qemu with ``kvm`` and
    vfio-passthru

  - ``ghr`` has ``memlock unlimited`` and can thus launch qemu guest with a lot
    of memory

To install a system have a look at the following section and for details, then
look at the scripts and config files referred to there.

System bootstrap
----------------

Via the Hetzner Robot console:

* Make sure you have a sensible firewall-template applied
* Activate the rescue system
* Power reset the system

Once it is booted, log into the rescue system for the bare-metal machine, and
run the following to fetch the scripts and config files::

	# Grab the scripts and files
	curl -L https://github.com/xnvme/xnvme/archive/refs/heads/next.tar.gz \
		| tar --strip-components=5 -xz xnvme-next/docs/contributing/ci/runners

	# Run the install-script (replace XX with the id of the system e.g. 01)
	cd hetzner-setup
	./hetzner_setup.sh XX
	reboot

.. note::

   Replace ``XX`` with the numerically unique identifier of the system. This
   will be utilized to construct the hostname using the naming convention
   described above.

.. warning::

   The ssh-key utilized via the rescue system is added to ``authorized_keys``.
   Since it is not possible to **SSH** into the box as ``root`` with
   login/password, then only the owner of the key will be able to login.

The system will reboot after the above commands execute without error. The
contents of the ``hetzner-setup`` folder are copied into the system and will be
available after reboot.

Post-Install
------------

Once the system is up, **SSH** into it as ``root`` and run::

    # Run post-install tasks (user configuration, system setup, etc.)
    cd hetzner-setup
    ./postinstall.sh

    # Set password for the 'ghr' user
    passwd ghr

After this, complete the following tasks:

- Retrieve the Windows guest image
- Set up the GitHub Runner
- Reboot the system

Details are provided in the following sections.

Windows Guest Image
-------------------

Unlike the other guests, the Windows image cannot be distributed. Therefore,
you must manually retrieve it from the private Hetzner Storage Box. The
``postinstall.sh`` script has already prepared everything needed to launch
a Windows guest, including UEFI, OVMF, and TPM support.

Download the image using the command below, replacing your username with your
Storage Box ``<USERNAME>``::

  export HSB_USERNAME=<USERNAME>

  # Download the Windows image from Hetzner Storage Box
  wget \
    --http-user ${HSB_USERNAME} \
    --ask-password \
    --directory-prefix /home/ghr/system_imaging/disk/ \
    https://${HSB_USERNAME}.your-storagebox.de/win11.qcow2

  # Ensure access
  chown -R ghr:ghr /home/ghr/system_imaging/disk/

Once the image is in place, the system is ready to receive jobs from GitHub.
The next section explains how to install the runner.

.. note::

   The Windows guest image is 64 GB in size. Downloading it takes approximately
   10 minutes.


Runner Registration
-------------------

Switch to the ``ghr`` user::

	su - ghr

Setup env. vars. for config::

	export RUNNER_USER=ghr
	export URL=https://github.com/xnvme/xnvme
	export TOKEN={SUPER_SECRET}

.. note::

   You find the **TOKEN** in the GitHub web interface when adding a new runner
   under ``Settings / Runners / New self-hosted runner``.

Then run::

	cd actions-runner
	./config.sh --unattended \
		--url ${URL} \
		--token ${TOKEN} \
		--no-default-labels \
		--labels self-hosted,linux,amd64,maas \
		--replace \
		--name ${HOSTNAME}-runner-01

.. note::

   Currently, there is a ``1:1:1`` relationship between ``Host:Runner:Guest``.
   If this changes, adjust the runner name accordingly in the command above.

Install the runner with permission to use ``vfio-pci```::

	# Install the service
	sudo ./svc.sh install ghr
	sudo sed -i '/^\[Service\]/a LimitMEMLOCK=infinity' /etc/systemd/system/actions.runner.*.service
	sudo systemctl daemon-reload
	sudo ./svc.sh status

Do not start the service at this point, to avoid it getting a CI job. Instead,
``reboot`` the system to ensure that it boots and services start as expected.
