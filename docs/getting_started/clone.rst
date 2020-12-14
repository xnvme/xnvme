Retrieve the **xNVMe** repository from
`GitHUB <https://github.com/OpenMPDK/xNVMe>`_:

.. code-block:: bash

  # Clone the xNVMe repos
  git clone https://github.com/OpenMPDK/xNVMe.git --recursive xnvme
  cd xnvme

  # make: auto-configure xNVMe, build third-party libraries, and xNVMe itself
  make

  # make install: install via the apt package manager
  sudo make install-deb

  # make install: install in the "usual" manner
  sudo make install

.. note:: The repository uses git-submodules, so make sure you are cloning with
  ``--recursive``, if you overlooked that, then invoke ``git submodule update
  --init --recursive``
