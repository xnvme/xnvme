(sec-background-memory)=

# Registering memory

Memory from `xnvme_buf_alloc()` is usable as command payload on every backend.
Memory the application allocated itself becomes usable once it is registered
with `xnvme_buf_register()`, and is given back with `xnvme_buf_unregister()`.
What registering does differs per backend: the library may program an IOMMU on
the caller's behalf, export the memory as a dma-buf, hand it to SPDK, or do
nothing at all.

```c
size_t align;

xnvme_buf_get_alignment(dev, &align);
buf = aligned_alloc(align, nbytes);
xnvme_buf_register(dev, buf, nbytes);
/* ... commands on buf ... */
xnvme_buf_unregister(dev, buf);
```

The rules are the same on every backend that registers memory:

- Each register is given back by one unregister.
- Registering a range within a registered range is counted on it. Any other
  overlap fails with `-EBUSY`.
- Registering memory from `xnvme_buf_alloc()` does nothing.
- `vaddr` and `nbytes` are multiples of the alignment from
  `xnvme_buf_get_alignment()`.

## Per backend

| Memory interface | Takes | Registering does | Alignment |
|---|---|---|---|
| `posix`, `hugepage` (Linux, FreeBSD, macOS) | Any host memory | Nothing | See {ref}`sec-background-memory-io-path` |
| `windows` | Any host memory | Nothing | See {ref}`sec-background-memory-io-path` |
| `io_uring_dmabuf` | Host and GPU memory | Host: nothing. GPU: {ref}`a dma-buf export, registered with io_uring <sec-platforms-linux-async-io_uring_dmabuf>` | Page |
| `libvfn` | Host memory | {ref}`An IOMMU mapping <sec-backends-libvfn-memory>` | Page |
| `spdk` over PCIe and RDMA | {ref}`Host memory, depending on the IOMMU <sec-backends-spdk-memory>` | {ref}`spdk_mem_register() <sec-backends-spdk-memory>` | 2 MiB |
| `spdk` over TCP | Any host memory | Nothing | See {ref}`sec-background-memory-io-path` |
| `upcie` | {ref}`Nothing <sec-backends-upcie-host-limitations>`, `-ENOSYS` | | |
| `upcie-cuda`, `upcie-hip` | GPU memory | {ref}`An entry in the device's address table, or, served, a dma-buf export registered with the server <sec-backends-upcie-cuda-memory-register>` | GPU page |
| `driverkit` (macOS) | Nothing, `-ENOSYS` | | |

(sec-background-memory-io-path)=

## Alignment of the I/O path

`xnvme_buf_get_alignment()` gives the larger of what registering needs and what
the I/O path needs of a payload. Where nothing is registered, only the I/O path
is left:

- Buffered I/O, the OS copies the payload, so any alignment serves, and
  `sizeof(void *)` is given.
- Direct I/O, `opts.direct`, the OS refuses a payload that is not aligned to
  what the device or file system needs. On Linux, that is `stx_dio_mem_align`
  from `statx()` with `STATX_DIOALIGN`. A page satisfies it, so the page size is
  given.
