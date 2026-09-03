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

- [safl/upcie v0.8.0](https://github.com/safl/upcie/releases/tag/v0.8.0)

```bash
sudo apt install ./dmabuf-import-dkms_0.2.0_all.deb
```

These ioctls began as a patch to the in-tree `udmabuf` driver, which meant
rebuilding the kernel to get them. That is no longer necessary, and the module
supersedes the `udmabuf-import` package that carried the patched version.

(sec-backends-upcie-cuda-iommu)=

## Enforcing IOMMU

Under `uio_pci_generic` the controller consumes physical addresses and the
registry hands it those. Under `vfio-pci` an IOMMU translates, so every address
it sees is an IOVA. Device memory cannot get one the way host memory does,
since `IOMMU_IOAS_MAP_FILE` rejects the dma-bufs the CUDA driver exports.

The `iommu-map-pa` module inserts the VRAM into the controller's domain
directly, and the registry's table holds the resulting IOVAs. It is published as
an asset of the same release as `dmabuf-import`, packaged for DKMS:

- [safl/upcie v0.8.0](https://github.com/safl/upcie/releases/tag/v0.8.0)

```bash
sudo apt install ./iommu-map-pa-dkms_0.2.0_all.deb
```

### IOVA window

The module maps at IOVAs of its own choosing without telling the domain's owner,
so a window is reserved for it: 64 GiB at 256 GiB, with the IOAS allowed every
usable IOVA except that. Override where it does not fit:

```bash
XNVME_UPCIE_GPU_IOVA_BASE=0x2000000000  # 128 GiB
XNVME_UPCIE_GPU_IOVA_SIZE=0x400000000   # 16 GiB
XNVME_UPCIE_GPU_IOVA_SLICE=0x80000000   # 2 GiB per controller
```

A window outside every usable IOVA range fails `xnvme_dev_open()` with `ERANGE`.

### Several controllers

A mapping reaches one IOMMU domain, so each controller gets its own slice of the
window and maps the heap into its own domain. Three limits follow, on the
`vfio-pci` path only:

- A slice is twice the size of the heap, leaving room for buffers registered
  with `xnvme_mem_map()`. Set the width with `XNVME_UPCIE_GPU_IOVA_SLICE`. A
  full slice fails the registration with `ENOSPC`.
- The window holds 31 controllers by default, and never more than 64. Past that
  `xnvme_dev_open()` fails with `ENOSPC`.
- Each controller reserves a translation table of its own, sized as
  {ref}`sec-backends-upcie-host` describes. `XNVME_UPCIE_VA_BITS` bounds it.

A controller attaching later brings its own reserved regions. Where one overlaps
the ranges the IOAS was told to allow, that attach fails with `EADDRINUSE`. Move
the window with `XNVME_UPCIE_GPU_IOVA_BASE`.

`uio_pci_generic` is unaffected. Physical addresses read the same from every
controller, so one table serves them all.

### Registering buffers

A buffer from `xnvme_buf_alloc()` comes off the heap, which every controller
maps, so it works on any of them.

A buffer handed to `xnvme_mem_map()` is registered against the device passed to
the call, so behind an IOMMU it resolves for that controller only. Register it
with each device it is used from, and unmap it against each:

```c
xnvme_mem_map(dev_a, buf, nbytes);
xnvme_mem_map(dev_b, buf, nbytes);
```

Under `uio_pci_generic` one table serves every controller, so a single
registration covers them all.

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

(sec-backends-upcie-cuda-gpu-domain)=

### GPU IOMMU domain

Needed only for GPU-resident queues, meaning `xnvmeperf cuda-run`, `cuda-verify`
and anything else built on {ref}`sec-api-c-gpu`. Host-driven I/O needs nothing
here.

With a GPU-resident queue the GPU writes the doorbell register itself, rather
than the CPU doing it, so the write is peer-to-peer traffic into the
controller's BAR0. It is translated by the GPU's own domain, not the
controller's that {ref}`sec-backends-upcie-cuda-iommu` sets up, and CUDA hands
the GPU a physical address. With the GPU in a translating domain nothing has
mapped it, so every write faults.

#### Recognising it

The run completes while every I/O fails, taking the queue's timeout rather than
`--runtime`:

```text
 Device                        IOPS      MiB/s   Failed
 0000:01:00.0                  2.30       0.01       32
```

The kernel log names the **GPU**, not the NVMe:

```text
nvidia 0000:2b:00.0: AMD-Vi: IO_PAGE_FAULT domain=0x0012 address=0xee901010
```

`0xee901010` is BAR0 plus `0x1010`, the submission-queue doorbell of queue 2. An
Intel host reports the equivalent under `DMAR:`.

#### Fixing it persistently

Boot with the GPU in a passthrough domain. Add `iommu.passthrough=1` to
`GRUB_CMDLINE_LINUX_DEFAULT` in `/etc/default/grub`, then
`sudo update-grub && sudo reboot`. This changes only the *default* domain type,
so `vfio-pci` still attaches its own enforcing domain to the NVMe.

#### Fixing it without a reboot

A group's domain type can be switched only while **no device in that group has a
driver bound**. Find the group and everything in it:

```bash
readlink -f /sys/bus/pci/devices/0000:2b:00.0/iommu_group
ls /sys/bus/pci/devices/0000:2b:00.0/iommu_group/devices/
```

The addresses and group number below are from one host. Substitute what those
report, then release, unbind, switch, and rebind:

```bash
sudo fuser -v /dev/nvidia*   # an empty result is what you want

sudo systemctl stop nvidia-dcgm nvidia-persistenced
sudo rmmod nvidia_uvm   # plus nvidia_drm and nvidia_modeset where loaded

echo 0000:2b:00.1 | sudo tee /sys/bus/pci/drivers/snd_hda_intel/unbind
echo 0000:2b:00.0 | sudo tee /sys/bus/pci/drivers/nvidia/unbind

echo identity | sudo tee /sys/kernel/iommu_groups/16/type

echo 0000:2b:00.0 | sudo tee /sys/bus/pci/drivers/nvidia/bind
echo 0000:2b:00.1 | sudo tee /sys/bus/pci/drivers/snd_hda_intel/bind
sudo modprobe nvidia_uvm
sudo systemctl start nvidia-dcgm
```

The switch fails with `EBUSY` if anything is still bound, and does **not**
survive a reboot.

#### Verifying the switch

```bash
cat /sys/kernel/iommu_groups/16/type      # identity
sudo xnvmeperf cuda-verify --be upcie-cuda --iosize 4096 --qdepth 32 \
    --nqueues 2 0000:01:00.0
sudo dmesg | grep -iE 'AMD-Vi|DMAR'
```

`cuda-verify` should report `0 mismatches` and `dmesg` nothing at all.

```{note}
This applies to any peer-to-peer write from a GPU into another device's BAR,
not just to xNVMe.
```

(sec-backends-upcie-cuda-gpu)=

## GPU-Resident Queue API

The **upcie-cuda** backend supports GPU-resident NVMe queue pairs via the
`libxnvme_cuda` API. See {ref}`sec-api-c-gpu` for the full API reference,
including host-side setup, CUDA kernel dispatch, and queue depth semantics.

GPU-resident queues need the setup described in
{ref}`sec-backends-upcie-cuda-gpu-domain`.

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

- **31 controllers per process under an enforcing IOMMU** at the default heap
  and window size, capped at 64. See {ref}`sec-backends-upcie-cuda-iommu`.
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
