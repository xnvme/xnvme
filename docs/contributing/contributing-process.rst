.. _sec-contributing-process:

Contribution Process
====================

* `Fork`_ the **xNVMe** repository on `GitHUB`_

* Make your changes, push them to your fork and create a `pull-request`_

  - The `pull-request` should target the ``next`` branch
  - For feedback/RFC then set up the `pull-request`_ as a **Draft**

* The CI will trigger as you create the PR and re-triggered upon update

  - The default CI-jobs consists of running style/linters and building xNVMe
  - When ready for integration it will be marked ``pr-ready-for-test`` and a
    larger suite of functional tests are triggered

* Go over the :ref:`sec-contributing-process-pr-checklist` and re-iterate on
  your pull-request / changes

.. _sec-contributing-process-pr-checklist:

Pull-Request Checklist
----------------------

Please check your pull-request for the following:

* The pull-request itself has a message describing the goal of the pull-request

* Commits are squashed such that each commit is an incremental step towards the
  goal

* All commits must have their commit messages formatted according to
  :ref:`sec-contributing-process-commit-messages`

* Commits are rebased on top of ``next``

* New functionality is accompanied by tests verifying it

* The tests are passing

* All review feedback is addressed

.. _sec-contributing-process-commit-messages:

Commit Messages
---------------

1. The first line is subject/title

   - Keep it at a max. of 72 chars, if possible
   - Lower-case is preferred
   - Do **not** end with a punctuation
   - Use the **imperative mood** e.g. ``add`` rather than ``added``/``adds``
   - Prefix with the component(s) e.g. ``component: ...``
    - If the commit touches the API (``libxnvme*.h``), then prefix with
      ``api:``
    - If the commit touches a backend, then prefix with specific backend e.g.
      ``be/linux: ...``
    - If the commit touches a "core" component, then prefix that, e.g.
      ``buf: ...``, ``ident: ...``
    - In case of multiple components, then separate with a comma e.g.
      ``ident,be/linux: ...```
   - **Example**: ``be/libaio: add spin-while-waiting for opts.poll_io``

2. Then an empty line.

3. Then a description (may be omitted for truly trivial changes).

   - Should motivate the change and explain what and why, not how

4. Then an empty line again (if it has a description).

5. Then a `Signed-off-by` tag with your real name and email.

   - For example: ``Signed-off-by: Foo Bar <foo.bar@example.com>``
   - Use ``git commit -s ...`` to add the sign-off automatically
   - By adding ``Signed-off-by``, you indicate that you agree to the `DCO`_

Take a look at the commit history (``git log --follow <files>``), for the files
you are changing, that should give you an idea of the "component" prefix(es) as
well as the other items on the list above.

.. _Discord: https://discord.com/invite/XCbBX9DmKf
.. _Fork: https://github.com/OpenMPDK/xNVMe/fork
.. _GitHUB: https://github.com/OpenMPDK/xNVMe/issues
.. _discussions: https://github.com/OpenMPDK/xNVMe/discussions
.. _git-hooks: https://git-scm.com/book/en/v2/Customizing-Git-Git-Hooks
.. _issues: https://github.com/OpenMPDK/xNVMe/issues
.. _meson: https://mesonbuild.com/
.. _pre-commit: https://pre-commit.com/
.. _pull-request: https://github.com/OpenMPDK/xNVMe/pulls
.. _DCO: https://developercertificate.org/
