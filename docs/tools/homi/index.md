(sec-tools-homi)=
# homi

**homi**, Host-Orchestrated Multi-path I/O, opens a set of NVMe devices and
holds them open until it is told to stop. It does no I/O of its own.

The name borrows from ANA deliberately. Where ANA has the controller report
the paths to a namespace and the host choose among them, this arranges them
from the host: each connected client gets queues of its own to the same
namespace, and the controller is never asked which clients exist.

Its purpose is to provide the long-lived **server** process for
{ref}`sec-backends-upcie-cplane`. The server owns the DMA hugepages and the
controller hardware state, and it must outlive every client connected to it.
Running **homi** puts that responsibility in a process of its own, so its
clients can come and go freely. A client may be another process or an
accelerator.

{ref}`sec-backends-upcie-cplane` is only available for the
{ref}`sec-backends-upcie` backend and its GPU variants, and for the
{ref}`sec-backends-spdk` backend. **homi** is therefore useful where one of
those is built: `upcie`, `upcie-cuda` and `upcie-hip` on Linux, and `spdk` on
Linux and FreeBSD. On Windows it exits with `-ENOTSUP`, since no
backend that can share a controller is available there. On other platforms it
runs, but opening a device with a non-zero `homi_id` fails with `-ENOTSUP`.

```{literalinclude} homi_usage.out
:language: bash
```

## `start`: Hold devices open

```{literalinclude} homi_start_usage.out
:language: bash
```

Opens every device given as a positional argument, then blocks until
signalled.

Every process that should share these controllers must be started with the
same id.

Roles are not assigned by the tool: whichever process claims a given
`homi_id` first becomes the server (see
{ref}`sec-backends-upcie-cplane-model`). Starting **homi** before any client
is therefore what makes it the server.

The heap is allocated once per process, on the first device open, so
`--host_heap_size` covers every device **homi** holds rather than being per
device.

Rather than the 1 GiB the backend would otherwise use, **homi** defaults to 16
MiB per device held. It needs only the admin queue and the sync queue pair
that opening a device creates, and each of those carries a request pool
costing 4 MiB, so 16 MiB per device is roughly double what is required.
Every process connected to a control plane allocates a heap of its own, so a
server
claiming the backend default would leave nothing in the hugepage pool for the
clients it exists to serve:

```bash
homi start 0000:03:00.0 --be upcie --homi-id 1 --host_heap_size 134217728
```

If any device fails to open, **homi** closes the devices it has already opened
and exits with an error, rather than holding a partial set open. Once all
devices are open it reports that it has started, then waits for `SIGINT`
(Ctrl+C) or `SIGTERM`. On either signal it closes all devices and exits.

Example: hold a single device open as the server for `homi_id` 1:

```bash
homi start 0000:03:00.0 --be upcie --homi-id 1
```

Example: hold several devices under the same `homi_id`:

```bash
homi start 0000:03:00.0 0000:04:00.0 --be upcie --homi-id 1
```

While it runs, clients connect by passing the same
`--homi-id`:

```bash
xnvme info 0000:03:00.0 --be upcie --homi-id 1
```

## `status`: Report whether a server is running

```{literalinclude} homi_status_usage.out
:language: bash
```

Answers whether anything is holding controllers, without opening a device:

```bash
homi status --homi-id 1
```

It reports whether a server is holding controllers under that id and, when
one is, what it holds: how many connections each controller has, how
many I/O submission and completion queues are in use, and how many the
controller allocated. It exits zero only when a server is running and every
controller it holds has finished coming up, so it can be used as a readiness
or health check without parsing its output.

It reports on the uPCIe backend only, and exists only where that backend does,
which is Linux and only when it was built in. Elsewhere the subcommand refuses
rather than reporting: **homi** itself still runs, since `--be spdk` is
able to share a controller on other platforms, but its bookkeeping is not
somewhere
this can read. Where uPCIe is present and a runtime belongs to another
backend, that runtime likewise reads as absent rather than as running.

Output is YAML, matching the rest of xNVMe, since the caller is as likely to
be a monitoring loop as a person:

```yaml
homi_id: 9
server_running: true
connections: 3
controllers:
  - uri: '0000:03:00.0'
    readable: true
    initialized: true
    connections: 2
    nsq_used: 6
    ncq_used: 6
    nsq_total: 16
    ncq_total: 16
ready: true
```

The top-level `connections` counts the sockets the server has open, including
the one `homi status` is using to ask, so a server serving two clients reads as
three. There is one per client process, however many controllers that process
reaches, since a request names the controller it is about. The per-controller
`connections` is that controller's own: the clients that asked the server for
it, whether or not they went on to take a queue, and not the one asking.
`ready` is what the exit status follows: it is false while a server holds the
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

What it inspects is whoever is serving that identifier. It connects to the
runtime's socket and asks, which answers most of the question by succeeding: a
socket that answers has the process holding the controllers behind it. Nothing
is opened, nothing is locked, and nothing is inferred from what a process left
in memory.

That removes a distinction the previous arrangement needed. A server that was
killed left a shared segment behind which read exactly like a live runtime, so
status had to report whether what it found was debris. There is nothing to
tell apart now: the process that holds the socket is the only thing that
answers on it, and when it dies the name stops working.

The queue totals are read once when the server brings a controller up, with
Get Features (Number of Queues). Set Features would let a driver ask for a
larger allocation, but the drives tested here reject it, so what is reported
is the allocation the controller made on its own.

## Running several servers

A host can run more than one server, each holding a disjoint set of
controllers under its own `homi_id`. Clients reach the one they want by
passing the matching id:

```bash
homi start 0000:03:00.0 0000:04:00.0 --be upcie --homi-id 1
homi start 0000:05:00.0 0000:06:00.0 --be upcie --homi-id 2
```

Two things are shared across all of them and have to be budgeted rather than
assumed. The hugepage pool is one: every process allocates a host heap of its
own, clients included, and a client that does not set
`--host_heap_size` takes the backend default of 1 GiB. Reserve enough
hugepages for the sum of every heap that will exist at once, not just for the
servers.

The controllers are the other. A controller can be held by one server only,
so the sets must not overlap; the second server to claim one fails rather
than sharing it.

## Running as a service

A server is only useful while it is running, which makes it a service rather
than a command someone remembers to start. A unit template for that is
installed with xNVMe, along with an example configuration file, so enabling an
instance is all that remains:

```bash
cp /etc/xnvme/homi.conf.example /etc/xnvme/homi-1.conf
systemctl enable --now xnvme-homi@1
```

The unit is installed where systemd says units belong, and its `ExecStart`
names the `homi` that this build installs, so a prefixed build points at its
own binary rather than at whichever one is first on `PATH`. Two build options
govern it:

`-Dsystemd=auto` is the default. It installs the unit where systemd is
present and stays quiet where it is not, which is any non-Linux platform and
any build configured with `-Dtools=false`. Use `-Dsystemd=disabled` to keep a
build from writing to the unit directory at all, and `-Dsystemd=enabled` to
have configuration fail instead of skipping it silently.

The unit reads its configuration from the `sysconfdir` of the same build, so
a prefixed install writes the example and reads the configuration under that
prefix rather than writing to one path and reading from another. Pass
`--sysconfdir=/etc` to keep it at the usual place regardless of prefix.

`-Dsystemd_unitdir=` overrides the location. Left empty, it is whatever
systemd reports through `pkg-config`, resolved against the configured prefix
rather than taken as the absolute path systemd reports, so `--prefix` and
`DESTDIR` are both honoured. Set it to place units somewhere else, such as
`-Dsystemd_unitdir=/etc/systemd/system` for a machine-local override.

The unit is templated on `homi_id`, matching the model above, so several
instances can run side by side. It does not bind controllers or reserve
hugepages: those are prerequisites it checks for and refuses to start without,
because inferring a machine's driver bindings from a unit file is worse than
failing with a clear message. `systemctl start` returns only once a client
could actually connect, which it establishes with `homi status`.

## Backends

**homi** works with any backend that advertises the control-plane capability,
which is `spdk`, `upcie`, `upcie-cuda` and `upcie-hip`:

```bash
homi start 0000:03:00.0 --be spdk --homi-id 1
```

With `upcie-cuda` and `upcie-hip`, **homi** additionally caps the GPU device
heap at 2 MiB rather than the backend default of 1 GiB. It never allocates
data buffers, which is all the device heap is used for, so claiming the
default would take VRAM away from the clients:

```bash
homi start 0000:03:00.0 --be upcie-cuda --homi-id 1
```

`homi status` is the exception: it asks over the socket uPCIe serves, so a
server started with `--be spdk` reads as absent rather than as running, and on
a platform without uPCIe the subcommand refuses outright. The same applies to
the systemd unit, whose readiness gate is that command.

```{seealso}
{ref}`sec-backends-upcie-cplane` covers the process model, startup handshake,
admin-queue serialization, and the known limitations of the control plane.
```
