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
nbytes = (nbytes + align - 1) & ~(align - 1);
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
| `posix` (Linux, FreeBSD, macOS), `hugepage` (Linux) | Any host memory | Nothing | See {ref}`sec-background-memory-io-path` |
| `windows` | Any host memory | Nothing | See {ref}`sec-background-memory-io-path` |
| `io_uring_dmabuf` | Host and GPU memory | Host: nothing. GPU: {ref}`a dma-buf export, registered with io_uring <sec-platforms-linux-async-io_uring_dmabuf>` | Page |
| `libvfn` | Host memory | {ref}`An IOMMU mapping <sec-backends-libvfn-memory>` | Page |
| `spdk` over PCIe | {ref}`Host memory, depending on the IOMMU <sec-backends-spdk-memory>` | {ref}`spdk_mem_register() <sec-backends-spdk-memory>` | 2 MiB |
| `spdk` over RDMA | Any host memory | {ref}`spdk_mem_register() <sec-backends-spdk-memory>` | 2 MiB |
| `spdk` over TCP | Any host memory | Nothing | See {ref}`sec-background-memory-io-path` |
| `upcie` | {ref}`Nothing <sec-backends-upcie-host-limitations>`, `-ENOSYS`; a dma-buf with `xnvme_buf_register_dmabuf()` | An IOMMU mapping of the dma-buf; served, the dma-buf registered with the server | Page |
| `upcie-cuda`, `upcie-hip` | GPU memory | {ref}`An entry in the device's address table, and under vfio-pci an IOMMU mapping; served, a dma-buf export registered with the server <sec-backends-upcie-cuda-memory-register>` | GPU page |
| `driverkit` (macOS) | Nothing, `-ENOSYS` | | |

`xnvme_buf_register_dmabuf()` registers a dma-buf from any exporter under
addresses the caller names, which the CPU need not be able to access. upcie
and io_uring_dmabuf take one, and upcie-cuda and upcie-hip take one as clients
of a homi server; upcie-cuda and upcie-hip without a server, and the other
backends, return `-ENOSYS`. Only a registration at the same address is counted
on a registered dma-buf; one at an address inside a registered range returns
`-EBUSY`. `xnvme_buf_unregister()` gives it back. On upcie, a dma-buf that
iommufd's `IOMMU_IOAS_MAP_FILE` does not take, such as a udmabuf, needs the
`dmabuf_import` and `iommu_map_pa` modules.

(sec-background-memory-props)=

## Properties of memory

`xnvme_buf_get_props()` reports three properties of a range of memory: whether
the CPU can read and write the range, the smallest page size of the mappings
that cover it, and the NUMA node of its pages. The library reads the mappings
of the process on each call, so the range need not be allocated by or
registered with xNVMe.

```c
struct xnvme_buf_props props;
int err;

err = xnvme_buf_get_props(buf, nbytes, &props);
if (!err && (props.cpu_access == XNVME_BUF_PROP_YES)) {
	/* ... the CPU may fill or check buf ... */
}
```

For a range that the process maps without read and write access, such as
addresses reserved with `PROT_NONE` to name a dma-buf, `cpu_access` is
`XNVME_BUF_PROP_NO`: a load or store there faults. An untouched page is on no
NUMA node, so `numa_node` is -1 until a page of the range is present. On
platforms other than Linux, `cpu_access` is `XNVME_BUF_PROP_UNKNOWN`,
`page_nbytes` is 0 and `numa_node` is -1.

The buffer helpers, `xnvme_buf_fill()`, `xnvme_buf_clear()`,
`xnvme_buf_memcpy()` and `xnvme_buf_diff()`, reach CUDA or HIP memory through
its runtime. A backend records the runtime of the memory it allocates with
`xnvme_buf_alloc()` and of the memory it registers: upcie-cuda and upcie-hip
for their heaps, io_uring_dmabuf for each allocation it exports, and the upcie
backends for the memory and dma-bufs registered with them. Memory without a
record, such as GPU memory that was neither allocated with `xnvme_buf_alloc()`
nor registered, is reached by the CPU.

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
