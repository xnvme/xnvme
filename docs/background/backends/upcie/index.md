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

For the host-memory backend, bind devices to `uio_pci_generic` for the legacy
path or `vfio-pci` for the IOMMU-backed path. The `upcie-cuda` backend still
requires the non-VFIO path.

### Privileges

**uPCIe** requires root for two reasons:

1. Reading `/proc/self/pagemap` to translate virtual to physical addresses
   for DMA.
2. Accessing VFIO device nodes and writing PCI config space via sysfs to
   enable Bus Master when needed.


```{toctree}
:maxdepth: 1
:hidden:

host
cuda
```
