.. _sec-contributing-spec-headers:

Producing the spec headers
==========================

A generator, ``toolbox/xnvme_spec_headers.py``, writes the headers that define
the commands, features, data structures, log pages, registers, identifiers and
status codes of the NVMe specifications. `senfd`_ reads the specification
documents into a data model. `yace`_ runs the generator, parses the headers
that the generator writes, checks that the headers use only the C subset that
yace accepts, and emits their pretty-printers and bindings.

The build runs neither senfd, the generator nor yace. A contributor runs them
on a machine that has the specification documents, and commits the headers.

NVM Express publishes the specification documents, and **xNVMe** does not
redistribute them. Never commit a document, a conversion of a document, or
the data model that senfd writes.

Prerequisites
-------------

* The ratified specification documents, as ``.pdf``, from `NVM Express`_,
  kept outside the repository, here in ``~/specs``.
* senfd and yace, in an environment of their own:

  .. code-block:: bash

     python3 -m venv ~/venvs/spec
     ~/venvs/spec/bin/pip install senfd==0.26.0 -r python/bindings/requirements.txt

  CI runs the tests of the generators with senfd 0.26.0 and the yace that
  ``python/bindings/requirements.txt`` pins, the yace that generates the
  bindings. The generator stops on a data model of a format other than 8, the
  format that senfd 0.26.0 writes.

* A C compiler, which yace uses to check what it parses.

Produce the data model
----------------------

.. code-block:: bash

   ~/venvs/spec/bin/senfd ~/specs/*.pdf --output /tmp/senfd

senfd writes ``/tmp/senfd/merged.spec.document.json``, which the generator
reads. ``senfd lint`` lists what senfd finds wrong in the documents as
candidate errata, each with its figure and page:

.. code-block:: bash

   ~/venvs/spec/bin/senfd lint --spec /tmp/senfd/merged.spec.document.json

The generator writes a field with an erratum as the figure gives it. Read the
list before trusting a header that the errata touch.

Generate the headers
--------------------

From the root of the repository:

.. code-block:: bash

   ~/venvs/spec/bin/yace \
       --generate toolbox/xnvme_spec_headers.py:SpecHeaders \
       --generator-option xnvme_spec.spec=/tmp/senfd/merged.spec.document.json \
       --output /tmp/spec \
       --emit capi

yace writes one header for each document to ``/tmp/spec/xnvme_spec``, named
by the key of the document, such as ``libxnvme_spec_base.h`` and
``libxnvme_spec_nvm.h``, and the pretty-printers to ``/tmp/spec/capi``. yace
fails the run on any diagnostic. ``/tmp/spec/libxnvme_spec_base.status``
holds the status of the run, ``0 diagnostics``.

The options of the generator, each given as
``--generator-option xnvme_spec.<key>=<value>``:

* ``spec``: the merged data model.
* ``prefix``: the prefix of every name, by default ``xnvme_spec``.
* ``commands``: the command sets or commands to write, such as ``nvm`` or
  ``nvm:write``. The default is all commands. ``none`` writes no commands.
* ``structures``: the structures to write, each by its key, or by the key of
  its document and its key, such as ``base:identify_controller``. The default
  is every byte layout except those that senfd gives the kind ``example``,
  ``status_values``, ``cited_fields`` or ``mapping``. ``none`` writes no
  structures.
* ``registers``: ``none`` writes no registers.
* ``flexible``: ``1`` writes the entries that follow a structure, such as the
  zone descriptors after the Report Zones header, as a flexible array member.
  yace reports a flexible array member as a diagnostic, so by default the doc
  of the structure names the entries instead.

Review the headers
------------------

For a command, the generator writes the opcode and the masks of the fields of
the dwords, and no struct: ``union xnvme_spec_sqe`` of ``libxnvme_spec.h``
holds the dwords of a submission queue entry, and the command preps write
them. The commands of the Management Interface are not in a submission queue
entry, but the data model records their dwords by number alone, so their
masks name them Command Dwords as well. A structure is a packed struct. The
registers of each document are one struct, such as
``struct xnvme_spec_base_registers``, with each register at its offset. A
field whose values the specification names has an enum of the values. The
module docstring of the generator states the rules for arrays, linked
figures, unions and big-endian fields.

Every name comes from the specification. A structure is named by its document
and the key that senfd gives it, and a field by its acronym. Each struct,
field, mask and constant cites its figure, such as
``(Base 2.4, 5.2.14.1, Figure 334, page 335, PDF page 363)``. Review a change
to the headers as a change to the specification: look up the cited figure,
and compare the diff with the committed headers.

Where the generator writes no member for a field, the doc of the struct names
the field, as for a field at an offset that depends on the data, such as
``(NZ+1)*64``, and for a field of bytes within a member, such as the Offset
and Length of a Heap Object Reference of Boot. By default, entries that follow
a structure, as many as the data says, are no member either. The doc of
``struct xnvme_spec_base_log_fdp_configurations`` says that FDP Configuration
Descriptor entries, counted by NUMFDPC, follow from byte 16, each a
``struct xnvme_spec_base_fdp_configuration_descriptor``, and names each later
descriptor that the figure shows as one of those entries, at an offset the
data gives. NUMFDPC is a 0's based value, which the data model does not
record, so the doc does not give the number of entries as the value of the
field. A layout that is only a list of entries as many as the data says, such
as the Namespace List of Base 2.4 Figure 140, has no struct, since the struct
would have no members. The generator writes nothing for a figure whose rows
cite the fields of other structures. Where the data model is wrong, fix senfd,
not the header: the next run overwrites the header.

Where the generator writes no mask for a field, the doc of the struct names
the field with its bits: a bit field of a member that is an array of bytes, a
field within a field that has a mask, since yace reports two masks of one
member whose bits overlap, and a field without an acronym. The comment of a
command or a feature names a field without an acronym too, and a field within
a field whose bits, as the data model gives them, lie beyond the bits of that
field, as NOCOMP within SO of the Key Value Store command; such a field has no
mask. A layout of bits that no member, array entry, entries after a struct or
dword of a command uses, such as the Log Specific Parameter fields of the log
pages of Base, has no masks. Bytes that no row of the figure covers are a
member named by their offset, such as ``rsvd0``. Where the figure elides rows
there, the doc of the member says so, and where the structure extends another,
the doc cites the figure of that structure. Otherwise the doc says that the
bytes are not in the figure.

Commit the headers
------------------

Copy the headers into ``include/`` unchanged. Add a new header to:

* the list of ``include/meson.build``
* ``libxnvme.h``
* ``INPUT`` in ``docs/tooling/doxy.cfg``
* the group ``nvme`` of ``NAMESPACES`` in
  ``docs/tooling/xnvme_docs/apigen.py``
* the ``exclude`` of the hook ``C-format-headers`` in
  ``.pre-commit-config.yaml``

pre-commit does not format the headers, because clang-format alternates
between two layouts of their long trailing comments, so each run changes the
headers. Each header names the version of senfd that wrote its data model, and
each citation names the revision of its document, so the commit message needs
neither. Run the generator again when a document is revised or the generator
changes.

.. _senfd: https://github.com/safl/senfd
.. _yace: https://github.com/safl/yace
.. _NVM Express: https://nvmexpress.org/specifications/
