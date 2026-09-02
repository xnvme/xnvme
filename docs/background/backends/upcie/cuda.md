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

A controller behind an IOMMU needs a second module. The addresses it consumes
are then IOVAs, and installing a mapping for GPU memory is what
`IOMMU_IOAS_MAP_FILE` would do, except that it does not accept a `dma-buf` a
GPU runtime exported. `iommu-map-pa` installs the mapping from the physical
addresses instead, serving `/dev/iommu_map_pa`, and ships the same way.

(sec-backends-upcie-cuda-upstream)=

### What upstream would have to change

None of the out-of-tree code here exists because the kernel cannot do these
things. It exists because the interfaces that would are closed to a `dma-buf`
that a GPU exported, or to memory that is not RAM. Three changes would retire
it:

- **A way to resolve a `dma-buf` to physical addresses.** This is what
  `dmabuf-import` does, and what a driver already does internally when it maps
  one for DMA. Nothing equivalent is exposed to user space.
- **`IOMMU_IOAS_MAP_FILE` accepting a GPU `dma-buf`.** It refuses one today,
  which is why the mapping is installed from physical addresses instead. This
  is the whole reason a GPU cannot be used with a controller opened directly
  under `vfio-pci`.
- **A CUDA runtime that can map a BAR it is handed.**
  `cuMemHostRegister(..., CU_MEMHOSTREGISTER_IOMEMORY)` puts a BAR in the GPU's
  address space, which is what lets a kernel ring a doorbell. It takes a host
  mapping and resolves it, and for a mapping made through a `vfio` device it
  will not: only the first page of one is accepted, and the doorbells are never
  in it. Taking a `dma-buf` wrapping the BAR, which `vfio` can export, would
  make the mapping a thing the runtime is given rather than something it has to
  work out from a virtual address. It would likely settle the other half of
  this too: with the IOMMU translating rather than passing through, a doorbell
  registered this way stops carrying I/O, and a handle the runtime is given
  leaves less room for the mapping to be right in one domain and not the
  other.

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

- **`vfio-pci` needs the mapping installed by hand.** Describing the GPU heap
  by mapping its `dma-buf` is what `IOMMU_IOAS_MAP_FILE` would do, and it
  refuses one a GPU runtime exported, so the mapping goes in from the physical
  addresses behind it instead. That happens either way: a server does it for a
  client, and a process holding the controller does it for itself. See
  {ref}`sec-backends-upcie-cuda-upstream`.
- **Doorbells come from `sysfs` under `vfio-pci`.** A queue the GPU submits on
  needs the doorbell page in the GPU's address space, and the CUDA runtime will
  not take it from a `vfio` mapping, so it is taken from the BAR's `resource0`
  instead. Both name the same registers.
- **I/O the GPU issues needs no server.** A controller this process opened
  builds the same queue in the same device memory, and behind an IOMMU installs
  the mapping itself rather than being given one.
- **I/O the GPU issues needs identity-mapped domains.** With the IOMMU
  translating, `DMA-FQ` groups rather than `identity`, a queue is built and the
  doorbell registers, but no completion arrives and nothing faults on either
  the IOMMU or the GPU. Boot with `iommu=pt` for this. What a client submits
  from the host, payloads in device memory included, works either way.
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
