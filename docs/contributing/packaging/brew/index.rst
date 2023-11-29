.. _sec-contributing-packaging-brew:

macOS (``brew``)
----------------

Notes are provided on updating the package / brew-formula for **xNVMe**. At the
bottom you will find notes on the initial creation of the formula. Do this:

.. code-block:: bash

    # Fork the **homebrew-core** repository

    git clone https://github.com/Homebrew/homebrew-core/

    # Add your fork to your local install
    brew tap homebrew/core
    cd $(brew --repository homebrew/core)
    git remote add <your_fork> <uri_of_your_clone>
    git fetch <your_fork>
    git checkout -b xnvme_update master

    # Edit the xNVMe formula
    brew edit xnvme

    # Test it
    brew uninstall --force xnvme
    HOMEBREW_NO_INSTALL_FROM_API=1 brew install --build-from-source xnvme
    # this was needed...
    brew link --overwrite
    brew test xnvme
    brew audit --strict xnvme
    brew style xnvme

Commit and push the changes:

.. code-block:: bash

  # Commit changes
  git add .
  git commit -s -m "xnvme major.minor.patch"

  # Push them
  git push your_fork xnvme_update

* Setup PR and address any issues

Initial creation of the package
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A couple of useful commands:

.. code-block:: bash

  # On macOS use this to calculate the checksum of the src-archive
  shasum -a 256

  # Test the formula
  HOMEBREW_NO_INSTALL_FROM_API=1 brew install --build-from-source xnvme.rb
  brew test xnvme.rb
  brew audit --strict xnvme

  # Only needed on creation?
  brew audit --new xnvme
  brew audit --new-formula xnvme
