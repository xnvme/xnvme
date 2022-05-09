.. _sec-dynamic-loading:

===========================
 Dynamically loading xNVMe
===========================

In this seciton, you will find two examples of dynamically loading the
**xNVMe** shared library and via it utilize the :ref:`sec-c-api` along with
notes on common pitfalls when doing so.

The example code demonstrates how to utilize
:ref:`sec-c-api-xnvme-func-xnvme_enumerate` with callback function, callback
arguments and associated data-structures and API functions. The example will
first be presented in C and then in Python.

In C
====

Here ``dlopen()`` is used to load the library and pointers to the relevant
functions are returned by ``dlsym()``.

.. literalinclude:: enumerate_example.c
   :language: c

.. note:: When dynamically loading a library then you can only load symbols,
   such as functions. Entities such as type-definitions, enums, functions
   declared ``static inline``, MACROs etc. are not available as loadable
   symbols. However, these can be obtained by importing the respective headers.

.. note:: In the above example most of the code is "boiler-plate" doing
   error-handling when loading functions. This might seem a bit excessive,
   however, it is required for a language such as C, since there are no
   exceptions being raised upon error.

Try compiling the above example, e.g. on a Linux system with ``gcc`` do:

.. literalinclude:: 000_compile_example.cmd
   :language: bash

This will produce an executable named ``enumerate_example``. You can run it by
executing:

.. literalinclude:: 010_run_c_example.cmd
   :language: bash

The should produce output similar to:

.. literalinclude:: 010_run_c_example.out
   :language: bash

.. note:: ``pkg-config`` is used to locate where the shared-library. Make sure
   you have it and xNVMe installed on the system or just provide an absolute
   path to ``libxnvme-shared.so``.

.. note:: If you see no output, then try running it as super-user or via
   ``sudo``

In Python
=========

Here `ctypes` is used to load the library. Unlike C, then each function does
not need to be be explicitly loaded, additionally some assistance is given to
locate the library.

.. literalinclude:: enumerate_example.py
   :language: python

.. note:: In case `ctypes.util.find_library()/ctypes.CDLL()` cannot find/load
   the library then try providing an absolute path to the library instead. In
   case you have ``pkg-config`` installed then it can help you locate the
   library.

.. note:: With ``ctypes`` C structs are mapped to Python via
   ``ctypes.Structure`` classes. The order in which the fields are declared
   determines which member they are mapped to, not their name.

The example can be run like so:

.. literalinclude:: 020_run_python_example.cmd
   :language: bash

The command should produce output similar to:

.. literalinclude:: 020_run_python_example.out
   :language: bash

.. note:: If you see no output, then try running it as super-user or via
   ``sudo``

