.. _sec-contributing-release:

Release Checklist
=================

With every release of **xNVMe**, the following tasks must be ticked off.
Initially, the window of features are closed, that is, all PRs intended for the
release are integrated on ``main`` and all tests are passing. Then:

* Bump the version-number commit with message ``ver: bump to vX.Y.Z``

  - See the ``git log`` for previous commits bumping the version-number, to
    check that you bump everywhere needed.

* Update man-pages and Bash completion-scripts

  - Build and install the version-bumped **xNVMe**
  - Then run: ``make gen-man-pages gen-bash-completions``
  - Commit the man-pages with message: ``docs(man): update for vX.Y.Z``
  - Commit the Bash-completions-scripts with message: ``feat(toolbox/completions): update for vX.Y.Z``

* Update ``CONTRIBUTORS.rst``

  - Get the list of contributors with: ``git log <previous-tag>..HEAD --pretty=format:"%an <%aE>" | sort | uniq``
  - Update the lists accordingly
  - Commit changes with the message: ``CONTRIBUTORS: update for vX.Y.Z``

* Update ``CHANGELOG.rst``

  - Go over the changes and summarized the different scopes
  - Commit changes with the message: ``CHANGELOG: update for vX.Y.Z``

* Open a pull-request for the release-prep commits

  - The release-prep commits (version bump, man-pages, completions,
    CONTRIBUTORS, CHANGELOG) go through PR review like any other change
  - Get review and wait for tests to finalize / pass
  - Double-check the generated docs at https://xnvme.io/ (served from ``main``)
  - Merge onto ``main``

* Tag ``main`` as ``vX.Y.Z`` and push the tag

* Create a release on **GitHUB**

  - Goto the GitHUB page and create a release for the tag ``vX.Y.Z``
  - Use a title similar to the previous releases
  - Add the content similar to the previous releases, that is refer to the
    ``CHANGELOG.rst`` and click the "Generate release notes" button
  - Add the artifacts generated on the **tag** (src-archive,
    test-verify-results, Python sdist)
  - Make sure that the source-archives follow this convention:

   * ``xnvme-0.7.4.tar.gz`` - Source archive without subprojects
   * ``xnvme-fat-0.7.4.tar.gz`` - Full source archive including SPDK sources

* Publish the Python package

  - We wish to publish the Python package that have been utilized for testing
    and provided as artifact for the release. Thus, download and rename it,
    then upload it.
  - Rename: ``xnvme-py-sdist.tar.gz xnvme-X.Y.Z.tar.gz``
  - Upload: ``twine upload xnvme-X.Y.Z.tar.gz``

* Publish the Rust crate

  - Create an API token on https://crates.io
  - Set up credentials locally ``cargo login``
  - Upload: ``cd rust/xnvme-sys && cargo publish``
