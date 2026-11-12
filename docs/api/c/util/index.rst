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

Pretty-Printers (:ref:`sec-api-c-xnvme_pp`)
  The pretty-printers of the **xNVMe** API, of its structures, enums and
  library information, are declared here, apart from those of the
  command-line library and the inline timer printers, so that a binding can
  leave them out and print in its own language.

Miscellaneous Utilities (:ref:`sec-api-c-xnvme_util`)
  This module includes utilities for measuring wall-clock time and
  miscellaneous helpers such as ``xnvme_is_pow2()`` and preprocessor macros
  like ``XNVME_DEBUG``.


.. toctree::
   :hidden:
   :glob:

   xnvme_*
