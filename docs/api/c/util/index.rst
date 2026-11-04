.. _sec-api-c-util:

Utilities
#########

Library Capabilities (:ref:`sec-api-c-xnvme_libconf`)
  This module allows introspective querying of the **xNVMe** library for its
  capabilities.

Library Version (:ref:`sec-api-c-xnvme_ver`)
  This module provides functionality for introspectively querying the **xNVMe**
  library for version information.

Sharing a Controller (:ref:`sec-api-c-xnvme_cplane`)
  This module lets one process serve a controller it has opened to other
  processes, handing them the descriptors they need to do I/O against the
  same device.

Auxiliary Pretty-Printers (:ref:`sec-api-c-xnvme_pp`)
  All structures and enums in the **xNVMe** API have pretty-printers. These
  are usually defined within the namespace/module of the data structure.
  The pretty-printers provided here are either for the top-level namespace
  (``xnvme_``) or for those that do not have a specific namespace.

Deprecated Functions (:ref:`sec-api-c-xnvme_compat`)
  Functions kept so code written for earlier releases builds unchanged; in a
  debug build, each warns when first called, and names the function replacing
  it.

Miscellaneous Utilities (:ref:`sec-api-c-xnvme_util`)
  This module includes utilities for measuring wall-clock time and
  miscellaneous helpers such as ``xnvme_is_pow2()`` and preprocessor macros
  like ``XNVME_DEBUG``.


.. toctree::
   :hidden:
   :glob:

   xnvme_*
