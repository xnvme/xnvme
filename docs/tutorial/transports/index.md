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
`nvme_target_probe`, and `nvme_target_stop` cijoe scripts; `start` and `stop`
are switched via `--nvme-provider`, `probe` discovers whichever is listening.
The initiator side (xNVMe access via a fabrics URI) is identical between the
two and is documented below.

```{toctree}
:hidden:

spdk/index
linux/index
```

## Script configuration

`nvme_target_start.py`, `nvme_target_probe.py`, and `nvme_target_stop.py`
each take their own set of arguments; `--transport-name` is the one all three
share.

`nvme_target_start.py` accepts the following arguments:

| Argument            | Default     | Description                                                        |
|---------------------|-------------|--------------------------------------------------------------------|
| `--nvme-provider`   | `spdk`      | `spdk` or `linux`.                                                 |
| `--nvme-trtype`     | `tcp`       | Transport type.                                                    |
| `--nvme-adrfam`     | `ipv4`      | Address family.                                                    |
| `--transport-name`  | None        | CIJOE Transport to use. Defaults to first-found, if not specified. |

`nvme_target_probe.py` accepts the following arguments:

| Argument            | Default     | Description                                                        |
|---------------------|-------------|--------------------------------------------------------------------|
| `--nvme-trtype`     | `tcp`       | Transport type.                                                    |
| `--transport-name`  | None        | CIJOE Transport to use. Defaults to first-found, if not specified. |

`nvme_target_stop.py` accepts the following arguments:

| Argument            | Default     | Description                                                        |
|---------------------|-------------|--------------------------------------------------------------------|
| `--nvme-provider`   | `spdk`      | `spdk` or `linux`.                                                 |
| `--transport-name`  | None        | CIJOE Transport to use. Defaults to first-found, if not specified. |

The devices to export are the entries labelled `fabrics` (legacy label
name) in the cijoe configuration. Each entry becomes one subsystem. An entry
has these fields:

| Field         | Description                                                     |
|---------------|-----------------------------------------------------------------|
| `uri`         | Address and port to listen on, e.g. `127.0.0.1:4420`.           |
| `pcie_id`     | PCIe address of the device, e.g. `0000:04:00.0`.                |
| `subnqn`      | NQN of the subsystem.                                           |
| `passthrough` | `true` to pass commands through to the device. Default `false`. |
| `device_path` | Block device, e.g. `/dev/nvme3n1`. Linux provider only.         |
| `ctrl_path`   | Controller, e.g. `/dev/nvme1`. Linux provider with passthrough. |

Give every entry its own port, so the initiator can find each subsystem
without a `--subnqn`. Command sets such as ZNS and KV need passthrough.

`nvme_target_probe.py` probes the first entry.

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
