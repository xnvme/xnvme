.. _sec-contributing-spec-headers:

Producing the spec headers
==========================

The headers that define the commands, features, data structures, log pages,
registers, identifiers and status codes of the NVMe specifications are written
by a generator rather than by hand. `senfd`_ reads the specification documents
into a data model. `yace`_ runs the generator,
``toolbox/xnvme_spec_headers.py``, and the generator writes the headers from
that data model. yace then parses the generated headers as it parses any
header, checks that they use only the C subset that yace accepts, and emits
their pretty-printers and bindings.

The build runs neither senfd, the generator nor yace. A contributor runs them
on a machine that has the specification documents, and commits the generated
headers. Building **xNVMe** needs neither senfd, yace nor the documents.

NVM Express publishes the specification documents, and **xNVMe** does not
redistribute them. Never commit a document, a conversion of one, or the data
model senfd writes from them. The headers carry no text of the documents
beyond the names, acronyms and figure numbers they cite.

Prerequisites
-------------

* The ratified specification documents, as ``.pdf``, from `NVM Express`_.
  Keep them outside the repository, here in ``~/specs``.
* senfd and yace, in an environment of their own:

  .. code-block:: bash

     python3 -m venv ~/venvs/spec
     ~/venvs/spec/bin/pip install senfd yace

  The generator needs senfd 0.19.0 or later and yace 0.9.0 or later. yace
  0.9.0 is the first release that runs a generator from its file.
  ``senfd --version`` and ``yace --version`` say what you have.

* A C compiler, which yace uses to check what it parses.

Produce the data model
----------------------

senfd reads each document into a data model, and merges the data models into
one:

.. code-block:: bash

   ~/venvs/spec/bin/senfd ~/specs/*.pdf --output /tmp/senfd

This writes ``/tmp/senfd/merged.spec.document.json``, which is what the
generator reads. The documents are not uniform, and what senfd finds wrong in
them is listed as candidate errata, each with the figure and page it is on:

.. code-block:: bash

   ~/venvs/spec/bin/senfd lint --spec /tmp/senfd/merged.spec.document.json

A field with an erratum is still written as the figure gives it, so read the
list before trusting a header the errata touch.

Generate the headers
--------------------

From the root of the repository, yace runs the generator from its file and
parses the headers that the generator writes:

.. code-block:: bash

   ~/venvs/spec/bin/yace \
       --generate toolbox/xnvme_spec_headers.py:SpecHeaders \
       --generator-option xnvme_spec.spec=/tmp/senfd/merged.spec.document.json \
       --output /tmp/spec \
       --strict \
       --emit capi

The headers are in ``/tmp/spec/xnvme_spec``, one for every document, named by
its abbreviation: ``libxnvme_spec_base.h``, ``libxnvme_spec_nvm.h``,
``libxnvme_spec_kv.h``, ``libxnvme_spec_zns.h``, ``libxnvme_spec_tcp.h`` and so
on. The pretty-printers of them are in
``/tmp/spec/capi``.

``--strict`` makes any diagnostic an error: what the generator writes must be
in yace's C subset, as a header written by hand is. The status of the run is
in ``/tmp/spec/libxnvme_spec_base.status`` and ends in ``0 diagnostics``.

The options of the generator, each given as
``--generator-option xnvme_spec.<key>=<value>``, are:

* ``spec``, the merged data model.
* ``prefix`` of every name, by default ``xnvme_spec``.
* ``commands`` selects the command sets or commands to write, such as ``nvm``
  or ``nvm:write``. The default is all commands. ``none`` writes no commands.
  For a command, the generator writes its opcode and the masks of the fields
  of its dwords, and no struct: ``union xnvme_cmd_sqe`` of ``libxnvme_cmd.h``
  holds the dwords of every command, and the command preps make them. A field
  whose values the specification names gets an enum of them, such as ZSA of
  Zone Management Send. When two fields select the operation, such as DTYPE
  and DOPER of Directive Send, the operations are an enum of the second field
  for each value of the first, named by that value: DOPER when DTYPE is 1h.
* ``structures`` selects the structures to write. Name a structure by its
  name, or by the abbreviation of its document and its name, such as
  ``base:identify_controller``. The default is every byte layout except the
  examples. ``none`` writes no structures. A field whose values the
  specification names gets an enum of them, as for the commands. An array
  whose entries the data model links to another figure, such as the Power
  State Descriptors of Identify Controller, takes the layout of that figure:
  an entry of 1, 2, 4 or 8 bytes keeps its integer type and gets masks named
  by the entry, such as ``XNVME_SPEC_NVM_LBA_FORMAT_*`` for ``lbaf``, and a
  larger entry is a struct of its own. The fields of the protection
  information formats are big-endian, so a field of more than one byte is an
  array of bytes.
* ``registers`` set to ``none`` leaves out the registers. Otherwise the
  generator writes the registers of each document as one struct, such as
  ``struct xnvme_spec_base_registers``, with each register at its offset. The
  generator also writes the masks of the fields of each register, and an enum
  of the values of each field whose values the specification names.
* ``flexible`` set to ``1`` writes the entries that follow a structure, such
  as the zone descriptors that follow the Report Zones header, as a flexible
  array member. yace reports a flexible array member as a diagnostic, because
  the member has no length to bind. By default the doc of the structure
  therefore names the entries instead.

Review the headers
------------------

Every name comes from the specification. A structure is named by its document
and its name, and a field by its acronym. Each struct, field, mask and
constant cites its figure, such as
``(Base 2.4, 5.2.14.1, Figure 334, page 335, PDF page 363)``. Review a change
to the headers as a change to the specification: look up the cited figure for
what changed, and compare the diff against the headers committed before.

The generator writes what the data model has. Where the generator cannot place
something, the header says so. When a field is at an offset that depends on
the data, such as ``(NZ+1)*64``, the doc of the struct that the field follows
names the field. When the specification describes a dword differently under
each of several conditions, the doc of the member names the descriptions that
were not used. When a figure shows the entries of a list, the generator writes
a struct of one entry, and the doc of the struct says where the figure shows
the entries. The generator does not write a figure whose rows cite the fields
of other structures. Where the data model is wrong, fix senfd, not the header,
so the next run does not undo the fix.

Commit the headers
------------------

Copy the headers into ``include/`` as the generator writes them, and add any
new one to the list of ``include/meson.build`` and to ``libxnvme.h``, where
the build takes them as any other header. pre-commit does not format them:
clang-format does not settle on their long trailing comments, and the headers
are never edited by hand. Each header says the version of senfd that produced its data
model, and each citation the revision of its document, so the commit needs no
record of them.
yace is not run by the build; run it again, as above, when a document is
revised or the generator changes.

.. _senfd: https://github.com/safl/senfd
.. _yace: https://github.com/safl/yace
.. _NVM Express: https://nvmexpress.org/specifications/
