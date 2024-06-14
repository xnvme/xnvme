.. _sec-toolchain-freebsd:

FreeBSD
-------

FreeBSD (13)
~~~~~~~~~~~~

Install the required toolchain and libraries, with sufficient system privileges
(e.g. as ``root`` or with ``sudo``), by executing the commands below. You can
run this from the root of the **xNVMe** by invoking::

  sudo ./xnvme/toolbox/pkgs/freebsd-13.sh

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/freebsd-13.sh
   :language: bash
   :lines: 8-

.. note::
   A Docker-image is provided via ``ghcr.io``, specifically
   ``ghcr.io/xnvme/xnvme-deps-freebsd-13:next``. This Docker-image contains
   all the software described above.

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


.. note::
   Interfaces; libaio, liburing, and libvfn are not supported on FreeBSD.


