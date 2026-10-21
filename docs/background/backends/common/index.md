(sec-backends-common)=

# Cross-platform implementations

These implementations are shared by several platforms, each in a directory of
its own under `lib/backend/`. For example, instead of the Linux, FreeBSD, and
macOS platforms each implementing POSIX asynchronous I/O, `lib/backend/posix`
provides it once, and they all include it in their backend configs.

Similarly, **xNVMe** provides several software-only implementations of the
**xNVMe** API, such as `lib/backend/thrpool`, which implements the
asynchronous parts of the **xNVMe** API with a thread pool, and
`lib/backend/emu` and `lib/backend/nil`.

Each platform combines these with its own implementations into predefined
backend configs (`xnvme_be_config`), providing tested and validated
combinations. In addition, `lib/backend/ramdisk` provides simple device
emulation, and `lib/backend/shim` emulates the NVMe admin commands for
regular files.

## Instrumentation

* Memory Allocators

  - `posix`, Use **libc** `malloc()/free()` with `sysconf()` for
    alignment, add link to the cross-platform section

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
