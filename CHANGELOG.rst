Changelog
=========

The repository is tagged using semantic versioning, e.g. ``v1.2.3``.

Each upcoming release is available, as is, with its history subject to change,
on a branch named ``rX.Y.Z``. Once released, the history of ``rX.Y.Z`` is fixed
and tagged ``vX.Y.Z`` and pushed to the master branch.

The history from one patch-version to another is likely to be rebased. E.g.
squashed into the history of the previous until the next minor release is made.

Changes are described in this file in a section named matching the version tag.

The "Backlog" section describe changes on the roadmap for xNVMe.

Known Issues
------------

* Fabrics

  - Errors are observed when constructing payloads with an mdts exceeding
    16 * 1024 bytes. Thus a work-around is applied bounding the derived
    ``mdts_nbytes`` associated with the device geometry.

* Binaries

  - xNVMe does not build with ``-march=native``, however, SPDK/DPDK does.
    Thus, when the xNVMe library "bundles" SPDK/DPDK and the binaries are
    linked, then they can contain ISA-specific instructions.
    If you invoke e.g. ``xnvme enum`` and see the message ``Illegal
    Instruction``, then this is why.

* ``be::spdk``

  - Does not build on Alpine Linux
  - Re-initialization fails, e.g. repeatedly calling ``xnvme_enumerate()``

* ``be::linux``

  - When enabling the use of ``io_uring`` or ``libaio`` via
    ``?async={iou,aio}``, then all async. commands are sent via the chosen
    async. path. Take note, that these async. paths only supports read and
    write.  Commands such as the Simple-Copy-Command, Append, and
    Zone-Management are not supported in upstream Linux in this manner. This
    means, as a user that you must sent non-read/write commands with mode
    ``XNVME_CMD_SYNC``.

v0.0.21
-------

* Refactored backend interface

  - Changed to support interchangeable ``sync`` and ``async`` implementations

* The Linux backend ``be::linux``
  - Merged ``be:lioc``, ``be:laio``, ``be:liou``, and ``nil`` into one backend
    ``be:linux``, having the async-implementation be an engine parameter
    controllable via uri-opt ``?async`` values: ``thr``, ``aio``, ``iou``,
    ``nil``.
  - Added proper support for the Linux Block Device model, replacing the
    ``?pseudo`` option with ``sync`` interfaces ``nvme_ioctl`` and
    ``block_ioctl``. Gracefully falling back to the Block Layer when the given
    device is not an NVMe device, and thus supporting everything the Linux
    Block Supports including the Zoned Block Device model
  - Added support for ``XNVME_CMD_ASYNC`` for ``ioctl``-driven commands. This
    provides an async.interface to Linux driver-ioctls(), for commands other
    than read/write.  Next step is to make it run fast by providing a less
    costly kernel path. This path is enabled via ``?async=thr``.
  - With these changes, the build-configuration of backends has changed and
    documentation describes how to enable/disable the different backends, sync,
    and async implementations

* Changed command behavior

  - api-functions taking command-options, e.g.  ``xnvme_cmd_pass``,
    ``znd_cmd_mgmt_send``, now **require** that either ``XNVME_CMD_SYNC`` or
    ``XNVME_CMD_ASYNC`` is given as argument. When none is given, negated
    ``EINVAL`` is returned.

* xNVMe fio io-engine

  - Replace ``--be`` option with ``--async``, this makes it a easier to
    instrument ``fio`` to use a different async. implementation in the Linux
    backend of xNVMe. Previously it relied on schema-prefix, the prefix-prefix
    was annoying to use with fio as it required escape-chars.

  - ``fio`` scripts and docs have been updated with the new ``--async`` argument

  - ``fio`` scripts simplified and aligned such that they all three can be used
    in the same manner using the ``--sector=default`` and ``--sector=override``
    to override ``rw``, ``iodepth``, and ``bs`` via environment variables.

* Third-party libraries

  - Added Linux/UAPI version to ``xnvme library-info``, this can give a good
    hint on why certain features aren't behaving as expected, such as the Linux
    versions without the Zoned Block headers
  - Updated to fio/v3.23

* A general handful of code-cleanups and fixes, both on style as well as
  potential issues such local-vars shadowing global-vars, potential arithmetic
  overflows

* Continous Integration

  - Added testing of Linux paths using Nullblock instances in addition to
    emulated NVMe devices

  - Added integration of GitHUB/CodeQL, since Semmle got acquired by GitHUB,
    this will replace the lgtm.com integration.

v0.0.20
-------

* Third-party libraries

  - Updated to fio/v3.22
  - Made fio available to the third-party SPDK build
  - Added build of SPDK fio io-engine
  - Fixed missing update of third-party version-strings

* The xNVMe fio io-engine

  - Several fixes to locking/serialization and error-handling
  - Adjusted to changes in upstream ZBD support
  - Changed the zoned fio-example to not be timebased, since it could lead to
    the verify-job never getting to the verify-part when running on emulated
    devices
  - Increased ``ramp_time`` in comparison-script
  - Fixed memory issue due to missing ``get_file_size``

* Backends

  - Added a backend ``nwrp`` the NULL-Async-IO backend, purpose of which is to
    troubleshoot and benchmark the async-io path

* General

  - A bunch of fixes including bad format-strings, out-of-bound / array
    overflows, non-atomic locks, improper error-path handling

* CI

  - Added workflow generating docker-image with latest source, providing
    everything needed to build xNVMe and latest qemu to deploy and experiment
    with xNVMe on emulated NVMe devices
  - Added workflow doing Coverity scan and uploading results for analysis
  - Added ``fio`` binary and SPDK fio io-engines as artifacts. During testing,
    fio is needed, however, the test-environment might not have the same
    version available as the io-engines are built against, usually xNVMe is
    built against the latest release which might not have made it into the
    package repos.

v0.0.19
-------

* Third-party libraries

  - Updated to liburing/v0.7, SPDK/v20.07, fio/v3.21
  - Updated docs describing new third-party requirements for building
  - Adjusted patches and build-system to changes

* Fabrics: SPDK-patches enabling zone-changes over Fabrics

* Added public-domain CI

  - Primarily using GitHUB Actions / Workflows
  - Aux. analysis via lgtm.com
  - Updated docs and scripts for CI via GitHUB Actions

* Updated support for the NVMe Simple-Copy-Command (SCC)

  - Targeting TP 2020.05.04 (Ratified)
  - Added ``tests/scc.c`` testing for SCC-support, print identify fields, and
    exercises the command itself

v0.0.18
-------

* Third-party libraries: SPDK

  - Updated tracking of SPDK to current master(7dbaf54bf) and adjusted linkage
  - Removed patches that are now upstream
  - Updated nvmf/IOCS support

* Fixed non-IOCS device identification

v0.0.17
-------

* Third Party libraries

  - The organization of these has changed such that tracking them and applying
    patches is easier
  - The versions / git-revision info from bundled libraries bundled can now be
    queried via the api calls 'xnvme_3p_ver_*()'
  - The CLI tool 'xnvme' produces these upon request via 'xnvme library-info'
  - Most of the third-party libraries have been updated to, at the time of
    writing, latest versions

* The xNVMe fio IO engine

  - It now supports Zoned Devices!
    It does so by mapping the Zoned Command Set to the ZBD Kernel abstraction
  - It now supports multiple devices!
    Minor caveat; when using multiple-devices then one cannot mix backends
  - The engine was developed against fio-3.20, other versions might pose issues
    with the IO-engine interface leading to segfaults when running or just
    exiting. It should now produce a meaningful error-message when this
    happens.

* be:liou, the io_uring backend

  - Added opcode-checking via the "new" probing feature
  - Replaced READV/WRITEV with READ/WRITE
  - Build of ``be::liou`` on Alpine Linux

* Added ``be::laio`` the Linux/libaio backend

  - A great supplement to the IOCTL, io_uring, and SPDK backends

* Added initial support for NVMe-oF / Fabrics

  - xnvme_dev_open(): 'uri' argument on the form: "fab:<HOST>:<PORT>?nsid=xyz"
  - xnvme_enumerate(): 'sys_uri' argument on the form "fab:<HOST>:<PORT>"
  - Command-line utility: 'xnvme enum' takes '--uri "fab:<HOST>:<PORT>"'
  - See the "docs/tutorial/fabrics.rst" for details

* Added support for I/O Command Set

  - Convenience functions to retrieve command-set specific identity
  - Misc. definitions in the ``libxnvme_spec.h`` headers
  - Utilization of these via the CLI tools ``xnvme`` and ``lblk``

* Added support for Namespace Types (TP 4056 2020-03-05) [verified]

  - Patched SPDK to allow Command Set Selection
  - Added identifier option "?css=0xHEX" for Controller Configuration

* Added support for the Zoned Command Set

  - Support is encapsulated in the library header 'libznd.h'
  - Convenience functions for Zoned Commands
    For example: znd_cmd_mgmt_send(), znd_cmd_mgmt_send(), znd_cmd_append()
    Helpers for retrieving zone-reports with and with descriptor extensions
  - Support in fio via the xNVMe fio I/O Engine
  - CLI tool 'zoned' for convenient command-line management/inspection of zoned
    devices

* Added handling of extended-LBA

  - Expanded ``geometry`` with ``lba_extended`` informing whether
    extended-LBAs are in effect. That is, when ``flbas.bit4`` is set AND the
    current ``lbaf.ms`` is not zero.
  - Expanded ``geometry`` with ``lba_nbytes``, which will always contain the
    size of an LBA in bytes. When ``lba_extended`` is cleared to zero, then
    ``lba_nbytes`` is ``lbaf.ds``, in bytes, when ``lba_extended`` is set to 1,
    then ``lba_nbytes`` is ``lbaf.ds + lbaf.ms``.
  - When ``lba_extended`` is cleared to 0 then the API I/O helpers expect to be
    passed ``dbuf``, and ``mbuf``. When ``lba_extended`` is set to 1, then the
    API I/O helpers expect ``dbuf`` to contain data and meta-data, and expect
    ``mbuf`` to be ``NULL``.

* And a bunch of fixes
  - xnvmec: fixed errno assignment and decode
  - be: added comment on failed attempt at _blockdevice_geometry()
  - Fixed a build-issue on ARM
  - Updated backend documentation and added link to online docs in README

v0.0.16
-------

* Initial public release of xNVMe

Backlog
-------

* Release User-space NVMe Meta-filesystem

* docs

  - Expand documentation on Fabrics setup
  - Expand library usage examples

* For the Linux backend ``be:linux``, replace the device enumeration with the
  encapsulations provided by ``libnvme``

* build

  - Provide convenient means of probing source origin e.g. repos, tarball etc.
