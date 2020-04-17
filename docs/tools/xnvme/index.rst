.. _sec-tools-xnvme:

xnvme
#####

.. literalinclude:: xnvme_usage.out
   :language: bash

Enumerate devices
=================

Enumerate devices, that is, controllers and namespaces available to the system:

.. literalinclude:: xnvme_enum_usage.cmd
   :language: bash

.. literalinclude:: xnvme_enum_usage.out
   :language: bash

Device Information
==================

Device information:

.. literalinclude:: xnvme_info_usage.cmd
   :language: bash

.. literalinclude:: xnvme_info_usage.out
   :language: bash

Controller Identification
=========================

Controller identification:

.. literalinclude:: xnvme_idfy_ctrlr_usage.cmd
   :language: bash

.. literalinclude:: xnvme_idfy_ctrlr_usage.out
   :language: bash

Namespace Identification
========================

Namespace identification:

.. literalinclude:: xnvme_idfy_ns_usage.cmd
   :language: bash

.. literalinclude:: xnvme_idfy_ns_usage.out
   :language: bash

User-defined Identification
===========================

User-defined identification:

.. literalinclude:: xnvme_idfy_usage.cmd
   :language: bash

.. literalinclude:: xnvme_idfy_usage.out
   :language: bash

logs: Error-Information
=======================

Retrieving the error-information log:

.. literalinclude:: xnvme_log_erri_usage.cmd
   :language: bash

.. literalinclude:: xnvme_log_erri_usage.out
   :language: bash

logs: Health-Information
========================

Retrieving the S.M.A.R.T. / health information log:

.. literalinclude:: xnvme_log_health_usage.cmd
   :language: bash

.. literalinclude:: xnvme_log_health_usage.out
   :language: bash

logs: User-Defined Log
======================

Retrieve a log as defined by you:

.. literalinclude:: xnvme_log_usage.cmd
   :language: bash

.. literalinclude:: xnvme_log_usage.out
   :language: bash

Library Information
===================

Retrieve information about the library which ``xnvme`` is linked with:

.. literalinclude:: xnvme_library_info.cmd
   :language: bash

.. literalinclude:: xnvme_library_info.out
   :language: bash
