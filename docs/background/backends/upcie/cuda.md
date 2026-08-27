(sec-backends-upcie-cuda)=

# uPCIe CUDA

```{warning}
This backend requires an out-of-tree kernel module that is not yet upstream.
See {ref}`sec-backends-upcie-cuda-kernel` for details.
```

The **upcie-cuda** backend enables direct **PCIe peer-to-peer (P2P)** data
transfers between the NVMe device and the GPU, with I/O buffers allocated from
a heap backed by **CUDA device memory**.

Physical addresses for GPU memory are resolved once at initialization through
the Linux `dma-buf` interface. The CUDA driver exports memory at 64 KiB
granularity (one dma-buf page per 64 KiB of GPU memory), and these physical
addresses are stored in a lookup table (LUT) indexed by GPU page. At
submission time, PRP entries are built at **4 KiB granularity** by computing
sub-page offsets within each 64 KiB LUT entry. This matches the host page
size used by NVMe for PRP construction and sector alignment.

(sec-backends-upcie-cuda-memory)=

## Memory Architecture

This backend uses a **hybrid memory model**:

| Structure | Location | Reason |
|-----------|----------|--------|
| Data buffers (`xnvme_buf_alloc`) | GPU device memory (CUDA heap, 1 GiB) | Transferred directly by the NVMe controller via PCIe P2P, bypassing host DRAM |
| SQ, CQ, PRP lists | Host hugepage memory (host heap, 256 MiB) | The CPU writes and the NVMe controller DMA-reads these structures; host-accessible memory is required |

The NVMe **data** path goes GPU ↔ NVMe without touching host DRAM. The
**control** path (submission queue entries, completion queue entries, PRP lists)
still flows through host memory. As a result, both the CUDA heap and the host
hugepage runtime are initialized when the first **upcie-cuda** device is opened.

A caller can hand over device memory it allocated itself with
`xnvme_mem_map()`, which registers the range through the same registry the
heap uses, so such a buffer is usable exactly as one from
`xnvme_buf_alloc()`. Registering a range twice is cheap, since what it
covers is refcounted.

(sec-backends-upcie-cuda-kernel)=

## Kernel Module

Physical address resolution for CUDA device memory relies on importing a
`dma-buf` exported by the CUDA driver into the kernel's DMA subsystem. The
ioctls that do so are not upstream. They ship as `dmabuf-import`, a standalone
out-of-tree module serving `/dev/dmabuf_import`, packaged for DKMS.

This backend requires **dmabuf-import 0.2.0**, published as an asset of the
uPCIe release the headers here are vendored from:

- [safl/upcie v0.7.0](https://github.com/safl/upcie/releases/tag/v0.7.0)

```bash
sudo apt install ./dmabuf-import-dkms_0.2.0_all.deb
```

These ioctls began as a patch to the in-tree `udmabuf` driver, which meant
rebuilding the kernel to get them. That is no longer necessary, and the module
supersedes the `udmabuf-import` package that carried the patched version.

(sec-backends-upcie-cuda-iommu)=

## Enforcing IOMMU

Under `uio_pci_generic` the controller consumes physical addresses, and the
registry hands it exactly those. Under `vfio-pci` an IOMMU translates for it, so
every address it sees is an IOVA -- and device memory cannot get one the way host
memory does: `IOMMU_IOAS_MAP_FILE` rejects the dma-bufs the CUDA driver exports.

The `iommu-map-pa` module inserts the VRAM into the controller's IOMMU domain
directly, and the registry's table then holds the resulting IOVAs. Nothing else
about the backend changes: the same registry enumerates the addresses, and the
command paths resolve every payload through the same per-device dmamem. It ships
alongside `dmabuf-import` as an asset of the uPCIe release the headers here are
vendored from, and is packaged for DKMS:

```bash
sudo apt install ./iommu-map-pa-dkms_0.2.0_all.deb
```

### IOVA window

The module maps at an IOVA of its own choosing without telling the domain's
owner, so a slice has to be reserved for it or the kernel will hand the same
range out twice. The backend claims 64 GiB at 256 GiB, which clears host RAM and
fits the aperture a three-level domain hands out, then tells the IOAS to allow
every usable IOVA except that window.

Where the default does not fit -- a smaller aperture, or a host with more RAM
than the base clears -- override it:

```bash
XNVME_UPCIE_GPU_IOVA_BASE=0x2000000000  # 128 GiB
XNVME_UPCIE_GPU_IOVA_SIZE=0x400000000   # 16 GiB
XNVME_UPCIE_GPU_IOVA_SLICE=0x80000000   # 2 GiB per controller
```

A window that lies outside every usable IOVA range fails `xnvme_dev_open()`
with `ERANGE`.

### Several controllers

The GPU heap is process-wide, but the addresses it is reached by are not: a
mapping is installed in one IOMMU domain, and what a controller may use is
whatever its own domain translates. So the window is divided into one slice per
controller, and each controller maps the heap into its own domain at its own
slice.

That costs mapping the heap once per controller, and one translation table per
controller. It buys not having to know whether two controllers share a domain.
They usually do -- every controller in a process attaches to a single IOAS, and
iommufd gives them one domain unless their IOMMU capabilities differ -- but
"usually" is not something the data path can be built on. With distinct slices
the question does not arise: shared or not, no controller is handed an IOVA
another controller installed.

Three limits follow, all on the `vfio-pci` path only:

- A slice holds the heap twice over, because `xnvme_mem_map()` installs a
  mapping of its own for every buffer handed to it, out of the same slice. Where
  registered buffers need more than the heap's own size, set the width outright
  with `XNVME_UPCIE_GPU_IOVA_SLICE`; a slice that fills up fails the
  registration with `ENOSPC`.
- The window holds `XNVME_UPCIE_GPU_IOVA_SIZE / slice` controllers, so 32 by
  default. Past that `xnvme_dev_open()` fails with `ENOSPC`; raise the window
  size, or lower `device_heap_size`.
- Each controller reserves its own address-translation table, sized as
  {ref}`sec-backends-upcie-host` describes. Where that reservation matters --
  `ulimit -v`, or `vm.overcommit_memory=2` -- lower `XNVME_UPCIE_VA_BITS` to
  bound it.

A controller attaching after the window is reserved brings its own reserved
regions, which the kernel checks against the ranges the IOAS was told to allow.
In the uncommon case that one of them overlaps, that controller's attach fails
with `EADDRINUSE`; move the window with `XNVME_UPCIE_GPU_IOVA_BASE`.

`uio_pci_generic` is unaffected by all of this: physical addresses read the same
from every controller, so one table serves them all, as before.

### Registering buffers

A buffer from `xnvme_buf_alloc()` comes off the heap, and every controller maps
the whole heap, so it is usable on any of them with nothing further done to it.

A buffer handed over with `xnvme_mem_map()` is not. It is memory the caller
allocated, outside the heap, and it is registered into the table of the device
passed to the call -- so under an enforcing IOMMU it resolves for that controller
and no other. To use one buffer on several controllers, register it with each:

```c
xnvme_mem_map(dev_a, buf, nbytes);
xnvme_mem_map(dev_b, buf, nbytes);   /* the same buffer, for another controller */
```

`xnvme_mem_unmap()` is per-device in the same way, and each registration has to
be undone against the device it was made against.

Under `uio_pci_generic` this asymmetry disappears: the controllers share one
table, so registering against any one of them covers them all. Code written for
that path and moved onto `vfio-pci` is where it tends to surface -- as a command
failing to resolve a buffer that a different controller registered.

(sec-backends-upcie-cuda-config)=

## System Configuration

### Hardware Requirements

P2P transfers require a GPU with a sufficiently large **BAR1** window. BAR1
maps GPU device memory into the host PCIe address space and must be large
enough to cover the CUDA heap (1 GiB by default). To check the available BAR1
size:

```bash
nvidia-smi -q -d memory
```

Look for the `BAR1 Memory Usage` section. If `Total` BAR1 is smaller than the
heap size, initialization will fail.

### Hugepages

In addition to the CUDA heap, opening an **upcie-cuda** device also initializes
the host hugepage runtime (256 MiB) used to hold NVMe control structures. Follow
the hugepage setup steps in {ref}`sec-backends-upcie-host` before opening an
**upcie-cuda** device.

(sec-backends-upcie-cuda-gpu)=

## GPU-Resident Queue API

The **upcie-cuda** backend supports GPU-resident NVMe queue pairs via the
`libxnvme_cuda` API. See {ref}`sec-api-c-gpu` for the full API reference,
including host-side setup, CUDA kernel dispatch, and queue depth semantics.

### The GPU's own IOMMU domain

A GPU-resident queue rings the doorbell itself, which is a peer-to-peer write
from the GPU into the controller's BAR0. That write is translated by the *GPU's*
IOMMU domain, not the controller's, and CUDA gives the GPU the BAR's physical
address -- so with the GPU in a translating domain it faults:

    nvidia 0000:2b:00.0: AMD-Vi: IO_PAGE_FAULT domain=0x0012 address=0xee901010

The GPU therefore has to sit in a passthrough domain, whatever the controller is
bound to. Boot with `iommu.passthrough=1`, which leaves vfio free to attach its
own enforcing domain to the NVMe, or switch the GPU's group at runtime with all
of its devices' drivers unbound:

```bash
echo identity | sudo tee /sys/kernel/iommu_groups/<N>/type
```

This is a constraint on GPUDirect P2P generally rather than on this backend:
host-driven queues are unaffected, since the CPU writes the doorbell.

(sec-backends-upcie-cuda-validation)=

## Validation

The cijoe workflow ``test-gpu.yaml`` exercises the **upcie-cuda** backend
against a PCIe NVMe device. Devices must be tagged with the ``cuda`` label in
your cijoe configuration:

```toml
[[devices]]
uri = "0000:xx:00.0"
nsid = 1
labels = ["dev", "pcie", "nvm", "cuda"]
driver_attachment = "userspace"
```

Run the workflow with:

```bash
cd cijoe && cijoe workflows/test-gpu.yaml --config configs/<your-config>.toml
```

(sec-backends-upcie-cuda-limitations)=

## Limitations

- **64 controllers per process under an enforcing IOMMU**, each holding a slice
  of the IOVA window and a translation table of its own. See
  {ref}`sec-backends-upcie-cuda-iommu`.
- **GPU 0 only.** The CUDA context and heap are always created on CUDA device
  0. Multiple GPU support is not implemented.
- **1 GiB heap.** The CUDA heap is fixed at 1 GiB. Allocations beyond this
  limit return `ENOMEM`.
- **No PRP list chaining.** Each request has a single 4 KiB PRP list page
  (512 entries). Combined with PRP1, the maximum transfer size per command
  is 513 × 4 KiB ≈ 2 MiB.
- **No `buf_realloc`.** Buffer reallocation is not implemented and returns
  `ENOSYS`.
- **No pseudo commands.** Show registers, controller reset, subsystem reset,
  and namespace rescan all return `ENOSYS`.
