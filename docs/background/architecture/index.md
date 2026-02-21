# xNVMe Backend Architecture Refactor

## Overview

This refactor separates concerns into:
- **Platform**: OS-specific device discovery and lifecycle (one per build, compile-time selected)
- **Backend configs**: Complete I/O path configurations (`xnvme_be_config`), selected at runtime
  via `opts.be` matching `xnvme_be_config.attr.name`, or automatically via URI classification
- **cref**: Centralized controller reference counting

## Terminology

| Term                  | Meaning                                                                |
| --------------------- | ---------------------------------------------------------------------- |
| **io-path**           | How I/O is submitted (io_uring_cmd, io_uring, libaio, spdk, etc.)      |
| **backend config**    | A `xnvme_be_config` -- complete recipe of async+sync+admin+dev+mem     |
| **OS-managed**        | Device owned by kernel driver, **xNVMe** selects io-path               |
| **Userspace-managed** | Device owned by userspace driver, io-path bundled                      |
| **multipath**         | Multiple io-paths active concurrently (future)                         |
| **kernel_driver**     | Kernel module the device is bound to (nvme, vfio-pci, uio_pci_generic) |
| **caps**              | Capability bitmask on each config, matched against URI classification  |

### kernel_driver vs opts.be

Two related but distinct concepts:

- **`xnvme_ident.kernel_driver`** - Kernel module the device is currently bound to.
  Populated by scan for PCIe devices, reflects current system state.
  Empty for fabrics (transport is encoded in URI).

  | Platform | Values                          | Notes                                    |
  | -------- | ------------------------------- | ---------------------------------------- |
  | Linux    | nvme, vfio-pci, uio_pci_generic | Well-defined, visible in sysfs           |
  | FreeBSD  | nvme                            | Userspace binding TBD                    |
  | Windows  | (empty)                         | No userspace binding mechanism           |
  | macOS    | (TBD)                           | DriverKit visibility needs investigation |

- **`xnvme_opts.be`** - Which backend config to use for I/O.
  Matches on `xnvme_be_config.attr.name`. When set, only configs with a
  matching name are considered.
  Examples: "io_uring", "io_uring_cmd", "libaio", "thrpool", "emu",
  "io_uring_bdev", "spdk", "vfio", "ramdisk", "driverkit"

The kernel_driver informs backend config compatibility:
- `kernel_driver=nvme` → can use linux configs (io_uring, libaio, etc.)
- `kernel_driver=vfio-pci` → must use vfio config
- `kernel_driver=uio_pci_generic` → must use uio config

## Current Problems

1. Multiple backends compete with duplicated logic (SPDK and libvfn both do refcounting)
2. No conflict detection between user-space drivers on the same device
3. Mixins allow invalid combinations (e.g., `async=spdk, sync=io_uring`)
4. Semantic mismatches when mixing NVMe admin with block I/O ("shim problem")
5. Confusing errors from probing: when multiple backends are tried, the returned error
   is from the last attempt, not the most relevant failure
6. Debug noise: each probed backend emits debug output, obscuring what actually happened

## Future: IPC and Shared Access

This refactor also prepares for inter-process communication (IPC). The goal is to
enable sharing of device/controller handles and I/O queues between processes:

- A daemon process initializes a userspace driver and opens a controller
- Other processes connect to the daemon and obtain their own I/O queue pairs
- Each process works directly with its queue, bypassing the daemon for I/O
- The daemon manages controller lifecycle; clients manage their queues

Centralized reference counting (cref) and clean config separation are prerequisites
for this model - the daemon must own the controller while clients hold queue references.

### SPDK Multi-Process Mode Compatibility

SPDK already supports multi-process sharing. **xNVMe**'s IPC model should align with it:

**SPDK's model:**
- Primary process: initializes SPDK env, attaches controllers, creates shared memory
- Secondary processes: connect to primary's shared memory, allocate their own queue pairs
- Shared memory contains: controller handles, namespace data, memory pools
- Each process has exclusive access to its queue pairs

**xNVMe alignment:**

| Concept              | SPDK               | **xNVMe**         |
| -------------------- | ------------------ | ----------------- |
| Process role         | Primary/Secondary  | Daemon/Client     |
| Controller ownership | Primary            | Daemon (via cref) |
| Queue ownership      | Per-process        | Per-process       |
| Shared state         | SPDK shared memory | Driver-specific   |

**Approach:** Follow SPDK's explicit model - no automatic detection.

```c
// Primary (daemon) - initializes controller, creates shared memory
xnvme_dev_open("0000:03:00.0", &(struct xnvme_opts){
    .be = "spdk",
    .shm_id = 1,  // Explicit shared memory ID
});

// Secondary (client) - connects to existing primary
xnvme_dev_open("0000:03:00.0", &(struct xnvme_opts){
    .be = "spdk",
    .shm_id = 1,
    .secondary = true,
});
```

**Why explicit over automatic:**
- No race conditions (role is known upfront)
- Clear ownership semantics
- No stale shared memory issues
- Predictable behavior
- Familiar to SPDK users (same model)

### Queue Management for IPC

Each process owns its queues exclusively. Queues are not shared between processes.

**Ownership model:**
- Controller: owned by primary, shared via cref
- Queues: owned by allocating process, not shared

**Queue lifecycle:**
```c
// Primary (daemon)
struct xnvme_dev *dev = xnvme_dev_open(...);  // Owns controller
struct xnvme_queue *q1 = xnvme_queue_init(dev, 128, 0);  // Daemon's queue

// Secondary (client)
struct xnvme_dev *dev = xnvme_dev_open(...);  // Connects to controller
struct xnvme_queue *q2 = xnvme_queue_init(dev, 128, 0);  // Client's own queue
// q2 is independent of q1, allocated from shared memory pool
```

**Queue allocation:**
- Primary initializes memory pools in shared memory
- Secondary allocates queues from shared pools
- Each queue belongs to one process
- Queue memory comes from driver's shared memory region

**Queue cleanup:**
- `xnvme_queue_term()` returns queue resources to pool
- Process exit should clean up its queues
- Primary exit terminates all - secondaries lose access

**Deferred:** Core-binding support (SPDK reactor affinity) - punted for now.

**Open questions:**
- How do non-SPDK drivers (vfio, uio) implement similar sharing?

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                            xNVMe                            │
├─────────────────────────────────────────────────────────────┤
│  g_xnvme_platform (compile-time selected via #ifdef)        │
│    ├── name: "linux" | "freebsd" | "macos" | "windows"      │
│    ├── backends[] (NULL-terminated xnvme_be_config array)   │
│    ├── classify()  -- URI → xnvme_be_cap bitmask            │
│    ├── dev_open()                                           │
│    ├── scan()                                               │
│    └── enumerate()                                          │
├─────────────────────────────────────────────────────────────┤
│  Backend Configs (platform->backends[])                     │
│                                                             │
│  Each xnvme_be_config is a complete recipe:                 │
│    { async, sync, admin, dev, mem, mem_overrides, attr }    │
│                                                             │
│  OS-managed configs:                                        │
│    linux: io_uring_cmd+nvme, io_uring+nvme, libaio+nvme,    │
│           io_uring+block, libaio+block, thrpool, emu, ...   │
│    - Each config has caps indicating device type support    │
│    - Mem overridable (posix default, hugepage optional)     │
│                                                             │
│  Userspace-managed configs:                                 │
│    spdk, vfio, uio, driverkit                               │
│    - Full lifecycle (init/term, dev_open/close)             │
│    - Bundle all interfaces (async, sync, admin, mem)        │
│    - Exclusive access via cref                              │
├─────────────────────────────────────────────────────────────┤
│  Cross-platform components                                  │
│                                                             │
│  CBI: emu, thrpool, nil, posix mem, psync                   │
│  Ramdisk: virtual device driver (reusable by any platform)  │
│  cref: controller reference counting                        │
└─────────────────────────────────────────────────────────────┘
```

## Data Structures

### URI as Convenience

The URI is a convenience for the user at the CLI boundary. It is parsed
immediately at `dev_open()` into structured fields and never used for internal
logic. After parsing, all library code works with the structured `xnvme_ident`
fields -- no string matching, no re-parsing.

**Principle:** URI in, structured data out. The URI does not survive past the
entry point.

```
User types:   /dev/ng0n1
                  ↓
dev_open():   xnvme_ident_from_uri() → populates structured fields
                  ↓
Library uses: ident.trtype, ident.traddr, ident.nsid, ...
```

This means:
- `xnvme_ident` stores parsed components, not a URI string
- Scan results are structured data, not URI strings
- `xnvme_ident_to_uri()` exists for display/logging only
- No internal code path ever inspects a URI to make decisions

### xnvme_ident

Device identifier with parsed transport components:

```c
struct xnvme_ident {
	uint8_t trtype;         // XNVME_TRTYPE_PCIE, _TCP, _RDMA
	char traddr[256];       // IP address or PCI BDF
	char trsvcid[32];       // Port (fabrics) or empty
	char subnqn[256];       // Subsystem NQN
	uint32_t dtype;         // Device type
	uint32_t nsid;          // Namespace ID
	uint8_t csi;            // Command set identifier
	char kernel_driver[32]; // OS binding (PCIe only)
};

// Parse URI string into ident components (entry point only)
int xnvme_ident_from_uri(const char *uri, struct xnvme_ident *ident);

// Construct URI string from ident (display/logging only)
int xnvme_ident_to_uri(const struct xnvme_ident *ident, char *uri, size_t len);
```

**Pretty-printing:** The `xnvme_ident` pretty-printer (`xnvme_ident_pr()`)
should be type-aware -- only show fields that are meaningful for the identified
device. For example:

| Field         | PCIe (kernel_driver=nvme) | PCIe (vfio-pci/uio) | TCP/RDMA   |
| ------------- | ------------------------- | ------------------- | ---------- |
| traddr        | BDF                       | BDF                 | IP address |
| trsvcid       | omit                      | omit                | port       |
| subnqn        | omit                      | omit                | show       |
| nsid          | show (from sysfs)         | omit                | show       |
| kernel_driver | show                      | show                | omit       |

This avoids printing empty or irrelevant fields (e.g., `subnqn: ""` for a
local PCIe device, `kernel_driver: ""` for a fabrics target).

### API

```c
// URI-based open - parses URI immediately, then works with structured ident
struct xnvme_dev *xnvme_dev_open(const char *uri, struct xnvme_opts *opts);

// Ident-based open - skips parsing, for programmatic use and scan results
struct xnvme_dev *xnvme_dev_open_ident(const struct xnvme_ident *ident,
                                       struct xnvme_opts *opts);
```

Both paths converge immediately: `xnvme_dev_open()` calls
`xnvme_ident_from_uri()` then delegates to `xnvme_dev_open_ident()`.

**Usage scenarios:**

```c
// CLI tools: user provides URI string
struct xnvme_dev *dev = xnvme_dev_open("/dev/ng0n1", NULL);

// Programmatic: construct ident directly
struct xnvme_ident ident = {
    .trtype = XNVME_TRTYPE_PCIE,
    .dtype = XNVME_DTYPE_NVME_CHAR,
    .nsid = 1,
};
strncpy(ident.traddr, "0000:03:00.0", sizeof(ident.traddr));
struct xnvme_dev *dev = xnvme_dev_open_ident(&ident, NULL);

// Scan results: already structured, no URI round-trip
void scan_cb(const struct xnvme_scan_entry *entry, void *args)
{
    struct xnvme_ident ident = {
        .trtype = XNVME_TRTYPE_PCIE,
        .nsid = entry->nsid,
    };
    strncpy(ident.traddr, entry->ctrlr->bdf, sizeof(ident.traddr));
    struct xnvme_dev *dev = xnvme_dev_open_ident(&ident, NULL);
}
```

### xnvme_opts

```c
struct xnvme_opts {
	const char *be;       // Backend config name (matches xnvme_be_config.attr.name)
	const char *async;    // Async interface filter
	const char *sync;     // Sync interface filter
	const char *admin;    // Admin interface filter
	const char *mem;      // Memory allocator override (e.g., "hugepage")
	uint32_t nsid;        // Namespace ID (default: 1)
	uint32_t shm_id;      // Shared memory ID for IPC (0 = no sharing)
	const char *hostnqn;  // Optional: host NQN for fabrics access control
	// ... platform-specific fields
};
```

**Usage:**

```c
// Userspace-managed drivers (mem is fixed by config)
xnvme_dev_open("0000:03:00.0", &(struct xnvme_opts){.be = "spdk"});
xnvme_dev_open("0000:03:00.0", &(struct xnvme_opts){.be = "vfio"});

// OS-managed -- select specific config by name
xnvme_dev_open("/dev/ng0n1", &(struct xnvme_opts){.be = "io_uring_cmd"});
xnvme_dev_open("/dev/nvme0n1", &(struct xnvme_opts){.be = "io_uring"});

// OS-managed with hugepage memory
xnvme_dev_open("/dev/nvme0n1",
	       &(struct xnvme_opts){.be = "io_uring", .mem = "hugepage"});

// Default -- auto-select via URI classification and caps matching
xnvme_dev_open("/dev/nvme0n1", NULL);
```

### xnvme_be_config

Each backend config is a complete "recipe" for populating a `struct xnvme_be`.
The `attr.caps` bitmask indicates which device types the config supports, enabling
URI-based automatic selection.

```c
enum xnvme_be_cap {
	XNVME_BE_CAP_NVME_CDEV = 0x1 << 0, // NVMe char device (/dev/ng0n1)
	XNVME_BE_CAP_NVME_BDEV = 0x1 << 1, // NVMe block device (/dev/nvme0n1)
	XNVME_BE_CAP_BDEV      = 0x1 << 2, // Generic block device
	XNVME_BE_CAP_FILE      = 0x1 << 3, // Regular file
	XNVME_BE_CAP_RAMDISK   = 0x1 << 4, // Ramdisk virtual device
	XNVME_BE_CAP_NVME_PCIE = 0x1 << 5, // NVMe over PCIe (user-space)
	XNVME_BE_CAP_NVME_TCP  = 0x1 << 6, // NVMe over TCP (user-space)
	XNVME_BE_CAP_NVME_RDMA = 0x1 << 7, // NVMe over RDMA (user-space)
};

struct xnvme_be_config {
	const struct xnvme_be_async *async;
	const struct xnvme_be_sync *sync;
	const struct xnvme_be_admin *admin;
	const struct xnvme_be_dev *dev;
	const struct xnvme_be_mem *mem;
	const struct xnvme_be_mem *const *mem_overrides; // Optional overrides
	struct xnvme_be_attr attr;  // { name, descr, caps, enabled }
};
```

**Example configs (Linux):**

```c
// NVMe passthrough via io_uring_cmd
const struct xnvme_be_config g_xnvme_be_linux_ucmd_nvme = {
	.async = &g_xnvme_be_linux_async_ucmd,
	.sync = &g_xnvme_be_linux_sync_nvme,
	.admin = &g_xnvme_be_linux_admin_nvme,
	.dev = &g_xnvme_be_dev_linux,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.mem_overrides = (const struct xnvme_be_mem *const[]){
		&g_xnvme_be_linux_mem_hugepage, NULL,
	},
	.attr = {.name = "io_uring_cmd",
		 .descr = "io_uring passthru with NVMe ioctl",
		 .caps = XNVME_BE_CAP_NVME_CDEV | XNVME_BE_CAP_NVME_BDEV,
		 .enabled = 1},
};

// File I/O via io_uring
const struct xnvme_be_config g_xnvme_be_linux_iou_file = {
	.async = &g_xnvme_be_linux_async_liburing,
	.sync = &g_xnvme_be_cbi_sync_psync,
	.admin = &g_xnvme_be_cbi_admin_shim,
	.dev = &g_xnvme_be_dev_linux,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.mem_overrides = (const struct xnvme_be_mem *const[]){
		&g_xnvme_be_linux_mem_hugepage, NULL,
	},
	.attr = {.name = "io_uring_file",
		 .descr = "io_uring with file I/O",
		 .caps = XNVME_BE_CAP_FILE,
		 .enabled = 1},
};

// SPDK userspace driver
const struct xnvme_be_config g_xnvme_be_spdk = {
	.async = &g_xnvme_be_spdk_async,
	.sync = &g_xnvme_be_spdk_sync,
	.admin = &g_xnvme_be_spdk_admin,
	.dev = &g_xnvme_be_spdk_dev,
	.mem = &g_xnvme_be_spdk_mem,
	.attr = {.name = "spdk",
		 .descr = "SPDK userspace NVMe driver",
		 .caps = XNVME_BE_CAP_NVME_PCIE | XNVME_BE_CAP_NVME_TCP
		       | XNVME_BE_CAP_NVME_RDMA,
		 .enabled = 1},
};
```

### Backend Config Definitions

Each platform defines a flat array of `xnvme_be_config` pointers. The ordering
determines default priority -- the first matching config wins.

**Linux configs:**

| Name (`attr.name`) | async        | sync    | admin   | caps                 |
| ------------------ | ------------ | ------- | ------- | -------------------- |
| `emu`              | emu          | nvme    | nvme    | NVME_CDEV, NVME_BDEV |
| `emu_bdev`         | emu          | block   | block   | NVME_BDEV, BDEV      |
| `io_uring_cmd`     | io_uring_cmd | nvme    | nvme    | NVME_CDEV, NVME_BDEV |
| `io_uring`         | io_uring     | nvme    | nvme    | NVME_CDEV, NVME_BDEV |
| `io_uring_bdev`    | io_uring     | block   | block   | NVME_BDEV, BDEV      |
| `libaio`           | libaio       | nvme    | nvme    | NVME_CDEV, NVME_BDEV |
| `libaio_bdev`      | libaio       | block   | block   | NVME_BDEV, BDEV      |
| `posix`            | posix        | nvme    | nvme    | NVME_CDEV, NVME_BDEV |
| `thrpool`          | thrpool      | nvme    | nvme    | NVME_CDEV, NVME_BDEV |
| `thrpool_bdev`     | thrpool      | block   | block   | NVME_BDEV, BDEV      |
| `nil`              | nil          | nvme    | nvme    | NVME_CDEV, NVME_BDEV |
| `emu_file`         | emu          | psync   | shim    | FILE                 |
| `thrpool_file`     | thrpool      | psync   | shim    | FILE                 |
| `io_uring_file`    | io_uring     | psync   | shim    | FILE                 |
| `libaio_file`      | libaio       | psync   | shim    | FILE                 |
| `ramdisk`          | nil          | ramdisk | ramdisk | RAMDISK              |
| `ramdisk_thrpool`  | thrpool      | ramdisk | ramdisk | RAMDISK              |
| `ramdisk_emu`      | emu          | ramdisk | ramdisk | RAMDISK              |

The `mem` on all Linux configs defaults to `posix` with `hugepage` available
as an override via `opts.mem`.

### URI Format

URIs identify devices only - no query parameters. All configuration via opts.

| Type     | Format                            | Example                                            |
| -------- | --------------------------------- | -------------------------------------------------- |
| PCIe BDF | `<domain>:<bus>:<dev>.<func>`     | `0000:03:00.0`                                     |
| TCP      | `nvme+tcp://<host>:<port>/<nqn>`  | `nvme+tcp://192.168.1.100:4420/nqn.example:subsys` |
| RDMA     | `nvme+rdma://<host>:<port>/<nqn>` | `nvme+rdma://10.0.0.1:4420/nqn.example:subsys`     |
| Path     | filesystem path                   | `/dev/ng0n1`, `/dev/nvme0n1`, `/tmp/file`          |

### URI Classification and Caps Matching

Each platform implements a `classify()` callback that maps a URI to a
`xnvme_be_cap` bitmask. This classification is used to filter the backends
array during config selection.

**Linux classification (via `stat()`):**

| stat() result                | Cap         |
| ---------------------------- | ----------- |
| `S_ISREG`                    | `FILE`      |
| `S_ISCHR`                    | `NVME_CDEV` |
| `S_ISBLK` + basename `nvme*` | `NVME_BDEV` |
| `S_ISBLK` + other            | `BDEV`      |
| stat fails + ends with "GB"  | `RAMDISK`   |
| stat fails + PCI BDF pattern | `NVME_PCIE` |
| stat fails + contains `:`    | `NVME_TCP`  |

Each platform has its own classify function because device naming conventions
differ:
- Linux: char devices (`/dev/ng*`), block devices (`/dev/nvme*`, `/dev/sd*`)
- FreeBSD: char devices (`/dev/nvme0ns1`)
- macOS: block devices (`/dev/diskN`), DriverKit (`MacVFN-*`)
- Windows: device handles (`\\.\PhysicalDriveN`)

**Caps filter bypass:** When the user explicitly specifies `opts.async`,
`opts.sync`, or `opts.admin`, the caps filter is skipped entirely. This allows
users to force specific interface combinations regardless of URI classification.

### Device Type Detection

Backend determines device type by parsing URI and using stat():

```c
enum xnvme_dev_type detect_device_type(const char *uri) {
    // 1. Check for PCIe BDF (e.g., "0000:03:00.0")
    if (xnvme_ident_uri_is_pcie(uri))
        return PCIE_BDF;

    // 2. Check for TCP/RDMA endpoint
    if (xnvme_ident_uri_is_tcp(uri))
        return TCP;
    if (xnvme_ident_uri_is_rdma(uri))
        return RDMA;

    // 3. Use stat() for filesystem paths
    struct stat st;
    if (stat(uri, &st) < 0)
        return UNKNOWN;

    if (S_ISCHR(st.st_mode))
        return NVME_CHAR;   // /dev/ng0n1

    if (S_ISBLK(st.st_mode)) {
        // Use naming convention to distinguish NVMe vs generic block
        const char *basename = strrchr(uri, '/');
        if (basename && strncmp(basename + 1, "nvme", 4) == 0)
            return NVME_BLOCK;  // /dev/nvme0n1
        return BLOCK;           // /dev/sda
    }

    if (S_ISREG(st.st_mode))
        return FILE;

    return UNKNOWN;
}
```

**Device type to sync/admin mapping (Linux):**

| Device Type | Sync     | Admin    | Notes                        |
| ----------- | -------- | -------- | ---------------------------- |
| PCIE_BDF    | (driver) | (driver) | Userspace driver handles all |
| TCP/RDMA    | (driver) | (driver) | SPDK handles all             |
| CHAR        | nvme     | nvme     | NVMe char device passthrough |
| BLOCK       | nvme     | nvme     | NVMe block or generic block  |
| FILE        | psync    | shim     | Regular file                 |

### Ramdisk

Ramdisk is a cross-platform virtual device driver - not a backend, not formally
CBI, but reusable like CBI.

```c
// Ramdisk via URI pattern
xnvme_dev_open("1GB", NULL);

// Or via backend option
xnvme_dev_open("1GB", &(struct xnvme_opts){.be = "ramdisk"});
```

- Memory-backed NVMe device emulation
- Uses CBI async wrappers (thrpool, emu, nil)
- Available on all platforms
- Config provides its own sync/admin/dev

### Migration: opts.async/sync/admin

Each config now has a unique `opts.be` name, so `opts.be` alone selects the
exact config. The fields `opts.async`, `opts.sync`, `opts.admin` remain in
the API as additional filters on the config array, matching against the `id`
field of each config's interface:

```c
// Preferred: select config by unique name
xnvme_dev_open("/dev/nvme0n1", &(struct xnvme_opts){
    .be = "io_uring",
});

// Legacy: filter by async interface (still works)
xnvme_dev_open("/dev/nvme0n1", &(struct xnvme_opts){
    .async = "io_uring",
});  // First config with async.id == "io_uring" and matching caps

// Force specific combination (bypasses caps filter)
xnvme_dev_open("/dev/nvme0n1", &(struct xnvme_opts){
    .async = "io_uring",
    .sync = "nvme",
    .admin = "nvme"
});
```

When `opts.async`, `opts.sync`, or `opts.admin` are set, the caps filter is
bypassed -- the user is explicitly choosing their config.

## Backend Config Compatibility

### Userspace-Managed Configs

Userspace configs bundle everything and don't allow mixing:

| Config name | async     | sync      | admin     | mem       | Platforms      | Library |
| ----------- | --------- | --------- | --------- | --------- | -------------- | ------- |
| spdk        | spdk      | spdk      | spdk      | spdk      | Linux, FreeBSD | SPDK    |
| vfio        | vfio      | vfio      | vfio      | vfio      | Linux          | libvfn  |
| uio         | uio       | uio       | uio       | hugepage  | Linux          | uPCIe   |
| driverkit   | driverkit | driverkit | driverkit | driverkit | macOS          | MacVFN  |

### CBI / Virtual Components

Cross-platform common backend implementations:

| Component | Type   | Description                              |
| --------- | ------ | ---------------------------------------- |
| `thrpool` | async  | Thread pool wrapping sync interface      |
| `emu`     | async  | Emulated async using blocking sync calls |
| `nil`     | async  | Null async (no-op)                       |
| `ramdisk` | device | Virtual NVMe device backed by memory     |
| `psync`   | sync   | POSIX pread/pwrite                       |
| `posix`   | mem    | POSIX memory allocation                  |

**Note:** `thrpool` and `emu` are async wrappers. Their device type compatibility
is inherited from the underlying sync driver, not intrinsic to themselves.

## Platform-Specific Details

### Linux

Primary platform with fullest io-path support.

| Device Type | Path Example    | Description                         |
| ----------- | --------------- | ----------------------------------- |
| nvme_char   | `/dev/ng0n1`    | NVMe character device (passthrough) |
| nvme_block  | `/dev/nvme0n1`  | NVMe block device                   |
| block       | `/dev/sda`      | Generic block device                |
| file        | `/path/to/file` | Regular file                        |

| io-path        | nvme_char | nvme_block | block | file |
| -------------- | --------- | ---------- | ----- | ---- |
| `io_uring_cmd` | Y         | ---------- | ----- | ---- |
| `io_uring`     | Y         | Y          | Y     | Y    |
| `libaio`       | --------- | Y          | Y     | Y    |

| Device Type | Sync  | Admin | Default io-path |
| ----------- | ----- | ----- | --------------- |
| nvme_char   | nvme  | nvme  | io_uring_cmd    |
| nvme_block  | nvme  | nvme  | io_uring        |
| block       | psync | block | io_uring        |
| file        | psync | shim  | io_uring        |

**Default selection:** When no opts are specified, the URI is classified and
the first config with matching caps is used -- no probing, no fallback cascade.

### FreeBSD

| Device Type | Path Example    | Description           |
| ----------- | --------------- | --------------------- |
| nvme_ns     | `/dev/nvme0ns1` | NVMe namespace device |
| nvd         | `/dev/nvd0`     | NVMe disk device      |
| file        | `/path/to/file` | Regular file          |

| io-path   | nvme_ns | nvd | file |
| --------- | ------- | --- | ---- |
| `kqueue`  | Y       | Y   | Y    |
| `posix`   | Y       | Y   | Y    |
| `thrpool` | Y       | Y   | Y    |
| `emu`     | Y       | Y   | Y    |

| Device Type | Sync  | Admin | Default io-path |
| ----------- | ----- | ----- | --------------- |
| nvme_ns     | nvme  | nvme  | kqueue          |
| nvd         | psync | nvme  | kqueue          |
| file        | psync | shim  | kqueue          |

### Windows

| Device Type | Path Example         | Description                   |
| ----------- | -------------------- | ----------------------------- |
| block       | `\\.\PhysicalDrive0` | Block device (including NVMe) |
| file        | `C:\path\to\file`    | Regular file                  |

| io-path   | block | file |
| --------- | ----- | ---- |
| `iocp`    | Y     | Y    |
| `iocp_th` | Y     | Y    |
| `io_ring` | Y     | Y    |
| `thrpool` | Y     | Y    |
| `emu`     | Y     | Y    |

**Note:** `io_ring` on Windows is experimental and currently only supports reads.
This is a Windows API limitation, not an **xNVMe** restriction.

| Device Type | Sync | Admin | Default io-path |
| ----------- | ---- | ----- | --------------- |
| block       | file | scsi  | iocp            |
| file        | file | shim  | iocp            |

**Note:** NVMe devices on Windows appear as regular block devices. Most NVMe
commands must be translated to SCSI. Only a limited set of NVMe admin commands
(**Identify**, **Get Log Page**, **Get/Set Features**) can be issued directly via
`IOCTL_STORAGE_PROTOCOL_COMMAND`.

### macOS

**OS-managed (IOKit):**

| Device Type | Path Example    | Description  |
| ----------- | --------------- | ------------ |
| block       | `/dev/disk0`    | Block device |
| file        | `/path/to/file` | Regular file |

| io-path   | block | file |
| --------- | ----- | ---- |
| `posix`   | Y     | Y    |
| `thrpool` | Y     | Y    |
| `emu`     | Y     | Y    |

| Device Type | Sync  | Admin | Default io-path |
| ----------- | ----- | ----- | --------------- |
| block       | psync | macos | thrpool         |
| file        | psync | shim  | thrpool         |

Limited: no NVMe passthrough via OS path, only SMART data via IOKit.

**Userspace-managed (MacVFN):**

For full NVMe passthrough, use `opts.be = "driverkit"`. This uses MacVFN's
DriverKit-based userspace driver. The latest version maintains block-device
functionality while the driver is loaded, allowing concurrent OS and **xNVMe** access.

## Device Discovery

### Design Principles

Scan answers one question: **"where do I start?"** It provides a lightweight
inventory of available devices without opening them. The result is a topology
(not a flat list) that reflects the physical/logical hierarchy.

Local PCIe scan and fabrics discovery are **separate operations**:
- Local scan is pure OS (sysfs, pciconf) -- no driver dependency, no NVMe protocol
- Fabrics discovery requires driver logic (connect, NVMe Discovery Log Page)

They are kept separate because scan must work without any driver initialized.

### Scan Topology

Scan results are structured as a topology, not flat `xnvme_ident` entries.
The hierarchy reflects the physical device tree:

```
BDF (0000:03:00.0)
  └── Controller (nvme0)
        ├── Namespace (nsid=1, csi=NVM)
        └── Namespace (nsid=2, csi=KV)
```

**Important:** The Linux kernel device name "n1" does **not** mean nsid=1.
The nsid must be read from sysfs (`/sys/class/nvme/nvme0/nvme0n1/nsid`), not
derived from the device name.

```c
struct xnvme_scan_ns {
	uint32_t nsid;          // Actual namespace ID (from sysfs, not device name)
	uint8_t csi;            // Command set identifier
};

struct xnvme_scan_ctrlr {
	char name[32];              // Kernel name: "nvme0"
	char kernel_driver[32];     // "nvme", "vfio-pci", "uio_pci_generic"
	struct xnvme_scan_ns *ns;   // Array of namespaces
	uint32_t nns;               // Number of namespaces
};

struct xnvme_scan_entry {
	char bdf[16];                   // PCI BDF: "0000:03:00.0"
	struct xnvme_scan_ctrlr *ctrlr; // Array of controllers (usually 1)
	uint32_t nctrlr;                // Number of controllers
};

struct xnvme_scan_result {
	struct xnvme_scan_entry *entries;
	uint32_t nentries;
};
```

The topology does not model subsystems -- **xNVMe** cannot open a subsystem,
so there is no reason to expose it in scan results.

### Scan API

```c
// Local PCIe scan -- returns topology of local devices
int xnvme_scan(struct xnvme_scan_result *result);

// Free scan result
void xnvme_scan_result_free(struct xnvme_scan_result *result);
```

Scan is always local. No `sys_uri` parameter, no driver involvement.

**From scan result to dev_open:**

```c
struct xnvme_scan_result result = {0};
xnvme_scan(&result);

for (uint32_t i = 0; i < result.nentries; i++) {
    struct xnvme_scan_entry *entry = &result.entries[i];
    for (uint32_t c = 0; c < entry->nctrlr; c++) {
        struct xnvme_scan_ctrlr *ctrlr = &entry->ctrlr[c];
        for (uint32_t n = 0; n < ctrlr->nns; n++) {
            struct xnvme_ident ident = {
                .trtype = XNVME_TRTYPE_PCIE,
                .nsid = ctrlr->ns[n].nsid,
                .csi = ctrlr->ns[n].csi,
            };
            strncpy(ident.traddr, entry->bdf, sizeof(ident.traddr));
            strncpy(ident.kernel_driver, ctrlr->kernel_driver,
                    sizeof(ident.kernel_driver));
            // Now open with xnvme_dev_open_ident(&ident, opts)
        }
    }
}
xnvme_scan_result_free(&result);
```

### Enumerate

Full device inspection. Implemented once in core, not in backends. Uses scan +
dev_open internally:

```c
int
xnvme_enumerate(struct xnvme_opts *opts, xnvme_enumerate_cb cb, void *cb_args)
{
	struct xnvme_scan_result result = {0};
	int err = xnvme_scan(&result);
	if (err) {
		return err;
	}
	// For each namespace in topology: construct ident, dev_open, callback
	// ...
	xnvme_scan_result_free(&result);
	return 0;
}
```

### Fabrics Discovery

Fabrics discovery is a separate API that requires a backend config with
discovery support. It connects to a discovery controller and retrieves the
Discovery Log Page.

```c
// Fabrics discovery -- requires backend (e.g. SPDK for TCP/RDMA)
int xnvme_discover(const char *uri, struct xnvme_opts *opts,
                   xnvme_discover_cb cb, void *cb_args);
```

This is **not** part of `xnvme_scan()` because:
- It requires driver initialization (SPDK env, etc.)
- It uses NVMe protocol (connect, identify, discovery log)
- Scan must work without any driver

### Fabrics Discovery Architecture

| Transport | Requirements                           | Provider                  |
| --------- | -------------------------------------- | ------------------------- |
| PCIe      | OS APIs only (sysfs, pciconf)          | Platform (scan)           |
| TCP       | NVMe protocol (connect, discovery log) | Backend config (discover) |
| RDMA      | NVMe protocol (connect, discovery log) | Backend config (discover) |

**Discovery implementations:**

| Config    | discover() | Transports |
| --------- | ---------- | ---------- |
| spdk      | Yes        | TCP, RDMA  |
| vfio      | No         | ---------- |
| uio       | No         | ---------- |
| driverkit | No         | ---------- |

SPDK's `discover()`:
1. Connect to discovery controller at `uri`
2. Retrieve Discovery Log Page (list of subsystems)
3. Callback for each discovered target

**URI format for TCP/RDMA:**
```
nvme+tcp://<address>:<port>/<nqn>
nvme+rdma://<address>:<port>/<nqn>

Examples:
  nvme+tcp://192.168.1.100:4420/nqn.2024-01.com.example:subsys1
  nvme+rdma://10.0.0.1:4420/nqn.2024-01.com.example:subsys2
```

**cref for TCP/RDMA:**
- Key by structured ident (transport + address + NQN), not URI string
- Same controller/namespace model applies
- Driver provides transport-specific attach/detach

## Backend and Platform Structure

The architecture uses a two-level hierarchy: a platform that owns an array of
backend configs. The platform handles OS-specific discovery and lifecycle while
configs define complete I/O path recipes.

### Data Structures

```c
// xnvme_platform.h
struct xnvme_platform {
	const char *name;
	const struct xnvme_be_config *const *backends; // NULL-terminated config array

	uint32_t (*classify)(const char *uri); // URI → xnvme_be_cap bitmask

	int (*dev_open)(struct xnvme_dev *dev, struct xnvme_opts *opts);
	int (*scan)(const char *sys_uri, struct xnvme_opts *opts,
	            xnvme_scan_cb cb_func, void *cb_args);
	int (*enumerate)(const char *sys_uri, struct xnvme_opts *opts,
	                 xnvme_enumerate_cb cb_func, void *cb_args);
};

// Shared implementations - all platforms use these by default
int xnvme_platform_dev_open(struct xnvme_dev *dev, struct xnvme_opts *opts);
int xnvme_platform_enumerate(const char *sys_uri, struct xnvme_opts *opts,
                             xnvme_enumerate_cb cb_func, void *cb_args);

extern struct xnvme_platform g_xnvme_platform_linux;
extern struct xnvme_platform g_xnvme_platform_freebsd;
extern struct xnvme_platform g_xnvme_platform_macos;
extern struct xnvme_platform g_xnvme_platform_windows;

// Global platform pointer - compile-time selected
extern struct xnvme_platform *g_xnvme_platform;
```

### Shared Platform Operations

All platform operations use shared implementations by default. `dev_open`
iterates `platform->backends[]`, applies filters, and calls the first
matching config's `dev_open`:

```c
// Shared implementations in xnvme_platform.c
int xnvme_platform_dev_open(...)   // Iterate configs, open device
int xnvme_platform_enumerate(...)  // Use scan + dev_open + Identify NS List
```

The public API delegates to the platform:

```c
// Public API wrappers
int xnvme_scan(...) {
	return g_xnvme_platform->scan(...);
}
int xnvme_enumerate(...) {
	return g_xnvme_platform->enumerate(...);
}
```

Platforms provide their own scan implementation (e.g., Linux uses uPCIe for
PCIe enumeration) but share dev_open and enumerate.

**Motivation for platform-level operations:**

This design prepares for several improvements:

1. **Reusable PCIe enumeration**: Moving `xnvme_scan()` from per-backend
   implementations to a shared platform-level implementation using uPCIe.
   Instead of each driver implementing its own scan logic, the platform
   provides a single PCIe enumeration that all drivers can use.

2. **Centralized reference counting**: Userspace drivers (spdk, vfio) require
   controller reference counting. By handling this at the platform level,
   drivers become simpler and conflict detection between drivers becomes
   possible.

3. **Clean separation of concerns**: The platform handles OS-specific device
   discovery and lifecycle, while backend configs focus purely on I/O path
   definitions.

### Compile-Time Platform Selection

The global platform pointer is set at compile-time via preprocessor conditionals:

```c
// xnvme_platform.c
struct xnvme_platform *g_xnvme_platform =
#if defined(XNVME_PLATFORM_LINUX_ENABLED)
	&g_xnvme_platform_linux;
#elif defined(XNVME_PLATFORM_FREEBSD_ENABLED)
	&g_xnvme_platform_freebsd;
#elif defined(XNVME_PLATFORM_MACOS_ENABLED)
	&g_xnvme_platform_macos;
#elif defined(XNVME_PLATFORM_WINDOWS_ENABLED)
	&g_xnvme_platform_windows;
#else
#error "No platform enabled. Define one of XNVME_PLATFORM_{LINUX,FREEBSD,MACOS,WINDOWS}_ENABLED"
#endif
```

### Platform-Specific Config Arrays

Each platform defines its own array of available backend configs:

```c
// Example: Linux platform
struct xnvme_platform g_xnvme_platform_linux = {
	.name = "linux",
	.classify = xnvme_platform_linux_classify,
	.backends = (const struct xnvme_be_config *const[]){
#ifdef XNVME_BE_SPDK_ENABLED
		&g_xnvme_be_spdk,
#endif
#ifdef XNVME_BE_VFIO_ENABLED
		&g_xnvme_be_vfio,
#endif
		&g_xnvme_be_linux_emu_nvme,
#ifdef XNVME_BE_LINUX_LIBURING_ENABLED
		&g_xnvme_be_linux_ucmd_nvme,
		&g_xnvme_be_linux_iou_nvme,
#endif
		// ... more configs ...
		&g_xnvme_be_linux_emu_file,
		&g_xnvme_be_linux_thrpool_file,
#ifdef XNVME_BE_RAMDISK_ENABLED
		&g_xnvme_be_ramdisk_nil,
		&g_xnvme_be_ramdisk_thrpool,
		&g_xnvme_be_ramdisk_emu,
#endif
		NULL,
	},
	.dev_open = xnvme_platform_dev_open,
	.scan = xnvme_platform_linux_scan,
	.enumerate = xnvme_platform_enumerate,
};
```

### Registry Removal Strategy

The old `g_xnvme_be_registry[]` mixed backends and drivers:

```c
// Old approach (removed)
static struct xnvme_be *g_xnvme_be_registry[] = {
	&xnvme_be_spdk,            // Now a config in platform->backends
	&xnvme_be_linux,           // Replaced by g_xnvme_platform_linux
	&xnvme_be_fbsd,            // Replaced by g_xnvme_platform_freebsd
	// ...
	NULL,
};
```

**Migration (done):**

1. **Platform structs created** - each OS defines `g_xnvme_platform_<os>`
2. **Configs in platform** - `platform->backends` array replaces registry
3. **Single platform pointer** - `g_xnvme_platform` set at compile-time
4. **be_factory() removed** - replaced by `platform->dev_open()` which calls
   shared `xnvme_platform_dev_open()` implementation

**Result:** Clear separation between:
- Platform (single global pointer, OS-specific, contains config array and operations)
- Backend configs (multiple per platform, runtime selectable via opts or caps)

## Config Selection Logic

The selection loop in `xnvme_platform_dev_open()` applies filters in order:

```c
int
xnvme_platform_dev_open(struct xnvme_dev *dev, struct xnvme_opts *opts)
{
	int be_opts = opts && (opts->async || opts->sync || opts->admin);
	uint32_t uri_cap = 0;

	// Classify URI only when user hasn't specified explicit backend opts
	if (!be_opts && g_xnvme_platform->classify) {
		uri_cap = g_xnvme_platform->classify(dev->ident.uri);
	}

	for (int i = 0; g_xnvme_platform->backends[i]; ++i) {
		const struct xnvme_be_config *cfg = g_xnvme_platform->backends[i];

		// Filter: enabled
		if (!cfg->attr.enabled) continue;

		// Filter: opts.be matches config name
		if (opts->be && strcmp(opts->be, cfg->attr.name)) continue;

		// Filter: opts.async/sync/admin match interface ids
		if (opts->async && strcmp(opts->async, cfg->async->id)) continue;
		if (opts->sync && strcmp(opts->sync, cfg->sync->id)) continue;
		if (opts->admin && strcmp(opts->admin, cfg->admin->id)) continue;

		// Filter: caps -- skip configs that can't handle this URI type
		if (uri_cap && cfg->attr.caps && !(cfg->attr.caps & uri_cap))
			continue;

		// ... mem override handling ...

		dev->be = be;
		err = be.dev.dev_open(dev);
		if (!err) return 0;
		if (err == -EPERM) return err;

		// When caps matched, this was the right config -- return its error
		if (uri_cap && cfg->attr.caps) return err;
	}
	return -ENXIO;
}
```

**Key behaviors:**
- When `uri_cap` matches a config's caps, that config is tried and its error
  returned directly (no further probing)
- When user specifies `opts.async`/`opts.sync`/`opts.admin`, caps filter is
  bypassed (user knows what they want)
- First matching config wins -- array ordering defines priority
- Permission errors (`EPERM`) always stop immediately

**Benefits over old probing approach:**

Old behavior (problematic):
```
DEBUG: trying spdk... failed (ENODEV)
DEBUG: trying linux_nvme... failed (EPERM)
DEBUG: trying linux_block... failed (EINVAL)
Error: EINVAL  # User sees last error, not the relevant one
```

After refactor (caps-matched):
```
DEBUG: uri='/dev/nvme0n1' classified cap=0x2 (NVME_BDEV)
DEBUG: selected config: emu
Error: EPERM  # User sees the actual problem
```

- URI classified once, correct config selected immediately
- One meaningful error returned
- Debug logs show the classification and selection

## Error Codes

Consistent error codes for each failure mode:

| Scenario                 | Error     | Description                                              |
| ------------------------ | --------- | -------------------------------------------------------- |
| No matching config       | `ENXIO`   | No config matches URI + opts combination                 |
| Config not supported     | `ENOTSUP` | Config's `dev_open()` failed (e.g., no io_uring support) |
| Mem not supported        | `EINVAL`  | `opts.mem` not in config's mem_overrides array           |
| Device not found         | `ENOENT`  | URI doesn't resolve to a device                          |
| Permission denied        | `EPERM`   | Insufficient permissions to access device                |
| Device busy              | `EBUSY`   | Device in use by another driver (conflict)               |
| Controller attach failed | `EIO`     | Driver-specific attach failure                           |

**Principle:** Return the error from the caps-matched config.
Caps matching ensures the error is always relevant.

## Controller Reference Counting (cref)

Centralized management for userspace-managed drivers. The platform handles all
refcounting - drivers provide open/close logic but are unaware of refcounting.

```c
struct cref_entry {
	char uri[384]; // Controller URI (PCI BDF)
	int refcount;
	enum { SHAREABLE, EXCLUSIVE } mode;
	void *ctrlr; // Controller handle with namespace array
	int (*destructor)(void *);
};
```

**Controller and namespace model:**
- Controller owns namespace array, initialized eagerly at attach
- `ctrlr->ns[nsid]` provides direct lookup
- cref tracks controllers only - no namespace refcounting
- Multiple `xnvme_dev` handles can reference same namespace

**Platform responsibility:**
```c
int
xnvme_platform_dev_open(struct xnvme_dev *dev, struct xnvme_opts *opts)
{
	if (is_userspace_config(cfg)) {
		void *ctrlr = cref_get(ctrlr_uri);
		if (!ctrlr) {
			// First open - attach controller, init all namespaces
			err = cfg->dev->dev_open(dev);
			if (err) {
				return err;
			}
			cref_insert(ctrlr_uri, dev->ctrlr, cfg->dev->dev_close);
		}
		dev->ctrlr = ctrlr;
		dev->ns = ctrlr->ns[nsid]; // Direct array lookup
		return 0;
	}
	// OS-managed path...
}

void
xnvme_platform_dev_close(struct xnvme_dev *dev)
{
	if (is_userspace_config(cfg)) {
		cref_put(ctrlr_uri); // Detaches controller when refcount hits zero
		return;
	}
	// OS-managed path...
}
```

**Access modes:**
- Shareable: OS-managed configs can coexist
- Exclusive: userspace-managed configs are mutually exclusive

## File Organization

```
lib/
├── xnvme_platform.c              # Shared dev_open, enumerate
├── xnvme_platform_linux.c        # Linux: scan, classify
├── xnvme_platform_freebsd.c      # FreeBSD: scan, classify
├── xnvme_platform_macos.c        # macOS: scan, classify
├── xnvme_platform_windows.c      # Windows: scan, classify
│
├── xnvme_be_linux.c              # Linux configs + helpers
├── xnvme_be_linux_dev.c          # Linux: dev_open, dev_close
├── xnvme_be_linux_sync_nvme.c    # Linux: NVMe ioctl sync
├── xnvme_be_linux_sync_block.c   # Linux: block layer sync
├── xnvme_be_linux_admin_nvme.c   # Linux: NVMe ioctl admin
├── xnvme_be_linux_admin_block.c  # Linux: block layer admin
├── xnvme_be_linux_async_*.c      # Linux: io_uring, libaio, etc.
├── xnvme_be_linux_mem_hugepage.c # Linux: hugepage memory
│
├── xnvme_be_fbsd*.c              # FreeBSD configs + implementations
├── xnvme_be_macos*.c             # macOS configs + implementations
├── xnvme_be_macos_driverkit*.c   # macOS DriverKit configs + implementations
├── xnvme_be_windows*.c           # Windows configs + implementations
│
├── xnvme_be_spdk*.c              # SPDK userspace driver
├── xnvme_be_vfio*.c              # libvfn userspace driver
│
├── xnvme_be_cbi_*.c              # Cross-platform components (emu, thrpool, psync, etc.)
├── xnvme_be_cref.c               # Controller reference counting
├── xnvme_be_ramdisk*.c           # Ramdisk virtual device driver
└── xnvme_be_nosys.c              # Stubs for disabled features
```

## Naming Convention

| Prefix                | Meaning                                                   |
| --------------------- | --------------------------------------------------------- |
| `xnvme_platform_<os>` | Platform (OS-specific scan, classify)                     |
| `xnvme_be_<os>_`      | Backend implementation (configs, sync, admin, async, mem) |
| `xnvme_be_<name>`     | Userspace driver (spdk, vfio)                             |
| `xnvme_be_cbi_`       | Cross-platform backend implementation                     |
| `xnvme_be_ramdisk`    | Ramdisk virtual device                                    |
| `xnvme_be_cref`       | Controller reference counting                             |

## Migration Progress

### Phase 1: Foundation (done)
- [x] Add `xnvme_scan()` API
- [x] Add `xnvme_ident.kernel_driver` field (rename from `driver`)
- [x] Implement Linux PCIe scan via uPCIe
- [x] Add controller reference-counting module (`xnvme_be_cref`)
- [x] Introduce `xnvme_platform` with backends and function pointers
  - `g_xnvme_platform` compile-time selected
  - `platform->backends[]` array for per-platform config list
  - `platform->dev_open/scan/enumerate` function pointers

### Phase 2: Flat Configs and Caps-Based Selection (done)
- [x] Implement `xnvme_scan()` for all platforms:
  - Linux: uPCIe
  - FreeBSD: pciconf
  - Windows: SetupAPI
  - macOS: IOKit
- [x] Controller-only `dev_open` with `nsid=0` (SPDK, VFIO)
- [x] Move enumerate into platform using scan + dev_open + Identify NS List
- [x] Replace mixin system with flat `xnvme_be_config` lists
  - Each config is a complete recipe (async+sync+admin+dev+mem)
  - `attr.descr` describes the config for debugging/display
  - `attr.caps` bitmask for device type compatibility
- [x] Add URI classification via per-platform `classify()` callback
  - Linux: stat() + pattern matching
  - FreeBSD: stat() + pattern matching
  - macOS: stat() + DriverKit pattern
  - Windows: device handle prefix matching
- [x] Implement caps-based config selection in `xnvme_platform_dev_open()`
  - URI classified → caps filter → first matching config
  - Early return on caps match (meaningful error)
  - Bypass caps when user specifies explicit async/sync/admin opts

### Phase 3: Userspace Drivers (done)
- [x] Move SPDK from backend to userspace driver
  - Remove SPDK-internal controller refcounting
  - Rely on platform for ref-counting via cref
- [x] Move libvfn from backend to userspace driver
  - Remove libvfn-internal controller refcounting
  - Rely on platform for ref-counting via cref
- [x] Move DriverKit from separate backend to userspace driver within macOS backend
- [x] Remove all driver-specific refcounting - rely solely on platform cref

### Phase 4: Cleanup
- [ ] Remove `opts.be` matching on old backend names (after transition)
- [ ] After release: remove `opts.async`, `opts.sync`, `opts.admin`
- [ ] Consolidate config naming for consistency

### Phase 5: Single Backend (done)
- [x] Remove backend registry (g_xnvme_be_registry)
- [x] Compile-time backend selection via #ifdef
- [x] Introduce `xnvme_platform` structure with `g_xnvme_platform` global

## Documentation Restructure

Replace `docs/backends/` with `docs/background/` to reflect the new architecture
without losing existing content.

### Current Structure (to be replaced)
```
docs/backends/
├── linux/
├── freebsd/
├── macos/
├── windows/
├── spdk/           # Treated as peer to Linux
├── libvfn/         # Treated as peer to Linux
└── common/
    └── interface/
```

### New Structure
```
docs/
├── getting_started/
├── background/                 # Replaces backends/
│   ├── index.rst               # Intro: what xNVMe abstracts
│   ├── architecture/           # Design rationale (this document)
│   │   └── index.rst
│   ├── platforms/              # OS-specific setup and configuration
│   │   ├── index.rst           # Platform overview and matrix
│   │   ├── linux/              # From backends/linux/
│   │   ├── freebsd/            # From backends/freebsd/
│   │   ├── macos/              # From backends/macos/
│   │   └── windows/            # From backends/windows/
│   └── backends/               # Backend config documentation
│       ├── index.rst           # Config taxonomy, selection logic
│       ├── spdk.rst            # From backends/spdk/
│       ├── vfio.rst            # From backends/libvfn/
│       ├── io_uring.rst        # From backends/linux/io_uring*.rst
│       ├── libaio.rst          # From backends/linux/libaio.rst
│       ├── kqueue.rst          # FreeBSD async
│       ├── iocp.rst            # From backends/windows/iocp*.rst
│       ├── thrpool.rst         # From backends/common/thrpool.rst
│       ├── emu.rst             # From backends/common/emu.rst
│       ├── ramdisk.rst         # From backends/common/ramdisk.rst
│       └── psync.rst           # From backends/common/psync.rst
├── tutorial/
├── tools/
├── api/
├── contributing/
└── ...
```

### Content Migration

**Platforms** (OS-specific, moved as-is):
- System configuration requirements
- Device identification schemes
- Platform-specific setup scripts

**Backends** (reorganized, each page covers):
- What the backend config does
- Which platforms support it
- How to select it (via `opts.be`, `opts.async`, etc.)
- Requirements and dependencies

**Architecture** (new):
- This design document
- Config taxonomy and selection logic
- Compatibility matrices
