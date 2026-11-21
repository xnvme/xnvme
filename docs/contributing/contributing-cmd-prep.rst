.. _sec-contributing-cmd-prep:

Producing the command preps
===========================

A generator, ``toolbox/xnvme_cmd_prep.py``, writes the functions that prepare
NVMe commands, ``xnvme_cmd_prep_<set>_<command>_sized()``, the records of
their arguments, and a static inline function
``xnvme_cmd_prep_<set>_<command>()`` for each, which passes the size of the
record. The generator reads the data model that `senfd`_ makes of the
specification documents. :ref:`sec-contributing-spec-headers` lists the
prerequisites and describes how to produce the data model. A contributor runs
the generator and commits the files. The build needs neither senfd nor the
documents.

Generate the preps
------------------

From the root of the repository:

.. code-block:: bash

   ~/venvs/spec/bin/python toolbox/xnvme_cmd_prep.py /tmp/prep \
       --spec /tmp/senfd/merged.spec.document.json

The generator writes one header for each command set,
``/tmp/prep/xnvme_cmd_prep/libxnvme_cmd_prep_<set>.h``, with ``<set>`` one of
``adm``, ``io``, ``nvm``, ``zns`` and ``kv``, and one source file,
``/tmp/prep/xnvme_cmd_prep/xnvme_cmd_prep.c``. Copy the headers to
``include/`` and the source to ``lib/nvme/``, and format them with
``clang-format -i``, as pre-commit does. Add a new header to:

* the list of ``include/meson.build``
* ``libxnvme.h``
* ``INPUT`` in ``docs/tooling/doxy.cfg``
* the group ``nvme`` of ``NAMESPACES`` in
  ``docs/tooling/xnvme_docs/apigen.py``

Add a new source file to ``lib/nvme/meson.build``, and the ``_sized``
function of a new command to the section of its header in
``lib/libxnvme.map``; the static inline function is not exported.

Each header defines a macro named after its command set, such as
``XNVME_CMD_PREP_NVM``. A program that also builds against a library without
the preps tests the macro with ``#ifdef``.

yace can also run the generator and parse the headers:

.. code-block:: bash

   ~/venvs/spec/bin/yace \
       --generate toolbox/xnvme_cmd_prep.py:CmdPrep \
       --generator-option xnvme_cmd_prep.spec=/tmp/senfd/merged.spec.document.json \
       --output /tmp/prep

Run it without ``--emit``. The headers use ``union xnvme_spec_sqe`` of
``libxnvme_spec.h`` without including it, so the checks that
``--emit capi`` builds fail on the headers alone.

The records only grow
---------------------

The static inline function passes ``sizeof(*args)`` as ``args_nbytes``, the
size of the record that the caller was compiled with. The ``_sized`` function
reads the members that the caller has and takes the other members as zero, so
a program built against an older header works with a newer library. This
compatibility holds only while each record grows at its end. The ``_sized``
declaration documents ``args`` with yace's tag ``@recparam`` in place of
``@param``, so that each binding passes the size of the record as well. yace
takes the size from the parameter after the record, named ``args_nbytes``.
The size of every record is a multiple of 8 bytes, and the function returns
``-EINVAL`` for an ``args_nbytes`` that is zero or not a multiple of 8.

No record has padding that the compiler adds. Wherever a C compiler would pad,
the generator writes a reserved member, named ``rsvd`` and its byte offset,
such as ``rsvd21`` of the record of Read: before a member that needs
alignment, and at the end of the record, up to a multiple of 8 bytes. The
function returns ``-EINVAL`` for reserved bytes that are not zero. The source
file asserts the size of each record, so a record with padding does not
compile.

The generator reads the committed ``include/libxnvme_cmd_prep_*.h`` and writes
the members of each record in their committed order, with their committed
types. A member that a new revision of a specification adds follows them. The
new member replaces the reserved bytes at the end of the record when it fits
there, and the size of the record stays the same. An older library then
returns ``-EINVAL`` to a program that is built against the newer header and
sets the member, because the older library has those bytes as reserved. When
the reserved bytes are used up, the record grows by 8 bytes and ends in a new
reserved member. In both cases the earlier members keep their offsets. The
generator derives the reserved members again from the layout, so do not name a
reserved member in a program.

The generator stops, with a list of the changes, when a member or a command
is gone from the data model, or when the type of a member changed. Each
breaks a caller compiled against the committed headers. Such a change needs a
major version of the library and a new soname: delete the committed headers,
and the generator writes every record in the order of the data model.

Review the preps
----------------

Each record cites the figure of each of its dwords. Each member names its
field and the bits of the field. Each function writes its fields as
``prep_bits(sqe, <dword>, <bit>, <width>, <value>)``, in the order of the
dwords. Review a change to the preps as a change to the specification: look
up the cited figure, and compare the function with the committed function.

In some commands a field selects the use of other dwords, as FID selects the
use of CDW11 to CDW15 of Set Features. The data model lists these dwords as
delegated, and the record has a raw member, such as ``cdw11``, for each,
except for a dword that the command describes itself, as CDW14 of Set
Features.

In other commands an operation describes a dword, as the operation that SEL
selects describes CDW11 of Migration Send. The data model lists the dwords of
each operation. The record has a raw member for each such dword and cites the
figure of each operation. An operation can instead lay out the bits of one
field of the command, as the FDP Events of Get Log Page do for LSP. The data
model marks such dwords as refining that field, and the record has the field
of the command. The generator stops on any other dword that the command and
one of its operations both describe.

The generator stops on a field that would make a record that does not compile
or a function that drops a value: a member name that is no C identifier, is a
keyword of C or C++ or is a macro such as ``errno``, two members of one name,
a field wider than 64 bits, the parts of a value with a gap between them, a
part of a value under a condition, a field whose bits are not bits of its
dwords, two fields that share a bit unless their conditions exclude each
other, a field or a raw member in the bits of the opcode, of the command
identifier or, when the record has ``nsid``, of CDW1, a condition on a field
that is no member, a field in two descriptions of a dword whose conditions do
not cover every value, and two commands of a set that give one function name.

Where the data model is wrong, fix senfd, not the generated files.

.. _senfd: https://github.com/safl/senfd
