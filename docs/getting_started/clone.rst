.. code-block:: bash

  # Clone the xNVMe repos into the folder 'xnvme'
  git clone https://github.com/OpenMPDK/xNVMe.git xnvme

.. note:: The xNVMe build-system uses ``meson/ninja`` and its
   subproject-feature with wraps, dependencies such as fio, libvfn, and SPDK
   are fetched by the build-system (meson), not as previously done via
   git-submodules.
