Manual Pages
============

The manual pages, man, of the examples, tests and tools are not committed. They
are generated from the ``--help`` of the programs by the script::

  toolbox/xnvmec_generator.py

The ``man-pages`` job of the ``verify`` workflow generates them on GitHub, and
the source archives of a release carry them, so installing from a source
archive installs them, and installing from a checkout installs none.

To generate them in a checkout, with the programs built in ``builddir`` and
``txt2man`` installed, run the mk-helper::

  make gen-man-pages

which writes them into the folders ``examples``, ``tests`` and ``tools`` here,
for the build to install.
