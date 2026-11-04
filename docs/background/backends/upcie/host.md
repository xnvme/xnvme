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

With the controller on `vfio-pci`, through iommufd or type1, the heap is mapped
at an IOVA equal to its address, as is memory registered with
`xnvme_buf_register()`, so both translate alike. User addresses lie far above
the IOVAs a platform reserves low, listed in
`/sys/bus/pci/devices/<bdf>/iommu_group/reserved_regions`, but have to fit the
IOMMU's aperture, typically 48 bits, and stay clear of the device-memory window
(see `XNVME_UPCIE_IOVA_BASE`).

Where the heap does not fit there, as behind an IOMMU of 39 bits, it is mapped
at 4 GiB instead, and nothing can be registered. Setting
`XNVME_UPCIE_HOST_IOVA_BASE` maps it at the IOVA given, with the same
consequence. That suits a process that registers nothing, behind an IOMMU whose
page table grows deeper with the highest IOVA mapped, as an AMD IOMMU's does:

```bash
XNVME_UPCIE_HOST_IOVA_BASE=0x100000000  # 4 GiB
```

The heap then has to end below the device-memory window, and stay clear of the
reserved regions.

`uio_pci_generic` is unaffected, as its DMA addresses are physical.

(sec-backends-upcie-host-limitations)=

## Limitations

- **No `buf_realloc`.** Buffer reallocation is not implemented and returns
  `ENOSYS`.
- **Registering memory** (`xnvme_buf_register()` / `xnvme_buf_unregister()`):
  - Through an IOMMU (iommufd or type1), any memory aligned to the larger of the
    CPU's page and the IOMMU's smallest page, where the heap sits at its
    address (see above), and outside the device-memory window, which fails
    with `ERANGE`. The pages are pinned, and count against the locked-memory
    limit unless the process has `CAP_IPC_LOCK`. A client of a server registers
    only when it could map the server's heap at the server's address, and
    counts what it registers against its own locked-memory limit.
  - A client of a server maps at an IOVA equal to its address, in an address
    space shared with every other client, so `xnvme_buf_register()` fails with
    `EEXIST` where another client holds that range. Allocate the memory
    elsewhere and register it again. This is rare with a few clients
    registering a few GiB each, more common with large registrations or many
    clients, and certain for two copies of one program running with
    address-space randomisation off. A client unregisters what it still has
    registered as it closes; only one that dies first leaves mappings behind,
    which stay until the server stops.
  - To map at all, a client is given the server's iommufd or vfio container,
    with which it can also remove the server's heap, or another client's
    memory, from the device's reach. The clients of a server are one trust
    domain, as they already are through the heap and BAR0 they share.
  - With `uio_pci_generic`, only memory in hugetlb pages of the heap's hugepage
    size, aligned to it, since only those keep the physical address the device
    was given. Reading that address needs `CAP_SYS_ADMIN`. Memory mapped
    `MAP_PRIVATE` does not keep it across `fork()`: whichever of parent and
    child writes first is moved to a copy, and if that is the parent, the
    device goes on using the page the child now has. A process that forks
    while memory is registered should map it `MAP_SHARED`, or mark it
    `MADV_DONTFORK`.

  `xnvme_buf_get_alignment()` reports the alignment for the case at hand, and
  `ENOSYS` where registering is not possible.
- **Controller reset** and **subsystem reset** are not supported. These
  operations disable the controller and re-enable it, but do not re-program the
  admin submission/completion queues. Without full re-initialization the
  controller times out waiting for the first admin command after re-enable.
  Both pseudo commands return `ENOSYS`.
