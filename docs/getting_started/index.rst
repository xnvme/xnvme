.. _sec-gs:

=================
 Getting Started
=================

Here you will be taken through the process of :ref:`sec-gs-build` **xNVMe**,
which includes retrieving the **xNVMe** source, installing a **C** compiler /
toolchain, libraries, and then building and installing **xNVMe** itself.
With **xNVMe** in place on your system, an :ref:`sec-gs-example` is provided
for you to :ref:`sec-gs-run`.

.. _sec-gs-build:

Building and installing
=======================

Below are examples of building and installing **xNVMe** on different operating
systems. Additional examples and detailed information can be found in
the :ref:`sec-toolchain` section, covering numerous :ref:`sec-toolchain-linux`
distributions, as well as :ref:`sec-toolchain-freebsd`, :ref:`sec-toolchain-macos`,
and :ref:`sec-toolchain-windows` versions.

.. tabs::

    .. tab:: Linux

        .. literalinclude:: build_xnvme_linux.cmd
           :language: bash

    .. tab:: FreeBSD

        .. literalinclude:: build_xnvme_freebsd.cmd
           :language: bash

    .. tab:: macOS

        .. literalinclude:: build_xnvme_macos.cmd
           :language: bash

    .. tab:: Windows

        .. literalinclude:: build_xnvme_windows.cmd
           :language: powershell

        .. note:: Run this in an elevated Powershell session!

Building system software can be challenging. If you encounter errors when
following the steps above, and if :xref-meson:`meson<>` is new to you,
a couple of pointers are provided to help you out. You can also look at
the :ref:`sec-gs-troubleshooting` section for known issues.

Build-errors
  Details on the build errors can be seen by inspecting the log file at
  ``builddir/meson-logs/meson-log.txt``.

Rebuilding
  In case rebuilding fails, e.g., due to errors during ``meson setup`` or
  missing toolchain/dependencies, remove the ``builddir`` folder. If errors
  persist, try also removing the **SPDK** subproject at ``subprojects/spdk``.

Customizing-the-build
  If you want to customize the build, e.g., install into a different location,
  this is handled by :xref-meson-options-builtin:`meson built-in options<>`.
  In addition, you can inspect ``meson_options.txt`` which contains options
  specific to **xNVMe**.

.. _sec-gs-example:

Example Program
===============

This example code illustrates how to use the **xNVMe** library (``libxnvme``)
in **C** to perform the following tasks with an NVMe device: open the device,
probe and retrieve device information, print device information, and finally
close the device.

.. literalinclude:: ../../examples/xnvme_hello.c
   :language: c
   :lines: 5-

Additional examples of using the **xNVMe** C API can be found in the **xNVMe**
repository. This includes explicit :xref-xnvme-repository-dir:`example<examples>`
code, the source for the command-line :xref-xnvme-repository-dir:`tools<tools>`,
and :xref-xnvme-repository-dir:`tests<tests>`.
For examples of efficient and high-performance integration, refer to
the :xref-fio-repository-file:`fio xNVMe I/O engine<engines/xnvme.c>` and
the :xref-spdk-repository-file:`SPDK xNVMe bdev<module/bdev/xnvme>` module.

Furthermore, while the example given above is written in **C**, bindings
for the **xNVMe** C API are also available for other languages. These
include :ref:`sec-api-python` ( :xref-xnvme-python:`Pypi<>` ) bindings
and :ref:`sec-api-rust` ( :xref-xnvme-rust-xnvme-sys:`crates.io<>` ) bindings.

.. _sec-gs-run:

Compile, link, and run!
=======================

**xNVMe** provides a :xref-pkg-config-site:`pkg-config<>` file (``xnvme.pc``),
it is populated to fit the version of **xNVMe** installed on the system. Use it
to ensure you get the correct flags. For example, like so:

.. tabs::

    .. tab:: Linux

        .. literalinclude:: build_example_linux.cmd
           :language: bash

        When doing so, it will print output equivalent to the following.

        .. literalinclude:: build_example_linux.out
           :language: yaml

    .. tab:: Linux (SPDK/NVMe)

        .. literalinclude:: build_example_linux_spdk.cmd
           :language: bash

        When doing so, it will print output equivalent to the following.

        .. literalinclude:: build_example_linux_spdk.out
           :language: yaml

    .. tab:: FreeBSD

        .. literalinclude:: build_example_freebsd.cmd
           :language: bash

        When doing so, it will print output equivalent to the following.

        .. literalinclude:: build_example_freebsd.out
           :language: yaml

    .. tab:: macOS

        .. literalinclude:: build_example_macos.cmd
           :language: bash

        When doing so, it will print output equivalent to the following.

        .. literalinclude:: build_example_macos.out
           :language: yaml

    .. tab:: Windows

        .. literalinclude:: build_example_windows.cmd
           :language: powershell

        When doing so, it will print output equivalent to the following.

        .. literalinclude:: build_example_windows.out
           :language: yaml

        .. note:: Run this in an elevated Powershell session!

As the compilation and execution of the :ref:`sec-gs-example` demonstrate,
platform and interface-specific concerns exist. However, the **xNVMe** API
and library encapsulate these differences, enabling the same example code to
function uniformly across all platforms. This provides a unified platform and
system interface abstraction.

The library encapsulation in **xNVMe** is referred to as :ref:`sec-backends`.
In the example above, only a fraction of the supported system interfaces are
shown. The section on :ref:`sec-backends` provide additional documentation on
this, as well as describing other runtime concerns such as system setup and
configuration.

With the library and toolchain in place, you might want to explore
the :ref:`sec-tools` and :ref:`sec-api`. For this, you can use your own storage
devices. However, you can also look at the :ref:`sec-tutorials` section,
specifically the :ref:`sec-tutorials-devs-linux` section, which will enable you
to develop and test using emulated storage devices with support for features
such as such as Zoned Namespaces, Key-Value, and Flexible-Data-Placement.

In case things aren't working for you then have a look at the following section
on :ref:`sec-gs-troubleshooting`.

.. _sec-gs-troubleshooting:

Troubleshooting
===============

This section is here to give you pointers to troubleshoot whether **xNVMe** or
your system is misbehaving. First off ensure that you are you have the setup
your system, please refer to the :ref:`sec-toolchain` section for details
on installing compilers, libraries etc. and the :ref:`sec-backends` section
on notes on system configuration and runtime instrumentation of the library
backends.

Should you not find the answers you are looking here, then please
reach out by raising an :xref-github-xnvme-issues:`issue<>` or go
to :xref-discord-xnvme:`Discord<>` for synchronous interaction.

Build Errors
------------

**xNVMe** adheres to best practices and conventions for system software. It
links with other libraries available on the system and downgrades functionality
to disable dependent modules if a library is unavailable. However, **xNVMe**
diverges in one area: the subproject build of **SPDK**. In this case, **xNVMe**
retrieves the **SPDK** repository, patches the source, and builds it for linkage
with **xNVMe**. Consequently, most reported issues are related to this process
and are addressed accordingly.

Shallow source-archive
~~~~~~~~~~~~~~~~~~~~~~

If you are getting errors while attempting to configure and build **xNVMe**
then it is likely due to one of the following:

* You are building in an **offline** environment and only have a shallow
  source-archive or a git-repository without subprojects.

The full source-archive is made available with each release and downloadable
from the `GitHUB Release page <https://github.com/xnvme/xnvme/releases>`_
release page. It contains the **xNVMe** source code along with all the
third-party subproject source of **SPDK**.

Missing dependencies / toolchain
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* You are missing dependencies, see the :ref:`sec-toolchain` for
  installing these on FreeBSD and a handful of different Linux Distributions

Once you have the full source of xNVMe, third-party library dependencies, and
setup the toolchain then run the following to ensure that the xNVMe repository
is clean from any artifacts left behind by previous build failures::

  make clobber

And then go back to the :ref:`sec-gs-build` and follow the steps there.

.. note::
   When running ``make clobber`` then everything not comitted is "lost". Thus,
   if you are developing/modifying **xNVMe**, then make you commit of stash your
   changes before running it.
