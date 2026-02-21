(sec-backends-common)=

# Common Backend Impl (CBI)

The common backend implementations are implementations for re-use by other
platforms. For example, instead of the Linux, FreeBSD, and macOS platforms all
implementing the same POSIX asynchronous I/O interface, it is instead provided
here, and they can all include it in their backend configs.

Similarly, **xNVMe** provides several software-only implementations of the
**xNVMe** API, such as the **thrpool** which implements the asynchronous parts
of the **xNVMe** API by using a thread-pool.

Each platform composes these CBI components into predefined backend configs
(`xnvme_be_config`), providing tested and validated combinations. In addition to
common implementations and interface emulation, **xNVMe** also provides simple
device emulation via the **ramdisk** backend implementation.

## Instrumentation

* Memory Allocators

  - `posix`, Use **libc** `malloc()/free()` with `sysconf()` for
    alignment, add link to CBI

* Asynchronous Interfaces

  -  `nil`, **xNVMe** null-IO, does nothing but complete submitted commands,
     for experimentation only to evaluate **xNVMe** encapsulation overhead
  - `emu`, ..
  - `thrpool`, ..
  - `posix`, ..

* Synchronous Interfaces

  - `psync`, Use pread()/write() for synchronous I/O



```{toctree}
:maxdepth: 2
:hidden:

psync
aio
emu
thrpool
nil
ramdisk
```
