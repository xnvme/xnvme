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

Known Bugs
----------

* It is unknown for which Kernel-version the ``io_uring`` backend works
* The async-interface tests does not verify that it is creating more queues
  than what is supported by the device

v0.0.10
-------

* xnvmec: changed description of '--sel' cli-arg
* spec:public:
  - Added FIDs 0x6 and 0x7
  - Added pp for FID 0x7 (number of queues)
* documentation: update and fix
* CHANGELOG: Update description of release-methodology and versioning
* build: removed 'CONFIG'
  - It served the purpose of providing default values
  - This leads to multiple sources for default values e.g. CONFIG,
    './configure', and 'CMakeLists.txt', so it is now moved from CONFIG to
    './configure'. This does not get rid of multiple config-sources, however it
    reduces it by one.
* build:cmake: added config-option to enable/disable bundling of system deps
  - default: disabled
* Removed ``include/bsd/sys/queue.h`` as it conflicts with the one available on
  the system used by others.
* build:cmake: moved xNVMe specifics from 'bundle' function into CMakeLists
  - The cmake-bundle function is now generic, e.g. usable by libraries looking
    to bundle libs together
* Refactor ``xnvme_ret`` to ``xnvme_req``
* Refactor ``cli`` to ``tools``
* be:liou: fixed submission checking
* be: added ``xnvme_ident`` prefixes e.g. ``lioc://``, ``pci://``
  - The prefix determines which backend to use for the given device uri
  - This functionality replaces the use of the ``XNVME_BE`` variable
  - Regular path-specs, e.g. ``/dev/nvme0n1`` are still supported, however,
    there is a tie when both ``XNVME_BE_LIOC`` and ``XNVME_BE_LIOU`` is
    enabled. Which backend should be used?
    This relies on the order, e.g. the first backend to ``ident_from_uri``
    successfully is used. Currently, ``LIOU`` is tried before ``LIOC``.
* be:lioc: prefixed enumeration result with ``lioc://``
* be:liou: prefixed enumeration result with ``liou://``
* be: remove use of the ``XNVME_BE`` variable
* be: moved 'be:nosys' out of 'xnvme_be.c' into 'xnvme_be_nosys.c'
* be: moved 'xnvme_ident.c' and 'xnvme_enumeration.c' into 'xnvme_be.c'
* tests: async.-interface do not try and create more than supported as reported
  by get-features FID(0x7)
* dev: removed ``xnvme_dev_get_be_id``
* sgl: changed to ``sys/queue.h`` available on the system
* xnvme:public: removed backend identifiers from public
* docs: fixed deprecated backend descriptions
* docs: added missing descriptions ``XNVME_BE_LIOU``
* docs: removed descriptions of ``XNVME_BE``

Backlog
-------

* tools: align parameters for the different tools, e.g. just use ``--slba``
  instead of having three ``--slba``, ``--zslba``, and ``--lba`` for the same
  thing
* Fix construction of more queues than supported
* build: have different compile-options for the SPDK and liburing as they rely
  on non-ISO features
* Not updating SPDK to 17.10 due to:
  - https://review.gerrithub.io/c/spdk/spdk/+/478349
* ident: fix limited ident alloc by adding re-sizable array encapsulation
* Fix build warnings
  - Consider surrendering to gnu11
  - Consider unused arguments in SPDK backend
* Make sure that a single custom kernel is usable for all Linux and SPDK
  backend
* Provide `./configure` options to overrule the default search path for the
  Linux / FreeBSD NVMe driver IOCTL headers. For whichever reason one might
  have to build against a specific `IOCTL` header.
* Removed backend 'ids' and rely entirely on the 'name'
  - Hmm... or just don't make backend identifers public... have them be
    internal...
  - Get to remove the shared enumeration variable
  - Backends can be added / removed from the library with minimal overlap
* Provide a cli-tool showing the backend support
  - E.g. 'xnvme libinfo'
  - Printing out version information
  - Backends, with "enabled" to true/false
  - Other 'constant' factors of the library / backends
    - SPDK/DPDK version
    - liburing version
    - Identifier prefixes for the different backends
* Re-consider enumeration
  - Should it enumerate using all backends, or only for a specific be-id?
  - What should the default behavior be?
  - How should enumerations be presented in the cli-tools?
* Fabrics, determine usage with NVMe-OF and write documentation / tutorial
* Add a helper-function ``xnvme_req_explain(dev, *req)``, producing an
  "explanation" of the given request
  - E.g. when using ``be:liou`` we cannot get the NVMe completion entry, by
    having this function, one could get explanations on how things are the
    SC/SCT field are constructed from errno
  - It can also provide textual representation of SC and SCT, such that instead
    of 0xBC it would inform of "Request failed because of INVALID_WRITE"
* be:liou: add support for the Append command
* be:liou: add registration of fds and buffers
* be:liou: add support for constructing multiple sqe's before entering the ring
* be:laio: Add a new backend, deriving from lioc, extending with async behavior
  using libaio
* be:faio: Add a new backend, deriving from fioc, extending with async behavior
  with preferred methods for FreeBSD
* be:spdk: determine whether constructing multiple sqe's before ringing the
  doorbell? Is that an improvement likely, similar to entering the io_uring?
* scripts: have the xnvmec_generator scan for examples, tests, and tools
  instead of specifying them directly
* pp: add a generator of data-structure pretty-printer
* pp: add support for JSON (only do this it they are auto-generated)
* Add Open-Channel 2.0 support
* Add higher-level io support ala liblightnvm VBLK / append-only streams
* Add support for SNIA KV-SPEC
* Add support for Samsung KV-SPEC
* Add support for an NVMe based computation-storage API
* Add source-analysis tools to CI, e.g. LGMT, cochinelle
* Add runtime-analysis tools to CI, e.g. valgrind and memchecking
* Have one source for the version-number

liblightnvm inheritance, relation and direction
-----------------------------------------------

-  Increased convenience for generic NVMe usage, primary focus remain on
   convenience for emerging storage interfaces e.g. Zoned Namespaces and
   Open-Channel SSDs but convenience for commonly used NVMe commands and
   representations are now available and a priority as well

-  Expanded cmd-interface and its generality, e.g. ``_cmd_idfy`` was a
   OCSSD command specific, now it is a generic idfy command conveniently
   parameterized for the idfy structure (CNS) of interest, e.g.
   idfy-controller, idfy-namespace, namespace list, OCSSD geometry etc.
   all of your NVMe identification needs, with increased convenience

-  xNVMe has a generic ``nvm_cmd_log`` where ``liblightnvm`` had only
   OCSSD specific ``znd_rprt``. ``nvm_cmd_log`` is generic NVMe and can
   be used to fetch log-pages for any ``CNS``. The ``nvm_cmd_log``
   primitive is used for higher-level ``znd_report_from_dev``,
   ``znd_dev_get_rzaddr``, and ``znd_changes_from_dev``

-  Refactored cmd-interface to always take buffers as arguments instead
   of allocating them, e.g. it is the responsibility of the allocator to
   de-allocate, have the responsibility split is something the library
   aims to minimize. Exceptions being where the size is unknown /
   variable or otherwise very inconvenient for the caller to allocate.

-  There is not a `buf_set` in the C API, might be added later to CLI

-  There is not a boiler-plate `bp` in the C API. This is because CLI, Tests,
   and Examples all use the same features of the CLI library for such tasks.

-  CLI library has been simplified, evar-parsing removed and thus the
   only entry for evars is ``XNVME_BE``, the parsing of which is handled
   by the base library not the CLI interface.
   There is now only a single positional argument, short-options are replaced
   by long options as either flags e.g. "--verbose" or with arguments e.g.
   "--nsid 0xNSID". long options can be specified to be required or optional
   for the command.

- CLI library is now part of the main-library, it was silly that it was
  separate as it depended strongly on the main-library.

- xNVMe vendors/bundles dependencies and integrates the source via
  git-submodules

Cleanup
-------

-  Refactor get-log-page struct with headers to have generic names, e.g.
   ``nentries`` and ``entries``
-  Refactor doc-string descriptions.
-  Consider a better rsvd convention in structs, byte offset?
-  Fix comments for doxygen e.g. ``///< # Parallel Unit Groups`` creates
   bold text", change to e.g. ``///< Nr. of Parallel Unit Groups``
-  Consider proper way to continous verify correctness of pretty-printers /
   YAML
-  ALL print functions must implement at least YAML representation
-  ALL print-functions must take a 'flag' parameter, e.g. to modify
   format from YAML to something else. Add helper / human readable
   comments to the chosen output representation
