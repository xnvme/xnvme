.. _sec-toolchain-freebsd:

FreeBSD
-------

FreeBSD (14)
~~~~~~~~~~~~

Install the required toolchain and libraries by running the package installation
script  provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as ``root`` or with
``sudo``):

.. code-block:: bash

  sudo ./xnvme/toolbox/pkgs/freebsd-14.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/freebsd-14.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-freebsd-14:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


.. note::
   Interfaces; libaio, liburing, and libvfn are not supported on FreeBSD.


FreeBSD (15)
~~~~~~~~~~~~

Install the required toolchain and libraries by running the package installation
script  provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as ``root`` or with
``sudo``):

.. code-block:: bash

  sudo ./xnvme/toolbox/pkgs/freebsd-15.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/freebsd-15.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-freebsd-15:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


.. note::
   Interfaces; libaio, liburing, and libvfn are not supported on FreeBSD.


