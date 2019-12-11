Changelog
=========

The repository is tagged using semantic versioning, e.g. ``v1.2.3``.

Each upcoming release is available, as is, with its history subject to change,
on a branch named ``rX.Y.Z``. Once released, the history of ``rX.Y.Z`` is fixed
and tagged ``vX.Y.Z`` and pushed to the master branch.

The history from one patch-version to another is likely to be rebased. E.g.
squashed into the history of the previous until the next minor release is made.

Changes are described in this file in a section named matching the version tag.

The "Backlog" section describe changes on the roadmap for xNVMe.

Known Issues
------------

* ``be::liou`` does not build on Alpine Linux

* ``be::spdk`` does not build on Alpine Linux

* ``be::lioc`` does not support ``XNVME_CMD_ASYNC``

v0.0.16
-------

* Initial public release of xNVMe

Backlog
-------

* Release support for Namespace Types (Pending spec release)

* Release support for Zoned Namespaces (Pending spec release)

* Release User-space NVMe Meta-filesystem

* Release the libaio backend implementation ``be:laio``

* docs

  - Expand documentation on Fabrics setup
  - Expand library usage examples

* For the Linux Kernel backends ``be:{lioc,liou}`` replace device enumeration
  with the encapsulations provided by``libnvme``

* build

  - Add version/origin/build introspection
  - Provide convenient means of probing linkage information
  - Provide convenient means of git ref-tags when available
  - Provide convenient means of probing source origin e.g. repos, tarball etc.

* scc

  - Update implementation
