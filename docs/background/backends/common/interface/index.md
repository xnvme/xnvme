(sec-backends-intf)=

# Backend Interface

The core of the **xNVMe** API is encapsulated by a backend interface. The backend
interface consists of implementations of the following.

## Attributes

* `name`, name of the backend config
* `descr`, human-readable description
* `caps`, bitmask of capabilities (`xnvme_be_cap`) used for URI-based config selection

## Device

* `enumerate()`, lists devices
* `dev_from_ident()`, open of a given device and return a handle
* `dev_close()`, close a device handle

## Memory

* `buf_alloc()`, allocate buffer usable with commands
* `buf_free()`, de-allocate buffers
* `buf_vtophys()`
* `buf_realloc()`

## Asynchronous

* `cmd_io()`, submit an IO-command to a qpair
* `poke()`, process completions on a qpair
* `wait()`, wait until all outstanding commands are completed

* `init()`, initialize a qpair
* `term()`, terminate a qpair
* `supported()`, informed whether async. commands are supported

## Synchronous

* `cmd_io()`, submit an IO-command and wait for its completion
* `cmd_admin()`, submit and Admin-command and wait for its completion
* `supported()`, inform wether sync. commands are supported

# Backend Interface - Implementation

Each platform defines its available backend configs in a
`xnvme_platform_<os>.c` file, registering them via the
`g_xnvme_platform` structure. A backend config (`xnvme_be_config`) is a
complete recipe combining async, sync, admin, dev, and mem implementations.

Have a look at the existing implementations:

* `lib/xnvme_platform_*.c` — platform registration and config arrays
* `lib/xnvme_be_*.c` — backend implementations
* `include/xnvme_be_registry.h` — extern declarations for all configs

And the definition of the backend interface in:

* `include/xnvme_be.h`
