.. _sec-api-c:

C
=

The **xNVMe** library is modular and namespaced. The **recommended** way of
consuming the library is by using ``#include <libxnvme.h>``. The content of
the :ref:`sec-api-c-header` is provided below for reference.

The :ref:`sec-api-c-header` is an **umbrella header**, meaning it includes
all public headers of the **xNVMe** project and the necessary third-party
definitions. If you have a valid reason for not using this header, you can
construct your own custom includes due to the modularity and consistent
namespacing conventions of the project. For details on creating custom includes,
refer to :ref:`sec-api-c-custom-include`.

.. _sec-api-c-header:

Header
------

.. literalinclude:: ../../../include/libxnvme.h
   :language: c

.. _sec-api-c-custom-include:

Custom Include
--------------

The **xNVMe** library header (``libxnvme.h``) serves as a single point of
entry, encompassing all necessary inclusions. However, you might be reading
this because you want to use only a subset of **xNVMe**, extract specific
definitions, or integrate the definitions/library in a particular way. Here are
a few important points to know about the **xNVMe** headers:

* The **public** headers **never** include other headers.

  - The exception to this rule is ``libxnvme.h`` itself.
  - All other headers (``libxnvme_*.h``) rely on the symbols they depend on to
    be defined elsewhere, for example, by the header that included them, such as
    ``libxnvme.h``.

  - Why this rule?
      It provides a single place to define scalar types for integers, floats, 	
      and booleans, as well as system headers and other third-party definitions.
      For instance, someone might have a project where they use part of the
      **xNVMe** headers for **NVMe** definitions but have their own variations
      of scalar types. By adhering to this rule, all of the ``libxnvme_*.h``
      headers can be recomposed and integrated in any desired manner.

* There are no include or C++ guards.

  - Why this rule?
      Including guards in each header file would be redundant since the common
      practice for **xNVMe** is to include ``libxnvme.h``, which already has
      an include and C++ guard, eliminating the need for those in every header.
      However, if you choose to selectively include **xNVMe** headers, you will
      need to set up your own guards.
 
.. toctree::
   :maxdepth: 2
   :hidden:

   core/index
   nvme/index
   file/index
   util/index
   cli/index
   examples/index