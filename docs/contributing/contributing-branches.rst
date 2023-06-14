.. _sec-contributing-branches:

Branches
========

``main`` Stable branch
   This is tagged with releases

   Always moves forward

   Changes go through integration and testing on ``next``
   before integrated on ``main``.

``next`` Staging / integration
   This is where GitHUB pull-requests should point to.

``rc`` Release candidate prepping
   This provides the "maintenance" changes when no more
   features are going in, this includes:
   
   version-bump
   man-pages
   bash-completions
   documentation-generation (API and Command output)
   CHANGELOG.rst
   CONTRIBUTORS.md

   Once these are done, and no regressions are found, then
   ``rc`` is integrated on ``next``
   ``next`` is integrated on ``main``
   ``main`` is tagged with the version-number (``vX.Y.Z``)

``docker`` Consumed by GitHUB Actions producing xNVMe docker images
