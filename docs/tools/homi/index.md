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

## `status`: Report whether a primary is running

```{literalinclude} homi_status_usage.out
:language: bash
```

Answers whether anything is holding controllers, without opening a device:

```bash
homi status --shm_id 1
```

It reports whether a primary is holding controllers under that id and, when
one is, what it holds: how many processes are attached to each controller, how
many I/O submission and completion queues are in use, and how many the
controller allocated. It exits zero only when a primary is running and every
controller it holds has finished coming up, so it can be used as a readiness
or health check without parsing its output.

It reports on the uPCIe backend only, and exists only where that backend does,
which is Linux and only when it was built in. Elsewhere the subcommand refuses
rather than reporting: **homi** itself still runs, since `--be spdk` is
multi-process capable on other platforms, but its bookkeeping is not somewhere
this can read. Where uPCIe is present and a runtime belongs to another backend,
that runtime likewise reads as absent rather than as running.

Output is YAML, matching the rest of xNVMe, since the caller is as likely to
be a monitoring loop as a person:

```yaml
shm_id: 9
primary_running: true
attached: 3
controllers:
  - uri: '0000:03:00.0'
    readable: true
    initialized: true
    attached: 2
    nsq_used: 6
    ncq_used: 6
    nsq_total: 16
    ncq_total: 16
ready: true
```

The top-level `attached` counts the processes sharing the runtime, the primary
included, so a primary holding controllers for two secondaries reads as three.
`ready` is what the exit status follows: it is false while a primary holds the
role but its controllers are still coming up, which is a window a service
manager would otherwise mistake for being up.

`nsq_used` and `ncq_used` are necessarily equal: a queue pair is created as
one submission and one completion queue sharing a queue identifier, and the
identifier is what is tracked. They are reported separately so each reads
against its own total. The totals are the queues the controller has allocated,
which need not agree, and the smaller is what limits queue pairs; both are
given rather than their minimum so it is visible which one binds. They are
omitted entirely when the controller did not report them, since absent says
unknown where zero would say none.

What it inspects are two things, neither of which opens a device and neither
of which takes a lock at all. Whether a primary is alive is asked of the
library, which owns the role-election lock the primary holds for its lifetime
and can ask whether it is held without taking it. That distinction matters
more than it looks: the same lock is what a starting process tests to decide
whether it is the primary, so a probe that held it even for the moment between
acquiring and releasing could make that process demote itself to secondary and
then wait for a primary that never arrives. What the primary holds comes from
the runtime's shared segment, likewise read through the library rather than by
reaching into anything.

Two sources are consulted and they can disagree, which is why
`primary_running` and the controller list are not the same question. A primary
that is killed never unlinks its segment, so the controllers it recorded
outlive it. The next primary to claim the id clears them, but until then they
are debris rather than a holding. Only the lock says whether anything is
actually held, so the recorded list is reported only when it is, and
`stale_segment` says when what remains is debris. A segment written by an
incompatible build counts as debris too, and is reported as such rather than
being read at this build's offsets: both segments carry a version stamp, and
one that does not match is refused.

The controllers are recorded there by the library as the primary opens them,
because holding them is what being a primary means. Recording it anywhere else
would only describe primaries that remembered to do it, and this way a runtime
claimed by `xnvmeperf` or by a library consumer is as visible as one claimed
by `homi`. It also answers a question nothing else can: POSIX cannot
enumerate shared memory objects, and the per-controller segments do not carry
the `shm_id`, so without the runtime writing it down the association is
unrecoverable.

The queue totals are read once when the primary brings a controller up, with
Get Features (Number of Queues). Set Features would let a driver ask for a
larger allocation, but the drives tested here reject it, so what is reported
is the allocation the controller made on its own.

## Running several primaries

A host can run more than one primary, each holding a disjoint set of
controllers under its own `shm_id`. Secondaries reach the one they want by
passing the matching id:

```bash
homi start 0000:03:00.0 0000:04:00.0 --be upcie --shm_id 1
homi start 0000:05:00.0 0000:06:00.0 --be upcie --shm_id 2
```

Two things are shared across all of them and have to be budgeted rather than
assumed. The hugepage pool is one: every process allocates a host heap of its
own, secondaries included, and a secondary that does not set
`--host_heap_size` takes the backend default of 1 GiB. Reserve enough
hugepages for the sum of every heap that will exist at once, not just for the
primaries.

The controllers are the other. A controller can be held by one primary only,
so the sets must not overlap; the second primary to claim one fails rather
than sharing it.

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
