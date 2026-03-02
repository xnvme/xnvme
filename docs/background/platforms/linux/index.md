(sec-platforms-linux)=

# Linux

(sec-platforms-linux-configuration)=

## System Configuration

Linux is one of several supported platforms. **xNVMe** is likely to work on most
Linux distributions, however to ensure compatibility we recommend using one of
the distributions listed in the {ref}`sec-toolchain` section.

To use the async. interfaces `libaio`, `io_uring`, and `io_uring_cmd`
they need to be enabled in the Linux Kernel. You can check for support by
running the command `cat /boot/config-$(uname -r)`. If the output includes
`CONFIG_AIO=y` and `CONFIG_IO_URING=y`, then there is support for `libaio`
and `io_uring`, respectively. Note, it is slightly more complicated to
check whether `io_uring_cmd` is supported. In general, you can assume it is
supported if your kernel version is 5.19 or newer and `io_uring` is supported.

(sec-platforms-linux-identification)=

## Device Identification

Linux exposes storage devices as files using the following naming schemes:

`/dev/nvme{ctrlr_num}`
: **NVMe** Controller as character device

`/dev/nvme{ctrlr_num}n{namespace_num}`
: **NVMe** Namespace as block device (Command-Sets: NVM, ZNS)

`/dev/ng{ctrlr_num}n{namespace_num}`
: **NVMe** Namespace as character device (Command-sets: Key-Value, ZNS-nonpo2, etc.)

`/dev/sd{a-z}`
: Block device representing multiple devices: **SCSI**, **SATA**,
  **USB**-Sticks, etc.

`/dev/hd{a-z}`
: **IDE** Drives (Harddisk, CD/DVD Drives)

(sec-platforms-linux-instrumentation)=

## Backend Configs

The Linux platform provides the following backend configs. Some of these use
**cross-platform
implementations** from the **Common Backend Implementations** (CBI). For these,
links point to descriptions in the CBI section. For the Linux-specific
interfaces, descriptions are available in the corresponding subsection.

* Memory Allocators

  - `posix`, Use **libc** `malloc()/free()` with `sysconf()` for
    alignment, add link to CBI
  - `hugepage`, Pre-allocated hugepage heap using uPCIe's `hostmem` library.
    Defaults to `memfd_create(MFD_HUGETLB)` (no filesystem mount required).
    Pages are pinned and physical addresses resolved at initialization.

    Environment variables:

    | Variable | Values | Default |
    |----------|--------|---------|
    | `XNVME_HUGEPAGE_BACKEND` | `memfd`, `hugetlbfs` | `memfd` |
    | `XNVME_HUGETLB_PATH` | filesystem path | `/mnt/huge` (implies `hugetlbfs`) |
    | `XNVME_HUGEPAGE_HEAP_SIZE` | bytes | `268435456` (256 MB) |

* Asynchronous Interfaces

  - `libaio`, Use Linux aio for Asynchronous I/O
  - `io_uring`, Use Linux io_uring for Asynchronous I/O
  - `io_uring_cmd`, Use Linux io_uring passthru command for Asynchronous I/O
  - `emu`, add link to CBI
  - `posix`, add link to CBI
  - `thrpool`, add link to CBI
  - `nil`, add link to CBI

* Synchronous Interfaces

  - `nvme`, Use Linux NVMe Driver ioctl() for synchronous I/O
  - `psync`, Use pread()/write() for synchronous I/O
  - `block`, Use Linux Block Layer ioctl() and pread()/pwrite() for I/O

* Admin Interfaces

  - `nvme`, Use Linux NVMe Driver ioctl() for admin commands
  - `block`, Use Linux Block Layer ioctl() and sysfs for admin commands

* Device Interfaces

  - `linux`, Use Linux file/dev handles and enumerate NVMe devices
  - `upcie`, User-space NVMe driver via direct PCIe BAR access and
    hugepage-based DMA. See {ref}`sec-backends-upcie`.

By default the Linux platform will use `emu` for async. emulation wrapping
around the synchronous Linux `nvme` driver ioctl-interface. This is done as
it has the broadest command-support. For efficiency, one can tap into using the
`io_uring` and `io_uring_cmd`.

### Examples

Thus, when using to get a handle to device, then pass the path to the device in
question, you can experiment by using the `xnvme` cli-tool. Such as:

```bash
    # Command
    xnvme info /dev/sda

    # Output
    xnvme_dev:
      xnvme_ident:
        uri: '/dev/sda'
        dtype: 0x3
        nsid: 0x1
        csi: 0x1f
        subnqn: ''
      xnvme_be:
        dev: {id: 'linux'}
        admin: {id: 'block'}
        sync: {id: 'block'}
        async: {id: 'emu'}
        mem: {id: 'posix'}
        attr: {name: 'emu_bdev', descr: 'Emulated async with block layer', caps: 0x6}
      xnvme_opts:
        be: 'emu_bdev'
        mem: 'posix'
        dev: 'linux'
        admin: 'block'
        sync: 'block'
        async: 'emu'
      xnvme_geo:
        type: XNVME_GEO_CONVENTIONAL
        npugrp: 1
        npunit: 1
        nzone: 1
        nsect: 15669248
        nbytes: 512
        nbytes_oob: 0
        tbytes: 8022654976
        mdts_nbytes: 262144
        lba_nbytes: 512
        lba_extended: 0
        ssw: 9
        pi_type: 0
        pi_loc: 0
        pi_format: 0
```

```{toctree}
:maxdepth: 2
:hidden:

ioctl
block
libaio
io_uring
io_uring_cmd
```
