=============================
 xNVMe third-party directory
=============================

This file and directory contain information about the software on which
xNVMe depend. The licenses and links to project web-pages and sources are
provided and a brief summary of how xNVMe uses the third-party software.

REPOS/subprojects/spdk/
  Summary: For the xNVMe backend using SPDK, then this git submodule is
  provided and tagged with the SPDK version on which xNVMe has been tested.

  License: BSD
  https://github.com/spdk/spdk/blob/master/LICENSE

  Repos: https://github.com/spdk/spdk
  Website: https://spdk.io/

REPOS/subprojects/liburing
  Summary: For the xNVMe backend for Linux, then this git submodule is provided
  and tagged with the liburing version which xNVMe has been tested.

  License: LGPL

  Repos: https://git.kernel.dk/liburing
  Website: https://git.kernel.dk/cgit/liburing/

REPOS/subprojects/fio
  Summary: For the xNVMe fio IO engine, then this git submodule is provided
  and tagged with the fio version which xNVMe has been tested.

  License: GPL-2.0

  Repos: https://git.kernel.dk/fio
  Website: https://fio.readthedocs.io/en/latest/

REPOS/toolbox/xnvme-driver.sh
  Summary: This script is based on the setup.sh and its associated scripts from
  SPDK. It is added here in a single-file for easy distribution with xNVMe.

  License: BSD
  https://github.com/spdk/spdk/blob/master/LICENSE

  Repos: https://github.com/spdk/spdk
  Website: https://spdk.io/

SYSTEM:freebsd:/
  Summary: Headers for the FreeBSD Kernel NVMe driver IOCTLs are used by the
  `be:fioc`, then headers for backend. These headers are not distributed with
  xNVMe but rather expected to be available on the system where xNVMe is built.

  License: BSD

  Repos: https://svnweb.freebsd.org/
  Website: https://www.freebsd.org/

SYSTEM::linux:/
  Summary: Headers for the Linux Kernel NVMe driver IOCTLs are used by the
  ``be:lioc`` backend. These headers are not distributed with xNVMe but rather
  expected to be available on the system where xNVMe is built.

  License: GPL-2.0 WITH Linux-syscall-note
  https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/include/uapi/linux/nvme_ioctl.h?h=v5.3-rc8

  Repos: https://github.com/spdk/spdk
  Website: https://www.linuxfoundation.org/
