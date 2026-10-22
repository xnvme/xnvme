(sec-platforms-linux-async-io_uring_dmabuf)=

# Async. I/O on GPU memory via `io_uring_dmabuf`

```{warning}
This needs a kernel that is not yet released and a `liburing` that is not yet
upstream. See
{ref}`sec-platforms-linux-async-io_uring_dmabuf-kernel`.
```

The `io_uring_dmabuf` backend lets `io_uring` read and write GPU memory on a
regular NVMe block device. A CUDA or HIP allocation is exported as a `dma-buf`
and registered with `io_uring` as a fixed buffer. The NVMe controller then
transfers directly to and from the GPU over PCIe peer-to-peer.

Unlike {ref}`sec-backends-upcie-cuda`, this uses the kernel NVMe driver, so the
device is e.g. `/dev/nvme0n1`.

## Usage

Select the backend with `opts.be` or `--be`:

```bash
xnvmeperf run /dev/nvme0n1 --be io_uring_dmabuf --direct \
    --iopattern randread --iosize 4096 --qdepth 32 --runtime 10 --cpumask 0x1
```

Map GPU memory with `xnvme_mem_map()` and release it with `xnvme_mem_unmap()`.
Buffers from `xnvme_buf_alloc()` are in host memory, IO on them is regular
`io_uring` IO.

```c
cudaMalloc(&buf, nbytes);
xnvme_mem_map(dev, buf, nbytes);
```

`xnvme_mem_map()` registers the allocation with the ring of every queue of the
device, and a new queue registers the allocations already mapped. IO on a
mapped buffer is submitted as `IORING_OP_READ_FIXED` / `IORING_OP_WRITE_FIXED`.

## Limitations

| | |
|---|---|
| Target | A raw NVMe block device opened with `O_DIRECT` |
| Polling | Not supported, `IORING_SETUP_IOPOLL` fails queue initialization |
| Vectored | Not supported (`xnvme_cmd_passv()`) |
| Size | A `dma-buf` of at most 1 TiB |
| Count | At most 256 mapped allocations per device |

(sec-platforms-linux-async-io_uring_dmabuf-kernel)=

## Kernel

The `io_uring` and `nvme-pci` support is applied to the `io_uring` tree, but
not yet in a released kernel:

- [rw-dmabuf-v11](https://lore.kernel.org/all/cover.1791975048.git.asml.silence@gmail.com/)
- [rw-dmabuf](https://github.com/isilence/linux/commits/rw-dmabuf-v11/)

Building needs a `liburing` with the matching UAPI, and CUDA or HIP:

- [rw-dmabuf-tests](https://github.com/isilence/liburing/tree/rw-dmabuf-tests-v8)

Without them, the backend is not built.

(sec-platforms-linux-async-io_uring_dmabuf-iommu)=

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
