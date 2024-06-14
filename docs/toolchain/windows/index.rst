.. _sec-toolchain-windows:

Windows
-------

Windows (2022)
~~~~~~~~~~~~~~


Install the necessary toolchain and libraries with appropriate system privileges
(e.g., in an elevated PowerShell session) by executing the script::

  .\xnvme\toolbox\pkgs\windows-2022.ps1

Or, run the commands contained within the script manually:

.. literalinclude:: ../../../toolbox/pkgs/windows-2022.ps1
   :language: powershell

Then go ahead and configure, build and install using ``meson``:

.. literalinclude:: ../../../toolbox/pkgs/default-build.sh
   :language: bash
   :lines: 2-


.. note::
   In case you see ``.dll`` loader-errors, then check that the environment
   variable ``PATH`` contains the various library locations of the toolchain.
   Interfaces; libaio, liburing, libvfn, and SPDK are not supported on
   Windows.



