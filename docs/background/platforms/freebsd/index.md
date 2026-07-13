(sec-platforms-freebsd)=

# FreeBSD

(sec-platforms-freebsd-configuration)=

## System Configuration

FreeBSD is one of several supported platforms. **xNVMe** is likely to work on
multiple versions of FreeBSD, however to ensure compatibility we recommend using
a version listed in the {ref}`sec-toolchain` section.

(sec-platforms-freebsd-contigmem)=

### User-space DMA memory (`contigmem`)

The `spdk` backend gets its DMA memory from the `contigmem` kernel module,
which the `xnvme-driver` script loads. It pre-allocates a pool of
physically-contiguous buffers, `CONTIGMEM_BUFSZ` MB each (`256` by default), up
to a total of `HUGEMEM` MB (`2048` by default). The pool holds at most 64
buffers, so `HUGEMEM / CONTIGMEM_BUFSZ` must not exceed 64.

`CONTIGMEM_BUFSZ` is a hard upper bound on a single DMA allocation: a buffer
cannot span multiple `contigmem` buffers, so `xnvme_buf_alloc()` cannot return
more than `CONTIGMEM_BUFSZ` MB no matter how much of the pool is free.

On a fragmented heap, a large physically-contiguous buffer can be impossible to
place even when plenty of memory is free, and loading `contigmem` then fails
with `contigmalloc failed for buffer N` in `dmesg`. Smaller buffers are easier
to place, so lowering `CONTIGMEM_BUFSZ` avoids the failure:

```bash
CONTIGMEM_BUFSZ=64 xnvme-driver
```

(sec-platforms-freebsd-identification)=

## Device Identification

FreeBSD exposes NVMe devices as files using the following naming schemes:

`/dev/nvme{ctrlr_id}`
: **NVMe** Controller device node

`/dev/nvme{ctrlr_id}ns{ns_id}`
: **NVMe** Namespace as device node

`/dev/nda{num}`
: **NVMe** Namespace as storage device node

`/dev/nvd{num}`
: **NVMe** Namespace as disk drive

For details see the man-pages for `nvme`, `nda`, `nvd`, `pci`, `disk`,
and `nvmecontrol`.

(sec-platforms-freebsd-instrumentation)=

## Backend Configs

The FreeBSD platform provides the following backend configs. Some of these use
**cross-platform
implementations** from the **Common Backend Implementations** (CBI). For these,
links point to descriptions in the CBI section. For the FreeBSD-specific
interfaces, descriptions are available in the corresponding subsections.

* Memory Allocators

  - `posix`, Use **libc** `malloc()/free()` with `sysconf()` for
    alignment, add link to CBI

* Asynchronous Interfaces

  - `kqueue`, Use kqueue based aio for Asynchronous I/O
  - `posix`, add link to CBI
  - `emu`, add link to CBI
  - `thrpool`, add link to CBI
  - `nil`, add link to CBI

* Synchronous Interfaces

  - `nvme`, Use FreeBSD NVMe Driver ioctl() for synchronous I/O
  - `psync`, Use pread()/write() for synchronous I/O

* Admin Interfaces

  - `nvme`, Use FreeBSD NVMe Driver ioctl() for admin commands

* Device Interfaces

  - `freebsd`, Use FreeBSD file/dev handles and enumerate NVMe devices

```{toctree}
:maxdepth: 2
:hidden:

ioctl
```
