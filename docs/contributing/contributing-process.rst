.. _sec-contributing-process:

Contribution Process
####################

* `Fork`_ the **xNVMe** repository on `GitHUB`_

* Make your changes, push them to your fork and create a `pull-request`_

  - The `pull-request` should target the ``main`` branch
  - For feedback/RFC then set up the `pull-request`_ as a **Draft**

* The CI will trigger as you create the PR and re-triggered upon update

  - The default CI-jobs consists of running style/linters and building **xNVMe**
  - Average time for a full functional verification run is about 40min
  - The documentation is built early, without a guest and so without running
    the commands, and the builds and the verification wait for it
  - A pull-request changing only ``docs/`` and top-level Markdown files skips
    the builds and the verification. Merged, it runs only what the website
    needs, and publishes it. A changed ``.cmd`` file runs everything, since only
    the guest runs the commands
  - Merging requires the check ``ci-result``, which passes when every job did.
    A pull-request changing code also needs the verification on physical
    devices, which runs once the pull-request has the label ``maas``; until
    then ``ci-result`` fails
  - The benchmarks run weekly, on a manual run with the job ``bench``, and on
    a pull-request with the label ``bench``; they are not part of
    ``ci-result``, so they never block merging

* Go over the :ref:`sec-contributing-process-pr-checklist` and re-iterate on
  your pull-request / changes

.. _sec-contributing-process-pr-checklist:

Pull-Request Checklist
======================

Please check your pull-request for the following:

* The pull-request itself has a message describing the goal of the pull-request

* Commits are squashed such that each commit is an incremental step towards the
  goal

* All commits must have their commit messages formatted according to
  :ref:`sec-contributing-process-commit-messages`

* Neither commit messages nor source code refer to GitHub pull requests or
  issues, see :ref:`sec-contributing-process-references`

* Commits are rebased on top of ``main``

* New functionality is accompanied by tests verifying it

* The tests are passing

* All review feedback is addressed

.. _sec-contributing-process-commit-messages:

Commit Messages
===============

Messages must follow the :xref-conventional-commits:`Conventional Commits<>`
specification. And in addition, for readability:

1. The first line is subject/title

   - Keep it at a max. of 72 chars, if possible
   - Lower-case is preferred
   - Do **not** end with a punctuation
   - Use the **imperative mood** e.g. ``add`` rather than ``added``/``adds``
   - A couple of examples:

    - ``feat(be/ramdisk): add support for compare``
    - ``feat(build): bump libvfn to v4.0.1``
    - ``refactor(docs): move toolchain section down after troubleshooting``

2. Then an empty line.

3. Then a description (may be omitted for truly trivial changes).

   - Should motivate the change and explain what and why, not how

4. Then an empty line again (if it has a description).

5. Then a `Signed-off-by` tag with your real name and email.

   - For example: ``Signed-off-by: Foo Bar <foo.bar@example.com>``
   - Use ``git commit -s ...`` to add the sign-off automatically
   - By adding ``Signed-off-by``, you indicate that you agree to the `DCO`_

Take a look at the commit history (``git log --follow <files>``), for the files
you are changing, that should give you an idea of the "component" as well as the
other items on the list above.

.. _sec-contributing-process-references:

References to GitHub
====================

Commit messages and source code do not refer to pull requests, issues, or
anything else GitHub numbers or links. Git is the record, and GitHub is a
service on top of it. The history and the code outlive it: they are cloned,
mirrored, moved to another host, and read offline, where ``#811`` names nothing
or something else.

Say instead what the reference would have pointed at. Name the behaviour, the
function, or the commit, and name a commit by its abbreviated hash and its
subject. The same goes for closing keywords such as ``Fixes #123`` and for
links to GitHub.

Although not allowed in commit messages, such references are highly encouraged
in pull-request descriptions. A pull-request description lives on GitHub, so
link the issues and pull-requests it relates to, and put a closing keyword
there.

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
