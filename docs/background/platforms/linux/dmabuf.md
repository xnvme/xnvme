(sec-platforms-linux-dmabuf)=

# GPU memory via `dma-buf`

```{warning}
This relies on a kernel series that is not yet upstream, and on a `liburing`
carrying the UAPI arriving with it. See {ref}`sec-platforms-linux-dmabuf-kernel`.
```

The `linux-dmabuf` memory manager lets an ordinary block-device read or write
land in GPU memory. A CUDA or HIP allocation is exported as a `dma-buf` and
registered with `io_uring` as a fixed buffer; `nvme-pci` maps it once at
registration and keeps the resulting PRP list, so the NVMe controller transfers
to and from the GPU over PCIe peer-to-peer.

Unlike {ref}`sec-backends-upcie-cuda`, which reaches the same destination with a
user-space driver, this path stays on the kernel NVMe driver. The device is the
one the kernel already owns, e.g. `/dev/nvme0n1`.

## Memory manager

Select it with `opts.mem` or `--mem`:

```bash
xnvmeperf run /dev/nvme0n1 --be io_uring_bdev --mem linux-dmabuf --direct \
    --iopattern randread --iosize 4096 --qdepth 32 --runtime 10 --cpumask 0x1
```

The manager offers device memory through `xnvme_mem_map()` only. Buffers from
`xnvme_buf_alloc()` stay on the host.

```c
cudaMalloc(&buf, nbytes);
xnvme_mem_map(dev, buf, nbytes);
```

CUDA and HIP allocations are both accepted. The export covers the allocation
enclosing the address rather than the range asked for, since a `dma-buf` is made
from an allocation base and a page-aligned length. Offsets are unaffected, being
taken from that same base.

`xnvme_mem_unmap()` releases the registration.

## Submission

A buffer covered by a registered mapping has no address the kernel can take. It
travels as an index into the buffer-table of the ring plus an offset into the
mapping, which only the fixed opcodes accept:

* `IORING_OP_READ_FIXED` / `IORING_OP_WRITE_FIXED`

A buffer-table belongs to one ring and `dma-buf` entries cannot be cloned into
another, so each queue keeps its own. A ring brings its table up to date the
first time a command carries a registered buffer, and again once a mapping has
been added or removed since.

## Limitations

The kernel series sets these, and the backend refuses what it cannot serve:

| | |
|---|---|
| Target | A raw block device backed by `nvme-pci`, opened with `O_DIRECT` |
| Polling | Unavailable; registration refuses a ring set up with `IORING_SETUP_IOPOLL` |
| Vectored | Unavailable; `xnvme_cmd_passv()` has no fixed-buffer equivalent |
| Size | A `dma-buf` of at most 1 GiB |

(sec-platforms-linux-dmabuf-kernel)=

## Kernel

The `io_uring` and `nvme-pci` support is not upstream. It is under review as:

- [rw-dmabuf](https://github.com/isilence/linux/commits/rw-dmabuf-v5/)

Building against it needs a `liburing` carrying the UAPI, which is likewise not
in a release yet:

- [rw-dmabuf-tests](https://github.com/isilence/liburing/tree/rw-dmabuf-tests-v5)

Without those definitions the path compiles out and a mapped buffer is refused
with `ENOSYS`, rather than handed on as an address the kernel would misread.

(sec-platforms-linux-dmabuf-iommu)=

## Enforcing IOMMU

The CUDA and HIP drivers export a `dma-buf` describing physical addresses. With
an IOMMU translating for the NVMe device, those are not addresses it can use.
Left alone, the transfer completes as though it had moved data while the IOMMU
faults and drops the writes.

`xnvme_mem_map()` closes that gap itself. It reads the device's IOMMU group from
sysfs, and where something translates it installs the physical range in that
device's domain at an IOVA equal to the physical address, since the addresses
reaching the device are the ones the exporting driver hands the kernel.
`xnvme_mem_unmap()` releases it.

That needs the same two out-of-tree modules {ref}`sec-backends-upcie-cuda`
relies on: `dmabuf-import` to read the physical ranges out of the exported
`dma-buf`, and `iommu-map-pa` to install them. See
{ref}`sec-backends-upcie-cuda-kernel` and {ref}`sec-backends-upcie-cuda-iommu`
for where they come from and how they are installed.

Without them, a registration against a device something translates for fails
with `ENOSYS` rather than succeeding into a transfer that moves nothing. The
alternative is to keep the IOMMU out of the way for the device, with `iommu=pt`,
`amd_iommu=off` or `intel_iommu=off` on the kernel command line, where the
addresses stand as they are and no mapping is installed.

```{note}
The mapping covers one run of physical addresses, so a `dma-buf` the driver
backs discontiguously is refused with `ENOTSUP`. The module also picks the IOVA
without telling the domain's owner; see the caveats under
{ref}`sec-backends-upcie-cuda-iommu`.
```
