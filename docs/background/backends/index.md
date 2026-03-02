(sec-backends)=

# Backend Configs

Each platform registers a set of backend configs — combinations of
async, sync, admin, dev, and mem implementations. These configs implement
the {ref}`sec-backends-intf` within the **xNVMe** library, offering a unified
interface through the **xNVMe** {ref}`sec-api`. This enables you to switch
between system interfaces, libraries, and backend configs at runtime without
altering your application logic.

While **xNVMe** abstracts away most differences, it's still important to
understand the specifics related to your platform, system interfaces, and
supporting libraries. For each platform we will cover the following topics:
System configuration, device identification, and available backend configs.

System Configuration
: Some platforms or I/O interfaces require you to configure your system in
  a certain way to use them. Examples include using a specific OS kernel or
  running a script to attach or detach devices.

Device Identification
: The schema for identifying devices differ across platforms and
  interfaces. Varying from file device handles, e.g. `/dev/nvme0n1`, to
  PCI device handles, e.g. `0000:02:00.0`, and NVMe/TCP endpoints, e.g.
  `172.10.10.10:4420`.

Backend Config Selection
: On a given platform, multiple configs may be available. At runtime,
  the **xNVMe** library selects a config with **caps** matching the target
  device. You can override this selection via `opts.be`, but to do so you
  need to know what the available configs are, what they are named, and
  what they offer.

The valid combinations of interfaces and platforms are listed below:

|  | {ref}`sec-platforms-linux` | {ref}`sec-platforms-freebsd` | {ref}`sec-platforms-windows` | {ref}`sec-platforms-macos` | {ref}`sec-backends-spdk` | {ref}`sec-backends-libvfn` | {ref}`sec-backends-upcie` |
|---|---|---|---|---|---|---|---|
| io_uring | **yes** | no | no | no | no | no | no |
| io_uring passthru | **yes** | no | no | no | no | no | no |
| libaio | **yes** | no | no | no | no | no | no |
| kqueue | no | **yes** | no | no | no | no | no |
| POSIX aio | **yes** | **yes** | no | **yes** | no | no | no |
| NVMe Driver (vfio-pci) | no | no | no | no | **yes** | **yes** | no |
| NVMe Driver (uio-generic) | no | no | no | no | **yes** | no | **yes** |
| Driverkit (MacVFN) | no | no | no | **yes** | no | no | no |
| I/O Control Ports | no | no | **yes** | no | no | no | no |
| I/O Ring | no | no | **yes** | no | no | no | no |
| emu | **yes** | **yes** | **yes** | **yes** | **yes** | **yes** | **yes** |
| thrpool | **yes** | **yes** | **yes** | **yes** | **yes** | **yes** | **yes** |
| nil | **yes** | **yes** | **yes** | **yes** | **yes** | **yes** | **yes** |

|  | {ref}`sec-platforms-linux` | {ref}`sec-platforms-freebsd` | {ref}`sec-platforms-windows` | {ref}`sec-platforms-macos` | {ref}`sec-backends-spdk` | {ref}`sec-backends-libvfn` | {ref}`sec-backends-upcie` |
|---|---|---|---|---|---|---|---|
| psync | **yes** | **yes** | **yes** | **yes** | no | no | no |
| Block-Layer ioctl() | **yes** | no | **yes** | no | no | no | no |
| NVMe Driver ioctl() | **yes** | **yes** | **yes** | no | no | no | no |
| NVMe Driver (vfio-pci) | no | no | no | no | **yes** | **yes** | no |
| NVMe Driver (uio-generic) | no | no | no | no | **yes** | no | **yes** |

|  | {ref}`sec-platforms-linux` | {ref}`sec-platforms-freebsd` | {ref}`sec-platforms-windows` | {ref}`sec-platforms-macos` | {ref}`sec-backends-spdk` | {ref}`sec-backends-libvfn` | {ref}`sec-backends-upcie` |
|---|---|---|---|---|---|---|---|
| libc | **yes** | **yes** | **yes** | **yes** | **yes** | **yes** | no |
| hugepages | **yes** | no | no | no | no | no | **yes** |


```{toctree}
:maxdepth: 2
:hidden:

spdk/index
libvfn/index
upcie/index
xnvme_be_macos
common/index
common/interface/index
```
