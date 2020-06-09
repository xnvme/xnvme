.. _sec-tutorials-nvme-intro-cmd-sets:

NVMe Command Sets
=================

The NVMe specification has since its first version defined the **Admin Command
Set** providing essential administrative and management commands including:
identify and get-log-page.
For I/O, the NVMe specification has since its inception defined the **NVM
Command Set**, which includes the most essential I/O commands: read and write.

The commands included in the two sets has grown since then, some commands being
mandatory others optional.

With emerging technologies such as Zoned Namespaces, Key-Value SSDs, and
computational storage then the set of commands continue to grow, to encapsulate
the growth the NVMe specification now include a formal specification on how a
NVMe device can communicate support for different I/O command sets.

This section contains notes specific to I/O Command Sets, how to probe a device
for support and references to details on the specific command sets.

.. note:: Please see the NVMe specification for ground thruth, that is, the
  exact definitions and descriptions and use the information presented here as
  an informal introduction.

Concepts and Prelude
--------------------

This expands on the concepts of NVMe Controllers, Controller-registers,
Namespaces, and Commands.

I/O Command Set Identifier (**csi**), as the name suggests, then **csi**
identifies an I/O Command Set. Note, that in the context of NVMe-MI, it is
referred to as **iocsi**.

I/O Command Set Identifiers, are defined in their respective specifications,
e.g. the Zoned Namespace I/O Command Set specification declares the **csi**
value for Zoned I/O Command Set.

I/O Command Set Profile

I/O Command Set Combination Index (iocsci)

I/O Command Set Combination

Abbreviations
~~~~~~~~~~~~~

+--------------+--------------------------------------------------------------+
| Abbreviation | Description                                                  |
+==============+==============================================================+
| csi          | Command Set Identifier                                       |
+--------------+--------------------------------------------------------------+
| iocsci       | I/O Command Set Combination Index                            |
+--------------+--------------------------------------------------------------+
| iocsi        | I/O Command Set Identifier                                   |
+--------------+--------------------------------------------------------------+

Controller Command Set Support
------------------------------

I/O Command-set support is governed by the controller capabilities and its
configuration. That is, which capability is enabled in the configuration.

Specifically, the read-only field **CAP.CSS**, defines what is supported, and
the read-write field **CC.CSS** defines what is enabled.

Capability
~~~~~~~~~~

The **CAP** register, controller capabilities, has a 8bit-wide field (bits
44-37) named **CSS** indicating I/O Command Set(s) that the controller
supports

**cap.css**:

+-----------+-----------------------------------------------------------------+
| CAP-bits  | Definition                                                      |
+===========+=================================================================+
| 37        | Controller supports the NVM Command Set.                        |
|           |                                                                 |
|           | Controllers that support the NVM Command Set **shall** set      |
|           | this bit even if bit 43 is set to ‘1’.                          |
+-----------+-----------------------------------------------------------------+
| 42-38     | Reserved                                                        |
+-----------+-----------------------------------------------------------------+
| 43        | Controller supports one or more I/O Command Sets and supports   |
|           | the Identify I/O Command Set data structure (ref. ?).           |
|           |                                                                 |
|           | Controllers that support I/O Command Sets other than the        |
|           | NVM Command Set **shall** set bit 43 to ‘1’.                    |
|           |                                                                 |
|           | Controllers that only support the NVM Command Set **may** set   |
|           | this bit to ‘1’ to indicate support for the                     |
|           | Command Set Identifier field in commands that use the           |
|           | Command Set Identifier field.                                   |
+-----------+-----------------------------------------------------------------+

From the spec you can see that only the NVM Command Set has a bit dedicated to
indicate support. For any other I/O Command Set, supported is communicated in
**Identify I/O Command Set data structure**.

So, what can you do with the information in the controller registers? Find a
couple of examples below.

Determine whether the controller supports the NVM I/O Command Set:

* Check that **CAP-bit** 37 is set

Determine whether a controller supports any other I/O Command Set:

* Check whether **CAP-bit** 43 is set
* Retrieve the **Identify I/O Command Set data structure**

How do one check if a controller support I/O Command Set **xyz**?

Configuration
~~~~~~~~~~~~~

The **CC** register, controller configuration, has a 3bit-wide field (bits 6-4)
named **CSS** which shall be set prior to enabling the controller.

What can you enable here? Here are a couple of examples:

* Enable only the Admin Command Set
* Enable the Admin Command Set and the NVM Command Set
* Enable the Admin Command Set, the NVM Command Set, and any other I/O Command Set

Identify: I/O Command Set data structure
----------------------------------------

The controller registers **CAP.CSS** and **CC.CSS** are insufficient beyond the
NVM and Admin Command sets. For any I/O Command Set, other than NVM, then the
NVMe commands: identify, get-feature and set-feature, shall be used, in
addition to the controller registers, in order to determine capability and
configuring I/O Command Sets.

Example, sending this command using the **xNVMe** CLI:

.. literalinclude:: 100_xnvme_idfy_cs.cmd
   :language: bash

Should yield output on the form:

.. literalinclude:: 100_xnvme_idfy_cs.out
   :language: bash

Identify: related structures
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

There are an additional five **IOCS** related ``CNS`` values.

Identify Namespace (I/O Command Set specific)
cns, nsid, csi

Identify Controller (I/O Command Set specific)
cns, csi

Feature: I/O Command Set Profile
--------------------------------

The **I/O Command Set Profile** feature, identified by TBD, provides the I/O
Command Set Combination Index (``iocsci``) of the currently selected I/O
Command Set Combination. To change it the currently selected I/O Command Set
Combination, then use set-feature using a valid ``iocsci``.

Namespace Command Set Association
---------------------------------

A namespace is **associated** with exactly one I/O command set. So, how do you
determine which one it is?

FAQ
---

Q: Where are **Command Set Identifiers** defined?
A: TODO: I guess they should be defined in their respective I/O Command
Specifications? Looks like they are not defined in the base-spec.
