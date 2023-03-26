<!--
SPDX-FileCopyrightText: Samsung Electronics Co., Ltd

SPDX-License-Identifier: BSD-3-Clause
-->

![xNVMe Logo](/docs/_static/xnvme-logo-medium.png)

xNVMe: cross-platform libraries and tools for NVMe devices
============================================================

[![CI](https://github.com/OpenMPDK/xNVMe/workflows/linux-binaries-test/badge.svg)](https://github.com/OpenMPDK/xNVMe/actions/)
[![CI](https://github.com/OpenMPDK/xNVMe/workflows/linux-build-test/badge.svg)](https://github.com/OpenMPDK/xNVMe/actions/)
[![CI](https://github.com/OpenMPDK/xNVMe/workflows/style/badge.svg)](https://github.com/OpenMPDK/xNVMe/actions/)
[![Coverity](https://scan.coverity.com/projects/xNVMe/badge.svg)](https://scan.coverity.com/projects/xNVMe)
[![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit&logoColor=white)](https://github.com/pre-commit/pre-commit)
[![REUSE status](https://api.reuse.software/badge/github.com/OpenMPDK/xNVMe)](https://api.reuse.software/info/github.com/OpenMPDK/xNVMe)

See: https://xnvme.io/docs for documentation

- xNVMe, base NVMe specification (1.4) available as library and CLI `xnvme`
  - Memory Management
  - NVMe command interface
    | Synchronous commands
    | Asynchronous commands
  - Helpers / convenience functions for common operations
  - CLI-library for convenient derivative work
  - Multiple backend implementations
    | Linux SPDK
    | Linux IOCTL
    | Linux io_uring
    | Linux libaio
    | FreeBSD SPDK
    | FreeBSD IOCTL
- `libxnvme`, base NVMe Specification available as library and via CLI `xnvme`
- `libxnvme_nvm`, The NVM Commands Set
- `libxnvme_znd`, The Zoned Command Set available as a library and via CLI `zoned`
- `libkvs`, SNIA KV API implemented [TODO]
- `libocssd`, Open-Channel 2.0 support [TODO]
- `libWHATEVERYOUWANT`, Go ahead and implement what you need [TODO]

Contact and Contributions
=========================

xNVMe: is in active development and maintained by Simon A. F. Lund <simon.lund@samsung.com>, pull
requests are most welcome. See, CONTRIBUTORS.md for a list of contributors to the current and
previous versions of xNVMe. For a contributor-guidelines then have a look at the online
documentation:

* Web: https://xnvme.io/docs/latest/contributing/

* Join us on Discord: https://discord.gg/XCbBX9DmKf
