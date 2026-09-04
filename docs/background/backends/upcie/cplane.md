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
homi start --homi-id 1 --be upcie 0000:03:00.0

# Clients name the same id and connect to it
xnvme info 0000:03:00.0 --be upcie --homi-id 1
```

The identifier is `--homi-id`: the number a server listens under and a client
asks for. **homi** and **xnvmeperf** take that spelling and no other. Tools
that are not about sharing still accept `--shm_id`, which named this before
there was a control plane, and it goes on meaning a segment identifier for
{ref}`sec-backends-spdk`, which hands the value to DPDK. Either spelling sets
both, so a value given once reaches whichever backend reads it.

(sec-backends-upcie-cplane-model)=
## Process model

### Rendezvous

A server listens on a unix socket named for the identifier clients pass,
`/tmp/xnvme-homi-<homi_id>.sock`. A client derives the same name from
`--homi-id` and connects; finding nothing there means nobody is serving that
identifier, and the process builds its own runtime instead.

One socket for the whole runtime, whatever it holds, and one connection per
client process. Every request names the controller it is about, so a client
using several needs no more than the one connection, and nothing about what a
server holds has to be encoded in a file name.

Every message carries the version its sender speaks, and a peer speaking
another one is refused and then let go. Refusing is not enough on its own:
versions differ because the message does, so a refused peer is waiting for a
reply of a size the server does not send, and only the socket ending tells it
so.

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

One thread reads every connection. Most requests are memory and are answered
where they are read: handing over the descriptors, carving a buffer from the
heap, giving one back, reporting what is held. A request is taken as it arrives
rather than waited for, so a client that writes half a message and pauses is a
connection that is not ready yet, and costs a partly-filled buffer.

What reaches the controller cannot be answered there. An admin command can run
as long as a Format, and creating or deleting a queue is a round trip of its
own, so those go to a thread per controller; so does releasing what a client
held, since giving a queue back means telling the controller. That is the
granularity the admin queue already has, because completions are reaped from
the head with no record of who submitted and a submission does not check that
the answer it takes is its own. One thread draining a queue of pending work is
what serialises it, so no lock is needed over it.

One lock remains, over the DMA heap, whose free list has none of its own, and
the connection table. A controller's thread never holds it across an admin
command, so while a Format runs, clients of that controller can still allocate
and free buffers and ask for status; what waits is another admin command, or a
queue being created or deleted. Clients of the other controllers a server holds
are untouched throughout.

Creating and deleting a queue is the exception, and does hold it, because
allocating the memory and telling the controller about it are made in one call
and the allocation is the heap's. So the reader can be held for a Create or a
Delete round trip, which is bounded and short where an admin command is
neither. Splitting the call in two would remove even that, and has not been
worth doing.

The number of connections a server will hold at once is bounded, and by a
constant, since nothing about the hardware predicts it: a connection is a
client process, and most take one queue while some take none. What the
controllers have does bound the queues one connection may hold, which is a
separate limit and the one that refuses with `-ENOSPC`.

Within one client process, a queue belongs to one thread, as everywhere else
in xNVMe, but threads working separate queues are not expected to synchronise
with each other. The socket is the process's rather than the queue's or the
controller's, so asking for a queue, asking for a buffer and submitting an
admin command all meet on it, and a request and its reply are two calls that
must not interleave. The client serialises those so a caller does not have to.

A reply is never waited on. One thread answers every connection, so a client
that stops reading its replies while carrying on sending requests would stall
that thread and with it everybody else. The socket buffer holds thousands of
replies, so nothing but a client that has stopped reading can fill it, and
such a client is let go.

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
is to say it can reset the controller. What it maps is the whole of BAR0
rather than the doorbells it was given, so what a client can reach is not
bounded by what it was allocated: one that asked for a single queue can ring
the doorbells of every other queue on the controller, including queues held by
other clients. Under `vfio-pci` it also holds the device descriptor, and a GPU
client maps the same registers a second time through the BAR's sysfs resource,
which is the only mapping its kernels can be given.

Admission to the socket is therefore the whole of the boundary, and it is a
coarse one. It is not quite the whole of what a client needs, either: a GPU
issuing its own I/O reaches the doorbells through the BAR's sysfs resource
rather than the descriptor it was handed, so that client needs privileges of
its own rather than only what the server gives it. Handing out only the doorbell page a client was allocated would be
finer, and the register layout allows it where the page granularity happens to
line up, but that is not what this does. Clients are inside one trust domain by
construction; where that is unacceptable, the answer is the kernel driver,
which arbitrates because it owns the device.

### The message names the controller, so the socket does not have to

Every request carries the index of the controller it is about, and a reply to
a status request at index zero says how many there are, so a client walks the
range to find the one it wants and matches on the identifier the server
publishes.

The alternative is a socket per controller, and then the file name is the
addressing. That costs more than it looks: a client has to know what a server
holds before it can ask what a server holds, a controller with an awkward
identifier has to be spelled the same way at both ends, and a process using
several controllers pays a connection for each. Naming the controller in the
message removes all three, and it is four bytes.

### The server has to be answering

Initialising, allocating and admin all depend on the server being responsive.
Under the arrangement this replaced, a server stuck in a poll loop blocked
nobody, because those were reads of shared memory. That is the price of having
one place where a policy could be applied and of a record that needs no lock.

### GPU clients

A GPU client registers its own device memory and the server describes it to
the controller, so I/O a client submits from the host reaches VRAM under either
attachment mode. Behind an IOMMU that description cannot come from mapping the
dma-buf, since `IOMMU_IOAS_MAP_FILE` does not accept one exported by CUDA or
HIP; it is installed from the physical addresses behind it instead, by the
server here and by a process that opened the controller for itself.

I/O the GPU issues itself needs no server at all: a controller this process
opened builds the same queue in the same device memory, and only the identifier
and the admin queue come from somewhere else. It does ask for more of the
platform, and gets it only while the GPU's own domain passes through. The
controller's domain is not what decides: it translates either way, through the
one `vfio-pci` installs for it. The GPU stays on its own driver and uses the
default domain, so `iommu=pt` is the setting that matters. The queue lives in device memory and the doorbell has to be in
the GPU's address space; with domains that translate, what the CUDA runtime
installs for the BAR does not carry, and neither the IOMMU nor the GPU reports
anything, so the I/O simply never completes. GPU-initiated I/O therefore wants
identity-mapped domains for now.
