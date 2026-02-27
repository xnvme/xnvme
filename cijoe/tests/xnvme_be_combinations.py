def get_combinations():
    """Return all sensible backend configurations matching driver presets"""

    combos = {"linux": [], "freebsd": [], "windows": [], "macos": []}

    combos["linux"] = [
        # NVMe char device
        {
            "be": ["linux"],
            "async": ["io_uring_cmd"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["posix", "hugepage"],
            "label": ["cdev"],
        },
        # NVMe block device
        {
            "be": ["linux"],
            "async": ["io_uring"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev"],
        },
        {
            "be": ["linux"],
            "async": ["libaio"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev"],
        },
        {
            "be": ["linux"],
            "async": ["thrpool"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev"],
        },
        {
            "be": ["linux"],
            "async": ["emu"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev"],
        },
        # Generic block device
        {
            "be": ["linux"],
            "async": ["io_uring"],
            "sync": ["block"],
            "admin": ["block"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev"],
        },
        {
            "be": ["linux"],
            "async": ["libaio"],
            "sync": ["block"],
            "admin": ["block"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev"],
        },
        {
            "be": ["linux"],
            "async": ["thrpool"],
            "sync": ["block"],
            "admin": ["block"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev"],
        },
        {
            "be": ["linux"],
            "async": ["emu"],
            "sync": ["block"],
            "admin": ["block"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev"],
        },
        # File
        {
            "be": ["linux"],
            "async": ["io_uring"],
            "sync": ["psync"],
            "admin": ["file_as_ns"],
            "mem": ["posix", "hugepage"],
            "label": ["file"],
        },
        {
            "be": ["linux"],
            "async": ["libaio"],
            "sync": ["psync"],
            "admin": ["file_as_ns"],
            "mem": ["posix", "hugepage"],
            "label": ["file"],
        },
        {
            "be": ["linux"],
            "async": ["thrpool"],
            "sync": ["psync"],
            "admin": ["file_as_ns"],
            "mem": ["posix", "hugepage"],
            "label": ["file"],
        },
        {
            "be": ["linux"],
            "async": ["emu"],
            "sync": ["psync"],
            "admin": ["file_as_ns"],
            "mem": ["posix", "hugepage"],
            "label": ["file"],
        },
        # User-space NVMe-driver
        {
            "be": ["libvfn"],
            "async": ["libvfn"],
            "sync": ["libvfn"],
            "admin": ["libvfn"],
            "mem": ["libvfn"],
            "label": ["pcie"],
        },
        {
            "be": ["spdk"],
            "async": ["spdk"],
            "sync": ["spdk"],
            "admin": ["spdk"],
            "mem": ["spdk"],
            "label": ["pcie"],
        },
        {
            "be": ["spdk"],
            "async": ["spdk"],
            "sync": ["spdk"],
            "admin": ["spdk"],
            "mem": ["spdk"],
            "label": ["fabrics"],
        },
        # Ramdisk
        {
            "be": ["ramdisk"],
            "async": ["emu", "thrpool"],
            "sync": ["ramdisk"],
            "admin": ["ramdisk"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev", "ramdisk"],
        },
    ]

    combos["freebsd"] = [
        # NVMe device
        {
            "be": ["fbsd"],
            "async": ["kqueue"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["posix"],
            "label": ["bdev"],
        },
        {
            "be": ["fbsd"],
            "async": ["thrpool"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["posix"],
            "label": ["bdev"],
        },
        {
            "be": ["fbsd"],
            "async": ["emu"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["posix"],
            "label": ["bdev"],
        },
        # User-space NVMe-driver
        {
            "be": ["spdk"],
            "async": ["spdk"],
            "sync": ["spdk"],
            "admin": ["spdk"],
            "mem": ["spdk"],
            "label": ["pcie"],
        },
        # Ramdisk
        {
            "be": ["ramdisk"],
            "async": ["emu", "thrpool"],
            "sync": ["ramdisk"],
            "admin": ["ramdisk"],
            "mem": ["posix"],
            "label": ["bdev", "ramdisk"],
        },
    ]

    combos["windows"] = [
        # Block device
        {
            "be": ["windows"],
            "async": ["iocp"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["windows"],
            "label": ["bdev"],
        },
        {
            "be": ["windows"],
            "async": ["iocp_th"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["windows"],
            "label": ["bdev"],
        },
        {
            "be": ["windows"],
            "async": ["ioring"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["windows"],
            "label": ["bdev"],
        },
        {
            "be": ["windows"],
            "async": ["thrpool"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["windows"],
            "label": ["bdev"],
        },
        {
            "be": ["windows"],
            "async": ["emu"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["windows"],
            "label": ["bdev"],
        },
        # File
        {
            "be": ["windows"],
            "async": ["thrpool"],
            "sync": ["psync"],
            "admin": ["file_as_ns"],
            "mem": ["windows"],
            "label": ["file"],
        },
        {
            "be": ["windows"],
            "async": ["emu"],
            "sync": ["psync"],
            "admin": ["file_as_ns"],
            "mem": ["windows"],
            "label": ["file"],
        },
        # Ramdisk
        {
            "be": ["ramdisk"],
            "async": ["emu", "thrpool"],
            "sync": ["ramdisk"],
            "admin": ["ramdisk"],
            "mem": ["windows"],
            "label": ["bdev", "ramdisk"],
        },
    ]

    combos["macos"] = [
        # Userspace (DriverKit)
        {
            "be": ["driverkit"],
            "async": ["driverkit"],
            "sync": ["driverkit"],
            "admin": ["driverkit"],
            "mem": ["driverkit"],
            "label": ["macvfn", "pcie"],
        },
        {
            "be": ["driverkit"],
            "async": ["emu"],
            "sync": ["driverkit"],
            "admin": ["driverkit"],
            "mem": ["driverkit"],
            "label": ["macvfn", "pcie"],
        },
        # Ramdisk
        {
            "be": ["ramdisk"],
            "async": ["emu", "thrpool"],
            "sync": ["ramdisk"],
            "admin": ["ramdisk"],
            "mem": ["posix"],
            "label": ["bdev", "ramdisk"],
        },
    ]

    return combos
