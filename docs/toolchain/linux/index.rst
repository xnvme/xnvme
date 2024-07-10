.. _sec-toolchain-linux:

Linux
-----

Alpine Linux (latest)
~~~~~~~~~~~~~~~~~~~~~

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/alpine-latest.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/alpine-latest.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-alpine-latest:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


.. note::
   There are issues with SPDK/DPDK due to incompatibilities with the standard
   library provided by ``musl libc``. Additionally, the ``libexecinfo-dev``
   package is no longer available on Alpine.
   Additionally, libvfn also relies on ``libexecinfo-dev`` which is currently
   not available for Alpine Linux. Thus, it is also disabled.
   Pull-request fixing these issues are most welcome, until then, disable
   libvfn and spdk on Alpine.



Arch Linux (latest)
~~~~~~~~~~~~~~~~~~~

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/archlinux-latest.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/archlinux-latest.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-archlinux-latest:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


.. note::
   The build is configured to install with ``--prefix=/usr`` this is
   intentional such the the ``pkg-config`` files end up in the default search
   path on the system. If you do not want this, then remove ``--prefix=/usr``
   and adjust your ``$PKG_CONFIG_PATH`` accordingly.



Oracle Linux (9)
~~~~~~~~~~~~~~~~

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/oraclelinux-9.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/oraclelinux-9.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-oraclelinux-9:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-



Rocky Linux (9.2)
~~~~~~~~~~~~~~~~~

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/rockylinux-9.2.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/rockylinux-9.2.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-rockylinux-9.2:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-



CentOS Stream 9 (stream9)
~~~~~~~~~~~~~~~~~~~~~~~~~

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/centos-stream9.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/centos-stream9.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-centos-stream9:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-



Debian Testing (trixie)
~~~~~~~~~~~~~~~~~~~~~~~

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/debian-trixie.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/debian-trixie.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-debian-trixie:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-



Debian Stable (bookworm)
~~~~~~~~~~~~~~~~~~~~~~~~

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/debian-bookworm.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/debian-bookworm.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-debian-bookworm:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-



Debian Oldstable (bullseye)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/debian-bullseye.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/debian-bullseye.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-debian-bullseye:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-



Fedora (41)
~~~~~~~~~~~

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/fedora-41.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/fedora-41.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-fedora-41:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-



Fedora (40)
~~~~~~~~~~~

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/fedora-40.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/fedora-40.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-fedora-40:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-



Fedora (39)
~~~~~~~~~~~

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/fedora-39.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/fedora-39.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-fedora-39:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-



Ubuntu Latest (lunar)
~~~~~~~~~~~~~~~~~~~~~

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/ubuntu-lunar.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/ubuntu-lunar.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-ubuntu-lunar:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


.. note::
   All tools and libraries are available via system package-manager.


Ubuntu LTS (jammy)
~~~~~~~~~~~~~~~~~~

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/ubuntu-jammy.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/ubuntu-jammy.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-ubuntu-jammy:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


.. note::
   Installing liburing from source and meson + ninja via pip


Ubuntu LTS (focal)
~~~~~~~~~~~~~~~~~~

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/ubuntu-focal.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/ubuntu-focal.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-ubuntu-focal:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


.. note::
   Installing liburing from source and meson + ninja via pip


Gentoo (latest)
~~~~~~~~~~~~~~~

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/gentoo-latest.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/gentoo-latest.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-gentoo-latest:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


.. note::
   In case you get ``error adding symbols: DSO missing from command line``,
   during compilation, then add ``-ltinfo -lnurces`` to ``LDFLAGS`` as it is
   done in the commands above.
   The build is configured to install with ``--prefix=/usr`` this is
   intentional such the the ``pkg-config`` files end up in the default search
   path on the system. If you do not want this, then remove ``--prefix=/usr``
   and adjust your ``$PKG_CONFIG_PATH`` accordingly.



openSUSE (tumbleweed-latest)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/opensuse-tumbleweed-latest.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/opensuse-tumbleweed-latest.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-opensuse-tumbleweed-latest:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


.. note::
   All tools and libraries are available via system package-manager.


