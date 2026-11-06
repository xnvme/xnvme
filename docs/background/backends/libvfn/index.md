(sec-backends-libvfn)=

# libvfn

...

(sec-backends-libvfn-identifiers)=

## Device Identifiers

...

(sec-backends-libvfn-config)=

## System Configuration

...

(sec-backends-libvfn-instrumentation)=

## Instrumentation

...

(sec-backends-libvfn-memory)=

## Registering memory

`xnvme_buf_register()` maps the memory in the IOMMU context of the device with
`iommu_map_vaddr()`, so the library programs the IOMMU on the caller's behalf.
Memory from `xnvme_buf_alloc()` is mapped there already, so registering it does
nothing. Neither does registering any other memory that is mapped there
already. The memory is registered in whole pages. See
{ref}`sec-background-memory` for what registering does on the other backends.
