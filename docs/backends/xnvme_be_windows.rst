.. _sec-backends-windows:

Windows
=======

The Windows backend communicates with the Windows NVMe driver StorNVMe.sys via IOCTLs.
The backend relies on the NVMe pass-through through IOCTLs to submit admin,
IO and arbitrary user-defined commands.

Currently Windows backend will work only with the inbox NVMe driver.

Toolchain Support:

Run the below mention batch file to install listed packages:

.. literalinclude:: ../../toolbox/pkgs/windows-2022.bat
   :language: bash

**xNVMe** can be build by using given build.bat,

.. include:: ../getting_started/build_windows.rst

Windows support two schemas ``dev`` and ``file``.
* ``dev:`` uses NVMe pass-through IOCTLs
* ``file`` uses Windows Read/Write file

  # Use the IOCTL implementation or fail
  xnvme info \\.\PhysicalDrive1

  # Use the file implementation or fail
  xnvme info \\.\PhysicalDrive1


Note on Errors
--------------

Some commands may be issued through ``ioctl`` that are invalid. While most
commands will still be issued to the drive and the error relayed back to the
user, some violations may be picked up by the kernel cause the ``ioctl`` to
fail with ``-1`` and ``GetLastError()`` set to ``Invalid Argument``.

In this case, the backend will transform such errors into the NVMe equivalent.
See the documentation on :ref:`xnvme_cmd_ctx <sec-api-c-xnvme_cmd-struct-xnvme_cmd_ctx>`.
