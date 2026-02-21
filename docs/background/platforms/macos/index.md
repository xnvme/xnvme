(sec-platforms-macos)=

# macOS

(sec-platforms-macos-configuration)=

## System Configuration

macOS is one of several supported platforms. **xNVMe** is likely to work on
multiple versions of macOS, however to ensure compatibility we recommend using
a version listed in the {ref}`sec-toolchain` section.

(sec-platforms-macos-identification)=

## Device Identifiers

macOS exposes NVMe devices as files using the following naming schemes:

`/dev/disk{ctrlr_id}`
: **NVMe** Controller device node

(sec-platforms-macos-instrumentation)=

## Backend Configs

The macOS platform provides the following backend configs. Some of these use
**cross-platform
implementations** from the **Common Backend Implementations** (CBI). For these,
links point to descriptions in the CBI section. For the macOS-specific
interfaces, descriptions are available in the corresponding subsections.

* Memory Allocators

  - `posix`, Use **libc** `malloc()/free()` with `sysconf()` for
    alignment, add link to CBI

* Asynchronous Interfaces

  - `emu`, add link to CBI
  - `posix`, add link to CBI
  - `thrpool`, add link to CBI

* Synchronous Interfaces

  - `macos`, Use pread()/write() for synchronous I/O

* Admin Interfaces

  - `macos`, Use macOS NVMe SMART Interface admin commands

* Device Interfaces

  - `macos`, Use macOS file/dev handles and enumerate NVMe devices
