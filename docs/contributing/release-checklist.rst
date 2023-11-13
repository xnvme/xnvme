.. _sec-contributing-release:

Release Checklist
=================

With every release of **xNVMe**, the following tasks must be ticked off.
Initially, the window of features are closed, that is, all PRs intended for the
release are integrated on ``next`` and all tests are passing. Then:

* Create a branch named ``vX.Y.Z-rc1``

  - This must branch off from the ``next`` branch
  - Push this branch to you fork of **xNVMe** and setup a pull-request to
    ``next``

* Bump the version-number commit with message ``ver: bump to vX.Y.Z``

  - See the ``git log`` for previous commits bumping the version-number, to
    check that you bump everywhere needed.

* Update man-pages and Bash completion-scripts

  - Build and install the version-bumped **xNVMe**
  - Then run: ``make gen-man-pages gen-bash-completions``
  - Commit the man-pages with message: ``docs(man): update for vX.Y.Z``
  - Commit the Bash-completions-scripts with message: ``feat(toolbox/completions): update for vX.Y.Z``

* Update ``CONTRIBUTORS.rst``

  - Get the list of contributors with: ``git log main..HEAD~1 --pretty=format:"%an <%aE>" | sort | uniq``
  - Update the lists accordingly
  - Commit changes with the message: ``CONTRIBUTORS: update for vX.Y.Z``

* Update ``CHANGELOG.rst``

  - Go over the changes and summarized the different scopes
  - Commit changes with the message: ``CHANGELOG: update for vX.Y.Z``

* Integrate ``vX.Y.Z-rc1`` into ``next``

  - Get review of the PR
  - Wait for tests to finalize / pass
  - Merge onto ``next``

* Integrate ``next`` onto ``main``

  - Setup a PR of ``next`` onto ``main``
  - Review the PR
  - Wait for tests to finalize / pass
  - Double-check the generated docs at http://xnvme.io/docs/next

* Tag ``main`` as ``vX.Y.Z`` and push the tag

* Create a release on **GitHUB**

  - Goto the GitHUB page and create a release for the tag ``vX.Y.Z``
  - Use a title similar to the previous releases
  - Add the content similar to the previous releases, that is refer to the
    ``CHANGELOG.rst`` and click the "Generate release notes" button
  - Add the artifacts generated on the **tag** (src-archive,
    test-verify-results, Python sdist)

* Publish the Python package

  - We wish to publish the Python package that have been utilized for testing
    and provided as artifact for the release. Thus, download and rename it,
    then upload it.
  - Rename: ``xnvme-py-sdist.tar.gz xnvme-X.Y.Z.tar.gz``
  - Upload: ``twine upload xnvme-X.Y.Z.tar.gz``

* Publish the Rust crate

  - Upload: ``cd rust/xnvme-sys && cargo publish``
