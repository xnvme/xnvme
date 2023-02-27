.. _sec-tools-fio:

fio IO Engine
=============

The **xNVMe** `fio`_ IO engine is provided with upstream `fio`_. A nice
introduction to using it is provided by `Vincent Fu`_ and `Ankit Kumar`_ here:

* https://github.com/vincentkfu/fio-blog/wiki/xNVMe-ioengine-Part-1
* https://github.com/vincentkfu/fio-blog/wiki/xNVMe-ioengine-Part-2

To supplement the above, then a bit of information is provided here on building
`fio`_ with the **xNVMe** I/O engine enabled, as well as some known caveats.
However, the introduction above is well recommended read.

Enabling the xNVMe engine
-------------------------

First, consult the :ref:`sec-getting-started` section, to build and install
**xNVMe**. Then go ahead and build and install `fio`_ from source:

.. literalinclude:: ../../../toolbox/pkgs/emitter/templates/src-fio.sh.jinja
   :language: bash

That is all there is to it. The `fio`_ build-system will automatically detect that
xNVMe is installed on the system, link with it, and thereby enable the xNVMe
I/O engine.

.. note::
   In case you explicitly do **not** want `fio`_ to look for **xNVMe**, then
   you can instruct the fio build-system with ``./configure --disable-xnvme``.
   This might be useful when building external I/O engines, such as those
   provided by SPDK.

Caveats
-------

This section provides a couple of notes and insights to double check in case
you are facing unexpected issues.

Threading vs Forking
~~~~~~~~~~~~~~~~~~~~

* Must be used with ``--thread=1``

The above option is required as forking for parallel job execution is not
supported; aka threading is required.

Multi-device support
~~~~~~~~~~~~~~~~~~~~

* Multiple devices are supported, however, they all either have to be
  operating-system managed or all utilize a User space driver.

Details follow.

1) ``iomem_{alloc/free}`` introduces a limitation with regards to multiple
   devices. Specifically, the devices opened must use backends which share
   memory allocators. E.g. using ``be:linux`` + ``be:posix`` is fine, using
   ``be:linux`` + ``be:spdk`` is not.
   This is due to the `fio`_ 'io_mem_*' helpers are not tied to devices, as
   such, it is required that all devices opened use compatible
   buffer-allocators. Currently, the implementation does dot check for this
   unsupported use-case, and will thus lead to a runtime error.

2) The implementation assumes that ``thread_data.o.nr_files`` is available and
   that instances of ``fio_file.fileno`` are valued ``[0,
   thread_data.o.nr_files -1]``.
   This is to pre-allocate file-wrapping-structures, xnvme_fioe_fwrap, at I/O
   engine initialization time and to reference file-wrapping with constant-time
   lookup.

3) The ``_open()`` and ``_close()`` functions do not implement the "real"
   device/file opening, this is done in ``_init()`` and torn down in
   ``_cleanup()`` as the io-engine needs device handles ready for
   ``iomem_{alloc/free}``.

NVMe devices formatted with extended-LBA
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* These are currently not supported.

Details follow.

To support extended-lba initial work has been done in xNVMe, however, further
work is probably need for this to trickle up to the `fio`_ I/O engine, also, in
the io-engine ``>> ssw`` are used which does not account for extended LBA.

History
-------

Previously, then the **xNVMe** IO engine was external. That meant that the
engine was built externally from the built of `fio`_ itself, and the
source-code for the engine was provided with xNVMe.

From fio-v3.32, then **xNVMe** IO engine became available as an "internal"
engine i upstream `fio`_. That meant that the engine was built along with the
built of `fio`_ itself, and the source-code of the engine is distributed with
`fio`_.

From fio-v.34, then all features of the external **xNVMe** IO engine where
upstreamed and removed from the **xNVMe** repository. **xNVMe-v0.6.0** was the
last version to ship the external engine.

.. _fio: https://github.com/axboe/fio
.. _Vincent Fu: https://github.com/vincentkfu
.. _Ankit Kumar: https://github.com/ankit-sam
