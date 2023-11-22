.. _sec-tutorials-fdp-intro:

=========================
 Flexible Data Placement
=========================

In this section, you will find a guide on **FDP** aka Flexible Data Placement
support in xNVMe.
This will guide you with examples of all the supported **FDP** log pages,
Set/Get features, and I/O management commands. This will also cover the Write
command with hints from the perspective of FIO's xNVMe ioengine.

Concepts and Prelude
--------------------

**FDP** adds an enhancement to NVM Command Set by enabling host guided data
placement. This introduces a Reclaim Unit (**RU**) which is a logical
representation of non-volatile storage within a Reclaim Group that is able to
be physically erased by the controller without disturbing any other Reclaim
Units.

Placement Identifier is a data structure that specifies a Reclaim Group
Identifier and a Placement Handle that references a Reclaim Unit.

Placement Handle is a namespace scoped handle that maps to an Endurance group
scoped Reclaim Unit Handle which references a Reclaim Unit in each Reclaim
Group.

Reclaim Unit Handle (**RUH**) is a controller resource that references a
Reclaim Unit in each Reclaim Group.

For the complete information on **FDP**, please see the ratified technical proposal
TP4146 Flexible Data Placement 2022.11.30 Ratified, which can be found here
https://nvmexpress.org/wp-content/uploads/NVM-Express-2.0-Ratified-TPs_20230111.zip

Get log page
------------

There are 4 new log pages associated with **FDP**. These are FDP Configuration,
Reclaim Unit Handle Usage, FDP Statistics and FDP Events. All these log pages
are Endurance Group scoped and hence you need to specify Endurance Group
Identifier in Log Specific Identifier field.

For the 4 log pages mentioned above, you can use **xNVMe** CLI:

The Fdp configuration log page requires dynamic memory allocation, as there
can be multiple configuration each having multiple Reclaim Unit Handles.
You will have to specify the data size in bytes. The command can be run like:

.. literalinclude:: 100_xnvme_log_fdp_config.cmd
   :language: bash

The command should produce output similar to:

.. literalinclude:: 100_xnvme_log_fdp_config.out
   :language: bash

For Fdp Statistics log page the command can be run like:

.. literalinclude:: 200_xnvme_log_fdp_stats.cmd
   :language: bash

The command should produce output similar to:

.. literalinclude:: 200_xnvme_log_fdp_stats.out
   :language: bash

Similar to the Fdp Configuration log page, you will have to specify the number
of Recalim Unit Handle Usage descriptors to fetch. The command can be run like:

.. literalinclude:: 300_xnvme_log_ruhu.cmd
   :language: bash

The command should produce output similar to:

.. literalinclude:: 300_xnvme_log_ruhu.out
   :language: bash

The Fdp Events log page will have multiple events. You will have to specify
the number of events you want to fetch. You also need to specify whether you
need host or controller events. This can be done by log specific parameter.
The complete command can be run like:

.. literalinclude:: 400_xnvme_log_fdp_events.cmd
   :language: bash

The command should produce output similar to:

.. literalinclude:: 400_xnvme_log_fdp_events.out
   :language: bash

Set and get-feature
-------------------

There are 2 new Set and Get Feature commands, introduced with **FDP**. These
are Flexible Data Placement which controls operation of FDP capability in the
specified Endurance Group, and FDP Events which controls if a controller
generates FDP Events associated with a specific Reclaim Unit Handle.

xNVMe does not support Namespace Management commands. Thus we cannot enable or
disable **FDP** by sending Set Feature command to the Endurance Group, as it
requires deletion of all namespaces in that Endurance Group. However you can
check the FDP capability by sending a Get Feature command. The command can be
run like:

.. literalinclude:: 010_xnvme_feature_get.cmd
   :language: bash

The command should produce output similar to:

.. literalinclude:: 010_xnvme_feature_get.out
   :language: bash

Command Dword 12 controls whether you want to enable or disable FDP Events.
You will have to specify number of events to enable or disable and Placement
Handle associated with it. These will be part of the Feat field. This will
require you to specify the size in bytes of the data buffer. To enable all the
Fdp Events you can run command like:

.. literalinclude:: 020_xnvme_set_fdp_events.cmd
   :language: bash

The command should produce output similar to:

.. literalinclude:: 020_xnvme_set_fdp_events.out
   :language: bash

You can get the status of supported FDP Events. The Command Dowrd 11 remains
the same as the Set Feature command. You can run the command like:

.. literalinclude:: 030_xnvme_feature_get.cmd
   :language: bash

The command should produce output similar to:

.. literalinclude:: 030_xnvme_feature_get.out
   :language: bash

I/O Management
--------------

Two I/O Management commands are introduced with FDP. These are I/O Management
Send and I/O Management Receive.

I/O management Receive supports Reclaim Unit Handle Status command. You will
have to specify the number of Recalim Unit Handle Status descriptors to fetch.
You can run the command like:

.. literalinclude:: 040_xnvme_fdp_ruhs.cmd
   :language: bash

The command should produce output similar to:

.. literalinclude:: 040_xnvme_fdp_ruhs.out
   :language: bash

I/O Management Send supports Reclaim Unit Handle Update command. You will have
to specify a Placement Identifier for this. You can run the command like:

.. literalinclude:: 050_xnvme_fdp_ruhu.cmd
   :language: bash

The command should produce output similar to:

.. literalinclude:: 050_xnvme_fdp_ruhu.out
   :language: bash

FIO xnvme ioengine
------------------

FIO's xNVMe ioengine provides FDP support since the 3.35 release.
This support is only there with nvme character device i.e. ``/dev/ng0n1``
and with userspace drivers such as ``spdk``.
Since the kernel support is limited to nvme character device, you can only use
the **FDP** functionality with ``xnvme_sync=nvme`` or
``xnvme_async=io_uring_cmd`` backends.

To enable the **FDP** mode, you will have to specify fio option ``fdp=1``.

Two additional optional **FDP** specific fio options can be specified.
These are:

``fdp_pli=x,y,..`` This can be used to specify index or comma separated
indicies of placement identifiers. The index or indicies refer to the
placement identifiers from reclaim unit handle status command. If you don't
specify this option, fio will use all the available placement identifiers
from reclaim unit handle status command.

``fdp_pli_select=str`` You can specify ``random`` or ``roundrobin`` as the
string literal. This tells fio which placement identifer to select next after
every write operation. If you don't specify this option, fio will round robin
over the available placement identifers.

Have a look at example configuration file at:
https://github.com/axboe/fio/blob/master/examples/xnvme-fdp.fio

This configuration tells fio to use placement identifers present at index 4
and 5 in reclaim unit handle usage command. By default we are using round
robin mechanism for selecting the next placement identifier.

Using the above mentioned configuration, you can run the fio command like
this:

.. literalinclude:: 001_xnvme_fdp_fio.cmd
   :language: bash

The command should produce output similar to:

.. literalinclude:: 001_xnvme_fdp_fio.out
   :language: bash

.. note:: If you see no output, then try running it as super-user or via
   ``sudo``
