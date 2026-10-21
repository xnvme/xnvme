Conventions
===========

A brief overview of conventions used for the **xNVMe** C codebase.

C: format
---------

clang-format is utilized to format the code. There are two clang-format-option
files, one for the headers (``toolbox/clang-format-h``) and one for the source
``toolbox/clang-format-c``.

You can setup your editor to use the format-files, or run clang-format via the
toolbox: ``make format-all``. This will format all files in the repository.

C: style
--------

* Declare variables

  - At the **top** of code-blocks; before any logic, e.g.

    - at the top of a function-code-block / loop-nest / anonymous-block
    - for-loop "control/iterator" variables should be declared and initialized
      in the for-loop header.

  - In the code-block nearest to its utilization

    - in a function, the top-most scope is commonly used for
      argument-unpacking, and the idiomatic ``int err`` variable, along with
      auxilary variables needed by the function body in the outer-most scope
    - variable-declaration in nested-block-scope is encouraged; still at the
      top of that block-scope. For example, when a variable is only ever used
      in a nested-block, then declare it within that scope

* Initialize variables

  - Using zero/constant-initialization; ``struct foo = {0};``
  - To unpack arguments; ``int simple = arg->is->alot->simple;``
  - Do not initialize using functions that require error-handling

* Global variables

  - Prefix them with ``g_``

* Compiler-specific features

  - For example, gnu/gcc-only is discouraged, as it hinders portability
  - Code should preferably translate using any of gcc/clang/icc/pgi
  - Exceptions to this would be code in backends utilizing features which are
    only available on the platform that the backend services

C: API
------

* The API and its backend implementations aims to be consistent with the following

  - Pass pointers to "objects" e.g. ``xnvme_cmd_pass(*ctx, ...);``
  - Pass double-pointers for "object-initialization" e.g. ``xnvme_queue(..., **queue);``
  - On success, return 0. On error, Return negative ``errno``.
  - Exceptions: API-calls mimicking legacy interfaces, such as the
    ``xnvme_buf_alloc()`` which is operating similar to ``malloc()/free()``.

* The library is not thread-safe. Only the following control-plane calls may
  be made concurrently, on every backend, and only with respect to each other:
  a call made while another thread uses the same device, queue or buffer for
  I/O is not covered

  - ``xnvme_dev_open()``, ``xnvme_dev_close()``, ``xnvme_enumerate()``,
    ``xnvme_queue_init()``, ``xnvme_queue_term()``, ``xnvme_buf_alloc()``,
    ``xnvme_buf_realloc()``, ``xnvme_buf_free()``, their ``xnvme_buf_phys_*()``
    variants, ``xnvme_buf_vtophys()``, ``xnvme_mem_map()``,
    ``xnvme_mem_unmap()``, ``xnvme_cuda_queue_create()`` and
    ``xnvme_cuda_queue_destroy()``; one process-wide lock serialises them
    (``lib/util/xnvme_lock.c``)
  - A queue, and the command contexts drawn from it, belongs to one thread at a
    time, and so do a device's synchronous and admin command paths
  - The device getters that identify the device on first use, such as
    ``xnvme_dev_get_geo()`` and ``xnvme_dev_get_ident()``, must not make that
    first call from several threads at once
  - Backend code that reaches the same process-wide state from threads of its
    own takes the same lock, ``xnvme_lock()``

* Be minimal with definitions in the public API

  - E.g. the 'sys/queue.h' was removed from the public API as it gave several
    head-aches when building **xNVMe** on multiple platforms. However, internally,
    specifically in backend implementations, assumptions can be made on the
    availability of certain headers and their general availability

* Each function in the API must have a command-line tool exercising it

  - The source-code for the command-line tools serve as examples of utilizing
    the C API in general and the tools are used during testing
  - The command-line utilities must use the ``libxnvme_cli.h`` as this provides a
    common command-line interface, the common-cli is nice as for usability,
    such that all cli-tools use the same arguments, it is also essential for
    instrumentation of not just the logic of the tool but also the library
    backend

C: Code organisation
--------------------

Where code lives and what it is named follow from what it does, so new code
has one place and one name. These rules are for people and coding agents
alike: follow them rather than the nearest existing example, and where a case
is not covered, raise it in review instead of inventing a convention.

Layout
~~~~~~

* ``include/`` holds the public headers, ``libxnvme*.h``, and nothing else
* ``lib/<group>/`` holds the implementation of the public API, where
  ``lib/<group>/xnvme_<topic>.c`` implements ``include/libxnvme_<topic>.h``,
  next to the private headers it shares, ``xnvme_<topic>.h``. The groups are
  the ones the API documentation uses, ``NAMESPACES`` in
  ``docs/tooling/xnvme_docs/apigen.py``, so a new public header goes in both

  - ``util``: utilities the other groups use: the lock, CRCs, LBA helpers,
    the version and the build configuration
  - ``core``: devices, buffers, commands, queues, geometry, identifiers and
    options
  - ``nvme``: the NVMe command sets, the spec and its printers, topology and
    protection information
  - ``file``: the file API
  - ``cli``: the command-line library the tools share

  ``lib/meson.build`` lists ``util`` first, then ``backend`` and
  ``platform``, which the public API is built on, then ``core``, ``nvme``,
  ``file`` and ``cli``

* ``lib/cuse/`` is a static library of its own for the tools, not part of
  ``libxnvme``
* ``lib/platform/`` holds what selects among the backend configs:
  ``xnvme_platform.c``, the device-open loop, and per OS
  ``lib/platform/<os>/xnvme_platform_<os>.c``, the scan, the classify and the
  list of configs the OS offers
* ``lib/backend/`` holds the backend interface, ``xnvme_be.h``, its shared
  code, ``xnvme_be.c``, ``xnvme_be_cref.c`` and ``xnvme_be_nosys.c``, the
  declarations of every config, ``xnvme_be_registry.h``, the state union,
  ``xnvme_be_state.h``, and one directory per mechanism or OS

The backend interface
~~~~~~~~~~~~~~~~~~~~~

A backend config, ``struct xnvme_be_config``, combines one implementation of
each backend interface:

* **dev** : device handles and device enumeration
* **mem** : memory allocation for command-payloads aka buffers
* **admin** : synchronous admin-command processing
* **sync** : synchronous I/O command processing
* **async** : asynchronous I/O command submission/completion via queues

Directories in ``lib/backend/``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* A directory is either one mechanism, named after the library or interface it
  uses, such as ``liburing``, ``libaio``, ``spdk``, ``upcie``, ``iocp``,
  ``posix`` or ``thrpool``, or one OS's own interfaces, ``linux``,
  ``freebsd``, ``macos`` and ``windows``
* A mechanism gets its own directory even when only one OS has it, as
  ``liburing`` and ``iocp`` do
* What an OS provides itself, such as its NVMe-driver ioctls, stays in that
  OS's directory even when every OS has an equivalent, since the equivalents
  share no code

Names
~~~~~

The prefix names what a file or symbol implements, and the rest names which
one. Basenames are unique across ``lib/``, so every private header shares one
include path.

* ``g_xnvme_be_<dir>_<interface>[_<ident>]`` is an implementation of a backend
  interface, in ``lib/backend/<dir>/xnvme_be_<dir>_<interface>[_<ident>].c``,
  one implementation per file. The ``<ident>`` tells implementations of the
  same interface apart, comes after the interface, and is left out when there
  is only one

  - Example: ``g_xnvme_be_liburing_async_ucmd``, in
    ``lib/backend/liburing/xnvme_be_liburing_async_ucmd.c``
  - Example: ``g_xnvme_be_upcie_dev_cuda``, in
    ``lib/backend/upcie/xnvme_be_upcie_dev_cuda.c``

* ``xnvme_be_<dir>_<topic>.c`` holds code that more than one file of the
  directory uses, such as ``xnvme_be_linux_nvme.c`` for the NVMe-driver ioctl
  wrapper. A topic file defines no ``struct xnvme_be_<interface>``
* ``xnvme_be_<dir>.h`` is the directory's header: the ``extern`` declarations
  of its implementations and of its shared functions. A header that configs
  outside the directory include, such as ``xnvme_be_liburing.h``, keeps what
  needs the directory's external library behind its
  ``XNVME_BE_<DIR>_ENABLED``, so it builds where the library is absent

  - A topic whose declarations need headers the rest of the directory must
    not see, such as a GPU runtime, gets ``xnvme_be_<dir>_<topic>.h``

* ``xnvme_be_<dir>_state.h`` holds the directory's state, ``struct
  xnvme_be_<dir>_state``. It needs no external headers: a type from a
  library or SDK is held behind a pointer or an integer of its size, so
  ``lib/backend/xnvme_be_state.h`` can include every one of them
* A function is ``static`` unless another file needs it, and is then named
  ``xnvme_be_<dir>_<function>()`` and declared in ``xnvme_be_<dir>.h``, such
  as ``xnvme_be_posix_sync_cmd_io()``. The exception is a function of the
  public API that a backend implements, such as ``xnvme_cuda_queue_create()``
* A config is named after the name ``--be`` selects it by, with ``-`` as
  ``_``. One whose parts all come from one directory lives in
  ``lib/backend/<dir>/xnvme_be_<dir>_configs.c`` as ``g_xnvme_be_<name>``,
  such as ``g_xnvme_be_spdk`` and ``g_xnvme_be_upcie_cuda``. One that combines
  directories lives with the platform that offers it, in
  ``lib/platform/<os>/xnvme_platform_<os>_configs.c``, as
  ``g_xnvme_platform_<os>_<name>``, such as
  ``g_xnvme_platform_linux_libaio_bdev``. Every config is declared in
  ``lib/backend/xnvme_be_registry.h``
* A platform's files are ``xnvme_platform_<os>.c`` and
  ``xnvme_platform_<os>_configs.c``
* A mechanism directory has one build option, ``be_<dir>``, and one macro,
  ``XNVME_BE_<DIR>_ENABLED``, which ``xnvme library-info`` lists. An OS
  directory is built with its platform, ``XNVME_PLATFORM_<OS>_ENABLED``. A feature
  within a directory is ``XNVME_BE_<DIR>_<FEATURE>_ENABLED``, such as
  ``XNVME_BE_LINUX_BLOCK_ENABLED``; one outside the backend is
  ``XNVME_<FEATURE>_ENABLED``, such as ``XNVME_LIBISAL_ENABLED``

The backend state
~~~~~~~~~~~~~~~~~

* All implementations in a config share one state, ``dev->be.state``, and the
  config's dev implementation decides its layout. ``lib/backend/xnvme_be_state.h``
  lists every layout as a member of ``union xnvme_be_state``, and
  implementations reach it through ``xnvme_be_state(dev)``, as in
  ``&xnvme_be_state(dev)->linux_state``. Nothing casts ``dev->be.state``
* Where implementations from another directory read the state, the layouts
  share a first member, so the read is defined by C's rule on common initial
  sequences, and the state header asserts it

  - The posix and shim implementations read ``posix_state``, so the Linux,
    FreeBSD and macOS states embed ``struct xnvme_be_posix_state`` as their
    first member, ``posix``, asserted with ``XNVME_BE_POSIX_STATE_FIRST()``
  - ``xnvme_platform_dev_open()`` stores a shared controller in ``ctrlr``, so
    the states of the dev implementations that share controllers start with
    ``ctrlr``, asserted with ``XNVME_BE_CTRLR_STATE_FIRST()``

Dependencies
~~~~~~~~~~~~

* A directory that needs another directory, or an OS, says so in
  ``meson.build``: its option requires the other, and the configure step fails
  when the other is disabled or absent. liburing and libaio require Linux, and
  posix cannot be disabled on Linux, FreeBSD and macOS, whose directories take
  their memory and psync from it
* A disabled directory keeps its implementations, compiled to the functions
  of ``xnvme_be_nosys.h``. A config using a disabled ``emu``, ``nil``,
  ``thrpool`` or ``shim`` stays and reports ``-ENOSYS``; one using any other
  disabled directory is left out of the platform's list. ``nosys`` is for a
  directory that is switched off, never a way to leave a broken dependency to
  fail at runtime
* A directory includes another directory's header only for what it uses

Where new code goes
~~~~~~~~~~~~~~~~~~~

1. Part of the public API: ``lib/<group>/xnvme_<topic>.c``, declared in
   ``include/libxnvme_<topic>.h``, with ``xnvme_<topic>`` added to the same
   group in ``NAMESPACES``
2. A new library or mechanism: a new directory, ``lib/backend/<name>/``, with
   its header, its state header, one file per implementation, a
   ``meson.build`` adding its sources and include directory, its option
   ``be_<name>`` and macro ``XNVME_BE_<NAME>_ENABLED``, and its configs if
   they need nothing from other directories
3. Something an OS provides itself: that OS's directory,
   ``lib/backend/<os>/``
4. A config combining directories: the platform's
   ``xnvme_platform_<os>_configs.c``, listed in the platform's preset table in
   ``xnvme_platform_<os>.c`` and declared in ``xnvme_be_registry.h``
