def get_combinations():
    """Return all sensible backend configurations matching driver presets"""

    combos = {"linux": [], "freebsd": [], "windows": [], "macos": []}

    combos["linux"] = [
        # NVMe char device
        {
            "be": ["io_uring_cmd"],
            "async": ["io_uring_cmd"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["posix", "hugepage"],
            "label": ["cdev"],
        },
        # NVMe block device
        {
            "be": ["io_uring"],
            "async": ["io_uring"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev"],
        },
        {
            "be": ["libaio"],
            "async": ["libaio"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev"],
        },
        {
            "be": ["thrpool"],
            "async": ["thrpool"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev"],
        },
        {
            "be": ["emu"],
            "async": ["emu"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev"],
        },
        # Generic block device
        {
            "be": ["io_uring_bdev"],
            "async": ["io_uring"],
            "sync": ["block"],
            "admin": ["block"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev"],
        },
        {
            "be": ["libaio_bdev"],
            "async": ["libaio"],
            "sync": ["block"],
            "admin": ["block"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev"],
        },
        {
            "be": ["thrpool_bdev"],
            "async": ["thrpool"],
            "sync": ["block"],
            "admin": ["block"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev"],
        },
        {
            "be": ["emu_bdev"],
            "async": ["emu"],
            "sync": ["block"],
            "admin": ["block"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev"],
        },
        # File
        {
            "be": ["io_uring_file"],
            "async": ["io_uring"],
            "sync": ["psync"],
            "admin": ["file_as_ns"],
            "mem": ["posix", "hugepage"],
            "label": ["file"],
        },
        {
            "be": ["libaio_file"],
            "async": ["libaio"],
            "sync": ["psync"],
            "admin": ["file_as_ns"],
            "mem": ["posix", "hugepage"],
            "label": ["file"],
        },
        {
            "be": ["thrpool_file"],
            "async": ["thrpool"],
            "sync": ["psync"],
            "admin": ["file_as_ns"],
            "mem": ["posix", "hugepage"],
            "label": ["file"],
        },
        {
            "be": ["emu_file"],
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
            "be": ["ramdisk_emu"],
            "async": ["emu"],
            "sync": ["ramdisk"],
            "admin": ["ramdisk"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev", "ramdisk"],
        },
        {
            "be": ["ramdisk_thrpool"],
            "async": ["thrpool"],
            "sync": ["ramdisk"],
            "admin": ["ramdisk"],
            "mem": ["posix", "hugepage"],
            "label": ["bdev", "ramdisk"],
        },
    ]

    combos["freebsd"] = [
        # NVMe device
        {
            "be": ["kqueue"],
            "async": ["kqueue"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["posix"],
            "label": ["bdev"],
        },
        {
            "be": ["thrpool"],
            "async": ["thrpool"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["posix"],
            "label": ["bdev"],
        },
        {
            "be": ["emu"],
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
            "be": ["ramdisk_emu"],
            "async": ["emu"],
            "sync": ["ramdisk"],
            "admin": ["ramdisk"],
            "mem": ["posix"],
            "label": ["bdev", "ramdisk"],
        },
        {
            "be": ["ramdisk_thrpool"],
            "async": ["thrpool"],
            "sync": ["ramdisk"],
            "admin": ["ramdisk"],
            "mem": ["posix"],
            "label": ["bdev", "ramdisk"],
        },
    ]

    combos["windows"] = [
        # Block device
        {
            "be": ["iocp"],
            "async": ["iocp"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["windows"],
            "label": ["bdev"],
        },
        {
            "be": ["iocp_th"],
            "async": ["iocp_th"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["windows"],
            "label": ["bdev"],
        },
        {
            "be": ["ioring"],
            "async": ["ioring"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["windows"],
            "label": ["bdev"],
        },
        {
            "be": ["thrpool"],
            "async": ["thrpool"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["windows"],
            "label": ["bdev"],
        },
        {
            "be": ["emu"],
            "async": ["emu"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "mem": ["windows"],
            "label": ["bdev"],
        },
        # File
        {
            "be": ["thrpool_file"],
            "async": ["thrpool"],
            "sync": ["psync"],
            "admin": ["file_as_ns"],
            "mem": ["windows"],
            "label": ["file"],
        },
        {
            "be": ["iocp_file"],
            "async": ["iocp"],
            "sync": ["psync"],
            "admin": ["file_as_ns"],
            "mem": ["windows"],
            "label": ["file"],
        },
        # Ramdisk
        {
            "be": ["ramdisk_emu"],
            "async": ["emu"],
            "sync": ["ramdisk"],
            "admin": ["ramdisk"],
            "mem": ["windows"],
            "label": ["bdev", "ramdisk"],
        },
        {
            "be": ["ramdisk_thrpool"],
            "async": ["thrpool"],
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
            "be": ["driverkit_emu"],
            "async": ["emu"],
            "sync": ["driverkit"],
            "admin": ["driverkit"],
            "mem": ["driverkit"],
            "label": ["macvfn", "pcie"],
        },
        # Ramdisk
        {
            "be": ["ramdisk_emu"],
            "async": ["emu"],
            "sync": ["ramdisk"],
            "admin": ["ramdisk"],
            "mem": ["posix"],
            "label": ["bdev", "ramdisk"],
        },
        {
            "be": ["ramdisk_thrpool"],
            "async": ["thrpool"],
            "sync": ["ramdisk"],
            "admin": ["ramdisk"],
            "mem": ["posix"],
            "label": ["bdev", "ramdisk"],
        },
    ]

    return combos
