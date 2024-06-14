.. _sec-backends-common-async-nil:

Async. I/O via ``nil``
~~~~~~~~~~~~~~~~~~~~~~

The ``nil`` backend is entirely for debugging and measuring the IO-layer, all
the ``nil`` async. implementation does is queue up commands and when polled for
completion they are returned with success.
