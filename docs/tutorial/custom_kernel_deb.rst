Debian custom kernel package
============================

These are instructions for building a custom Linux Kernel as a Debian package on the system where it will be installed.

* The kernel will be built using the current Kernel configuration with minimal custom changes, you can add more.
* The kernel will be built as an installable Debian Package, this is conventient as it can easily be uninstalled again.
* The kernel is built from sources available in the folder ``$HOME/git/linux``.

Install pre-reqs::

	sudo apt-get install \
		bc \
		build-essential \
		cpio \
		flex \
		kmod \
		libelf-dev \
		libncurses5-dev \
		libssl-dev \
		linux-source \
		rsync \
		time

Base the kernel-configuration on the currently running kernel::

	cd ~/git/linux
	cp "/boot/config-$(uname -r)" .config

Edit the ``.config`` making sure that the following is set::

	CONFIG_SYSTEM_TRUSTED_KEYS = ""

And unless you need it, then disable debug info, with the option::

	CONFIG_DEBUG_INFO=n

It will significantly reduce the time it takes to build the kernel.
The build-time is about 8 minutes on a desktop platform based on AMD 5800x with a SATA SSD.
The build-time is about 15 minutes on a mobile platform based on AMD 5700u with a SATA SSD.
With ``CONFIG_DEBUG_INFO`` enabled, the time is closer to and beyond 60min.

Then go ahead and build::

	/usr/bin/time make -j`nproc` bindeb-pkg

The Debian packages are emitted in the parent directory, this can get messy.
So, go ahead and put the packages somewhere else, e.g.::

	mkdir - "$HOME/packages/build01"
	mv ../{*.deb,*.buildinfo,*.changes} "$HOME/packages/build01"

Go ahead and install it by invoking::

	sudo dpkg -i "$HOME/packages/build01/*.deb"

You organize the packages as you see fit, but something like the above is not a bad idea.