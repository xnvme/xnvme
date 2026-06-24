(sec-tutorials-transports)=

# NVMe Transports

The NVMe specification defines several transports for issuing commands to a
controller. **PCIe** is the local memory-mapped transport used by directly
attached SSDs. **TCP** and **RDMA** (iWARP, InfiniBand, RoCE) are
network-message transports for accessing a remote controller over the
network.

A note on vocabulary. Material you may have read elsewhere refers to
the network-message transports collectively as **NVMe-over-Fabrics**
(NVMe-oF). The NVMe 2.0 refactor folded the previously separate base and
fabrics specifications into a single document and unified the
vocabulary: PCIe, TCP, and RDMA are all just *transports* now. This
page uses the post-2.0 terminology; older guides and tools may still
say "fabrics" for the same network paths.

The terms *initiator* and *target* are not defined by the NVMe
specification at all. They are inherited from SCSI, where they have
precise meaning, and have become the common informal vocabulary used to
label the two roles involved in an NVMe network transport such as TCP
or RDMA: the *initiator* is the side issuing commands, the *target* is
the side processing them. This page uses them in that informal sense.

```{figure} ../../_static/transports.svg
:alt: Roles in an NVMe TCP transport setup
:align: center

Roles in an NVMe TCP transport setup
```

xNVMe currently implements only the initiator side of the NVMe protocol;
it does not provide an NVMe target. To exercise the TCP transport the
target role is delegated to one of two external implementations,
documented on their own pages. A native target along with device
emulation is on the roadmap, with no committed timeline.

[**SPDK Target**](spdk/index.md) uses the user-space `nvmf_tgt`
application. The PCIe device is bound to the userspace driver via
`xnvme-driver`, attached to `nvmf_tgt` as `Nvme0`, and published as a TCP
subsystem through `rpc.py`.

[**Linux Target**](linux/index.md) uses the in-tree `nvmet` driver
configured through `configfs`. The PCIe device is rebound to the kernel
NVMe driver so its local `/dev/nvmeXn1` can be exported as a `nvmet`
namespace.

Both providers are driven by the same `nvme_target_start`,
`nvme_target_probe`, and `nvme_target_stop` cijoe scripts, switched via
`--provider`. The initiator side (xNVMe access via TCP URI) is identical
between the two and is documented below.

```{toctree}
:hidden:

spdk/index
linux/index
```

## Shared configuration

The three cijoe scripts share the same set of flags:

| Argument       | Default       | Description                                  |
|----------------|---------------|----------------------------------------------|
| `--provider`   | `spdk`        | `spdk` or `linux`.                           |
| `--traddr`     | `127.0.0.1`   | Transport address (IP) for the listener.     |
| `--trsvcid`    | `4420`        | Transport service id (TCP port).             |
| `--trtype`     | `tcp`         | Transport type.                              |
| `--adrfam`     | `ipv4`        | Address family.                              |

The PCIe device to export and its subsystem NQN are read from a device
entry labelled `fabrics` (legacy label name) in the cijoe configuration.
The expected fields are `pcie_id` (e.g. `0000:04:00.0`), `subnqn` (the
target NQN), and `device_path` (the local `/dev/nvmeXn1` used by the
Linux provider).

## Initiator usage

The probe step in each provider's demo workflow exercises the initiator
side against the brought-up listener. The xNVMe commands it issues are
extracted from the live cijoe run and shown below:

```{literalinclude} ../../_static/transports/initiator.txt
:language: bash
```

The captured output for each command, along with the surrounding
`nvme discover` invocation, is visible in the embedded report on the
[SPDK Target](spdk/index.md) or [Linux Target](linux/index.md) page.

## Footguns and rules of thumb

The local loop-back scenario (initiator and target on the same machine
over `127.0.0.1`) is a useful sanity check but is bounded by CPU and
memory rather than the device, and its numbers should not be read as
representative of the TCP transport. The SPDK target pins `nvmf_tgt` to
core 1 via `-m [1]` and busy-polls that core at 100% even when idle, so
any initiator thread that also lands on core 1 starves immediately; pin
the initiator elsewhere with `--cpumask 0x4` (or any mask whose set bits
exclude bit 1). With the cores
split, the two processes still share L3 cache and memory channels, so at
small block sizes a single core can saturate the loop-back I/O path
while the device itself is barely touched, and at large block sizes
memory bandwidth becomes the next ceiling. The kernel's TCP/IP stack
also runs in full in the data path with its usual per-byte CPU cost, so
what looks like a fabric benchmark is, in this configuration, largely a
CPU benchmark.

Across separate machines the visible limit shifts to the link itself.
Use the fastest interface available; on direct back-to-back links raise
the MTU as high as the cards support
(`ip link set dev ${NETWORK_INTERFACE} mtu ${DESIRED_MTU}`). If the
device cannot be saturated through the available network, hide more
latency by using a larger block size and/or higher io-depth.
