.. _sec-api-c-file:

File
####

The **xNVMe** file API (:ref:`sec-api-c-xnvme_file`) provides a set of functions
for traditional operating-system managed files using file semantics and
functions similar to traditional I/O.

Internally, the **xNVMe** library employs a file shim that converts file
operations into NVMe commands and represents files as NVMe namespaces. As these
file operations reach the library backend, they are transformed into either
file-system native operations such as ``pwritev()``, ``preadv()``, or ``stat()``
when operating on an actual file.

.. note::
  The **xNVMe** file API is currently experimental and may undergo significant
  changes. Users are encouraged to test and provide feedback but should be
  cautious when using it in production environments.

.. toctree::
   :hidden:
   :glob:

   xnvme_*
