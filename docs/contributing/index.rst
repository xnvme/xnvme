.. _sec-contributing:

==============
 Contributing
==============

**xNVMe** is hosted on `GitHUB`_ where you can raise `issues`_, start a
`discussions`_, or set up a `pull-request`_ for contributing your changes.
This page contains information related to the latter. Specifically, the
:ref:`sec-contributing-process` is of relevance to you and the
:ref:`sec-contributing-toolbox` available for development.

In addition to `GitHUB`_ then you can join `Discord`_, there you can ping for
feedback on your `pull-request`_ or ask questions.

.. _sec-contributing-process:

Contribution Process
====================

* `Fork`_ the **xNVMe** repository on `GitHUB`_

* Make your changes, push them to you fork and create a `pull-request`_

  - The `pull-request` should target the ``next`` branch
  - If you just want feedback/WIP then set up the `pull-request`_ as a Draft

* The CI will trigger in as you create the PR and re-triggered upon update

  - The default CI-jobs consists of running style/linters and building xNVMe
  - When ready for integration it will be marked ``pr-ready-for-test`` and a
    larger suite of functional tests are triggered

Commit Messages
---------------

1. The first line is subject/title

   - Keep it at a max. of 72 chars, if possible
   - Prefix with the compoent e.g. ``be:linux: fix error-handling``
   - Lower-case is preferred
   - Use the "imperative mood" e.g. ``fix`` rather ``fixes``/``fixed``
   - Do **not** end with a punctuation

2. Then an empty line.

3. Then a description (may be omitted for truly trivial changes).

   - Should explain what and why not how

4. Then an empty line again (if it has a description).

5. Then a `Signed-off-by` tag with your real name and email.

   - For example: ``Signed-off-by: Foo Bar <foo.bar@example.com>``
   - Use ``git commit -s ...`` to add the sign-off automatically

.. _sec-contributing-toolbox:

Toolbox
=======

Supporting the developtment of **xNVMe** is a toolbox with a collection of
scripts in Python, Bash and a ``Makefile`` to instrument it. See section
:ref:`sec-contributing-toolbox-mkhelper` for a description of it.

For common linter/format tooling then the `pre-commit`_ framework is used and
described in section :ref:`sec-contributing-toolbox-precommit`.

.. _sec-contributing-toolbox-mkhelper:

Makefile helper
---------------

As described in :ref:`sec-getting_started`, then **xNVMe** uses `Meson`_,
however, during development usage of the :ref:`sec-contributing-toolbox` is
invoked via ``make`` based on the ``Makefile`` in the root of the **xNVMe**
repository.

The ``Makefile`` contains a bunch of helper-targets. Invoking ``make help``
provides a brief description of what is available:

.. literalinclude:: make.out
   :language: bash

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
.. _Fork: https://github.com/OpenMPDK/xNVMe/fork
.. _GitHUB: https://github.com/OpenMPDK/xNVMe/issues
.. _discussions: https://github.com/OpenMPDK/xNVMe/discussions
.. _git-hooks: https://git-scm.com/book/en/v2/Customizing-Git-Git-Hooks
.. _issues: https://github.com/OpenMPDK/xNVMe/issues
.. _meson: https://mesonbuild.com/
.. _pre-commit: https://pre-commit.com/
.. _pull-request: https://github.com/OpenMPDK/xNVMe/pulls
