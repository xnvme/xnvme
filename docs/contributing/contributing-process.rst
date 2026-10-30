.. _sec-contributing-process:

Contribution Process
####################

A change goes from a fork to ``main`` through a pull-request, in these steps.
What CI runs at each is described in :ref:`sec-contributing-labels`.

#. `Fork`_ the **xNVMe** repository on `GitHUB`_, make your changes, and push
   them to your fork.

#. Open a `pull-request`_ against ``main`` as a **Draft**. A draft runs no
   CI, so iterate on it freely, and ask for early feedback there.

#. Go over the :ref:`sec-contributing-process-pr-checklist`, then mark the
   pull-request **Ready for review**. That starts its first CI run, and the
   ``labeler`` adds the ``gate:`` labels its changed files suggest. The first
   run already counts those gates, without waiting for the labels.

#. A maintainer reviews the pull-request, and with it the gates: they add a
   ``gate:`` label the change needs and the rules missed, and remove one it
   does not need, then re-run only the ``gates`` job of the latest run, so the
   change takes effect without a new run of everything.

#. Push fixes for the review as they come. Each push starts a new run and
   cancels the one in progress. The labels stay, so each run opens the same
   gates. A gate can be added at any point, as the change grows into code it
   did not touch at first; the rules do not add it then, so the maintainer
   does.

#. When ``ci-result`` passes, whose summary says which gates passed, and the
   review approves, a maintainer merges the pull-request by rebase.

A full run of the fixed jobs takes about 40 minutes. The documentation is
built first, without a guest and so without running its commands, and the
builds and the verification wait for it.

.. _sec-contributing-labels:

What CI runs: always, and behind a gate
=======================================

CI runs two kinds of jobs on a pull-request. The fixed set runs on every
run, and nobody can turn it off. The gated jobs run only when the
pull-request carries their ``gate:`` label, so a maintainer decides, change
by change, whether it needs lab machines, emulated RDMA or GPU guests, or the
benchmarks.

A draft runs no CI at all, and marking it ready for review starts its first
run. A newer run of a pull-request, whatever started it, cancels the one in
progress, so none runs superseded.

The fixed set
-------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Job
     - Runs
   * - ``source-format-check``
     - pre-commit over the whole tree, the tests of the cijoe scripts and the
       toolbox, and the version files of a bump
   * - ``source-archive``, ``build``
     - The source archives, and the builds on Linux, macOS and Windows
   * - ``ffi-python``, ``ffi-rust``
     - The Python and Rust bindings
   * - ``verify-makefile``, ``verify-packaging``
     - The Makefile's targets that generate files, and the RPM and DEB
       packages of a checkout and of the source archive
   * - ``docgen-shallow``
     - The documentation, without a guest and so without running its commands

A pull-request changing only ``docs/`` and top-level Markdown files skips the
builds, the bindings, ``verify-makefile`` and ``verify-packaging``; a gate
opened on it runs as on any other. A changed ``.cmd`` file counts as code,
since only the guest runs the commands.

The gates
---------

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Label
     - Runs
   * - ``gate:emulated``
     - The verification in QEMU guests with emulated NVMe, over PCIe and TCP
   * - ``gate:hardware``
     - The verification on the lab machines: a real NVMe drive passed
       through to a guest, and the macOS machine
   * - ``gate:rdma``
     - The verification over NVMe/RDMA, in a guest with an emulated RDMA NIC
   * - ``gate:gpu``
     - The upcie-hip and io_uring_dmabuf backends, in a guest with an emulated
       GPU
   * - ``gate:perf``
     - The benchmarks on the lab machines, which fail on a regression beyond
       their noise. They also run weekly
   * - ``gate:docgen``
     - The documentation with its commands run in a guest, to catch a change
       that breaks it. What is published is built after the merge, by a run
       on ``main`` that ``publish-docs`` starts
   * - ``gate:all``
     - Every gate above, as for a release

The files a pull-request changes suggest gates, by the rules in
``.github/labeler.yml`` on ``main``. The ``labeler`` workflow adds their
labels once, when the pull-request is opened ready for review or leaves draft,
and the run that starts then counts them before they are added. From then on
the labels alone decide: the rules do not add them again, on a push, a re-run
or a reopening, so a gate a maintainer removed stays closed. A rule only adds a
label; one set by hand stays. Most changes to code open ``gate:emulated``, and
changes to ``docs/`` open ``gate:docgen``, so what the fixed set leaves out is
suggested where it matters.

The rules are a starting point, and maintainers adjust them per pull-request.
Add a gate the rules missed, for a change whose effect reaches further than
its paths suggest. Remove one the rules opened needlessly, such as
``gate:hardware`` for a change to a comment in a header, which spends lab
time on nothing. Rules that keep missing or keep opening too much are fixed in
``.github/labeler.yml``.

Adding or removing a label starts no run. The ``gates`` job reads the labels
when a run starts, so to act on a label changed since, re-run only ``gates``
on the latest run: GitHub then re-runs the jobs that need it, the gated jobs
and ``ci-result``, and nothing else.

``ci-result`` counts every job that ran, gated or fixed, and requires no
gate. So a green ``ci-result`` covers the fixed set and the gates that were
open, and no more: a pull-request without ``gate:hardware`` has not been on a
drive, and which gates a change needs before it merges is the reviewers'
judgement.

``ci-result`` lists each gate and what its jobs gave, telling a gate that was
not opened from one that was opened but did not run, as when a job it needs
failed. Its notice says how many gates passed, as in ``2/6 gates passed:
emulated, hardware``. The ``gates-report`` workflow keeps that table as one
comment on the pull-request, updated after every run of its head, so it can
be read there rather than in the run.

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
