.. _sec-backends-intf:

Backend Interface
=================

The core of the xNVMe API is encapsulated by a backend interface. The backend
interface consists of implementations of the following.

Attributes
----------

* ``name``, name of the backend
* ``schemes``, array of supported prefixes e.g. ``file``
* ``nschemes``, number of schemes
* ``enabled``, property indicating whether the backend-support is compiled in

Device
------

* ``enumerate()``, lists devices
* ``dev_from_ident()``, open of a given device and return a handle
* ``dev_close()``, close a device handle

Memory
------

* ``buf_alloc()``, allocate buffer usable with commands
* ``buf_free()``, de-allocate buffers
* ``buf_vtophys()``
* ``buf_realloc()``

Asynchronous
------------

* ``cmd_io()``, submit an IO-command to a qpair
* ``poke()``, process completions on a qpair
* ``wait()``, wait until all outstanding commands are completed

* ``init()``, initialize a qpair
* ``term()``, terminate a qpair
* ``supported()``, informed whether async. commands are supported

Synchronous
-----------

* ``cmd_io()``, submit an IO-command and wait for its completion
* ``cmd_admin()``, submit and Admin-command and wait for its completion
* ``supported()``, inform wether sync. commands are supported

Backend Interface - Implementation
==================================

If you want to add a backend for e.g. Windows 10, you could add the
implementation in ``src/xnvme_be_win10.c`` and then it to the backend registry
in ``include/xnvme_be_registry.h`` and ``src/xnvme_be.c``.

Have a look at the existing backend-implementations:

* ``xnvme_be.c``
* ``xnvme_be_*.c``
* ``include/xnvme_be_registry.h``

And the definition of the backend-interface in:

* ``include/xnvme_be.h``

