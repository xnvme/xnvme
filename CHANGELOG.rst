.. SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
..
.. SPDX-License-Identifier: BSD-3-Clause

Changelog
=========

The repository is tagged using semantic versioning, e.g. ``v1.2.3``.

The ``main`` branch contains the latest state of xNVMe, including changes
intended for the upcoming release. Upon release, ``main`` is tagged
``vX.Y.Z``.

Changes are described in this file in a section matching the version tag.

Known Issues
------------

See the file named ``ISSUES.rst`` in the root of the repository.

v0.8.1
------

v0.8.1 updates uPCIe to v0.12.1 for the GPU backends, makes ``xnvme qublk``
complete a failed command with the errno that the kernel NVMe driver gives the
same status, and fixes the build of the FreeBSD driver binding on FreeBSD 15.

* API extensions

  - ``enum xnvme_spec_status_code`` names the generic status codes Invalid
    Namespace or Format, Command Interrupted, LBA Out of Range, Capacity
    Exceeded, Namespace Not Ready and Reservation Conflict
  - ``enum xnvme_spec_media_status_code`` names the Media and Data Integrity
    Errors status codes from Write Fault to Deallocated or Unwritten Logical
    Block, and ``enum xnvme_spec_path_status_code`` the Host Pathing Error
  - ``enum xnvme_spec_nvm_cmd_cpl_sc`` names Conflicting Attributes and
    Invalid Protection Information

* Backends

  - uPCIe v0.12.1: ``dmamem_from_cuda_iommufd()`` and
    ``dmamem_from_hip_iommufd()`` set ``base_va``, so ``upcie-cuda`` and
    ``upcie-hip`` give a GPU heap mapped through iommufd the right IOVA, and
    ``dmamem_iommu_map_pa_populate()`` fails when an address on the first
    device is already mapped

* Command line

  - ``xnvme qublk`` completes a failed command with the errno that
    ``nvme_error_status()`` of the kernel NVMe driver gives the same status,
    for example ``EBADE`` for Reservation Conflict and ``ENOSPC`` for Capacity
    Exceeded. It completed every failure as ``EIO``, except a DISCARD or
    WRITE_ZEROES refused as unsupported, as ``EOPNOTSUPP``. Any other status,
    including an errno the library records before the device answers, is
    still ``EIO``

* Library and build

  - ``lib/platform/freebsd/xnvme_driver_freebsd.c`` includes ``<sys/param.h>``
    first. FreeBSD 15 failed to compile it, as ``<sys/linker.h>`` uses
    ``MAXPATHLEN`` and no header before it pulled in ``<sys/param.h>``

* Documentation

  - The material page lists the AiSIO talk given at SDC '26, with its slides
    and video, and the video of the talk given at SDC AI '26

v0.8.0
------

v0.8.0 comes more than two years and more than 900 commits after v0.7.5. xNVMe
promises no stability of its API before v1.0, and little of the API is as it
was in v0.7.5; the command line is redesigned. The changes that code and
scripts written for v0.7.5 are most likely to meet come first; the commit
messages and the documentation describe the rest.

* API changes

  ``libxnvme_compat.h`` keeps the names marked *(compat)*, as inline functions
  that call their replacement or as macros, so that code written for v0.7.5,
  such as the xNVMe engine of fio, builds unchanged.

  - ``xnvme_cmd_ctx_from_dev()`` and ``xnvme_file_get_cmd_ctx()``: use
    ``xnvme_dev_get_cmd_ctx()`` *(compat)*
  - ``xnvme_cmd_ctx_from_queue()``: use ``xnvme_queue_get_cmd_ctx()`` *(compat)*
  - ``xnvme_cmd_passv()``: use ``xnvme_cmd_pass_iov()``, which takes the
    meta-payload as a single buffer *(compat)*
  - ``xnvme_queue_wait()``: use ``xnvme_queue_drain()`` *(compat)*
  - ``xnvme_mem_map()`` and ``xnvme_mem_unmap()``: use ``xnvme_buf_register()``
    and ``xnvme_buf_unregister()`` *(compat)*
  - ``xnvme_opts_default()``: use ``xnvme_opts_set_defaults()`` on
    zero-initialized options *(compat)*
  - ``xnvme_libconf``: the extern array is replaced by ``struct xnvme_libconf``,
    returned by ``xnvme_libconf_get()``
  - ``xnvme_be_attr_list()`` and ``xnvme_be_attr_list_bundled()`` are removed;
    ``xnvme_be_attr_get()`` returns the attributes of a backend configuration
    by index
  - ``libxnvme_lba.h`` is removed, with ``struct xnvme_lba_range`` and its
    functions and printers
  - The macros of the zoned report are replaced by getters, such as
    ``xnvme_znd_report_get_descr()``; ``XNVME_ZND_REPORT_DESCR`` *(compat)*
  - ``xnvme_file_pread()`` and ``xnvme_file_pwrite()`` take the offset as
    ``uint64_t`` instead of ``off_t``
  - ``libxnvme_mem.h`` and ``libxnvme_spec_pp.h`` are removed; ``libxnvme.h``
    declares what remains of them
  - ``xnvme_buf_diff()`` returns 0 or a negative errno, and gives the number of
    differing bytes through its new ``diff`` parameter; ``xnvme_buf_clear()``
    and ``xnvme_buf_diff_pr()`` return an ``int``
  - ``xnvme_libconf_fpr()`` and ``xnvme_libconf_pr()`` take the
    ``struct xnvme_libconf`` to print; ``xnvme_be_attr_list_fpr()`` and
    ``xnvme_be_attr_list_pr()`` take no list
  - ``struct xnvme_be_attr`` holds a ``descr`` and the ``caps`` of a backend
    configuration instead of ``enabled``
  - The parameters that took an enum in ``xnvme_pi_ctx_init()``,
    ``xnvme_pi_size()``, ``xnvme_nvm_scopy()`` and the functions of
    ``libxnvme_znd.h`` take a ``uint32_t``
  - ``XNVME_ZND_REPORT_DEXT``, ``union xnvme_spec_vs_register`` and the
    ``XNVME_IDENT_*`` macros other than ``XNVME_IDENT_URI_LEN`` are removed
  - The printers, the ``_fpr``, ``_pr`` and ``_str`` functions, are declared in
    ``libxnvme_pp.h`` only, which ``libxnvme.h`` includes.
    ``xnvme_spec_feat_fpr()``, ``xnvme_spec_feat_pr()``,
    ``xnvme_spec_feat_fdp_events_pr()`` and ``xnvme_spec_drecv_sar_pr()`` take
    their records by pointer
  - The structs of the specification carry integers instead of C bit-fields,
    read and written through masks with ``xnvme_bits_get()`` and
    ``xnvme_bits_set()``. Code reading ``ns->flbas.format`` now reads
    ``xnvme_bits_get(ns->flbas, XNVME_SPEC_IDFY_NS_FLBAS_FORMAT)``

    - ``struct xnvme_spec_cmd`` is ``union xnvme_spec_sqe``, printed by
      ``xnvme_spec_sqe_fpr()`` and ``xnvme_spec_sqe_pr()``;
      ``xnvme_spec_cmd_fpr()`` and ``xnvme_spec_cmd_pr()`` *(compat)*
    - ``struct xnvme_spec_cpl`` is ``struct xnvme_spec_cqe`` *(compat)*
    - ``struct xnvme_spec_status`` is the integer ``status`` of the completion
      queue entry and of the Error Information log entry, read through
      ``XNVME_SPEC_CQE_STATUS_*`` and ``XNVME_SPEC_LOG_ERRI_ENTRY_STATUS_*``
    - The opcode is a byte, FUSE and PSDT are in ``flags`` behind
      ``XNVME_SPEC_CMD_COMMON_FLAGS_*``, and the data pointer is
      ``union xnvme_spec_dptr``
    - The fields of the identify, get log page, LBA format and power state
      structs are integers and masks. ``union xnvme_spec_idfy``, the bytes of
      the identify data and its views, replaces ``struct xnvme_spec_idfy`` in
      ``xnvme_adm_idfy()`` and the other identify functions
    - ``XNVME_SPEC_IDFY_NSDSCR`` is ``XNVME_SPEC_IDFY_NS_DESC`` *(compat)*

* API extensions

  - ``xnvme_queue_reap()`` stores the completed commands of a queue in an array
    that the caller passes, so one call retrieves many completions without a
    callback. Every backend implements it. ``xnvme_cmd_ctx_set_data()`` and
    ``xnvme_cmd_ctx_get_data()`` attach the caller's data to a command
  - ``xnvme_scan()``, in ``libxnvme_scan.h``, lists the NVMe controllers on
    Linux, FreeBSD, macOS and Windows, with the kernel driver of each, and on
    Linux its IOMMU group
  - ``xnvme_driver_attach()``, ``xnvme_driver_detach()``,
    ``xnvme_driver_default()`` and ``xnvme_driver_for_be()``, in
    ``libxnvme_driver.h``, bind a controller to the driver that a backend
    needs, on Linux and FreeBSD
  - ``xnvme_buf_register()``, ``xnvme_buf_unregister()``,
    ``xnvme_buf_get_alignment()`` and ``xnvme_buf_memcpy()``
  - ``xnvme_cplane_serve()``, ``xnvme_cplane_get_info()``,
    ``xnvme_cplane_get_ctrlr_info()`` and ``xnvme_cplane_server_alive()``, in
    ``libxnvme_cplane.h``, share a controller between processes over a socket
  - ``xnvme_cuda_queue_create()`` and ``xnvme_cuda_queue_destroy()``, in
    ``libxnvme_cuda.h``, place NVMe queues in GPU memory for I/O that the GPU
    issues
  - ``xnvme_adm_gfeat_nqueues()``, and the Sanitize Status log page as
    ``struct xnvme_spec_log_sanitize_entry``
  - ``xnvme_cli_dev_open_multi()``, ``xnvme_cli_dev_close_multi()`` and
    ``xnvme_cli_uri_parse()``; a device URI names its namespace with ``?nsid=``
  - ``xnvme_ver_vcs()`` returns the revision that the library was built from

* Backends

  - Each backend configuration has a name of its own, which ``--be`` and
    ``xnvme_opts.be`` take, such as ``io_uring`` or ``libaio_bdev``. The name
    of the platform, such as ``linux``, with ``--async``, ``--sync`` and
    ``--admin``, still selects one. ``fbsd`` is now ``freebsd``, ``vfio`` is
    now ``libvfn``, and the admin interface ``ramdisk_as_ns`` is now
    ``ramdisk``
  - ``upcie``, the uPCIe userspace NVMe driver, carried as a subproject, with
    asynchronous I/O, metadata, iovec payloads and a multi-process mode. A
    controller named by its PCIe address and no backend is now opened with
    ``upcie``; SPDK and libvfn are used when named
  - ``upcie-cuda``, for I/O into the memory of NVIDIA GPUs, issued by the CPU
    or by the GPU, and ``upcie-hip``, for I/O into the memory of AMD GPUs
  - ``io_uring_dmabuf``, io_uring with memory registered from a dma-buf
  - ``driverkit`` on macOS, through the MacVFN DriverKit driver
  - io_uring and libaio send Flush, and honour FUA on writes
  - SPDK v26.05, with ``iova_mode`` in ``xnvme_opts`` and on the command line,
    memory registration, and ``delay_cmd_submit`` enabled by default
  - libvfn v5.1.0
  - A controller opens without a namespace with ``nsid=0``
  - The ramdisk backend runs on Windows

* Command line

  - ``xnvme`` is the one program on ``PATH``. The tools, tests and examples
    install into ``libexecdir/xnvme`` as ``xnvme-<command>``,
    ``xnvme-test-<name>`` and ``xnvme-example-<name>``, and ``xnvme <name>``
    runs them. Bare ``xnvme`` shows the commands a level at a time, and the
    shell completion asks ``xnvme`` what to complete
  - ``xnvme driver --be <backend>`` binds the driver that a backend needs and
    replaces ``xnvme-driver``. The hugepages that ``xnvme-driver`` reserved are
    reserved with the ``hugepages`` tool, and the ``iommu`` tool shows and
    switches the IOMMU configuration on Linux; both install with ``pipx``, and
    ``xnvme env`` reports whether they are installed
  - The NVMe admin subcommands of ``xnvme``, such as ``idfy``, ``log-health``,
    ``feature-get`` and ``format``, are now ``xnvme test adm``. ``padc``,
    ``pioc`` and ``library-info`` are removed; ``xnvme cmd-admin``,
    ``xnvme cmd-io`` and ``xnvme env library`` replace them
  - The tools ``lblk``, ``zoned``, ``kvs``, ``xdd`` and ``xnvme_file`` are now
    the test programs ``nvm``, ``zns``, ``kv``, ``dd`` and ``file``, run as
    ``xnvme test <name>``
  - New commands: ``xnvme perf``, a benchmark, also of I/O into GPU memory;
    ``xnvme homi``, which holds controllers open and shares them with client
    processes, and exposes each as a CUSE device under ``/dev/xnvme/<bdf>``
    that answers the NVMe driver's ``ioctl()`` interface; ``xnvme qublk``, a
    ublk server that serves xNVMe devices, zoned ones included, as Linux block
    devices; ``xnvme env``; ``xnvme cmd-iov``. The systemd units
    ``xnvme-homi@`` and ``xnvme-qublk@`` run the two servers
  - A project outside xNVMe adds a command by installing ``xnvme-<name>`` into
    the ``libexecdir`` and a ``<name>.extension`` into the ``extensiondir`` that
    ``xnvme.pc`` names

* Library and build

  - Device open, close and enumeration, buffer allocation and queue setup and
    teardown hold one process-wide recursive lock
  - Hugepage memory comes from the uPCIe host-memory heap, backed by
    ``memfd_create(MFD_HUGETLB)``, and needs no mounted hugetlbfs.
    ``XNVME_HUGETLB_PATH`` is no longer required; when set, the heap is placed
    in the hugetlbfs it names. ``XNVME_HUGEPAGE_BACKEND`` chooses ``memfd`` or
    ``hugetlbfs``, and ``XNVME_HUGEPAGE_HEAP_SIZE`` sets the size of the heap
  - The build options ``be_ramdisk`` and ``cbi_*`` are replaced by
    ``with-ramdisk``, ``with-shim``, ``with-emu``, ``with-nil``, ``with-posix``
    and ``with-thrpool``. ``with-upcie``, ``with-cuda``, ``with-hip``,
    ``with-cuse``, ``with-driverkit``, ``with-iocp``, ``with-ioring`` and
    ``with-spdk-rdma`` are added, and ``systemd`` installs the systemd units
  - The build needs meson 0.63 or later, instead of 0.58
  - The CUSE devices of ``xnvme homi`` use ufuse v0.4.0, carried as a
    subproject
  - ``XNVME_EXEC_PATH`` and ``XNVME_EXTENSION_DIR`` replace the directories in
    which ``xnvme`` looks up its programs and extensions.
    ``XNVME_SPDK_IOVA_MODE`` sets the IOVA mode of SPDK when ``xnvme_opts``
    does not. The documentation of the upcie backend describes
    ``XNVME_UPCIE_VFIO_MODE``, ``XNVME_UPCIE_VA_BITS`` and
    ``XNVME_UPCIE_CUDA_DOORBELL_SHARE``
  - Release builds define ``NDEBUG``
  - ``xnvme --version`` prints the revision that the program was built from;
    a source archive records it in ``.vcs_tag``, and the archive of a release
    records its tag
  - The source archives carry no git metadata

* Packaging

  - Debian is now Trixie and Forky; Ubuntu is now 24.04, 26.04 and 26.10;
    Fedora is now 43 and 44; CentOS Stream is now 10; Rocky Linux is now 9.7
    and 10.1; Oracle Linux 9 is dropped
  - FreeBSD is now 14 and 15, and macOS is now 14, 15 and 26
  - xNVMe builds on NixOS as its Nix package
  - The RHEL-based distributions enable EPEL
  - The Debian packages are built from the source archive of the release

* Infrastructure

  - The CI jobs that need a guest, a drive or a GPU run when the pull-request
    has their ``gate:`` label, and ``ci-result`` decides whether a
    pull-request may merge. The guests run Debian Trixie instead of Debian
    Bullseye
  - A pull-request whose last commit is the version bump runs every CI job.
    When it is merged, ``publish-release`` tags ``main``, creates the GitHub
    release from that pull-request's run, and deploys its documentation;
    ``make publish`` uploads to PyPI and crates.io. v0.8.0 is the first release
    made this way

* Documentation

  - The command line and the API are described as they are now, with man
    pages for the command groups
  - The man pages name the version of xNVMe that they describe, and the bump
    check fails a release whose man pages name another
  - The documentation is written in reStructuredText and MyST Markdown

v0.7.5
------

For details, then see the commit-messages, for highlights see below.

* API Extensions

  - Add helper `xnvme_nvm_compare()`

    - Plus emulation via ramdisk

* API Refactoring

  These are the changes you need to account for if you are using the following
  functions defined in the public xNVMe API:

  - `xnvme_nvm_sanitize()`: arguments 'oipbp' and 'nodas' are now `bool`
  - `xnvme_znd_mgmt_recv()`: argument renamed from 'action' to `zra`
  - `xnvme_znd_mgmt_send()`: argument renamed from 'action_so' to `zsaso`
  - `xnvme_cli_args`
  
    - `{clear,direct,all,save,verbose,ad,idw,idr}*`: changed to `bool`
    - `action` renamed to `sanact`
    - `zrms` renamed to `zsa`
    
  - `xnvme_pi_generate`
  
    - `dbuf` renamed to `data_buf`
    - `mbuf` renamed to `md_buf`

  - Renamed `enum xnvme_retreive_opts` to `enum xnvme_retrieve_opts`

* Backends

  - The vfio backend now support iovec payloads

* Library

  - A version-script now controls library symbols, effectively preventing symbol
    leakage from linked libraries.
  - The build no longer fails when SPDK dependencies are not satisfied
  
    - Instead of failing the build, the SPDK backend is disabled.
    - SPDK is still "special" in the sense that xNVMe builds and links a patched
      version. However, the default behavior now is to disable the SPDK backend
      instead of failing the build when transient dependencies are missing.
      This is a much nicer behavior for users who simply grab xNVMe to consume
      `io_uring` command and NVMe driver ioctl interfaces.

  - Replaced custom shared/static library logic with default Meson handling

    - The default is to build both the static and shared libraries.
    - The tools, examples, and tests are by default linked with the shared
      version of the library.
    - To build only a single version, use '--default-library' specifying either
      'static' or 'shared'.

* Infrastructure

  - The ramdisk workflow is now running on macOS.
  - Expanded automated performance evaluation to include Windows and FreeBSD.
  - Latency report
  
    - Overhead per command evaluated on Linux, FreeBSD, and Windows.
    - Uses `fio`.
    
  - Scalability report
  
    - Single-core peak performance when using xNVMe via SPDK/bdev_xnvme.

* Documentation

  - The Sphinx-doc project theme now uses the (in)famous PyData Sphinx-theme.
  - Major overhaul: the documentation and xNVMe website are now unified as a
    Sphinx-doc project.
  - A complete rewrite of the getting-started, toolchain, and API sections.

v0.7.4
------

A lot of minor fixes, reduction of technical debt, and fixes in preperation for
Debian packaging. See the commits for details.

* Packaging, see subsection "packaging" in the Contributor section of the docs

* feat: Delayed identification, upon xnvme_dev_open() then a device was
  "identified" and its geometry "derived", this is dropped in favor of "adhoc" /
  "implicit" action when the data is requested.

* feat: The SPDK backend now supports iovec payloads

* feat: end-to-end data-protection is now available with helpers in the public
  API

* feat: Support for completion-notification via eventfd() by using helper
  function xnvme_queue_get_completion_fd()

v0.7.3
------

* Pseudo Commands

  - Encapsulation of functionality not NVMe-spec defined, yet common for
    drivers etc. encapsulated for backend-handling as "pseudo-commands" these
    are

    - xnvme_controller_reset()
    - xnvme_controller_get_registers()
    - xnvme_namespace_rescan()
    - xnvme_subsystem_reset()

  - Implemented in the SPDK and Linux backends
  - Usage via cli-tools is available

* Controller Handles

  - Having device-handles to NVMe-controllers is a necessity for e.g. reading
    controller-registers as provided via xnvme_controller_get_registers()

* Backend: SPDK

  - Support for the KV Command-Set over NVMe/TCP

v0.7.2
------

* API Refactoring

  - refactor(api): hoist inline-struct 'xnvme_opts.css'
  - refactor(api)!: hoist "xnvme_opts.oflags" and drop int-version
  - refactor(api): hoist and define xnvme_cmd_ctx_async
  - refactor(api): hoist and define xnvme_lba_range_attr

* Backends: libvfn/vfio

  - Fix use of the libvfn API (memory-mapping)
	- Bumping of the libvfn to libvfn 3.0.0-rc2, bumped to support
    eventfd-notification and fix errors with memory-mapping

* Backend: SPDK

  - feat(build/spdk): add libarchive dependency
  - feat(build/spdk): bump to v23.09

* Backends: Windows

  - Added support for SPDK in Windows via WPDK

Backend: linux / liburing

  - feat(be/liburing): turn on batching by default

* Feature: Pseudo-commands

	- Backend interface expanded with "pseudo-commands", this is to unify aspects
    which are not command-encapsulated; reading controller-registers, resetting
    subsystems etc.
	- api: add xnvme_subsystem_reset() / xnvme_controller_reset() /
    xnvme_namespace_rescan(), these construct "pseudo-commands" to instruct the
    backend to e.g. reset.

* Rust

  - xNVMe now has experimental Rust bindings via crate 'xnvme-sys'
  - feat(rust/xnvme-sys): add bindgen-emitted wrapper for libxnvme
  - feat(rust/xnvme_sys): add simplest example of call via bindings
  - feat(git/ignore): add 'rust/target' to ignore
  - feat(pcf,gha): add build of Rust crates


* Misc. Fixes

  - fix: format-strings (use PRI-macros instead of e.g. lu-specifiers)
  - fix(cli,enum): use xnvme_buf_virt_{alloc|free}() instead of malloc/free
  - fix: use zu format-specifier for size_t
  - fix(cli): change assignment for create_mode
  - fix(spec): adjust format-specifier for comp_temp
  - fix(be/libaio): remove comparison warning 
  - refactor(toolbox/xnvmec_generator): emit to subfolders
  - refactor(build/manpages)!: only install manpages for installed binaries
  - refactor(build/completions)!: only install completions for installed binaries
  - fix(meson/options): remove unused option shared_library
  - fix(meson/options): remove unused option enable-ysnp
  - fix(toolbox/pkgs): build libvfn as release

* Build

  - Cleanup helper targets

* cijoe

  - feat(cijoe): add workflow for provisioning via git

* Documentation

  - Added Tutorial for FDP
  - Added Overview of the xNVMe CI environment

v0.7.1
------

* API
  - Introduced memory-mapping API (libxnvme_mem.h), that is, when the user
    manages memory and thus does not want to use xnvme_buf_alloc() as doing so
    would introduce bounce-buffers.

* CLI
  - The passthru cli-interfaces now use --cdwXY arguments instead of
    "command-files", this alignes with nvme-cli and is arguably simpler

* Backends: Linux
  - Performance improvments in io_uring and io_uring_cmd backends due to the
    addition batching via delayed submission
  - Performance improvements to libaio due to the addition of the "ring-hack"

* Backends: Windows
  - Support added for writing via "ioring" on Windows, previously only batched
    reads were possible via this interface.

* Infrastructure; github-actions, docker-images, third-party etc. bump the
  reference toolchain platforms, that is
  - Debian is now Trixie, Bookworm and Bullseye.
  - Ubuntu is now Lunar, Jammy and Focal
  - Added Rocky Linux 9.2
  - Added Oracle Linux 9.
  - Dropped CentOS 7
  - Dropped CentOS Stream 8
  - Dropped openSUSE Leap 15.3 and 15.4, Tumbleweed remains

* Removal of a bunch of deprecated files and code. General *cleanup*.

v0.7.0
------

This release got a whole lot bigger than it ought to. Aiming for more
frequent releases in the future. A bunch of changes to API,
implementation, and even the license.

Regarding the license. For compatibility with GPL-licensed software.
That is, consumption of xNVMe from GPL-licences software, then xNVMe is
changed from APACHE to BSD-3-Clause. To better determine the license,
the project has aligned with the golden practice of REUSE-compliance
(https://reuse.software/).

* API
  * Refactored to provide a single "entry-point" header
  * Headers no longer include other headers (except for the entry-point)
  * Renamed the command-line api from xnvmec to xnvme_cli

* API / Command-Sets
  * Support for FDP and DSM
  * Support for Key-Value SSDs

* Backends
  * The POSIX backend is replaced by CBI -- Common Backend Implementation
  * Instead of "POSIX" then a handful of commonly useful implementations
    are provided, being "strictly" POSIX was an impractical restriction,
    such as lacking support for features which are provided on modern
    operating systems but not defined by POSIX.

* Backends: FreeBSD
  * Support for kqueue based aio

* Backends: SPDK
  * Now has options to control Admin and I/O command timeout
  * Removed patches no longer needed (available upstream)

* Backends: vfio
  * Removed as subproject and instead links "properly" with the
    system-provided libvfn

* tools
  * switched fio from "external" to "internal" / "upstream" fio engine

For additional details then please have a look at the commit-history,
the v0.7.0 related issue on GitHUB and PR.

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
