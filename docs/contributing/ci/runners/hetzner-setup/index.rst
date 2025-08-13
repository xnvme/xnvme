.. _sec-ci-runners-hetzner:

On Hetzner
==========

Qemu guests on MaaS from Hetzner **AX42**, using vfio-passthru for one of the
two NVMe SSDs on the host. SMT disabled and six cores pinned for the guest
machine, two left for Linux and the GitHub runner.

Hostname naming convention for the guest:

* ``xnvme-ci-hetzner-bookworm-amd64-XX``

Runner naming convention:

* ``xnvme-ci-hetzner-bookworm-amd64-XX-runner-YY``

Installation of the bare-metal instances are done via the Rescue system, which
you enable with the Hetzner Robot Console. To assist with the setup then a
scripts are provided making this less of an effort.

System Properties
-----------------

With the following hardware properties:

cpu
	AMD Ryzen 7 PRO 8700GE
mem
	64 GB
ssd
	2x Samsung 980 PRO 512MB

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
* Switch the system into rescue mode
* Power reset the system

Then, once it is booted then you can log into the rescue system for the
bare-metal machine, do so, and then run to following to fetch scripts and config
files::

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

Once the above is finished executing, without error, then the system will be
rebooted. Also, the contents of the folder ``hetzner-setup`` has been copied
into the system and is available once you boot the system. Thus, once the system
is up, then ssh into the system as ``root`` and run::

	cd hetzner-setup
	./postinstall.sh
	reboot

Once this has finished, then you can start using the system.

Runner Registration
-------------------

Switch to the ``ghr`` user::

	su - ghr
	cd actions-runner

Setup env. vars. for config::

	export RUNNER_USER=ghr
	export URL=https://github.com/xnvme/xnvme
	export TOKEN=AAEUTENDCL4VIOMKIZEAQW3ITXIWG

.. note::

   You find the **TOKEN** in the GitHub web interface.

Then run::

	./config.sh --unattended \
		--url ${URL} \
		--token ${TOKEN} \
		--no-default-labels \
		--labels self-hosted,linux,amd64,maas \
		--replace \
		--name ${HOSTNAME}-runner-01

.. note::

   Currently, then there is a ``1:1:1`` relation between ``Host:Runner:Guest``,
   however, should this change, then adjust the runner in the command above.

With the runner registered, then set it up as a service::

Now install the service, making it available upon reboot etc.::

	sudo ./svc.sh install ghr
	sudo ./svc.sh status
	sudo ./svc.sh start
	sudo ./svc.sh status

qemu-vfio-passthru
------------------

-device pcie-root-port,id=rp_nvme,chassis=2,slot=2 \
-device vfio-pci,host=0000:02:00.0,bus=rp_nvme