(sec-backends-upcie)=

# uPCIe

**xNVMe** provides kernel-bypassing backends implemented using **uPCIe**, a
minimal header-only **user space** NVMe driver. Unlike **SPDK**, **uPCIe** has
no reactor, threading model, or application framework — just direct PCIe BAR
access and DMA.

Two backend configs are available, differing in where I/O buffers are
allocated:

- **{ref}`sec-backends-upcie-host`** — DMA buffers in host memory, backed by
  hugepages.
- **{ref}`sec-backends-upcie-cuda`** — DMA buffers in GPU device memory,
  enabling PCIe peer-to-peer (P2P) transfers directly between the NVMe device
  and the GPU.

Additionally, the host-memory backend supports a **{ref}`sec-backends-upcie-mproc`**
mode in which several processes share a single NVMe controller via a designated
primary process.

(sec-backends-upcie-identifiers)=

## Device Identifiers

When using **user space** NVMe drivers, the operating-system kernel NVMe
driver is detached and the device is bound either to `uio_pci_generic` for the
legacy no-IOMMU path or to `vfio-pci` for the IOMMU-backed path. Thus, the
device files in `/dev/`, such as `/dev/nvme0n1`, are not available. Devices
are instead identified by their PCI id (`0000:03:00.0`), and namespace
identifier via the `--nsid` option.

(sec-backends-upcie-config)=

## System Configuration

### Driver Attachment

Use the `xnvme-driver` script to unbind the kernel NVMe driver and bind
devices to uio_pci_generic or vfio-pci:

```bash
xnvme-driver
```

**upcie** supports both `uio_pci_generic` (no-IOMMU path) and `vfio-pci`
(IOMMU-backed path). **upcie-cuda** requires the non-VFIO path.

### Privileges

**uPCIe** requires root for two reasons:

1. Reading `/proc/self/pagemap` to translate virtual to physical addresses
   for DMA.
2. Accessing VFIO device nodes and writing PCI config space via sysfs to
   enable Bus Master when needed.

(sec-backends-upcie-dual)=

## Dual-Backend Operation

The same PCIe device can be opened simultaneously with both **upcie** and
**upcie-cuda**. This lets one handle send I/O through host-memory buffers while
another sends I/O through GPU device-memory buffers, both going to the same NVMe
controller. Opening order does not matter; the controller is torn down only when
the last handle across both backends is closed. Attempting to open the same URI
with an unrelated backend while either uPCIe backend holds it returns `-EBUSY`.

Before opening both handles, complete the system configuration for each backend:
hugepages for {ref}`sec-backends-upcie-host` and the additional CUDA and kernel
requirements for {ref}`sec-backends-upcie-cuda`.

### Example

```c
struct xnvme_opts host_opts = xnvme_opts_default();
struct xnvme_opts cuda_opts = xnvme_opts_default();
host_opts.be = "upcie";
cuda_opts.be = "upcie-cuda";

struct xnvme_dev *host_dev = xnvme_dev_open("0000:03:00.0", &host_opts);
struct xnvme_dev *cuda_dev = xnvme_dev_open("0000:03:00.0", &cuda_opts);
```

```{toctree}
:maxdepth: 1
:hidden:

host
cuda
mproc
```
