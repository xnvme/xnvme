"""
Setup system for using hugepages and mount hugetlbfs
====================================================

Uses hugetlbfs to mount ...

Config Arguments
--------------

hugetlbfs.mount_point: path to where the hugetlbfs should be mounted
hugetlbfs.nr_hugepages: the number of hugepages to allocate

Retargetable: True
------------------
"""
import errno
import logging as log


def main(args, cijoe, step):
    """Setup hugetlbfs"""

    nr_hugepages = cijoe.config.options.get("hugetlbfs", {}).get("nr_hugepages", 128)
    mount_point = cijoe.config.options.get("hugetlbfs", {}).get(
        "mount_point", "/mnt/huge"
    )

    commands = [
        f"echo {nr_hugepages} > /proc/sys/vm/nr_hugepages",
        "echo never >/sys/kernel/mm/transparent_hugepage/enabled",
        f"mkdir -p {mount_point}",
        f"mount -t hugetlbfs -o pagesize=2048K,size=32M,min_size=2048K,nr_inodes={nr_hugepages} none {mount_point}",
    ]

    first_err = 0
    for command in commands:
        err, _ = cijoe.run(command)
        if err:
            log.error(f"cmd({command}), err({err})")
            first_err = first_err if first_err else err

    return first_err
