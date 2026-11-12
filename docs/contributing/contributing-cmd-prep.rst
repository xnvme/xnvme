.. _sec-contributing-cmd-prep:

Producing the command preps
===========================

A generator, ``toolbox/xnvme_cmd_prep.py``, writes the functions that prepare
NVMe commands, ``xnvme_cmd_prep_<set>_<command>()``, and the records of their
arguments. The generator reads the data model that `senfd`_ makes of the
specification documents, as the generator of the spec headers does.
:ref:`sec-contributing-spec-headers` lists the prerequisites and describes how
to produce the data model. A contributor runs the generator and commits what
it writes. The build needs neither senfd nor the documents.

Generate the preps
------------------

From the root of the repository:

.. code-block:: bash

   ~/venvs/spec/bin/python toolbox/xnvme_cmd_prep.py /tmp/prep \
       --spec /tmp/senfd/merged.spec.document.json

The generator writes a header for each command set,
``/tmp/prep/xnvme_cmd_prep/libxnvme_cmd_prep_<set>.h``, with ``<set>`` one of
``adm``, ``io``, ``nvm``, ``zns`` and ``kv``, and one source file for all five
command sets, ``/tmp/prep/xnvme_cmd_prep/xnvme_cmd_prep.c``. Copy the headers
to ``include/`` and the source to ``lib/nvme/``, and format them with
``clang-format -i``, as pre-commit does. Add the record and the function of a
new command to the section of its header in ``lib/libxnvme.map``.

Each header defines a macro named after its command set, such as
``XNVME_CMD_PREP_NVM``. A program that also builds against a library without
the preps tests the macro with ``#ifdef``.

yace can also run the generator, with
``--generate toolbox/xnvme_cmd_prep.py:CmdPrep``, and parse the headers it
writes, as yace does for the spec headers.

The records only grow
---------------------

A caller sets ``args_nbytes``, the first member of every record, to the size
of the record it was compiled with. The function takes the members the caller
has and treats the rest as zero, so a program built against an older header
keeps working with a newer library. This compatibility holds only while each
record grows at its end.

No record has padding that the compiler adds. The generator lays out the
members as a C compiler does. Wherever the compiler would pad, the generator
writes a reserved member, named ``rsvd`` and its byte offset, such as
``rsvd15``. A reserved member comes before a member that needs alignment, and
at the end of the record, up to a multiple of 8 bytes. The function refuses
reserved bytes that are not zero with ``-EINVAL``. The source file asserts the
size of each record, so a record with padding does not compile.

The generator therefore reads the committed ``include/libxnvme_cmd_prep_*.h``
and keeps each record's members in their committed order and with their
committed types. A member that a new revision of a specification adds is
written after them. The new member takes the place of the reserved bytes at
the end of the record when it fits there, and the size of the record stays
the same. A program built against the newer header that sets the member is
then refused by an older library, because the older library holds those bytes
reserved. When the reserved bytes are used up, the record grows by 8 bytes
and ends in a new reserved member. In both cases the members that were there
keep their offsets. The generator does not keep the reserved members of the
committed headers. It derives them again from the layout, so do not name a
reserved member in a program.

A member or a command that is gone from the data model,
or a member whose type changed, stops the generator with a list of what would
break a caller. Such a change waits for a major version of the library and a
new soname: delete the committed headers, and the generator writes every
record in the order of the data model.

Review the preps
----------------

Each record cites the figure of each of its dwords, each member names the
field and the bits it is written to, and each function writes its fields as
``prep_bits(sqe, <dword>, <bit>, <width>, <value>)``, in the order of the
dwords. Review a change to the preps as a change to the specification: look
up the cited figure, and compare the function with the one committed before.

The commands whose dwords a feature or a directive operation describes, Set
Features, Get Features, Directive Send and Directive Receive, have a raw
member for each such dword, such as ``cdw11``. ``SPECIFIC`` in the generator
lists them.

A command can also leave a dword to its operations. Migration Send leaves
CDW11 to the operation that SEL selects. The data model lists the dwords of
each operation, and the record has a raw member for each such dword. The
record cites the figure of each operation for the dword. A dword that the
command and one of its operations both describe stops the generator.

The data model can give a command a dword from the figure of something else.
It gives the Controller Data Queue command the CDW11 to CDW13 of the
Controller Data Queue feature. ``FOREIGN`` in the generator lists such dwords,
and the generator leaves them out. The generator stops when the dwords of a
command come from different sections of one document. Look up the figures
then, and list the dwords that are not of the command in ``FOREIGN``. Where
the data model is wrong, fix senfd, not the generated files.

.. _senfd: https://github.com/safl/senfd
