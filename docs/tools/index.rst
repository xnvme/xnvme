.. _sec-tools:

########################
 Command-Line Interface
########################

The command-line interface (CLI) consists of a low-level command `xnvme`,
providing access to most of what the `libxnvme` C API has to offer. And for
the use of `zoned` devices, the CLI `zoned` command is provided.

The purpose of these CLI are manifold:

 * Provide a convenient high-level management tool for Zoned Storage Devices
 * The CLI also serve as example code of using the :ref:`sec-c-apis`
 * The CLI itself provide tools for experimentation and debugging
 * A supplement to unit-testing the :ref:`sec-c-apis`

See the section `sec-cli-zoned` for description and usage examples of the
high-level utility.

And have a look at each sub section of the CLI is described by invoking the
tools with ``--help``.

.. toctree::
   :hidden:

   xnvme/index
   zoned/index
   lblk/index
