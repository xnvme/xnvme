(sec-contributing-process)=

# Contribution Process

* [Fork](https://github.com/xnvme/xnvme/fork) the **xNVMe** repository on
  {xref-github-xnvme}`GitHub<>`

* Make your changes, push them to your fork and create a
  {xref-github-xnvme-prs}`pull-request<>`

  * The pull-request should target the `main` branch
  * For feedback/RFC then set up the pull-request as a **Draft**

* The CI will trigger as you create the PR and re-trigger upon update

  * The default jobs run style/linters, build **xNVMe** on Linux, macOS and
    Windows, build the Python and Rust bindings, run the emulated functional
    verification, and generate the documentation
  * A full run takes about an hour; the wall-clock figure is higher when
    the self-hosted jobs queue behind another run
  * The jobs running on physical hardware and the benchmark jobs are gated on
    the `maas` label, see {ref}`sec-contributing-process-merging`
  * A maintainer can also run `verify`, `bench` or `docgen` manually via
    workflow-dispatch

* Go over the {ref}`sec-contributing-process-pr-checklist` and re-iterate on
  your pull-request / changes

(sec-contributing-process-merging)=

## Merging

`main` is protected, so a pull-request is merged only when all of the following
hold:

* Two approving reviews. Pushing to the branch dismisses existing approvals,
  and the approval must come after the last push

* Every review conversation is resolved

* The required checks pass. These are the two hardware jobs,
  `verify-physical` on macOS and `verify-maas` on Debian. Both are gated on the
  `maas` label, so a maintainer has to apply that label before the PR can
  become mergeable

* The history is linear. Rebase onto `main` rather than merging `main` into
  your branch

(sec-contributing-process-pr-checklist)=

## Pull-Request Checklist

Please check your pull-request for the following:

* The pull-request itself has a message describing the goal of the pull-request

* Commits are squashed such that each commit is an incremental step towards the
  goal

* All commits must have their commit messages formatted according to
  {ref}`sec-contributing-process-commit-messages`

* Commits are rebased on top of `main`

* New functionality is accompanied by tests verifying it

* The tests are passing

* All review feedback is addressed

(sec-contributing-process-commit-messages)=

## Commit Messages

Messages must follow the {xref-conventional-commits}`Conventional Commits<>`
specification. And in addition, for readability:

1. The first line is subject/title

   * Keep it at a max. of 72 chars, if possible
   * Lower-case is preferred
   * Do **not** end with a punctuation
   * Use the **imperative mood** e.g. `add` rather than `added`/`adds`
   * A couple of examples:

     * `feat(be/ramdisk): add support for compare`
     * `feat(build): bump libvfn to v4.0.1`
     * `refactor(docs): move toolchain section down after troubleshooting`

2. Then an empty line.

3. Then a description (may be omitted for truly trivial changes).

   * Should motivate the change and explain what and why, not how

4. Then an empty line again (if it has a description).

5. Then a `Signed-off-by` tag with your real name and email.

   * For example: `Signed-off-by: Foo Bar <foo.bar@example.com>`
   * Use `git commit -s ...` to add the sign-off automatically
   * By adding `Signed-off-by`, you indicate that you agree to the
     [DCO](https://developercertificate.org/)

Take a look at the commit history (`git log --follow <files>`), for the files
you are changing, that should give you an idea of the "component" as well as
the other items on the list above.
