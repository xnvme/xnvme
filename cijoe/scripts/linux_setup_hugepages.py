"""
Allocate hugepages for memfd-based hugepage memory
===================================================

Allocates hugepages and disables transparent hugepages.
The memfd hugepage backend does not require a hugetlbfs mount.

Config Arguments
--------------

hugetlbfs.nr_hugepages: the number of hugepages to allocate

Retargetable: True
------------------
"""

import logging as log


def main(args, cijoe):
    """Setup hugepages"""

    nr_hugepages = cijoe.getconf("hugetlbfs.nr_hugepages", 256)

    commands = [
        f"echo {nr_hugepages} > /proc/sys/vm/nr_hugepages",
        "echo never >/sys/kernel/mm/transparent_hugepage/enabled",
    ]

    first_err = 0
    for command in commands:
        err, _ = cijoe.run(command)
        if err:
            log.error(f"cmd({command}), err({err})")
            first_err = first_err if first_err else err

    return first_err
