(sec-backends-upcie-host)=

# uPCIe (host memory)

The **upcie** backend allocates I/O buffers from a DMA heap backed by
**hugepages** in host memory. Physical addresses are resolved via
`/proc/self/pagemap` and cached at allocation time, so no address translation
is needed on the I/O path.

(sec-backends-upcie-host-config)=

## System Configuration

### Hugepages

`xnvme-driver` configures hugepages as part of device setup. To allocate them
manually:

```bash
echo 128 | tee -a /proc/sys/vm/nr_hugepages
ulimit -l unlimited
```

Each hugepage is 2 MiB by default. The number required depends on the total
DMA buffer space needed by the application.

### IOVA of the heap

With the controller on `vfio-pci`, through iommufd or type1, the heap is mapped
at an IOVA equal to its address, as is memory mapped with `xnvme_mem_map()`, so
both translate alike. User addresses lie far above the IOVAs a platform reserves
low, listed in `/sys/bus/pci/devices/<bdf>/iommu_group/reserved_regions`, but
have to fit the IOMMU's aperture, typically 48 bits, and stay clear of the
device-memory window (see `XNVME_UPCIE_IOVA_BASE`).

`uio_pci_generic` is unaffected, as its DMA addresses are physical.

(sec-backends-upcie-host-limitations)=

## Limitations

- **No `buf_realloc`.** Buffer reallocation is not implemented and returns
  `ENOSYS`.
- **Memory mapping** (`mem_map` / `mem_unmap`):
  - Through an IOMMU (iommufd or type1), any page-aligned memory. A client of a
    server maps only when it could map the server's heap at the server's
    address, and counts what it maps against its own locked-memory limit.
  - A client of a server maps at an IOVA equal to its address, in an address
    space shared with every other client, so `xnvme_mem_map()` fails with `EEXIST`
    where another client holds that range. Allocate the memory elsewhere and
    map it again. This is rare with a few clients mapping a few GiB each, more
    common with large mappings or many clients, and certain for two copies of
    one program running with address-space randomisation off. A client unmaps
    what it still has mapped as it closes; only one that dies first leaves
    mappings behind, which stay until the server stops.
  - With `uio_pci_generic`, only memory in hugetlb pages of the heap's hugepage
    size, aligned to it, since only those keep the physical address the device
    was given. Reading that address needs `CAP_SYS_ADMIN`.

  `xnvme_mem_map_get_req()` reports the alignment for the case at hand, and
  `ENOSYS` where mapping is not possible.
- **Controller reset** and **subsystem reset** are not supported. These
  operations disable the controller and re-enable it, but do not re-program the
  admin submission/completion queues. Without full re-initialization the
  controller times out waiting for the first admin command after re-enable.
  Both pseudo commands return `ENOSYS`.
