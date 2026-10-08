(sec-platforms-linux-dmabuf)=

# GPU memory via `dma-buf`

```{warning}
This needs a kernel series and a `liburing` that are not yet upstream. See
{ref}`sec-platforms-linux-dmabuf-kernel`.
```

The `linux-dmabuf` memory manager lets `io_uring` read and write GPU memory on
a regular NVMe block device. A CUDA or HIP allocation is exported as a
`dma-buf` and registered with `io_uring` as a fixed buffer. The NVMe controller
then transfers directly to and from the GPU over PCIe peer-to-peer.

Unlike {ref}`sec-backends-upcie-cuda`, this uses the kernel NVMe driver, so the
device is e.g. `/dev/nvme0n1`.

## Usage

Select the memory manager with `opts.mem` or `--mem`:

```bash
xnvmeperf run /dev/nvme0n1 --be io_uring_bdev --mem linux-dmabuf --direct \
    --iopattern randread --iosize 4096 --qdepth 32 --runtime 10 --cpumask 0x1
```

Map GPU memory with `xnvme_mem_map()` and release it with `xnvme_mem_unmap()`.
Buffers from `xnvme_buf_alloc()` are in host memory.

```c
cudaMalloc(&buf, nbytes);
xnvme_mem_map(dev, buf, nbytes);
```

IO on a mapped buffer is submitted as `IORING_OP_READ_FIXED` /
`IORING_OP_WRITE_FIXED`.

## Limitations

| | |
|---|---|
| Target | A raw NVMe block device opened with `O_DIRECT` |
| Polling | Not supported (`IORING_SETUP_IOPOLL`) |
| Vectored | Not supported (`xnvme_cmd_passv()`) |
| Size | A `dma-buf` of at most 1 GiB |

(sec-platforms-linux-dmabuf-kernel)=

## Kernel

The `io_uring` and `nvme-pci` support is under review as:

- [rw-dmabuf](https://github.com/isilence/linux/commits/rw-dmabuf-v5/)

Building needs a `liburing` with the matching UAPI:

- [rw-dmabuf-tests](https://github.com/isilence/liburing/tree/rw-dmabuf-tests-v5)

Without it, IO on a mapped buffer fails with `ENOSYS`.

(sec-platforms-linux-dmabuf-iommu)=

## IOMMU

The `dma-buf` exported by CUDA and HIP holds physical addresses. When an IOMMU
translates for the NVMe device, `xnvme_mem_map()` maps those addresses into the
IOMMU domain of the device at IOVA == physical address.

This needs the `dmabuf-import` and `iommu-map-pa` kernel modules, see
{ref}`sec-backends-upcie-cuda-kernel` and {ref}`sec-backends-upcie-cuda-iommu`.
Without them `xnvme_mem_map()` fails. Alternatively, boot with `iommu=pt`,
`amd_iommu=off` or `intel_iommu=off`.

```{note}
The `dma-buf` must be physically contiguous. See
{ref}`sec-backends-upcie-cuda-iommu` for caveats of `iommu-map-pa`.
```
