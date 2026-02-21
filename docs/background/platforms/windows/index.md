(sec-platforms-windows)=

# Windows

The Windows platform communicates with the Windows NVMe driver StorNVMe.sys via
IOCTLs. It relies on the NVMe pass-through through IOCTLs to submit
admin, IO and arbitrary user-defined commands.

## System Configuration

Windows is one of several supported platforms. **xNVMe** is likely to work on
multiple versions of Windows, however Windows 10 or later is recommended as it
has all the features which **xNVMe** utilizes. For tested versions, see the
{ref}`sec-toolchain` section.

## Device Identifiers

Windows exposes NVMe devices as files using the following naming schemes:

`\\.\PhysicalDrive{ctrlr_id}`
: **NVMe** Controller device node


## Backend Configs

The Windows platform provides the following backend configs. Some of these use
**cross-platform
implementations** from the **Common Backend Implementations** (CBI). For these,
links point to descriptions in the CBI section. For the Windows-specific
interfaces, descriptions are available in the corresponding subsections.

* Memory Allocators

  - `windows`, Use Windows memory allocator

* Asynchronous Interfaces

  - `iocp`, Use Windows readfile/writefile with overlapped(iocp) for Asynchronous I/O
  - `iocp_th`, Use Windows readfile/writefile with thread based overlapped(iocp) for Asynchronous I/O
  - `io_ring`, Use Windows io_ring for Asynchronous I/O
  - `emu`, add link to CBI
  - `thrpool`, add link to CBI

* Synchronous Interfaces

  - `nvme`, Use Windows NVMe Driver ioctl() for synchronous I/O
  - `file`, Use Windows File System APIs and readfile()/writefile() for I/O

* Admin Interfaces

  - `nvme`, Use Windows NVMe Driver ioctl() for admin commands
  - `block`, Use Windows NVMe Driver ioctl() for admin commands
  - `file`, Use Windows File System APIs for admin commands

* Device Interfaces

  - `windows`, Use Windows file/dev handles and enumerate NVMe devices


```{toctree}
:maxdepth: 2
:hidden:

files
ioctl
iocp
iocp_th
io_ring
```
