Manual Pages
============

This folder contains manual pages, man, for examples, tests, and tools. These
are generated automatically by the script::

  toolbox/xnvmec_generator.py

As part of the release process or adhoc via the mk-helper::

  make gen-man-pages

And the odd-one-out::

  txt2man -t xnvme-driver -v xNVMe -s 1 -r xNVMe xnvme-driver.txt > tools/xnvme-driver.1
