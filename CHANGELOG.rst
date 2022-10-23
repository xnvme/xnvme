Changelog
=========

The repository is tagged using semantic versioning, e.g. ``v1.2.3``.

The ``next`` branch contains a preview of the upcoming release. The history on
``next`` is subject to change. Upon release, ``next`` is merged with ``main``
and  and tagged ``vX.Y.Z``.

Changes are described in this file in a section matching the version tag.

Known Issues
------------

See the file named ``ISSUES`` in the root of the repository.

v0.6.0
------

A handful of improvements for Windows, additional steps toward removing
third-party vendoring / bundling, addition of experimental tunable knobs for
io_uring/SQPOLL, and several improvements to testing and CI infra.

* API
  - Removed SLIST from API, although "sys/queue.h" are commonly available on
    Linux/FreeBSD, then they are not part of toolchain on Windows.

* Third-party
  - Bumped SPDK to v22.09, and with that removed mutliple out-of-tree patches
    for DPDK.

* CLI
  - The xNVMe command-line library (libxnvmec) and all the cli-tools using it
    are refactored to use common sets of command-line arguments. Along with
    this came a consistent set of CLI-arguments for admin/sync/async.

* Backends
  - ramdisk: The ramdisk got support for write-zeroes, iovec payloads and added
    to CI testing.
  - linux: support for a buffer-allocator using HUGEPAGES and tunable knobs for
    controlling the behavior of io_uring SQPOLL via environment variables.
  - windows: support for the experimental IORING Windows SPDK API and support
    for block devices (SCSI and SATA).
  - spdk: when controllers are re-used for device-handles, events are
    processed as a means to check whether the controller is still "alive"

* CI
  - scan-build now runs for each PR
  - basic tests are now running post-building testing using the RAMDISK
  - Testing of fabrics with TCP transport is now part of the setup

v0.5.0
------

A bit of expansion in the application of xNVMe with support for macOS, a
ramdisk backend, revival of Python language bindings and a refresh of the docs
on NVMe-over-Fabrics.

* API
  - Removed helpers for SGL
  - Add 'subnqn' to 'xnvme_ident', useful for NVMe-oF
  - Add 'hostnqn' to 'opts', useful for NVMe-oF
  - Add support directive-receive and write-with-directives
  - Spec adjusted for NVMe 2.0, still more work needed in this area

* Third-party
  - Bumped fio to v3.32
  - Bumped SPDK to v22.05
  - Removed liburing, now relies on on-system library instead of
    vendoring/bundling, documentation is updated to assist with library
    installation

* Tooling
  - Re-working testing using cijoe 0.9+, that is, switching to CIJOE/pytest for
    testing an CIJOE workflows for instrumentation
  - liburing is no longer bundled with xNVMe, that is, xNVMe now links with
    liburing as discovered on the system via pkg-config. This is done to avoid
    symbol collisions for applications linking or loading liburing and xNVMe.

* be:linux:async:libaio
  - When 'opts.poll_io' is set then poke() will return immmediatly and now wait
    for completions. This allows the use of trading CPU for more IOPS and lower
    per command latency.

* Additional user-space NVMe driver support via libvfn
  - Added 'be:vfio' providing another user-space driver via libvfn

* Preliminary support for macOS
  - Initial implementation using the "core" I/O mechanisms of sync-io, async
    emulation and the threadpool
  - Does enumeration of NVMe devices through the limited interface provided for
    user-space by the macOS kernel
  - Utilizes what is avaiable for admin-command submision

* Prelimiary support for a "ramdisk" device
  - be:ramdisk: added a backend mimicing the behavior of an NVMe NVM namespace
  - Intended as a test-vehicle providing a "device" without requiring anything
    but the consumption of main memory of the system
  - I/O is "stored" using main-memory

* Revival of the xNVMe Python language bindings
  - A re-introduction of the Python bindings, these are now generated and thus
    provide access to the entire xNVMe C API
  - They are added to the testing infrastructure ensuring that they are aligned
    with the library
  - In addition to simple ctypes bindings, then cython headers and bindings
    based on Cython are provided

* Documentation
  - Refreshed the NVMe-over-Fabrics tutorial
  - Expanded with descriptions on installing liburing
  - Expanded with a section for the WIP Python bindings

v0.4.0
------

This is a release with the sole purpose of changing the liburing subproject
from tracking 'master' to the next stable release that is liburing-2.2.

v0.3.0
------

This main feature of this release is the alignment of the ``io_uring_cmd``
implementation with the ``io_uring`` big-sqe/big-cqe approach to asynchronous
passthru of NVMe commands.

NOTE: the tracking of the liburing repository/subproject is changed from the
fixed tag ``liburing-2.1`` to the ``master`` branch. Thus, in case you
experience liburing related build-issues with this release, then it is most
likely due to changes on ``master``. As soon as ``liburing-2.2`` is released,
xNVMe will be released as well going back to stable tracking.  Thus, do not pin
your project to the xNVMe project tag for ``v0.3.0`` if you rely on the
``io_uring`` functionality.

* Asynchronous Passthru of NVMe Commands via ``io_uring``
  - There are no API changes to adjust to, the changes are encapsulated inside
    the implementation of ``be:linux:async:ucmd`` aka ``async=io_uring_cmd``.
  - The previous version of ``io_uring_cmd`` used indirect-commands, that is,
    the io_uring-sqe contained a pointer to the NVMe-command. This approach of
    passthrough via ``io_uring`` has been superseeded by the
    ``big-sqe/big-cqe`` approach with the NVMe-sqe embedded within the
    io_uring-sqe, and similar for the NVMe-cqe inside the io_uring-cqe.
  - This requires changes to how the ``io_uring`` is setup, this task is
    delegated to ``liburing`` and the subproject-wrap now tracks liburing
    ``master`` to do this.

* API
  - Fixed ``xnvme_enumerate()`` when ``NULL`` was passed as ``opts``, it now
    uses ``xnvme_default_opts()`` when no ``opts`` are given
  - Misc. fixes to docstrings missing descriptions

* cmd:
  - Fixed missing full-guard on full-guard in xnvme_cmd_passv()

* be:async:{emu,thrpool}: several fixes to command-processing
  - Fixed missing setup of completion errors
  - Fixed missing empty-guard in cmd_io{v}()

* fio IO engine
  - 3p:fio: bumped to v3.30
  - tools:fioe: fixed issue with iovec-payloads
  - tools:fioe: cleanup and alignment with upstream xNVMe fio IO engine
  - docs: removed deprecated information and re-written with usage examples

* tests:io_worker
  - Added a basic io_worker to verify the behavior of the
    submit-upon-completion

* tools:xdd
  - The ``xdd`` tool now provides an ``offset`` argument (in bytes), previously
    it started from 0

* Documentation
  - Re-introduced the ``tutorial`` section containing a guide to dynamically
    load xNVMe from C and Python
  - Added a Contributors section containing notes useful for first-time
    Contributors

v0.2.0
------

Main feature introduction is vectored I/O across a wider set of system
interfaces, that is via ioctl(), io_uring (ucmd) and preadv()/pwritev()
fallback.

* Support for vectored I/O via Linux: ioctl(), psync, and io_uring_cmd

* API
  - add xnvme_cmd_passv()
  - rename rename xnvme_queue_wait() to xnvme_queue_drain()

* be:io_uring_cmd:
  - Enabled NVME_IOCTL_IO64_CMD by default, when available for cmd_io()
  - Added support NVME_IOCTL_IO64_CMD_VEC over io_uring via cmd_iov()

* be:linux:nvme:
  - Enabled NVME_IOCTL_IO64_CMD by default, when available for cmd_io()
  - Added support NVME_IOCTL_IO64_CMD_VEC via cmd_iov()
  - Normalized error-handling for NVMe-ioctl interfaces, ioctl() as well as
    io_uring_cmd

* be:thrpool:
  - Added handling of cmd_iov(), providing a threadpool based fallback when
    io_uring_cmd is not available

* be:emu:
  - Added handling of cmd_iov(), providing a pseudo-async fallback when
    io_uring_cmd is not available

* Re-worked git-pre-commit using the 'pre-commit' framework
  - mk: added helpers invoking 'pre-commit', 'make format'/'make format-all'
  - mk: removed auto-setup of git-hooks
  - git: removed .githooks/pre-commit

* xNVMe fio io-engine
  - tools:fioe: use calloc instead of malloc
  - tools:fioe: changes according to fio coding conventions

* Library introspection
  - fix incorrect generation of third-party information
  - replace ``xnvme_3p`` with ``xnvme_libconf``
  - add all build-configs to ``xnvme_libconf``

* Command-line argumenter parser
  - xnvmec: fix missing setup of --direct

* CLI-fixes
  - zoned: fix description for identify namespace command

* Toolbox
  - mk: add script generating help-text on Makefile targets
  - meson: only do whole-archive in pkg-config when SPDK is enabled
  - scripts: replace astyle with clang-format
  - pcf: the pre-commit-framework is available for xNVMe

v0.1.0
------

Another infrastructure / fixes release.

* 3p:liburing
  - Bumped to 2.1
  - This breaks old distros: Debian Stretch and CentOS 7 but adds support for
    the latest Arch, Fedora, Tumbleweed, and Ubuntu

* docs
  - Added scripts and docs for: openSUSE, Fedora, CentOS Stream

v0.0.29
-------

Another infrastructure / fixes release.

* Re-worked the continous integration
  - Fixed the broken build of the "dockerized" source
  - Fixed build on FreeBSD
  - Added build and test of FreeBSD
  - Combined all workflows in a single workflow, this vastly improves how the
    CI is triggered and linked with artifacts and artifacts verified

* 3p:windows
  - Added definition for iovec, in preparation for iovec support

v0.0.28
-------

This and the previous release contain minimal library/logical changes as major
changes to the build-system and source organization is changed.

* Moved the libraries sources from 'src' to 'lib'

v0.0.27
-------

* Build-system migrated from CMake to meson
  The Makefile "frontend" to the build-system is still available, and
  instruments meson in the same manner it instrumented CMake. However, this is
  no longer intended for anything other than development. Meson is the way to
  go and the documentation thus describes how to use it rather than the
  make-helpers instrumenting meson.

* Reduced cpu-utilization on libaio and io_uring ``poke()`` implementations

v0.0.26
-------

Expanded platform support, updated experimental features, and extended
command-set-support for ZNS/ZRWA, along with a couple of fixes and third-party
updates.

* Third-party
  - fio, updated to 3.28
  - spdk, updated to v21.10

* Windows Support
  - xNVMe now builds on Windows, it uses the MinGW toolchain to be compatible
    with fio, however, xNVMe does also build with MSVC
  - Using IOCP for async I/O
  - Supports a limited number non-I/O commands via driver IOCTL mapping

* uring_cmd
  - Experimental interface updated for patch-set on top of 5.15 kernel

* Zoned Namespaces
  - Added support for Zone Random Write Area (ZRWA)

* Fixes
  - Linux Block Backend: fix and update sysfs processing
  - fio io-engine: Fix of xnvme_fioe_reset_wp() resetting one too many zones
  - Adjustments to CI and partly removed of deprecated 'schemes'

v0.0.25
-------

Major improvements to the usability of xNVMe and enchancements of the API
along with a couple of fixes.

* Encoding of runtime instrumentation, that is, selection of backend, async
  interface etc. has until now been encoded in the device URI, e.g.
  ``xnvme_dev_open("/dev/nvme0n1?async=io_uring")`` in order to use
  ``io_uring``, this has now been replaced by ``struct xnvme_opts``, making it
  much easier to instrument the library runtime via the API. The command-line
  is also affected, as the command-line parser is extended enabling parsing of
  said options along with the tests, examples, and tools are extended with
  these options.

* Device enumeration populated a list with device-identifiers, this has been
  replaced by invocation of a user-defined call-back function for each
  discovered device. Where instead of identifiers, device-handles are passed
  to the callback. This makes it much simpler to e.g. filtering namespace with
  a specific command-set.

* To support the above then most of information carried in the ``xnvme_ident``
  is removed, expect for the ``uri``, and extended with: ``dtype``, ``nsid``,
  and ``csi``. Where ``dtype`` denotes e.g. ``file``, ``block device``, ``NVMe
  controller``, ``NVMe Namespace``.

* The ``xnvme_znd_mgmt_send()`` has now has an explicit ``select_all`` argument
  for setting the matching command-field, this replaces the use of the
  non-standardized ``zrasf`` field associated enumeration-values.

* Documentation for building on Gentoo is added along with addition to the
  automated build-test.

* nvme:spec: expanded with PCIe-bar registers

* Support for enumeration and device-handles for Linux NVMe Namespaces
  represented in devfs as char-devices, e.g. ``/dev/ngXnY`` is added.

* **Experimental** support for sending NVMe commands over ``io_uring``
  infrastructure is added. Think of this as sending the **synchronous** NVMe
  Driver ``ioctl()`` commands via the **asynchronous** ``io_uring`` interface.
  You thus get the control and capabilities of the ioctl() with the efficiency
  of ``io_uring``.
  This feature is enabled by setting ``opts.async=io_uring_cmd`` via the API or
  ``--async=io_uring_cmd`` via the command-line. The feature is experimental as
  it depends on non-upstream Kernel Support.

v0.0.24
-------

A release primarily of fixes, a new thread-pool based async. implementation and
a third-party update of fio.

* Third-party
  - fio, updated to 3.27

* Backends
  - posix:async:thrpool: add async-implementation with async.emulation via
    threadpool processing

* A good handful of fixes, see the commit-messages for details

v0.0.23
-------

This release contains updates to third-party repositories along with any
changes necessary for xNVMe due to third-party changes.

* Third-party
  - SPDK updated to v21.04
  - liburing updated to v2.0
  - fio, not updated, due to a compiler-warning breaking the xNVMe build

This release contains another major refactoring of the API along with a handful
of fixes and updates. The goal of the refactoring is to further simplify the
"core" of the API.

* The buffer-allocator ``xnvme_buf_alloc()`` automatically selects the type of
  memory-allocator to use based on the device. However, it took a 'phys'
  argument which is only valid for very specific use-cases. Thus, this argument
  is removed and replaced by explicit ``physical`` allocators. This simplifies
  the "core" usage, without sacrificing low-level control, it is just provided
  via an explitcit interface instead.

* xNVMe now provides an API for file-system file-io
  - Plugs into the synchronous as well as the asynchronous xNVMe command API
  - I/O provided by ``xnvme_file_pread`` and ``xnvme_file_write``
  - Provides support for diirect and non-direct I/O
  - Two tools are provided utilizing the API ``xdd`` a simplified version of
    ``dd`` and ``xnvme_file`` utilizing sync. and sync. code-paths for
    load/dump/copy of files

* Examples
  - Add minimal examples for command submission and completion

* Backends
  - linux:fs: preliminary support for file-system I/O
  - linux:io_uring now does batched completion-handling
  - linux:io_uring now supports kernel-completion-polling (IOPOLL)
  - linux:io_uring fixes for use auto-handling of register-files
  - spdk now provides core-mask control via ident-uri-options
  - spdk now provides shared-memory group control via ident-uri-options

* A good handful of fixes, see the commit-messages for details

v0.0.22
-------

This release contains a major refactoring of the API along with a handful of
minor fixes. The refactoring goals are to align to existing nomenclature and
simplify usage.

* Reduce to five abstractions: devices, queues, commands, and command-contexts
  - Devices are base handles to NVMe Namespaces and a list of devices are
    retrieved via ``xnvme_enumerate()``, and handles to individual devices
    retrieved via ``xnvme_dev_open()`` and released via ``xnvme_dev_close()``.
  - The abstraction formerly known as an ``asynchronous context`` is now dubbed
    a ``queue``. The ``queue`` now has a ``capacity`` instead of a ``depth``.
  - ``queues`` are created on top of ``devices`` and belong to the device.
  - The definition, submission, and completion of a command is encapsulated in
    a context; the command-context. The command-context replaces the previous
    abstraction named the ``request``.
  - A command can reach a device via a ``queue``, in a deferred / asynchronous
    callback-based manner, or it go via the device in a synchronous / blocking
    manner. Regardless, the command needs a context, and the context is
    retrieved via ``xnvme_cmd_ctx_from_queue()`` or
    ``xnvme_cmd_ctx_from_dev()``.
  - Commands are passed down via ``xnvme_cmd_pass`` for NVMe IO Commands, and
    through ``xnvme_cmd_pass_admin`` for NVMe Admin Commands via the given
    command-context.

* Core API reduction
  - The core xNVMe API as provided by ``libxnvme.h`` it is reduced to a minimal
    interface. Auxilary helpers, convenience functions, and pretty-printers are
    no longer part of the core API but provided via individual header-files
  - The core of the xNVMe API thus consists of
    Device Handling: enumerate, dev_open, dev_close
    Memory: alloc, realloc, free, vtophys, virt_alloc, virt_free
    Queueing: init, term, poke, wait, get_command_ctx, get_capacity, get_outstanding
    Commands: pass, pass_admin
    Supporting the four abstractions described above
  - The manual allocation of a request-pool / command-context-pool is no longer
    needed. xNVMe does not prevent you from creating one if you want to, but it
    is no longer required. Each 'queue' now provides a pre-allocated pool of
    resources, and the manual request-pool is thus replaced by a call to the
    function ``xnvme_cmd_ctx_from_queue()``. If you are familiar with
    ``io_uring`` then think of this function as the equivalent of
    ``io_uring_get_sqe()``.

* API re-organization
  - Previously each command-set had its own top-level namespace, e.g. functions
    and structures for the Zoned Command-Set was using ``znd_*``. This was
    slightly quirky since it still relied on core of the xNVMe namespace
    ``xnvme_*`` for device handles etc. Thus, the command-set specific APIs
    providing helper-functions and convenience are now nested in the xNVMe API
    Namespace e.g. ``znd_*`` is now ``xnvme_znd_*`` and provided via
    ``libxnvme_znd.h``.
  - The NVM Command-Set API was ``lblk_*`` it is now ``xnvme_nvm_*``, and
    provided via ``libxnvme_nvm.h``.

* be:linux: changed error-mapping for non-NVMe errors
  - The Linux block based and sync. interfaces does not provide the underlying
    NVMe command status code and status code type since this is hidden behind
    the block-interface. Previously, the NVMe-completion status-code was just
    assigned the ``errno`` provided by the Kernel, which is highly confusing.
    This behavior is replaced by assigning the status-code-type of
    "vendor-specific" to indicate the status-codes are not defined in the spec.

* be:linux:aio: fixed submission and completion paths
  - The submission, via ``cmd_io()``, of a single command would submit all
    outstanding command, effectively limiting queue-depth
  - The completion via ``poke()``/``wait()`` could potentially complete more
    than requested by the user
  - The encapsulation of io-control-blocks, array of io-control-block pointers,
    were all pointing to the same control-block. Note, this was not causing
    issues due to the short-coming in ``cmd_io()``.

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
