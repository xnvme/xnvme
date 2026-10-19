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

C: Backend Implementations
--------------------------

The backend interface is declared in the internal header ``xnvme_be.h``. It
consists of function-pointers for different tasks grouped together in structs.
Those different set of tasks are referred to as the function-interfaces, they
are labeled / grouped by:

* **dev** : device handles and device enumeration
* **mem** : memory allocation for command-payloads aka buffers
* **admin** : synchronous admin-command processing
* **sync** : synchronous I/O command processing
* **async** : asynchronous I/O command submission/completion via queues

Each directory in ``lib/backend/`` is one mechanism, such as ``liburing``,
``posix`` or ``spdk``, or one OS's own interfaces, such as ``linux``. Its
files follow from what they define:

* ``xnvme_be_<dir>.h`` is the directory's header. It holds the state-struct
  ``xnvme_be_<dir>_state``, the ``extern`` declarations of the directory's
  implementations, and the declarations of its shared helpers. It can be
  included anywhere: what needs the directory's external library, such as
  ``liburing.h``, sits behind the directory's ``XNVME_BE_*_ENABLED``. ``include/``
  is only for public headers

  - A topic whose declarations need headers the rest of the directory must
    not include, such as a GPU runtime, gets ``xnvme_be_<dir>_<topic>.h``

* All implementations in a config share one state, ``dev->be.state``, and
  the dev implementation decides its layout. A directory defines its layout
  in ``xnvme_be_<dir>_state.h``, which needs no external headers, and
  ``lib/backend/xnvme_be_state.h`` lists every layout as a member of ``union
  xnvme_be_state``. Implementations reach it through ``xnvme_be_state(dev)``,
  as in ``&xnvme_be_state(dev)->linux_state``, and never cast
  ``dev->be.state`` themselves

  - Where implementations from another directory read the state, the layouts
    share a first member, so the read is defined by C's rule on common initial
    sequences, and the header asserts it
  - The posix and shim implementations read ``posix_state``, so the Linux,
    FreeBSD and macOS states embed ``struct xnvme_be_posix_state`` as their
    first member, ``posix``, asserted with ``XNVME_BE_POSIX_STATE_FIRST()``
  - ``xnvme_platform_dev_open()`` stores a shared controller in ``ctrlr``, so
    the states of the dev implementations that share controllers start with
    ``ctrlr``, asserted with ``XNVME_BE_CTRLR_STATE_FIRST()``

* ``xnvme_be_<dir>_configs.c`` holds the backend configs whose parts all
  come from the directory, named ``g_xnvme_be_<name>`` after the name ``--be``
  selects them by, with ``-`` as ``_``, such as ``g_xnvme_be_spdk`` and
  ``g_xnvme_be_upcie_cuda``. A config that combines implementations from
  several directories, such as io_uring with the Linux NVMe ioctls and POSIX
  memory, lives with the platform that offers it, in
  ``lib/platform/<os>/xnvme_platform_<os>_configs.c``, and is named
  ``g_xnvme_platform_<os>_<name>``, such as
  ``g_xnvme_platform_linux_io_uring``. Every config is declared in
  ``lib/backend/xnvme_be_registry.h``

* Each implementation of a backend interface has a file of its own: the
  ``g_xnvme_be_<dir>_<interface>[_<ident>]`` it defines lives in
  ``xnvme_be_<dir>_<interface>[_<ident>].c``. The ``<ident>`` tells
  implementations of the same interface apart, and is left out when there is
  only one

  - Example: the **async** implementation using **io_uring** passthru is
    ``g_xnvme_be_liburing_async_ucmd``, in
    ``lib/backend/liburing/xnvme_be_liburing_async_ucmd.c``
  - Example: the Linux **sync** implementation using the NVMe-driver ioctls is
    ``g_xnvme_be_linux_sync_nvme``, in
    ``lib/backend/linux/xnvme_be_linux_sync_nvme.c``

* Code that more than one file in the directory uses lives in
  ``xnvme_be_<dir>_<topic>.c``, such as ``xnvme_be_linux_nvme.c`` for the
  NVMe-driver ioctl wrapper or ``xnvme_be_upcie_vfio.c``. A topic file defines
  no ``struct xnvme_be_<interface>``

* Functions are ``static`` unless another file needs them, and then they are
  named ``xnvme_be_<dir>_<topic>_<function>()`` and declared in
  ``xnvme_be_<dir>.h``

  - Example: ``xnvme_be_posix_sync_cmd_io()``, which the Linux block **sync**
    implementation reuses for reads and writes

* Each directory in ``lib/backend/`` has one build option, ``be_<dir>``, and
  one macro, ``XNVME_BE_<DIR>_ENABLED``, which ``xnvme library-info`` lists. A
  feature within a directory is ``XNVME_BE_<DIR>_<FEATURE>_ENABLED``, such as
  ``XNVME_BE_LINUX_BLOCK_ENABLED``

  - A disabled directory keeps its implementations, compiled to the functions
    of ``xnvme_be_nosys.h``. A config using a disabled ``emu``, ``nil``,
    ``thrpool`` or ``shim`` stays and reports ``-ENOSYS``; one using any
    other disabled directory is left out of the platform's list
