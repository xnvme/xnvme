.. _sec-api-c-util:

Utilities
#########

LBA Helpers (:ref:`sec-api-c-xnvme_lba`)
  This module includes functions for constructing an `xnvme_lba_range`, which is
  a struct representing a range of logical block addresses (LBAs).

Library Capabilities (:ref:`sec-api-c-xnvme_libconf`)
  This module allows introspective querying of the **xNVMe** library for its
  capabilities.

Library Version (:ref:`sec-api-c-xnvme_ver`)
  This module provides functionality for introspectively querying the **xNVMe**
  library for version information.

Auxiliary Pretty-Printers (:ref:`sec-api-c-xnvme_pp`)
  All structures and enums in the **xNVMe** API have pretty-printers. These
  are usually defined within the namespace/module of the data structure.
  The pretty-printers provided here are either for the top-level namespace
  (``xnvme_``) or for those that do not have a specific namespace.

Miscellaneous Utilities (:ref:`sec-api-c-xnvme_util`)
  This module includes utilities for measuring wall-clock time and
  miscellaneous helpers such as ``xnvme_is_pow2()`` and preprocessor macros
  like ``XNVME_DEBUG``.


.. toctree::
   :hidden:
   :glob:

   xnvme_*
