.. _sec-contributing-toolbox:

Toolbox
=======

Supporting the development of **xNVMe** is a toolbox with a collection of
scripts in Python, Bash and a ``Makefile`` to instrument it. See section
:ref:`sec-contributing-toolbox-mkhelper` for a description of it.

For common linter/format tooling then the `pre-commit`_ framework is used and
described in section :ref:`sec-contributing-toolbox-precommit`.

.. _sec-contributing-toolbox-mkhelper:

Makefile helper
---------------

As described in :ref:`sec-gs`,  **xNVMe** uses the `Meson`_
build-system, however, during development usage of the
:ref:`sec-contributing-toolbox` is invoked via ``make`` based on the
``Makefile`` in the root of the **xNVMe** repository.

The ``Makefile`` contains a bunch of helper-targets. Invoking ``make help``
provides a brief description of what is available:

.. literalinclude:: make.out
   :language: bash

Local Testing
-------------

To reproduce the CI test suite locally using Docker, see
:ref:`sec-tutorials-devs-linux-reproduce-ci`.

.. _sec-contributing-toolbox-precommit:

Pre-commit / Git-hooks
----------------------

xNVMe utilizes the `pre-commit`_ framework for wrapping various code-format and
linters tools. You can enable it to run via `git-hooks`_ or invoke it manually
via the :ref:`sec-contributing-toolbox-mkhelper` e.g.::

  # format all files
  make format-all

  # format only files with staged changes
  make format

.. _Discord: https://discord.com/invite/XCbBX9DmKf
.. _Fork: https://github.com/xnvme/xnvme/fork
.. _GitHUB: https://github.com/xnvme/xnvme/issues
.. _discussions: https://github.com/xnvme/xnvme/discussions
.. _git-hooks: https://git-scm.com/book/en/v2/Customizing-Git-Git-Hooks
.. _issues: https://github.com/xnvme/xnvme/issues
.. _meson: https://mesonbuild.com/
.. _pre-commit: https://pre-commit.com/
.. _pull-request: https://github.com/xnvme/xnvme/pulls
.. _DCO: https://developercertificate.org/
