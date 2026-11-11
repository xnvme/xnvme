.. _sec-api-c-nvme:

NVMe
####

It is recommended that you become familiar with the mechanics of
the :ref:`sec-api-c-core` API before diving into the **NVMe** specifics.
Regardless, here is a brief over of the **NVMe** portion of the **xNVMe** API.

Command Construction
====================

An NVMe command is a submission queue entry, ``union xnvme_spec_sqe``, of 64
bytes. With **xNVMe**, a command is built from a record of its fields, then
passed to the device with its payload.

Each command of the specifications has a record of its fields,
``struct xnvme_cmd_prep_<set>_<command>_args``, and a function preparing the
entry from it, ``xnvme_cmd_prep_<set>_<command>()``. Set ``args_nbytes`` to
the size of the record, and the fields the command needs. A field left out is
zero. Reading eight logical blocks from LBA 0 looks like this:

.. code-block:: c

   struct xnvme_cmd_ctx ctx;
   struct xnvme_cmd_prep_nvm_read_args args = {
           .args_nbytes = sizeof(args),
           .nsid = xnvme_dev_get_nsid(dev),
           .slba = 0,
           .nlb = 7,
   };
   int err;

   xnvme_dev_get_cmd_ctx(dev, &ctx);
   err = xnvme_cmd_prep_nvm_read(&ctx.cmd, &args);
   if (err) {
           return err;
   }
   err = xnvme_cmd_pass(&ctx, buf, 8 * lba_nbytes, NULL, 0);

The function clears the entry, then writes the opcode and each field at the
bits the specification gives it. It refuses with ``-EINVAL`` a value wider
than its field, and a field that the record's other fields rule out, such as
DSM of Read when CETYPE is set. It leaves the command identifier and the data
pointers to ``xnvme_cmd_pass()``, which sets them for the payload and submits
the command. Pass an admin command with ``xnvme_cmd_pass_admin()``, and a
vectored payload with ``xnvme_cmd_pass_iov()``.

The command context decides how the command completes. With a context from
``xnvme_dev_get_cmd_ctx()``, ``xnvme_cmd_pass()`` is synchronous, and the
completion is in ``ctx.cpl`` when ``xnvme_cmd_pass()`` returns. With a context
from ``xnvme_queue_get_cmd_ctx()``, ``xnvme_cmd_pass()`` is asynchronous; see
:ref:`sec-api-c-core`.

Records grow only at their end. A later revision of a specification can add
fields to a command, and those fields are added at the end of its record. A
program built against an older header sets a smaller ``args_nbytes``, and the
library takes the fields it lacks as zero. The program keeps running with a
newer library, without a rebuild.

A record has reserved members, named ``rsvd`` and a byte offset, where a
compiler would otherwise pad it. Leave them zero, as an initializer that
names the fields does. Do not name them: a later field can take their place.
The function refuses reserved bytes that are not zero with ``-EINVAL``.

Some commands leave a dword to an operation or a feature, as Migration Send
leaves CDW11 to the operation that SEL selects. The record has a raw member
for such a dword, such as ``cdw11``, and the record's documentation cites the
figure of each operation.

A field that no record has, such as a vendor-specific field, can be written
into ``ctx.cmd`` after the function and before the pass. A command that no
function prepares is written into ``ctx.cmd`` whole.

The records and functions are in a header for each command set:

**Admin** (:ref:`sec-api-c-xnvme_cmd_prep_adm`)
  The commands that manage a controller, such as Identify, Get Log Page, Get
  Features and Set Features, the commands that create and delete queues,
  Format NVM and Sanitize.

**I/O** (:ref:`sec-api-c-xnvme_cmd_prep_io`)
  The commands every I/O command set has: Flush, the reservation commands,
  Cancel, and I/O Management Send and Receive.

**NVM** (:ref:`sec-api-c-xnvme_cmd_prep_nvm`)
  Read and Write of logical blocks, and Compare, Write Zeroes, Dataset
  Management, Verify and Copy.

**Zoned Namespace** (:ref:`sec-api-c-xnvme_cmd_prep_zns`)
  Zone Append, Zone Management Send and Zone Management Receive. To work
  with zones, :ref:`sec-api-c-xnvme_znd` has helpers that report them and
  find them by state.

**Key Value** (:ref:`sec-api-c-xnvme_cmd_prep_kv`)
  Store, Retrieve, Delete, Exist and List.

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
