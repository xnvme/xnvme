Build-from-source Emitter
=========================

This generates the dependency-installation scripts in ``toolbox/pkgs``. The
files emitted here are added to version-control, since they are utilized to
bootstrap docker-images, ci-system, manual-from-source-installation.

Install requirements for this script to run::

        python3 -m pip install --user requirements.txt

This assumes that you have the Python ``user-base`` added to your ``PATH``. Then do::

        python3 emit.py

Adjusting to changes:

* When the xNVMe toolchain, or libraries change, then update the ``deps.yaml``
  file

* In case, platform-quirks need handling, then adjust/add the template in ``templates/``

Once changes are done, then run the emitter as previously described.

The emitted ``apt`` and ``pkg`` scripts skip their system upgrade when
``XNVME_PKGS_NO_UPGRADE=1`` is set, see the toolchain documentation. Keep that
switch working when changing those templates, and keep the default, which
upgrades, since the docker-images and CI rely on it.

.. note: changes to the scripts do not take effect on the CI until they are
   merged on the ``main`` branch. This is because docker-images are pre-built
   based on the content of the scripts on ``main``.
