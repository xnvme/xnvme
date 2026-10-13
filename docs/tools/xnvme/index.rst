.. _sec-tools-xnvme:

xnvme
#####

``xnvme`` is the entry point to the **xNVMe** command line. Run it without
arguments for the commands to start with and the groups the rest are in:

.. literalinclude:: xnvme_overview.cmd
   :language: bash

.. literalinclude:: xnvme_overview.out
   :language: bash

Run a group to list its commands, and append ``--help`` to any command for its
arguments. Commands such as ``perf``, ``homi`` and ``qublk``
are separate programs, which ``xnvme`` looks up on its exec path,
the install directory unless ``XNVME_EXEC_PATH`` names others;
``xnvme --exec-path`` prints it.

The installed test programs are listed and run the same way:

.. literalinclude:: xnvme_test.cmd
   :language: bash

.. literalinclude:: xnvme_test.out
   :language: bash

Every built-in command at once:

.. literalinclude:: xnvme_usage.out
   :language: bash

Subcommands
===========

.. toctree::

   enum/index
   feature-get/index
   feature-set/index
   format/index
   idfy-ctrlr/index
   idfy-ns/index
   idfy/index
   info/index
   log-erri/index
   log-health/index
   log/index
   padc/index
   pioc/index
   sanitize/index
