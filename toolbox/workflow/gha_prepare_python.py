"""
Install xNVMe Python Packages using source-tarball artifacts from GitHUB
========================================================================

The xNVMe Python packages:

* xnvme-core.tar.gz
* xnvme-cy-header.tar.gz
* xnvme-cy-bindings.tar.gz

Are expected to be available in 'step.with.xnvme_source'.
Additionally, 'step.with.xnvme_source' is expected to the be the root of an extracted
xNVMe source archive.

Step Arguments
--------------

step.with.source: The files listed above are expected to be available in
'step.with.source', it is also expected that 'step.with.source' is a directory
containing the xNVMe source in extracted form. That is the content of the
xnvme-source-tarball.

Retargetable: True
------------------
"""
import errno
import logging as log


def worklet_entry(args, cijoe, step):
    """Transfer artifacts"""

    xnvme_source = step.get("with", {}).get("xnvme_source", "/tmp/xnvme_source")
    if xnvme_source is None:
        log.error(f"invalid step({step})")
        return errno.EINVAL

    commands = ["python3 -m pip install xnvme-core.tar.gz --user"]

    hdr_path = f"{xnvme_source}/python/xnvme-cy-header/xnvme/cython_header/tests/"
    commands += [
        "python3 -m pip install xnvme-cy-header.tar.gz --user",
        f"cd {hdr_path}; python3 -m pip install -r requirements.txt",
        f"cd {hdr_path}; python3 setup.py build_ext --inplace",
        f"cd {hdr_path}; python3 -m pytest --cython-collect test_cython.pyx::test_dummy -v -s",
    ]

    commands += [
        "python3 -m pip install -r python/xnvme-cy-bindings/requirements.txt --user",
        "python3 -m pip install xnvme-cy-bindings.tar.gz --user -vvv --no-build-isolation",
        "python3 -m pip install -r python/xnvme-cy-bindings/xnvme/cython_bindings/tests/requirements.txt --user",
        "python3 -m pytest --pyargs xnvme.cython_bindings -k 'test_version' -s",
    ]

    first_err = 0
    for command in commands:
        err, _ = cijoe.run(command, cwd=xnvme_source)
        if err:
            log.error(f"cmd({command}), err({err})")
            first_err = first_err if first_err else err

    return first_err
