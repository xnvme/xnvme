(sec-platforms-linux-async-io_uring)=

# Async. I/O via `io_uring`

**xNVMe** supports `io_uring` by mapping the NVMe NVM Commands **Read**
and **Write** to `io_uring` opcodes:

* `IORING_OP_READ` / `IORING_OP_WRITE`

  - Mapped when using `xnvme_cmd_pass(..)` with payload as contiguous / fixed buffers

* `IORING_OP_READV` / `IORING_OP_WRITEV`

  - Mapped when using `xnvme_cmd_passv(...)` with payload as iovec

## Passthru

If you are looking to do command-passthru, that is, send arbitrary user-defined
NVMe commands via the Linux kernel NVMe driver, then `io_uring` is not the
backend configuration to use. Rather, select one of:

* `opts = {.be="io_uring_cmd"}`,NVMe passthru via `io_uring` big-SQE/big-CQE, see {ref}`sec-platforms-linux-async-io_uring_cmd`
* `opts = {.be="emu"}`,emulated async using synchronous NVMe ioctl passthru, see {ref}`sec-backends-common-async-emu`
* `opts = {.be="thrpool"}`,thread-pool based async using synchronous NVMe ioctl passthru, see {ref}`sec-backends-common-async-thrpool`

Or use a user-space driver such as {ref}`SPDK <sec-backends-spdk>` or
{ref}`libvfn <sec-backends-libvfn>`.
