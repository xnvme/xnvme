(sec-backends-upcie-hip)=

# uPCIe HIP

The **upcie-hip** backend is the AMD counterpart of
{ref}`sec-backends-upcie-cuda`. I/O buffers come from a heap of **HIP device
memory**, so the NVMe device transfers to and from the GPU peer-to-peer.

(sec-backends-upcie-hip-p2p-cq-mirror)=

## Completions in device memory

**upcie-hip** implements `XNVME_QUEUE_P2P_CQ_MIRROR` the way
{ref}`sec-backends-upcie-cuda-p2p-cq-mirror` describes it for **upcie-cuda**,
with a wavefront per queue. Its kernel is the only device code in the library
and is compiled by `hipcc` in a translation unit of its own, together with
the one function that launches it; the build links that object like any
other, and the rest of **upcie-hip** is C against the HIP runtime.

The completion queue of such a queue is not carved from the heap: the GPU
caches the heap, and the controller writes behind those caches. It is a
region of its own that the GPU does not cache, 2 MiB and aligned to that,
since the dma-buf export it is registered through wants a range of that
size. It is kept for the next queue of the process rather than freed, since
freeing device memory waits for the device and the resident kernel never
lets it idle. The slot table the host and the kernel share is the one
{ref}`sec-backends-upcie-cuda-p2p-cq-mirror` describes.
