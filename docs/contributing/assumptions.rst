.. _sec-contributing-assumptions:

==================
 Code Assumptions
==================

These are notes on contracts and assumptions in the **xNVMe** codebase. These
are written down separate from the code for more convenient reading. Thus,
whenever code-changes are made that alter assumptions and contracts described
herein, then this document must be updated as well as part of the change-set.

The device handle contract
==========================

Device handles are opaque pointers of type :c:struct:`xnvme_dev`.
Device handles are obtained via the public **xNVMe** API
call :c:func:`xnvme_dev_open()`, and released via a call
to :c:func:`xnvme_dev_close()`.

The entities involved in the implementation of this and their individual
responsibility in setting up the data-structure is described in this section
here.

These functions are implemented in the library-private **device module**
(``xnvme_dev.c``). The **device module** is responsible for allocating
and initializing the data-structure associated with and encapsulating the
device handle upon :c:func:`xnvme_dev_open()` and freeing these resources
upon :c:func:`xnvme_dev_close()`.

Device handles are system interface specific; file-descriptors on UNIX-like
systems, opaque pointers on Windows, typed pointers in user-space drivers
such as SPDK, etc. Thus, the **device module** is only responsible for generic
initialization, it delagates these system specifics to the **backend module**
(``xnvme_be.c``).

The library-private **backend module** in turn, defines a **backend
interface** (``xnvme_be.h``) and implements a **backend factory**
(:c:func:`xnvme_be_factory()`). The **backend factory** knows of all
**backends** implementing the **backend interface**.

Thus, to obtain a system specific handle it delegates to each **backend**,
in turn, the implementation of obtaining a system interface specific device
handle, for a given **device identifier**, is handled by the first **backend
implementation** (``xnvme_be_*_dev.c``) capable of producing a handle.

In other words, the **backend factory** chooses the first **backend** capable
of returning a system interface specific device handle and the system interface
specific handle produced by the **backend**.

The entities described above are those involved in the setup of a xNVMe
device handle. The xNVMe device handle is opaque, that is the details of its
representation is hidden from the user. Thus, functions are provided to retrieve
data about a given device handle.

However, described here are the details on how the representation is
constructed by the library-private: **device module**, **backend module**,
and instances of the **backend interface** implementation aka **backends**
(``xnvme_be_{spdk,vfio,linux,fbsd,windows}_*.c```).

Identification and Geometry
---------------------------

The structure encapsulating the **device handle** is library-private, however,
for contributions to **backends**, **device module**, and related, then a bit of
info on how it is setup is useful, it is provided below.

.. code-block:: bash

  # Assigned by xnvme_be_factory()
  dev.be

  # The backend-factory copies the opts and adjusts them to the options
  # describing the backend, thus showing which "configuration" it ended up with
  dev.opts 

  # Initialized by xnvme_dev_open() using helper xnvme_ident_from_uri(), then
  # backends further manipulate the structure
  dev.ident 

  # These are setup by the backend as it is "opening" the device, here it might
  # do simple things such as fstat() on Linux to determine whether the device
  # handle is a block-device or a char-device to indicate the ident.dtype
  dev.ident.dtype

  # The device module assigns this in the identification-helper _dev_idfy()
  dev.ident.subnqn

  # Each backend-implementation sets these up after they have created a
  # system-interface-specific device handle
  dev.ident.dtype
  dev.ident.nsid
  dev.ident.csi

  dev.is_derived # setup by xnvme_dev_get_*() access after calling xnvme_dev_derive_geometry()

  # These are assigned by the device module at "identification-time"
  dev.id.ctrlr
  dev.id.ns
  dev.idcss.ctrlr
  dev.idcss.ns
  dev.attempted_dev_idfy

  # Populated by the device module using the helper xnvme_dev_derive_geometry()
  dev.geo					

  # Set by xnvme_dev_derive_geometry()
  dev.attempted_derive_geo
