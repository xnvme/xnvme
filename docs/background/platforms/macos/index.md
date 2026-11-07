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

`/path/to/file`
: A regular file, opened by the `emu_file` and `thrpool_file` backend configs,
  and created when the open asks for it

(sec-platforms-macos-instrumentation)=

## Backend Configs

The macOS platform provides the following backend configs. Some of these use
**cross-platform
implementations**, each in a directory of its own under `lib/backend/`. For
these, links point to the cross-platform section. For the macOS-specific
interfaces, descriptions are available in the corresponding subsections.

* Memory Allocators

  - `posix`, Use **libc** `malloc()/free()` with `sysconf()` for
    alignment, see {ref}`sec-backends-common`

* Asynchronous Interfaces

  - `emu`, see {ref}`sec-backends-common-async-emu`
  - `posix`, see {ref}`sec-backends-common-async-posix`
  - `thrpool`, see {ref}`sec-backends-common-async-thrpool`

* Synchronous Interfaces

  - `macos`, Use pread()/write() for synchronous I/O
  - `psync`, Use pread()/write() on a regular file, see
    {ref}`sec-backends-common-sync-psync`

* Admin Interfaces

  - `macos`, Use macOS NVMe SMART Interface admin commands
  - `file_as_ns`, Answer admin commands for a regular file as a namespace

* Device Interfaces

  - `macos`, Use macOS file/dev handles and enumerate NVMe devices
