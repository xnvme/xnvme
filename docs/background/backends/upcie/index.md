(sec-backends-upcie)=

# uPCIe

**xNVMe** provides a kernel-bypassing backend implemented using **uPCIe**, a
minimal header-only **user space** NVMe driver. Unlike **SPDK**, **uPCIe** has no
reactor, threading model, or application framework — just direct PCIe BAR
access and hugepage-based DMA.

(sec-backends-upcie-identifiers)=

## Device Identifiers

When using **user space** NVMe drivers, the operating-system kernel NVMe driver is
detached and the device bound to `uio_pci_generic`. Thus, the device files in
`/dev/`, such as `/dev/nvme0n1` are not available. Devices are instead
identified by their PCI id (`0000:03:00.0`), and namespace identifier via the
`--nsid` option.

(sec-backends-upcie-config)=

## System Configuration

### Driver Attachment and Memory

Use the `xnvme-driver` script to unbind the kernel NVMe driver, configure
hugepages, and bind devices:

```bash
xnvme-driver
```

**uPCIe** does **not** work with IOMMU enabled. When IOMMU is active,
`xnvme-driver` will bind devices to `vfio-pci` by default, which is
incompatible with uPCIe. Ensure devices are bound to `uio_pci_generic`
instead.

### Privileges

**uPCIe** requires root for two reasons:

1. Reading `/proc/self/pagemap` to translate virtual to physical addresses
   for DMA.
2. Writing PCI config space via sysfs to enable Bus Master.

(sec-backends-upcie-limitations)=

## Limitations

- **Controller reset** and **subsystem reset** are not supported. These
  operations disable the controller and re-enable it, but do not re-program the
  admin submission/completion queues. Without full re-initialization the
  controller times out waiting for the first admin command after re-enable.
  Both pseudo commands return `ENOSYS`.

(sec-backends-upcie-instrumentation)=

## Instrumentation

...
