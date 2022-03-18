.. _sec-dynamic-loading:

========================================
 Dynamically loading the shared library
========================================

In this section, you will find an example of how to call the `xnvme_enumerate` function when dynamically loading the **xNVMe** shared library. The example serves to illustrate how one can go about doing this and some common pitfalls to watch out for. The example will first be presented in C and then in python.

To get an overview of what the **xNVMe** shared library contains refer to :ref:`sec-c-api`


Using the shared library in C
=============================
Here `dlopen` is used to load the library and pointers to the relevant functions are returned by `dlsym`.

Pitfalls:

Don't dismiss the error handling
  When `dlsym` can't find a symbol in the library, null is returned and the program will continue.
  Because of this, a missing symbol can be overlooked and cause the program to behave in unexpected ways or crash.
  This can lead to a lot of time wasted debugging. All of this can be avoided by immediately checking whether the symbol exists before moving on. 

`dlsym` will not find enumerators
  An enumerator are an *integer constant expression* and has no reference.
  Therefore, `dlsym` will not find it. Instead, the enumerators can be used as long the relevant headers are included.  

.. literalinclude:: enumerate_example.c
   :language: c


Running the example
-------------------
Before running the example make sure that the path given to `dlopen` is correct.

Next compile the example:

.. literalinclude:: compile_example.cmd
   :language: bash

Then run it:

.. literalinclude:: run_c_example.cmd
   :language: bash

The command should produce output similar to:

.. literalinclude:: run_c_example.out
   :language: bash

.. note:: If you see no output, you need to run the command as superuser.


Using the shared library in Python
==================================
Here `ctypes` is used to load the library.

Pitfalls:

`ctypes.CDLL` might not find the library returned by `ctypes.util.find_library`
  Due to a diffence in the paths searched by `ctypes.CDLL` and `ctypes.util.find_library`,
  `ctypes.CDLL` might fail to open the shared library.
  In this case simply hardcode the path to the library instead of using `ctypes.util.find_library`.
  I.e. `xnvme_shared_lib = "/usr/local/lib/libxnvme-shared.so"`

Order matters for structs
  The names given to the fields of the structures are completely arbitrary.
  Instead, it is the ordering of the fields that determine how they map to the structs in the shared library.

.. literalinclude:: enumerate_example.py
   :language: python


Running the example
-------------------
The example can be run like so:

.. literalinclude:: run_python_example.cmd
   :language: bash

The command should produce output similar to:

.. literalinclude:: run_python_example.out
   :language: bash

.. note:: If you see no output, you need to run the command as superuser.

