![xNVMe Logo](/docs/_static/xnvme-logo-medium.png)

xNVMe: cross-platform libraries and tools for NVMe devices
============================================================

[![CI](https://github.com/OpenMPDK/xNVMe/workflows/linux-binaries-test/badge.svg)](https://github.com/OpenMPDK/xNVMe/actions/)
[![CI](https://github.com/OpenMPDK/xNVMe/workflows/linux-build-test/badge.svg)](https://github.com/OpenMPDK/xNVMe/actions/)
[![CI](https://github.com/OpenMPDK/xNVMe/workflows/dockerize/badge.svg)](https://github.com/OpenMPDK/xNVMe/actions/)
[![CI](https://github.com/OpenMPDK/xNVMe/workflows/style/badge.svg)](https://github.com/OpenMPDK/xNVMe/actions/)
[![Coverity](https://scan.coverity.com/projects/xNVMe/badge.svg)](https://scan.coverity.com/projects/xNVMe)

See: https://xnvme.io/docs for documentation

- xNVMe, base NVMe specification (1.4) available as library and CLI `xnvme`
  - Memory Management
  - NVMe command interface
    | Synchronous commands
    | Asynchronous commands
  - Helpers / convenience functions for common operations
  - CLI-library for convenient derivative work
  - Multiple backend implementations
    | Linux + FreeBSD SPDK
    | FreeBSD IOCTL
    | Linux IOCTL
    | Linux IOCTL + io_uring async. IO
- libxnvme, base NVMe Specification available as library and CLI `xnvme`
- liblblk, NVM optional command-set / extension Specification `lblk`
- libznd, ZNS Specification available as library and CLI `zoned`
- libkvs, SNIA KV API implemented [TODO]
- libocssd, Open-Channel 2.0 support [TODO]
- libWHATEVERYOUWANT, Go ahead and implement what your need [TODO]

Contact and Contributions
=========================

xNVMe: is in active development and maintained by Simon A. F. Lund
<simon.lund@samsung.com>, pull requests are most welcome.
