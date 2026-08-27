(sec-backends-upcie-cplane)=
# uPCIe (control plane)

The **upcie** backend offers a **control plane** through which several
independent processes share NVMe controllers. One process **owns** a
controller: it opened the device, allocated the DMA memory, and holds the
admin queue. Any number of **clients** connect to what it owns and drive
their own I/O without re-initializing anything.

This is a host-orchestrated take on multiple paths to a namespace. ANA has
the controller report the paths and the host choose among them; here the host
makes them, since each client gets queues of its own to the same namespace and
the controller sees only the process that opened it. The parallel is in who
orchestrates rather than in the topology: ANA spans controllers, and this
shares one.

Ownership is not something a library user falls into. The server is
{ref}`sec-tools-homi`, which exists to be one, and a client that finds
nobody serving simply runs on its own.

```bash
# The server holds the controller for as long as it runs
homi start --cplane-id 1 --be upcie 0000:03:00.0

# Clients name the same id and connect to it
xnvme info 0000:03:00.0 --be upcie --cplane-id 1
```

The identifier is `--cplane-id`: the number a server listens under and a
client asks for. `--shm_id` still answers for it, since it named this before
there was a control plane and existing instrumentation passes it, but it names
no shared memory here. It keeps its own meaning for {ref}`sec-backends-spdk`,
which hands the value to DPDK, where it really is a segment identifier.

(sec-backends-upcie-cplane-model)=
## Process model

### Rendezvous

A server listens on a unix socket named for the identifier clients pass,
`/tmp/xnvme-homi-<cplane_id>.sock`. A client derives the same name from
`--cplane-id` and connects; finding nothing there means nobody is serving that
identifier, and the process builds its own runtime instead.

A socket rather than a shared segment because a segment cannot carry a file
descriptor, and under `vfio-pci` a descriptor is the only thing that grants
access to a device. It is a filesystem path rather than an abstract name
because an abstract address cannot be reached from another network namespace,
which a containerised client would need.

### What crosses when a connection is initialised

Descriptors, and offsets into what they describe:

- the DMA heap, which the client maps to see the same memory;
- BAR0, so that the client rings its own doorbells;
- the offset of a **runtime record**, written once by the server, naming the
  controller and where to find a description of the heap.

The heap description carries the physical address of each granule. The server
read those when it allocated, which needs `CAP_SYS_ADMIN`; leaving them where
the client will map them anyway is what lets an unprivileged client
translate at all.

### What a client does for itself, and what it asks for

A client submits I/O on queues it was allocated, ringing its own doorbell.
Nothing is on the socket during I/O.

Everything else is a request, because the resources belong to the server:

- **Queues.** Creating one means an admin command and memory from the heap, so
  the client asks and receives an identifier plus the offsets of its
  submission queue, completion queue and PRP scratch.
- **Memory.** The heap's allocator is the server's and its free list has no
  lock, so `xnvme_buf_alloc()` asks for an offset.
- **Admin commands.** There is one admin queue and it belongs to the server.
  The payload does not travel with the request: the command names an address
  the device can already reach, so an identify lands in the client's own
  buffer and only the command and its completion cross.

### Threads

The expected arrangement is several client processes at once, each allocating
buffers, creating queues and submitting admin commands, none of them
synchronising with any other. Making that safe is the server's job.

It serves one connection per thread. A request is read by blocking on the
socket it came from, so a client that writes half a message and pauses holds
its own thread and nothing else, and no part of the server is arranged around
the possibility.

Two locks say what is shared. One covers the DMA heap, whose free list has no
lock of its own, and the table of queues handed out. The other covers a
controller's admin queue, one per controller, because completions are reaped
from the head with no record of who submitted and a submission does not check
that the answer it takes is its own. Queue creation and deletion need both,
and take them in that order.

The second lock is what a command as long as a Format holds, and it is
deliberately not the first. While one runs, other clients of the same
controller can still allocate and free buffers, release queues and ask for
status; what waits is another admin command, or a queue being created or
deleted, which are the things that genuinely need the queue it is occupying.
Clients of the other controllers a server holds are untouched throughout.

A connection is a thread, so the number a controller will hold at once is
bounded, and the bound comes from the controller rather than from a constant:
what its queues can support, given a client may hold several, plus an
allowance for the clients that ask for none, since status and admin commands
need no queue. A server whose controller reports nothing falls back to a fixed
ceiling.

Within one client process, a queue belongs to one thread, as everywhere else
in xNVMe, but threads working separate queues are not expected to synchronise
with each other. The socket is the controller's rather than the queue's, so
asking for a queue, asking for a buffer and submitting an admin command all
meet on it, and a request and its reply are two calls that must not
interleave. The client serialises those per controller so a caller does not
have to.

### Liveness

The connection is the liveness signal. A client that exits cleanly hands its
queues and memory back; one that is killed mid-command does not, and the
socket closing is what tells the server to reclaim them, queues first so that
the controller loses its reach on an address before the address stops
resolving.

Nothing is left behind for anyone to clean up, and nothing has to be told
apart from debris: a socket that answers has a process behind it.

### Properties worth preserving

Three properties are easy to undo by accident and expensive to notice
afterwards, so they are worth naming.

A client holding several controllers keeps a connection and a runtime record
per controller. Sharing one across them makes every controller look like
whichever was opened first: the commands complete, the throughput looks
plausible, and the I/O lands on the wrong device.

Payload buffers are carved from hugepages of their own rather than sharing a
page with a queue. A controller is measurably slower at fetching submissions
and posting completions while payloads are written into the same 2 MiB page,
and first-fit will put them there unless the two allocations are kept apart.

A queue whose server has gone completes nothing, so a client watches its
control socket and gives up on what is in flight once the socket closes and a
command timeout has passed. Without that a client drains forever, holding the
heap it maps and the hugepages behind it.

(sec-backends-upcie-cplane-vulns)=
## Limitations

### No isolation between processes

A client maps BAR0 and can therefore ring any doorbell and write `CC`, which
is to say it can reset the controller. Under `vfio-pci` it also holds the
device descriptor. Clients are inside one trust domain by construction;
where that is unacceptable, the answer is the kernel driver, which arbitrates
because it owns the device.

### A connection reaches one controller; status reaches them all

No message names a controller, so one server listens on one socket per
controller it holds and answers each from the controller that socket belongs
to. A client therefore reaches exactly one controller per connection, and
initialising, allocating and admin all apply to that one.

Status is the exception, and it has to be: a caller asking what a server holds
cannot be told about one controller at a time without first knowing how many
there are. So a status request carries an index and the reply carries the
total, and a caller walks the range on any one connection. That keeps the
answer coming from the server rather than from what the socket names in `/tmp`
happen to look like.

### The server has to be answering

Initialising, allocating and admin all depend on the server being responsive.
Under the arrangement this replaced, a server stuck in a poll loop blocked
nobody, because those were reads of shared memory. That is the price of having
one place where a policy could be applied and of a record that needs no lock.

### GPU clients

A controller behind an IOMMU cannot yet DMA into VRAM: `IOMMU_IOAS_MAP_FILE`
does not accept dma-bufs exported by CUDA or HIP. GPU workloads therefore stay
on `uio_pci_generic` until that changes, and the vfio path serves
CPU-submitted I/O into host memory.
