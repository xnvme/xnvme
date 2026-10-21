(sec-platforms-freebsd)=

# FreeBSD

(sec-platforms-freebsd-configuration)=

## System Configuration

FreeBSD is one of several supported platforms. **xNVMe** is likely to work on
multiple versions of FreeBSD, however to ensure compatibility we recommend using
a version listed in the {ref}`sec-toolchain` section.

(sec-platforms-freebsd-contigmem)=

### Userspace driver and DMA memory

The `spdk` backend needs the controller on the `nic_uio` driver, and its DMA
memory from the `contigmem` kernel module. `xnvme driver --be spdk` moves a
controller onto `nic_uio`, and `xnvme driver --be posix` returns it to `nvme`:

```bash
xnvme driver --be spdk 0000:01:00.0
```

`contigmem` pre-allocates a pool of physically-contiguous buffers when it
loads, sized by two tunables, and holds at most 64 buffers. The buffer size is a
hard upper bound on a single DMA allocation: a buffer cannot span multiple
`contigmem` buffers, so `xnvme_buf_alloc()` cannot return more than one buffer
no matter how much of the pool is free.

On a fragmented heap, a large physically-contiguous buffer can be impossible to
place even when plenty of memory is free, and loading `contigmem` then fails
with `contigmalloc failed for buffer N` in `dmesg`. Smaller buffers are easier
to place. The `hugepages` tool, installed with `pipx install hugepages`, sets
the tunables and loads the module; this reserves 32 buffers of 64 MB:

```bash
hugepages setup --size 65536 --count 32
```

The tunables are read only when the module loads, so `hugepages setup`
reloads it, which fails while a process still has buffers mapped.
`hugepages setup --count 0` unloads it, and `hugepages info` shows what is
reserved. Without the tool, set `hw.contigmem.num_buffers` and
`hw.contigmem.buffer_size` with `kenv` and `kldload contigmem`.

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
