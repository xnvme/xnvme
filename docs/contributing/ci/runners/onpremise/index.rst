.. _sec-ci-on-premise:

On Premise
==========

This setup is being decommissioned, as all jobs -- except one -- have been
migrated to MaaS. The remaining on-premise job is for verification on macOS with
a custom NVMe driver stack and a physical NVMe device.

As a result, setup instructions are no longer documented, since the future
direction is to deploy runners on MaaS. However, this section remains to
document what is still in place or might be reactivated in the future.

bifrost
-------

Taking care of the firewall and VPN tasks is a 
:xref-fw-netgate-1100:`Netgate 1100<>` running **pfSense+** with the following
setup: 

* Firewall rules: default to deny
* DHCP Server with mac-address based IP assignment
* WireGuard Enabled

See the :xref-fw-netgate-1100-manual:`Netgate 1100 Manual<>` for details on how
to configures this.

pixie
-----

The ``pixie`` box hosts the self-hosted runners, that is, one behalf of the
macOS host. It is box running Debian Bookworm with the following specs.:

.. list-table:: Hardware Specs. for ``pixie``
   :widths: 30 15 15 40
   :header-rows: 1

   * - CPU
     - Memory
     - Motherboard
     - NVMe Devices
   * - Intel Pentium Silver N6005 @ 2.00GHz
     - 16GB SODIMM Synchronous 2667 MT/s
     - HARDKERNEL ODROID-H3
     - - 1x MEMPEK1W016GA

verify-macos
------------

.. list-table:: Hardware Specs. for ``verify-macos``
   :widths: 30 30 40
   :header-rows: 1

   * - CPU
     - Memory
     - NVMe Devices
   * - Apple M1
     - 16 GB
     - - 1x Intel Optane M10 16GB