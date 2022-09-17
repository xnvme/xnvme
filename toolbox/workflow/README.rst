Reproduce GitHUB Actions locally
================================

The **cijoe** workflows and configurations in this directory are used in the
xNVMe GitHUB actions. You can reproduce what is running on GitHUB by adjusting
the config-files, and provide the artifacts from the GitHUB action:

 * xnvme-core.tar.gz
 * xnvme-cy-bindings.tar.gz
 * xnvme-cy-header.tar.gz
 * xnvme.tar.gz

To do so, then:

 * Place the artifacts in ``/tmp/artifacts``
 * Change ``qemu.system_bin`` to point to your qemu-system-binary (qemu 7+)
 * Add the SSH-key(``keys/guest_key``) to your SSH-agent.

Then you should be able to run the following::

  # Provision and test on Debian Bullseye
  cijoe -c configs/debian-bullseye.config -w provision.workflow
  cijoe -c configs/debian-bullseye.config -w test-debian-bullseye.workflow

  # Provision and test on FreeBSD 13
  cijoe -c configs/freebsd-13.config -w provision.workflow
  cijoe -c configs/freebsd-13.config -w test-freebsd-13.workflow

  # Generate documentation (provisions qemu-guest and generates the docs)
  cijoe -c configs/debian-bullseye.config -w docgen.workflow

In case you are setting up the test-target using other tools, or just want to
run pytest directly, then the following two sections describe how to do that.

Running pytest from the repository
----------------------------------

Invoke pytest providing a configuration file and an output directory for
artifacts and captured output::

  pytest \
    --config configs/debian-bullseye.config \
    --output /tmp/somewhere \
   ../cijoe-pkg-xnvme/src/cijoe/xnvme/tests

The ``--config`` is needed to inform pytest about the environment you are
running in such as which devices it can use for testing. The information is
utilized by pytest to, among other things, do parametrization for xNVMe backend
configurations etc.

Running pytest via package
--------------------------

Same as above, but install the ``cijoe-pkg-xnvme`` first, then do::

  pytest \
    --config configs/debian-bullseye.config \
    --output /tmp/somewhere \
    --pyargs cijoe.xnvme.tests

The ``--config`` is needed to inform pytest about the environment you are
running in such as which devices it can use for testing. The information is
utilized by pytest to, among other things, do parametrization for xNVMe backend
configurations etc.

Create boot-images
==================

The ``debian-bullseye-amd64.qcow2`` is created by::

  cijoe -c configs/debian-bullseye.config -w bootimg-debian-bullseye-amd64.workflow

The ``freebsd-13.1-ksrc-amd64.qcow2`` is created by::

  cijoe -c configs/freebsd-13.config -w bootimg-freebsd-13-amd64.workflow
