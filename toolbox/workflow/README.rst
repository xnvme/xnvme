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
  cijoe -fp -c ghr-debian-bullseye.config -w provision.workflow
  cijoe -fp -c ghr-debian-bullseye.config -w test.workflow

  # Provision and test on FreeBSD 13
  cijoe -fp -c ghr-freebsd-13.config -w provision.workflow
  cijoe -fp -c ghr-freebsd-13.config -w test.workflow

  # Generate documentation (provisions qemu-guest and generates the docs)
  cijoe -fp -c ghr-debian-bullseye.config -w docgen.workflow
