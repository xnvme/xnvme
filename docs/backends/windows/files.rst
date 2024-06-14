.. _sec-backends-windows-files:

NVMe Driver and Regular File
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**xNVMe** can communicate with File System mounted devices via the operating
system generic APIs like **ReadFile** and **WriteFile** operations. This method
can be used to do operation on Regular Files.

You can check that this interface is behaving as expected by running:

.. literalinclude:: xnvme_win_info_fs.cmd
   :language: bash

Which should yield output equivalent to:

.. literalinclude:: xnvme_win_info_fs.out
   :language: bash
   :lines: 1-12

This tells you that **xNVMe** can communicate with the given regular file
and it informs you that it utilizes **nvme_ioctl** for synchronous command
execution and it uses **iocp** for asynchronous command execution. This method
can be used for file operations via **<driver name>:\<file name>** path.