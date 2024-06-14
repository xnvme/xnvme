.. _sec-api-c-cli:

Command-line Interface (CLI)
############################

The :ref:`sec-api-c-xnvme_cli` module provides functionality for
creating command-line interfaces for **xNVMe** related applications. Most
**xNVMe** :xref-xnvme-repository-dir:`examples<examples>`,
:xref-xnvme-repository-dir:`tools<tools>`,
and :xref-xnvme-repository-dir:`tests<tests>` use this module to offer a
coherent command-line interface, complete with bash completion scripts and man
pages.

Usage Example
=============

Below is a brief example using excerpts from the source code of
the :xref-github-xnvme:`xnvme<blob/main/tools/xnvme.c>` CLI tool. Notice how the
sub-command (``sub_info``) is implemented as a function, added to the ``g_subs``
array, integrated into the CLI setup in ``g_cli``, and utilized when running the
CLI with ``xnvme_cli_run()``.

.. note::
   Setting the ``XNVME_CLI_INIT_DEV_OPEN`` option will automatically open a device handle.

.. literalinclude:: ../../../../tools/xnvme.c
   :language: c
   :lines: 6-7,106-121,1164-1168,1195-1206,1696-1696,1697-1710

.. toctree::
   :hidden:
   :glob:

   xnvme_*
