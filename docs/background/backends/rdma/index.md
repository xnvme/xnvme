(sec-backends-rdma)=

# RDMA

The `rdma` backend is an integrated NVMe over Fabrics (NVMe-oF) host driver for
RDMA transports. It runs in user space on top of `libibverbs` and `librdmacm`,
and does not depend on SPDK.

(sec-backends-rdma-build)=

## Building

The backend is Linux only. It is built when `libibverbs` and `librdmacm`, with
their headers, are found, and the Meson option `with-rdma` is not `disabled`:

```bash
meson setup builddir -Dwith-rdma=enabled
```

(sec-backends-rdma-identifiers)=

## Device Identifiers

The device URI is the IPv4 address and port of the target, `<ip>:<port>`.
What the backend binds to depends on the `nsid` and `subnqn` options:

* No `subnqn`, or the discovery NQN: the discovery controller. The discovery log
  is retrieved when the device is opened, and up to three entries are used.
* A `subnqn` and `nsid` of `0`: the controller of that subsystem
* A `subnqn` and a namespace identifier: that namespace
* No `subnqn` and a namespace identifier: the namespace of the subsystem at that
  index in the discovery log

The `hostnqn` option sets the host NQN used in the Fabrics Connect command.

(sec-backends-rdma-limitations)=

## Limitations

* IPv4 addresses only
* Commands are described with keyed SGLs. Data buffers must be allocated with
  `xnvme_buf_alloc()` so they are registered with the RDMA device.
* Metadata buffers are not supported and fail with `-ENOSYS`
* Vectored (`iovec`) commands are not supported
* `xnvme_queue_wait()` and `xnvme_queue_get_completion_fd()` are not supported,
  use `xnvme_queue_poke()` or `xnvme_queue_drain()`
* The library is not thread-safe, which includes the queues of a device
