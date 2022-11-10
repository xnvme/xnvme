Conventions
===========

A brief overview of conventions used for the xNVMe C codebase.

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

* The API and its backend implementation cannot be assumed to be thread-safe

* Be minimal with definitions in the public API

  - E.g. the 'sys/queue.h' was removed from the public API as it gave several
    head-aches when building xNVMe on multiple platforms. However, internally,
    specifically in backend implementations, assumptions can be made on the
    availability of certain headers and their general availability

* Each function in the API must have a command-line tool exercising it

  - The source-code for the command-line tools serve as examples of utilizing
    the C API in general and the tools are used during testing
  - The command-line utilities must use the ``libxnvmec.h`` as this provides a
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

Additionally, a backend-state is available for specialization to a given
backend and its function-interface implementations.

* Should have a header-file defining the backend and what it exposes

  - Located in ``include/xnvme_be_<backend_name>.h``
  - Have a state-struct named ``xnvme_be_<backend_name>_state``
  - Provide ``extern struct xnvme_be_<interface> *`` definitions for the
    interface implementations, these are used when "mixing-in" the
    implementations into the backend

* Should be implemented in a file named ``lib/xnvme_be_<backend_name>_<interface>_<ident>.c``

  - The ``<ident>`` is a name uniquely identifying the interface-implementation
  - Example: the Linux backend, named **linux**, has the **async**
    implementation of the **async** interface utilizing **io_uring** in a file
    named: * ``xnvme_be_linux_async_liburing.c``
  - Example: The SPDK backend, named **spdk**, has the **async** implementation
    using the SPDK NVMe driver in a file named:
    * ``xnvme_be_spdk_async_nvme.c``

* Should use static function declarations to avoid symbol-leak

  - The ``cmd_io()`` backend interface function should be declared ``static int
    cmd_io(...)``

* In case an interface-function should be shared by other backends, then
  provide the full "name", backend/interface/ident/function

  - Example: for the Common Backend Implementations (CBI), the **sync**
    implementation using **psync** for the **cmd_iov()** interface function,
    then the function is named: ``xnvme_be_cbi_sync_psync_cmd_io()``
