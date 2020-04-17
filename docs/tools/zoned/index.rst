.. _sec-tools-zoned:

zoned
#####

.. literalinclude:: zoned_usage.out
   :language: none

Enumerate Zoned Devices on the system
=====================================

.. literalinclude:: zoned_enum_usage.cmd
   :language: bash

.. literalinclude:: zoned_enum_usage.out
   :language: bash

Enumerate the storage devices usable by the library and the ``zoned`` CLI on
the system:

.. literalinclude:: zoned_enum.cmd
   :language: bash

.. literalinclude:: zoned_enum.out
   :language: bash

Retrieve essential device information
=====================================

.. literalinclude:: zoned_info_usage.cmd
   :language: bash

.. literalinclude:: zoned_info_usage.out
   :language: bash

Retrieve device properties and attributes for a specific device identified by
e.g. ``/dev/nvme0n1``:

.. literalinclude:: zoned_info.cmd
   :language: bash

.. literalinclude:: zoned_info.out
   :language: bash

Retrieve Device Report
======================

.. literalinclude:: zoned_report_usage.cmd
   :language: bash

.. literalinclude:: zoned_report_usage.out
   :language: bash

Retrieve the entire device report
---------------------------------

Retrieve a complete report of zone information for a specific device identified
by e.g. ``/dev/nvme0n1``:

.. literalinclude:: zoned_report.cmd
   :language: bash

.. literalinclude:: zoned_report.out
   :language: bash

Retrieve a subset of the device report
--------------------------------------

.. literalinclude:: zoned_report_range.cmd
   :language: bash

.. literalinclude:: zoned_report_range.out
   :language: bash

Report Changes
==============

.. literalinclude:: zoned_changes_usage.cmd
   :language: bash

.. literalinclude:: zoned_changes_usage.out
   :language: bash

In relation to the above commands, then you can ask the device about which
zones has changed information since the last time you retrieve the report. You
do this using the ``changes`` sub-command:

.. literalinclude:: zoned_changes.uone.cmd
   :language: bash

.. literalinclude:: zoned_changes.uone.out
   :language: bash

Report Errors
=============

.. literalinclude:: zoned_errors_usage.cmd
   :language: bash

.. literalinclude:: zoned_errors_usage.out
   :language: bash

Get the errors logged on the given device:

.. literalinclude:: zoned_errors.cmd
   :language: bash

.. literalinclude:: zoned_errors.out
   :language: bash

Zone Management
===============

.. literalinclude:: zoned_mgmt_usage.cmd
   :language: bash

.. literalinclude:: zoned_mgmt_usage.out
   :language: bash

Explicitly controlling the Zone Management action:

.. literalinclude:: zoned_mgmt.cmd
   :language: bash

.. literalinclude:: zoned_mgmt.out
   :language: bash

Reading
=======

.. literalinclude:: zoned_read.cmd
   :language: bash

.. literalinclude:: zoned_read.out
   :language: bash

Writing
=======

.. literalinclude:: zoned_write_usage.cmd
   :language: bash

.. literalinclude:: zoned_write_usage.out
   :language: bash

Note, for a write to success it has to write at the write-pointer of an open or
empty zone. To use that, we reset the zone before writing.

.. literalinclude:: zoned_write.cmd
   :language: bash

.. literalinclude:: zoned_write.out
   :language: bash

Writing - with append
=====================

.. literalinclude:: zoned_append_usage.cmd
   :language: bash

.. literalinclude:: zoned_append_usage.out
   :language: bash

When writing with append then we no longer need to respect the write-pointer.
However, we still need to ensure that the Zone is not read-only, offline, or
full.

Write using `append`:

.. literalinclude:: zoned_append.cmd
   :language: bash

.. literalinclude:: zoned_append.out
   :language: bash
