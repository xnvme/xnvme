(sec-backends-upcie-host)=

# uPCIe (host memory)

The **upcie** backend allocates I/O buffers from a DMA heap backed by
**hugepages** in host memory. Physical addresses are resolved via
`/proc/self/pagemap` and cached at allocation time, so no address translation
is needed on the I/O path.

(sec-backends-upcie-host-config)=

## System Configuration

### Hugepages

Reserve them with the `hugepages` tool, `pipx install hugepages`, and lift the
memlock limit; `xnvme env` reports both:

```bash
hugepages setup --count 128
ulimit -l unlimited
```

Each hugepage is 2 MiB by default. The number required depends on the total
DMA buffer space needed by the application.

### IOVA of the heap

With the controller on `vfio-pci`, the heap is mapped before the controller is
attached, and has to stay clear of the IOVAs the platform reserves for it, listed
in `/sys/bus/pci/devices/<bdf>/iommu_group/reserved_regions`. Where it does not,
opening the controller fails, with `EADDRINUSE` through iommufd and `EINVAL`
through type1. The heap starts at 4 GiB by default, above the regions commonly
reserved low, and has to end below the device-memory window (see
`XNVME_UPCIE_IOVA_BASE`). Override where a controller reserves IOVAs there:

```bash
XNVME_UPCIE_HOST_IOVA_BASE=0x1000000000  # 64 GiB
```

`uio_pci_generic` is unaffected, as its DMA addresses are physical.

(sec-backends-upcie-host-limitations)=

## Limitations

- **No `buf_realloc`.** Buffer reallocation is not implemented and returns
  `ENOSYS`.
- **No memory mapping** (`mem_map` / `mem_unmap`). These return `ENOSYS`.
- **Controller reset** and **subsystem reset** are not supported. These
  operations disable the controller and re-enable it, but do not re-program the
  admin submission/completion queues. Without full re-initialization the
  controller times out waiting for the first admin command after re-enable.
  Both pseudo commands return `ENOSYS`.
