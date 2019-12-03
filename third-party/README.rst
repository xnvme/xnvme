=============================
 xNVMe third-party directory
=============================

This file and directory contain information about the software on which
xNVMe depend. The licenses and links to project web-pages and sources are
provided and a brief summary of how xNVMe uses the third-party software.

SYSTEM:freebsd:/
  Summary: For the FreeBSD backend, that is, `XNVME_BE_FBSD`, then headers for
  `IOCTLs` are needed. These, are not provided within this repos but rather
  expected to be available on the system where xNVMe is built and as such not
  distributed with the library.

  License: BSD

  Repos: https://svnweb.freebsd.org/
  Website: https://www.freebsd.org/

SYSTEM::linux:/
  Summary: For the Linux backend, that is, `XNVME_BE_LINUX`, then headers for
  `IOCTLs` are needed. These, are not provided within the library but rather
  expected to be available on the system where xNVMe is built and as such not
  distributed with the library.

  License: GPL-2.0 WITH Linux-syscall-note
  https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/include/uapi/linux/nvme_ioctl.h?h=v5.3-rc8

  Repos: https://github.com/spdk/spdk
  Website: https://www.linuxfoundation.org/

REPOS/third-party/liburing
  Summary: For the xNVMe backend for Linux, then this git subodule is provided
  and tagged with the liburing version which xNVMe has been tested.

  License: LGPL

  Repos: https://git.kernel.dk/liburing
  Website: https://git.kernel.dk/cgit/liburing/

REPOS/include/bsd/sys/queue.h
  Summary: within the SGL code, queues / lists are needed. An implement of
  queues are used from FreeBSD. Specifically, "sys/queue.h". NOTE: this is
  currently dropped in verbatim at "include/bsd/sys/queue.h"

  License: BSD

  Repos: https://svnweb.freebsd.org/
  Website: https://www.freebsd.org/

REPOS/third-party/spdk/
  Summary: For the xNVMe backend using SPDK, then this git submodule is
  provided and tagged with the SPDK version on which xNVMe has been tested.

  License: BSD
  https://github.com/spdk/spdk/blob/master/LICENSE

  Repos: https://github.com/spdk/spdk
  Website: https://spdk.io/

REPOS/scripts/xnvme-driver.sh
  Summary: This script is based on the setup.sh and its associated scripts from
  SPDK. It is added here in a single-file for easy distribution with xNVMe.

  License: BSD
  https://github.com/spdk/spdk/blob/master/LICENSE

  Repos: https://github.com/spdk/spdk
  Website: https://spdk.io/

TODO
====

Provide `./configure` options to overrule the default search path for the Linux
/ FreeBSD headers. For whichever reason one might have to build against a
specific `IOCTL` header.
