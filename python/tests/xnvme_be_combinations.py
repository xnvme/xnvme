def get_combinations():
    """Return all sensible backend configurations"""

    combos = {
        "linux": [],
        "freebsd": [],
        "windows": [],
    }

    combos["linux"] = [
        # Ramdisk
        {
            "be": ["ramdisk_emu"],
            "admin": ["ramdisk"],
            "async": ["emu"],
            "mem": ["posix", "hugepage"],
            "sync": ["ramdisk"],
            "label": ["bdev", "ramdisk"],
        },
        {
            "be": ["ramdisk_thrpool"],
            "admin": ["ramdisk"],
            "async": ["thrpool"],
            "mem": ["posix", "hugepage"],
            "sync": ["ramdisk"],
            "label": ["bdev", "ramdisk"],
        },
        # File-based I/O
        {
            "be": ["emu_file"],
            "admin": ["file_as_ns"],
            "async": ["emu"],
            "mem": ["posix", "hugepage"],
            "sync": ["psync"],
            "label": ["file"],
        },
        {
            "be": ["thrpool_file"],
            "admin": ["file_as_ns"],
            "async": ["thrpool"],
            "mem": ["posix", "hugepage"],
            "sync": ["psync"],
            "label": ["file"],
        },
        # NVMe char device
        {
            "be": ["io_uring_cmd"],
            "admin": ["nvme"],
            "async": ["io_uring_cmd"],
            "mem": ["posix", "hugepage"],
            "sync": ["nvme"],
            "label": ["cdev"],
        },
        # NVMe block device (admin=nvme, sync=nvme)
        {
            "be": ["emu"],
            "admin": ["nvme"],
            "async": ["emu"],
            "mem": ["posix", "hugepage"],
            "sync": ["nvme"],
            "label": ["bdev"],
        },
        {
            "be": ["thrpool"],
            "admin": ["nvme"],
            "async": ["thrpool"],
            "mem": ["posix", "hugepage"],
            "sync": ["nvme"],
            "label": ["bdev"],
        },
        {
            "be": ["posix"],
            "admin": ["nvme"],
            "async": ["posix"],
            "mem": ["posix", "hugepage"],
            "sync": ["nvme"],
            "label": ["bdev"],
        },
        {
            "be": ["io_uring"],
            "admin": ["nvme"],
            "async": ["io_uring"],
            "mem": ["posix", "hugepage"],
            "sync": ["nvme"],
            "label": ["bdev"],
        },
        {
            "be": ["libaio"],
            "admin": ["nvme"],
            "async": ["libaio"],
            "mem": ["posix", "hugepage"],
            "sync": ["nvme"],
            "label": ["bdev"],
        },
        # Generic block device (admin=block, sync=block)
        {
            "be": ["emu_bdev"],
            "admin": ["block"],
            "async": ["emu"],
            "mem": ["posix", "hugepage"],
            "sync": ["block"],
            "label": ["bdev"],
        },
        {
            "be": ["thrpool_bdev"],
            "admin": ["block"],
            "async": ["thrpool"],
            "mem": ["posix", "hugepage"],
            "sync": ["block"],
            "label": ["bdev"],
        },
        {
            "be": ["io_uring_bdev"],
            "admin": ["block"],
            "async": ["io_uring"],
            "mem": ["posix", "hugepage"],
            "sync": ["block"],
            "label": ["bdev"],
        },
        {
            "be": ["libaio_bdev"],
            "admin": ["block"],
            "async": ["libaio"],
            "mem": ["posix", "hugepage"],
            "sync": ["block"],
            "label": ["bdev"],
        },
        # User-space NVMe-driver
        {
            "be": ["libvfn"],
            "mem": ["libvfn"],
            "async": ["libvfn"],
            "sync": ["libvfn"],
            "admin": ["libvfn"],
            "label": ["pcie"],
        },
        {
            "be": ["spdk"],
            "mem": ["spdk"],
            "async": ["spdk"],
            "sync": ["spdk"],
            "admin": ["spdk"],
            "label": ["pcie"],
        },
        {
            "be": ["spdk"],
            "mem": ["spdk"],
            "async": ["spdk"],
            "sync": ["spdk"],
            "admin": ["spdk"],
            "label": ["fabrics"],
        },
    ]

    combos["freebsd"] = [
        {
            "be": ["kqueue"],
            "mem": ["posix"],
            "async": ["kqueue"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "label": ["bdev"],
        },
        {
            "be": ["thrpool"],
            "mem": ["posix"],
            "async": ["thrpool"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "label": ["bdev"],
        },
        {
            "be": ["emu"],
            "mem": ["posix"],
            "async": ["emu"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "label": ["bdev"],
        },
        {
            "be": ["posix"],
            "mem": ["posix"],
            "async": ["posix"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "label": ["bdev"],
        },
        {
            "be": ["spdk"],
            "mem": ["spdk"],
            "async": ["spdk"],
            "sync": ["spdk"],
            "admin": ["spdk"],
            "label": ["pcie"],
        },
    ]

    return combos
