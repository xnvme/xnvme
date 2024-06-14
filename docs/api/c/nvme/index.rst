.. _sec-api-c-nvme:

NVMe
####

It is recommended that you become familiar with the mechanics of
the :ref:`sec-api-c-core` API before diving into the **NVMe** specifics.
Regardless, here is a brief over of the **NVMe** portion of the **xNVMe** API.

Command-Sets
============

The command-capabilities of NVMe devices are grouped in defitions of
command-sets, these are representated in **xNVMe** in the following modules:

**NVM** (:ref:`sec-api-c-xnvme_nvm`)
  This describes collections of NAND memory as a range of logical block
  addresses, with commands to read and write them. This is first set of commands
  provided with NVMe and supports the well-known use-case of block-storage.

**ZNS** (:ref:`sec-api-c-xnvme_znd`)
  This an extension / alternate block-storage model, where NAND memory is still
  organized as a range of logical blocks, but furthermore arranged in **zones**.
  To work with zones, then one needs to query device of their sizes, state, and
  reset it when needed. And adhere to I/O constraints. Helpers for all this is
  provided in the API.

**KVS** (:ref:`sec-api-c-xnvme_kvs`)
  This describes colletcions of NAND memory as Key-Value pairs, thus commands
  for **KVS** are **insert/retrieve** rather than **write/read**. Additional
  commands are needed to list and delete. These are provided here.

**ADMIN** (:ref:`sec-api-c-xnvme_adm`)
  Lastly, general admin commands are provided, these include **identify**
  commands, these provide data on controllers, namespaces, both in a general
  form and in command-set specific versions. Getting log-pages, getting/setting
  features are also admin commands. These and more are  providede in the admin
  portion of the API.

Specification Definitions
=========================

:ref:`sec-api-c-xnvme_spec` is a **C** representation of the NVMe specification.
This includes most of the structures described across the base, NVM, ZNS, and
KV specifications.

In line with this, :ref:`sec-api-c-xnvme_pi` and :ref:`sec-api-c-xnvme_topology`
provides functions for NVMe operations which are not
commands. :ref:`sec-api-c-xnvme_pi` has functions for generating and verifying
protection info. :ref:`sec-api-c-xnvme_topology` has functions for subsystem (or
controller) reset, namespace rescan, and getting controller registers.

.. toctree::
   :hidden:
   :glob:

   xnvme_*
