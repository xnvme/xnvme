<!--
SPDX-FileCopyrightText: Samsung Electronics Co., Ltd

SPDX-License-Identifier: BSD-3-Clause
-->

![xNVMe Logo](/docs/_static/xnvme-logo-medium.png)

xNVMe: cross-platform libraries and tools for NVMe devices
==========================================================

[![CI](https://github.com/xnvme/xnvme/workflows/verify/badge.svg)](https://github.com/xnvme/xnvme/actions/)
[![Coverity](https://scan.coverity.com/projects/xNVMe/badge.svg)](https://scan.coverity.com/projects/xNVMe)
[![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit&logoColor=white)](https://github.com/pre-commit/pre-commit)
[![REUSE status](https://api.reuse.software/badge/github.com/xnvme/xnvme)](https://api.reuse.software/info/github.com/xnvme/xnvme)

See: https://xnvme.io/ for documentation

xNVMe provides APIs, libraries, and tools for user space programming of
NVMe devices on Linux, FreeBSD, macOS, and Windows, through a single C
API backed by the `libxnvme` library. The library carries backends
utilizing and interfacing with the following, chosen at runtime, so the
same application code runs unmodified across them:

- Linux: `io_uring`, `io_uring_cmd`, `libaio`, block-layer IOCTLs
- FreeBSD, macOS, Windows: the platform's native NVMe driver
- `SPDK`, the popular user space NVMe driver on Linux and FreeBSD, for
  CPU-initiated I/O
- `uPCIe`, a blazingly fast user space PCIe driver on Linux, with P2P data
  movement and support for CPU-initiated, Accelerator-assisted, and
  Accelerator-initiated I/O; currently compatible with CUDA and HIP (ROCm)

On top of `libxnvme`:

- Command sets: `libxnvme_nvm` (NVM), `libxnvme_znd` (Zoned), `libxnvme_kvs` (KV)
- CLI tools: `xnvme`, `lblk`, `zoned`, `kvs`, `xdd`
- `xnvmeperf`, synthetic workload generation with minimal overhead, to
  benchmark I/O processing at unprecedented efficiency across all of
  xNVMe's backends
- `homi`, the software equivalent of SR-IOV: a daemon enabling multiple
  users, different processes on the host as well as compute kernels on
  accelerators, to run I/O directly against a single NVMe controller
- `qublk`, a `ublk`-server implementation enabling kernel I/O and file
  systems to run in concert with the blazingly fast user space NVMe
  drivers

Contact and Contributions
=========================

xNVMe: is in active development and maintained by Simon A. F. Lund <simon.lund@samsung.com>, pull
requests are most welcome. See, CONTRIBUTORS.md for a list of contributors to the current and
previous versions of xNVMe. For a contributor-guidelines then have a look at the online
documentation:

* Web: https://xnvme.io/contributing/

* Join us on Discord: https://discord.gg/XCbBX9DmKf
