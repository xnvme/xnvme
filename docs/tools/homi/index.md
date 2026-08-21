(sec-tools-homi)=
# homi

**homi**, Host-Orchestrated Multi-process I/O, opens a set of NVMe devices and
holds them open until it is told to stop. It does no I/O of its own.

Its purpose is to provide the long-lived **primary** process for
{ref}`sec-backends-upcie-mproc`. The primary owns the DMA hugepages and the
controller hardware state, and it must outlive every secondary attached to it.
Running **homi** puts that responsibility in a process of its own, so the
processes actually doing I/O can come and go freely as secondaries.

{ref}`sec-backends-upcie-mproc` is only available for the
{ref}`sec-backends-upcie` backend and its GPU variants, and for the
{ref}`sec-backends-spdk` backend. **homi** is therefore useful where one of
those is built: `upcie`, `upcie-cuda` and `upcie-hip` on Linux, and `spdk` on
Linux and FreeBSD. On Windows it exits with `-ENOTSUP`, since no
multi-process capable backend is available there. On other platforms it runs,
but opening a device with a non-zero `shm_id` fails with `-ENOTSUP`.

```{literalinclude} homi_usage.out
:language: bash
```

## `start`: Hold devices open

```{literalinclude} homi_start_usage.out
:language: bash
```

Opens every device given as a positional argument, then blocks until
signalled.

`--shm_id` is required and sets the shared-memory id the devices are opened
with. Every process that should share these controllers must be started with
the same id. `--be` optionally selects the backend.

Roles are not assigned by the tool: whichever process claims a given `shm_id`
first becomes the primary (see {ref}`sec-backends-upcie-mproc-model`).
Starting **homi** before any secondary is therefore what makes it the primary.

`--host_heap_size` sets the size of the `upcie` host DMA heap, in bytes. The
heap is allocated once per process, on the first device open, so this value
covers every device **homi** holds rather than being per device.

Rather than the 1 GiB the backend would otherwise use, **homi** defaults to 16
MiB per device held. It needs only the admin queue and the sync queue pair
that opening a device creates, and each of those carries a request pool
costing 4 MiB -- so 16 MiB per device is roughly double what is required.
Every process in multi-process mode allocates a heap of its own, so a primary
claiming the backend default would leave nothing in the hugepage pool for the
secondaries it exists to serve:

```bash
homi start 0000:03:00.0 --be upcie --shm_id 1 --host_heap_size 134217728
```

If any device fails to open, **homi** closes the devices it has already opened
and exits with an error, rather than holding a partial set open. Once all
devices are open it reports that it has started, then waits for `SIGINT`
(Ctrl+C) or `SIGTERM`. On either signal it closes all devices and exits.

Example -- hold a single device open as the primary for `shm_id` 1:

```bash
homi start 0000:03:00.0 --be upcie --shm_id 1
```

Example -- hold several devices under the same `shm_id`:

```bash
homi start 0000:03:00.0 0000:04:00.0 --be upcie --shm_id 1
```

While it runs, other processes attach as secondaries by passing the same
`--shm_id`:

```bash
xnvme info 0000:03:00.0 --be upcie --shm_id 1
```

## Backends

**homi** works with any backend that advertises the multi-process capability,
which is `spdk`, `upcie`, `upcie-cuda` and `upcie-hip`:

```bash
homi start 0000:03:00.0 --be spdk --shm_id 1
```

With `upcie-cuda` and `upcie-hip`, **homi** additionally caps the GPU device
heap at 2 MiB rather than the backend default of 1 GiB. It never allocates
data buffers, which is all the device heap is used for, so claiming the
default would take VRAM away from the secondaries:

```bash
homi start 0000:03:00.0 --be upcie-cuda --shm_id 1
```

`homi status` is the exception: it reads uPCIe's shared segment directly, so a
primary started with `--be spdk` reads as absent rather than as running, and on
a platform without uPCIe the subcommand refuses outright. The same applies to
the systemd unit, whose readiness gate is that command.

```{seealso}
{ref}`sec-backends-upcie-mproc` covers the process model, startup handshake,
admin-queue serialization, and the known limitations of multi-process mode.
```
