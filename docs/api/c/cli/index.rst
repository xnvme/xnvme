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
CLI with ``xnvme_cli_run()``. The ``xnvme`` front-end handles its own commands
in ``cli_main()`` first, and leaves the rest to ``xnvme_cli_run()``.

.. note::
   Setting the ``XNVME_CLI_INIT_DEV_OPEN`` option will automatically open a device handle.

.. literalinclude:: ../../../../tools/xnvme.c
   :language: c
   :start-at: #include <libxnvme.h>
   :end-at: #include <libxnvme.h>

.. literalinclude:: ../../../../tools/xnvme.c
   :language: c
   :prepend: static int
   :start-at: sub_info(struct xnvme_cli *cli)
   :end-before: //

.. literalinclude:: ../../../../tools/xnvme.c
   :language: c
   :start-at: static struct xnvme_cli_sub g_subs[] = {
   :end-at: };

.. literalinclude:: ../../../../tools/xnvme.c
   :language: c
   :start-at: static struct xnvme_cli g_cli = {
   :end-at: };

.. literalinclude:: ../../../../tools/xnvme.c
   :language: c
   :prepend: int
   :start-at: main(int argc, char **argv)

.. toctree::
   :hidden:
   :glob:

   xnvme_*
