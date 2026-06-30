(sec-backends-upcie-mproc)=
# uPCIe (multi-process)

The **upcie** backend supports a **multi-process** mode in which several
independent processes share NVMe controllers. In a multi-process setup, there
will always be one **primary** process, and any number of **secondary**
processes. The **primary** owns the DMA hugepages and the controller hardware
state. The **secondary** processes attach to that state and drive their own
I/O without re-initializing the device.

Multi-process mode is opt-in and enabled per device by supplying a non-zero
**shared-memory id** via the `--shm_id` option (or the `shm_id` field of
`struct xnvme_opts`). All processes that should share a controller must be
launched with the **same** `shm_id`.

```bash
# Primary (first process to claim the shm_id) and secondaries use the same id
xnvme info 0000:03:00.0 --be upcie --shm_id 1
```

(sec-backends-upcie-mproc-model)=
## Process model

### Role election

Roles are decided with an advisory file lock (`flock`) on a lock file keyed by
`shm_id` (`/tmp/xnvme-upcie-flock-<shm_id>`):

- The first process to acquire the exclusive, non-blocking lock becomes the
  **primary**. It holds the lock for its entire lifetime.
- Any process that finds the lock already held becomes a **secondary**.

### Shared memory segments

Two kinds of POSIX shared-memory segments are used:

- **Runtime segment** (`/xnvme-upcie-shm-<shm_id>`) — one per `shm_id`, created
  by the primary. It carries the hugepage backing-file path and the primary's
  hugepage virtual base address so that secondaries can import the same DMA
  memory. A shared `refcount` keeps track of how many processes are attached.
  Terminating a primary process while `refcount > 1` will not block, but instead
  only warn that the state of secondary processes will be corrupted.
- **Controller segment** (`/xnvme-upcie-<bdf>`) — one per physical controller,
  created by the primary. It embeds the full `struct nvme_controller`, a robust
  process-shared mutex guarding the admin queue, the bound driver name, an
  attach `refcount`, and an `is_initialized` publication flag. Again, primary
  processes will not be blocked from terminating even if secondary processes are
  still attached.

### Device ownership

The controller segment is keyed by device BDF, independent of `shm_id`. To stop
two primaries — for example two processes launched with different `shm_id`s —
from both driving the same physical controller, the creating primary holds a
second advisory lock keyed by the BDF (`/tmp/xnvme-upcie-<bdf>-lock`) for as long
as it owns the controller. A process that cannot acquire this lock fails rather
than creating a conflicting segment.

Because the kernel releases the lock when its holder exits, the controller
segment is self-healing across a primary crash: the next primary acquires the
released lock, unlinks any segment the dead primary left behind, and creates a
fresh one.

### Hugepage sharing

Every process allocates it own private DMA heap from its own hugepages. This
means that I/O queue pairs and buffers are allocated from a private heap. The
admin PRP pools are also allocated on the private heap, but the admin queue itself
is allocated only once on the heap of the primary process. The primary
publishes its hugepage backing-file path and virtual base into the runtime shared
memory segment; a secondary imports that hugepage purely to reach the admin
submission/completion rings and does not allocate from the imported mapping. The
import lands at a kernel-chosen address, so the admin-queue pointers stored in
the controller segment (which are primary-relative) are relocated into the
secondary's mapping by the constant offset between the secondary's import base
and the primary's published base. PRP entries for a secondary's own commands are
translated against its private heap, whose physical pages it resolves itself.

(sec-backends-upcie-mproc-startup)=
## Startup handshake

Secondaries may be launched concurrently with the primary, so attachment is
guarded by two ordering barriers:

1. **Runtime segment** — a secondary will wait up to ~1 second for the primary
   to publish the hugepage information before importing hugepages.
2. **Controller segment** — a secondary will wait up to ~1 second for the
   per-controller segment to (a) exist, (b) be fully sized, and (c) have its
   `is_initialized` flag published before reading any shared field.

If the primary does not become ready within the timeout, attachment fails with
`-ENOENT`.

(sec-backends-upcie-mproc-admin)=
## Admin queue serialization

There is a single admin queue per controller, shared by all processes. Access
is serialized with a **robust, process-shared** mutex (`PTHREAD_PROCESS_SHARED`
and `PTHREAD_MUTEX_ROBUST`) held in the controller segment:

- On **lock**, the holder reloads the admin queue head/tail/phase from shared
  memory into its local controller view.
- On **unlock**, the holder publishes its head/tail/phase back to shared memory.

Synchronous admin commands and I/O queue-pair create/delete operations all run
under this mutex. I/O queue identifiers are tracked in a shared bitmap in the
controller segment and mutated only while the mutex is held, so each process
creates and tears down its own I/O queue pairs without colliding.

I/O **data** transfers are not serialized: each process owns its own I/O queue
pairs and submits to them independently.

(sec-backends-upcie-mproc-recovery)=
### Crash Recovery

Because the admin mutex is robust, if a process dies while holding it, the next
locker receives `EOWNERDEAD`. In that case this process will:

1. Drain any completions the dead owner may have left in the admin completion
   queue, advancing the shared head/phase and ringing the completion-queue
   doorbell. Since the shared head/phase are only published on unlock and
   therefore never run ahead of the device, draining can only reap real or
   phantom completions — never skip one.
2. Mark the mutex consistent (`pthread_mutex_consistent`) and proceed.

If the mutex has been marked unrecoverable, locking fails with `ENOTRECOVERABLE`.

(sec-backends-upcie-mproc-teardown)=
## Teardown

Each process decrements the relevant `refcount` on exit. When the primary tears
down a controller it deletes **all** I/O queue pairs still marked allocated in
the shared bitmap before closing the controller, and then unmaps and unlinks the
shared segments. Secondaries detach by deleting their own queue pairs, releasing
their local request pool and BAR mapping, and unmapping the shared segments
without unlinking or freeing DMA memory they do not own.

(sec-backends-upcie-mproc-vulns)=
## Vulnerabilities and limitations

Multi-process mode is functional on the cooperative happy path but has known
robustness and trust gaps. It is intended for trusted, same-user processes and
should not be treated as a security boundary.

### Partial crash robustness

Owner-death recovery covers **only** the admin mutex and the admin completion
queue. Other state left behind by a crashed process is **not** reclaimed:

- **Leaked I/O queues and queue ids.** If a secondary crashes, the I/O queue
  ids it allocated remain marked as in-use in the shared bitmap and the
  corresponding device-side queues are never deleted. Over many crash/restart
  cycles this can exhaust the queue-id space until the primary tears the
  controller down.
- **Stale attach refcounts.** A crashed process never decrements its
  `refcount`. The primary's "secondaries still attached" accounting therefore
  becomes permanently inflated after any crash. The refcount is advisory only,
  and therefore it is not used for blocking or preventing a primary from
  exiting.

### Primary lifetime

The primary owns the hugepages and the controller hardware. If the primary
exits — cleanly or otherwise — while secondaries are still attached, those
secondaries are left with admin-queue mapping and controller that may have been
closed and freed. A clean primary teardown only logs a warning in this case; it
does not prevent it. Operators must ensure the primary outlives all secondaries.

### No isolation between processes

All participating processes map the controller segment read/write, including the
embedded `struct nvme_controller`. A misbehaving or compromised secondary can
corrupt shared state and affect every other process sharing the controller.
There is no privilege separation: any process running as the same user that can
open the shared-memory segments can submit admin commands to the device.

### Startup ordering and timeouts

Attachment waits at most ~1 second at each handshake barrier. A primary that
takes longer than that to initialize a controller will cause secondaries to time
out with `-ENOENT`. The timeout is fixed and not currently configurable.

### Compatibility with SPDK

SPDK's own multi-process mode claims **all** free hugepages on the system at
startup. Because uPCIe also needs hugepages for its DMA memory, the two cannot
be expected to share a system, and running uPCIe and SPDK in multi-process
mode side by side cannot be not guaranteed to work.

### CUDA backend

Multi-process mode is **not supported** with the `upcie-cuda` backend. The
backend does not advertise the multi-process capability, so opening an
`upcie-cuda` device with a non-zero `shm_id` fails with `-ENOTSUP`.

The memory model is not the obstacle: `upcie-cuda` keeps all NVMe control
structures — the admin queue, the I/O submission/completion rings, PRP lists
and request pools — on the same host hugepage heap that multi-process mode
shares, and places only data buffers in per-process GPU memory, which is never
shared between processes. What is missing is serialization: the `upcie-cuda`
admin path submits to the shared admin queue without taking the per-controller
admin mutex, so concurrent processes would race on the admin submission ring.
Enabling multi-process mode for `upcie-cuda` would require serializing its admin
path under the same mutex used by the host backend.
