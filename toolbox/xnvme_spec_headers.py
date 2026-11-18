#!/usr/bin/env python3
"""
Generator of the xNVMe spec headers, from senfd's data model of the NVMe
specifications

yace runs the generator and parses the headers that the generator writes:

    yace --generate toolbox/xnvme_spec_headers.py:SpecHeaders \\
         --generator-option xnvme_spec.spec=merged.spec.document.json \\
         --output /tmp/spec --emit capi

The build does not run the generator. A contributor runs the generator and
commits the headers; see docs/contributing/contributing-spec-headers.rst.

The generator writes one header for each document of the data model, named by
the key of the document, such as libxnvme_spec_base.h. No header includes
another header. Names hold the keys that senfd gives the documents, the
command sets and the structures, in capitals for a constant, except that the
I/O commands of Base are "io", as in XNVME_SPEC_IO_FLUSH_OPCODE, and the
command set of the Management Interface is "mi".

Options

'spec' is the merged data model that senfd writes, of the format FORMAT; the
default is the data model in senfd's cache. 'prefix' is the prefix of every
name; the default is "xnvme_spec". 'commands' is a comma separated list of
command sets, such as "nvm", or of commands, such as "nvm:write"; the default
is all commands. 'structures' is a list of structures by key, or by document
and key, such as "base:identify_controller"; the default is every byte layout
whose kind is not in SKIPPED. 'commands', 'structures' and 'registers' each
take "none". 'flexible', set to "1", writes the entries that follow a struct
as a flexible array member, which yace reports; by default the doc of the
struct names the entries, with the field that 'counted_by' gives, which counts
them, and the size of each.

Enums

The opcodes of each command set, the feature identifiers, the log page
identifiers and the status codes are enums. The status codes have one enum for
each status code type, and one for the status codes of the Fabrics commands. A
field whose values the specification names has an enum of the values, named by
the mask of the field. A field within a field has an enum as well, as AF
within DSM of Read, XNVME_SPEC_NVM_READ_CDW13_DSM_AF_*, unless a field of its
acronym that is not within a field has an enum of the same values already, as
PSDT of the Fabrics command has for PSDT within CDW0 of each Fabrics Command
Type. Where a field has no mask, as a field within a field of a member of a
struct or a bit field of an array of bytes, its enum is named as its mask
would be. Where every field of a document with the acronym of the field has
the same values, as SGLDT of the SGL descriptors of Base, the enum is named by
the document and the acronym alone, XNVME_SPEC_BASE_SGLDT_*, and the generator
writes it once. Where the fields of an acronym have different values, the
values that more of them have than any other values have that enum, and the
other fields have enums by their masks: PSDT has the values of Base Figure 92
in two fields and those of Figure 96 in the Fabrics command. An acronym that
is the name of its field, as STATUS of Status is, has no enum of its own.

senfd's value_names() gives the name of a value; a value without a name, and a
reserved value, has no constant. Where two values of an enum have one name, as
values of the form factors of Management Interface 2.2 Figure 183 do, the
constant of each later value has its value as a suffix, such as
XNVME_SPEC_MI_FF_PCIE_2H. A constant of an enum is an int, so a value above
7FFFFFFFh is a macro after the enum.

A field that links to a table of values, as 'values_in' links CNS of Identify
to Base Figure 336, has the values of that table. A field that links to a
table of status values, as STATUS of the Connect response links to Base
Figure 106, has no enum: the codes of the table are values of the Status Code
of one Status Code Type, not values of the field. When two fields select the
operation of a command, such as DTYPE and DOPER of Directive Send, each value
of the first field has an enum of the second field.

Commands and features

For each command, the generator writes the masks of the fields of its dwords
and of its completion queue entry, named by the command, the dword and the
acronym of the field, such as XNVME_SPEC_ADMIN_ABORT_CQE_DW0_IANP. The
generator writes no struct for a command: union xnvme_spec_sqe of
libxnvme_spec.h holds the dwords of a submission queue entry. The commands of
the Management Interface and PCIe Commands command sets of the Management
Interface are not in a submission queue entry. The data model records their
dwords by number alone, so their masks name them Command Dwords as well, such
as XNVME_SPEC_MI_CONFIGURATION_GET_CDW0_CID. A pair of dwords that a figure
describes with fields across both, such as CDW10 and CDW11 of Read, is one
64-bit value, and its masks are named by both dwords. A pair that starts at an
odd dword is no 64-bit value, in the dwords of a command, of an operation, of a
feature and of a completion queue entry; its fields have no masks, and the
comment of the command or the feature names them. For each feature, the
generator writes the masks of the fields in the dwords of Set Features and in
Dword 0 of the completion queue entry, such as
XNVME_SPEC_BASE_FEATURE_TEMPERATURE_THRESHOLD_CDW11_THSEL. The masks of the
fields of an operation are named by the command, the operation and the dword,
such as XNVME_SPEC_ADMIN_DIRECTIVE_RECEIVE_ALLOCATE_RESOURCES_CDW12_NSR. An
operation whose figure lays out the whole SQE, as each Fabrics command does,
has the masks of each field by the dword or the pair of dwords that holds it,
such as XNVME_SPEC_FABRICS_FABRICS_CONNECT_CDW10_QID; a field across more than
two dwords, as SGL1, has no mask.

When the data model gives a dword more than one description, the generator
writes the masks of the description without a condition. When every
description has a condition, as for CDW13 of Write, the generator writes the
masks of each description in the order of the conditions. A field that more
than one description has at the same bits has one mask. The doc of a mask
names the conditions of the descriptions that have its field, unless senfd's
comparisons of those conditions cover every value, as "CETYPE is cleared to
0h" and "CETYPE is non-zero" do for DSPEC of Write. The generator writes no
mask for a field without an acronym, or for a field whose bits an earlier
field of the same condition covers. The comment of the command or the feature
names a field without an acronym with its bits, as Lost Host Communication
Notices of Asynchronous Event Configuration.

A field whose meaning depends on the value of another field, as ZRAS of Zone
Management Receive depends on ZRA, has cases in the data model. The doc of the
mask of the field names each case. A field of a case has a mask at its bits in
the field, such as XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZM_DL. A case
that is one field of all the bits has a mask of those bits, such as
XNVME_SPEC_ZNS_ZONE_MANAGEMENT_SEND_CDW13_ZSASO_ZRWAA. When one case names
values, those values are the enum of the field. A field within a field has a
mask at its bits, named by both fields, such as
XNVME_SPEC_NVM_WRITE_CDW12_PRINFO_PRACT, and so has a field within that field.
A field within a field whose bits, as the data model gives them, lie beyond
the bits of that field, as NOCOMP within SO of Key Value 1.4 Figure 31, has
no mask, and the comment of the command names it with those bits.

Registers

The registers of each document at offsets from the start of its registers are
one packed struct, with each register at its offset. The registers of a PCI
capability are one struct for each capability, at offsets from the
capability, such as xnvme_spec_pcie_pmcap_registers. A register whose offset
is a formula, as the doorbell registers of PCIe, is no member; the doc of the
struct names it, and it has no masks. The generator writes the masks of the
fields of each register that is a member, and the enums of their named
values.

Names

A name holds an acronym in capitals, as the specification writes it, except
that the digits an acronym begins with go to its end: 128CCS of the PCI
Express Device Capabilities 2 register is CCS128, since a binding names a mask
by the part after the name of its member.

Layouts

Each byte layout of the data model is a packed struct, named by its document
and its key: the data structures, the log pages, and the layouts without a
named kind, such as the PDUs of the transports. The struct has the fields at
their offsets, and masks for the bits of the fields. The bytes that no row of
the figure covers are a member named by their offset, such as rsvd0. Where
the figure elides rows there, the doc of the member says so. Where the
structure extends another, as NVM 1.3 Figure 126 extends Identify Controller,
the doc of the struct and of that member cite the figure of the other
structure. Otherwise the doc of the member says that the bytes are not in the
figure.

A field at an offset of a formula before a field at a fixed offset, as the
Stuff Bytes of the RPMB Data Frame, lies in the bytes before that field. Those
bytes are a member named by the fields they hold, and the doc of the struct
names each field with its range. A field at an offset of a formula after the
fields at fixed offsets is no member, and the doc of the struct names it with
its range as the data model gives it, as "bytes 33+KXLEN:34". The doc gives
the range of the first such field on the line of what follows the struct, as
"the first at bytes n:16" of the ANA Group Descriptors, since the ranges after
it use its variables.

A field of 1, 2, 4 or 8 bytes is an integer. A field of another size, a
string and a big-endian field are arrays of bytes, and so is an array of
strings or of big-endian entries, such as the Firmware Revisions of the
Firmware Slot Information.

The entries of an array, the fields of the data model with one 'array', are
one array member when the figure numbers them from 0 or from 1. The array has
the number of entries that the figure gives. Where the figure numbers the
entries from 1, the doc of the member gives their numbers, as "Submission
Queue Entry Dword 1 to 5" for SQEDW1 to SQEDW5 of Management Interface 2.2
Figure 139. Where the data gives the number and the specification gives the
most entries, 'maximum', the array has that many entries, and its doc names
the field that 'counted_by' gives, as NZID for the 511 Zone Identifiers of the
Changed Zone List of ZNS. Otherwise the array is flexible. An entry whose rows
the figure gives, as a Source Range of Copy, is a packed struct of those rows.

A field of bytes within a field of bytes, as the Offset and Length of a Heap
Object Reference of Boot, is no member. The doc of the struct names it with
its bytes in the member, and with the struct of a layout of its size that it
links to, as the Zone Descriptor of each entry of the Extended Report Zones
of ZNS. Within a field that is no member, as the Vendor Specific Info of an
AE Occurrence of the Management Interface, the doc also names the bit fields
of such a field of bytes, with their bits in it.

A field that a layout of bits lays out has the fields of that layout, and the
masks of those fields. Command Dword 0 of the Common Command Format has the
masks of Base Figure 92, and VER of Identify Controller has the masks of the
Version register. An array entry of 1, 2, 4 or 8 bytes keeps its integer type,
and its masks are named by the entry, such as XNVME_SPEC_NVM_LBA_FORMAT_LBADS.
A larger entry, such as the Power State Descriptor, is a packed struct, with
the integers that senfd.layout.words() gives. A field of bytes that links to a
byte layout of its size in its document is a member of the struct of that
layout: SGL1 of the Fabrics Command SQE is a struct
xnvme_spec_base_generic_sgl_descriptor_format. PRP1 and PRP2 stay uint64_t,
since the memory page size decides where the offset in a PRP entry ends.

A field wider than 8 bytes whose bit fields lie in its first 8 bytes, such as
the 32 bytes of Directives Supported of the Identify Directive, is an array
of integers, and the masks apply to its first element.

yace reports two masks of one member whose bits overlap, so a field within a
field that has a mask has no mask in a struct, as Function within OPC of
Command Dword 0. Nor has a bit field of a member that is an array of bytes,
as the Supported Events Bitmap of the Persistent Event log page, nor a field
without an acronym, as Result Code within RC of the CDP Response Data Frame.
The doc of the struct names those fields with their bits. A layout of bits has
masks only where a member, an array entry, the entries that follow a struct
or the dword of a command uses it; a layout that none uses, such as the Log
Specific Parameter fields of the log pages of Base, has no masks. The fields
of an entry that follows a struct have masks named by the entry where they
lie in an integer, as those of the Metadata Element Descriptor, whose Element
Value has a size the data gives.

A field whose cases give its bytes as other fields, as PSDT gives DPTR of the
Common Command Format as PRP1 and PRP2 or as SGL1, is a union named by the
struct and the field, such as xnvme_spec_base_common_command_format_dptr.

The dwords of the structures of HALVES are integers of the units that
senfd.layout.words() gives for their fields, each named by its widest field.
Dword 3 of the Completion Queue Entry is 'cid' and 'status', since xNVMe reads
the status of a command from the 16 bits that hold the Phase Tag and the
Status.

A field whose place depends on a condition, such as the PDU Header Digest of a
TCP PDU, ends the struct, and the doc of the struct names that field and the
fields after it.
"""

import re
from dataclasses import dataclass
from dataclasses import field as default
from dataclasses import replace
from pathlib import Path
from types import SimpleNamespace
from typing import Dict, List, Optional, Sequence, Set, Tuple, Union

import senfd.query
from senfd.documents.spec import (
    Array,
    Bits,
    Case,
    Command,
    CommandSet,
    Dwords,
    Entry,
    Feature,
    LogPage,
    Source,
    SpecDocument,
    Structure,
    alias,
    all_fields,
    document_key,
    value_names,
)
from senfd.layout import Word, words

# The format of senfd's data model that the generators read; senfd 0.26.0
# writes it
FORMAT = 8

# The C type of a field, by its size in bytes
TYPES = {1: "uint8_t", 2: "uint16_t", 4: "uint32_t", 8: "uint64_t"}

# The kinds of structure that the default of the option 'structures' does not
# write. The rows of a figure of cited fields are fields of other structures,
# so their offsets are not one layout, and the rows of a figure of a mapping
# give the dwords of a command or completion that the fields of a message map
# to, not the bytes of the message.
SKIPPED = ("example", "status_values", "cited_fields", "mapping")

# The abbreviations of the command sets that are not the key of their document,
# by the key of the command set
SET_ABBREVIATIONS = {"common_io": "io", "management_interface": "mi"}

# The structures, by document and key, whose dwords are integers of the units
# that senfd.layout.words() gives; see the module docstring
HALVES = {("base", "completion_queue_entry")}


def identifier(text: str) -> str:
    """'text' as a part of a C identifier: senfd's key of 'text', in capitals"""

    return alias(text).upper()


def part_of(acronym: str) -> str:
    """'acronym' as a part of a C identifier that a binding can use as a name
    on its own: the digits that an acronym begins with, as 128CCS of the PCI
    Express Device Capabilities 2 register does, go to its end, as CCS128"""

    name = identifier(acronym)
    digits = re.match(r"\d+(?=[A-Z])", name)
    return name[digits.end() :] + name[: digits.end()] if digits else name


def member_name(text: str) -> str:
    """'text' as the name of a member of a struct: senfd's key of 'text', with
    the digits that it begins with at its end, as part_of() moves those of an
    acronym, since a C identifier does not begin with a digit: the 2-Wire
    Pointer of Management Interface 2.2 Figure 184 is 'wire_pointer_2'"""

    name = alias(text)
    digits = re.match(r"(\d+)_", name)
    return f"{name[digits.end():]}_{digits.group(1)}" if digits else name


def load(path: Optional[str]) -> SpecDocument:
    """The data model at 'path', or else the data model in senfd's cache. Raises
    ValueError for a format other than FORMAT."""

    spec = senfd.query.load(Path(path) if path else None)
    if spec.meta.format != FORMAT:
        raise ValueError(
            f"the data model is of format {spec.meta.format}, written by senfd"
            f" {spec.meta.version}; the generators read format {FORMAT}, which senfd"
            " 0.26.0 writes"
        )
    return spec


def generated(headers: List[Path], sources: List[Path]):
    """The headers and sources that a generator wrote, as yace's Generated, or
    as a stand-in when yace is not installed"""

    try:
        from yace.generators import Generated

        return Generated(headers=headers, sources=sources)
    except ImportError:
        return SimpleNamespace(headers=headers, sources=sources)


def figure_of(source: Source) -> Tuple[str, int]:
    """The document and the figure number of 'source'"""

    return (source.document, source.figure_nr)


def doc_of(source: Source) -> str:
    """The key of the document of 'source', which names its header"""

    return document_key(source.document) or alias(source.document)


def set_key_of(command_set: CommandSet) -> str:
    """The abbreviation of 'command_set' in names: the key of the document of
    an I/O command set, such as "kv", the abbreviation in SET_ABBREVIATIONS,
    or else the key of the command set"""

    if command_set.kind == "io" and command_set.document:
        return command_set.document
    key = command_set.key or alias(command_set.name)
    return SET_ABBREVIATIONS.get(key, key)


def members_of(command: Command) -> List[Tuple[str, int, int]]:
    """The dwords of 'command' whose masks share a prefix: for each, the name,
    the first and the last dword. A pair of dwords that the data model
    describes with fields across both is one 64-bit value, where the pair is
    aligned to 64 bits."""

    pairs = {
        (d.lower, d.upper)
        for d in command.sqe
        if d.upper == d.lower + 1 and d.lower % 2 == 0
    }
    members = []
    dword = 0
    while dword < 16:
        if (dword, dword + 1) in pairs:
            members.append((f"cdw{dword}_{dword + 1}", dword, dword + 1))
            dword += 2
        else:
            members.append((f"cdw{dword}", dword, dword))
            dword += 1
    return members


def described(dwords: List[Dwords], lower: int, upper: int) -> List[Dwords]:
    """The descriptions among 'dwords' of the dwords 'lower' to 'upper' whose
    fields have masks: the descriptions without a condition, or else every
    description, in the order of the conditions"""

    these = [d for d in dwords if (d.lower, d.upper) == (lower, upper)]
    plain = [d for d in these if d.condition is None]
    if plain:
        return plain
    conditions = list(dict.fromkeys(d.condition for d in these))
    return sorted(these, key=lambda d: conditions.index(d.condition))


def bits_text(lower: int, upper: int) -> str:
    """The bits 'lower' to 'upper', as "bit 3" or "bits 7:4" """

    return f"bit {lower}" if lower == upper else f"bits {upper}:{lower}"


def formula_range(g: Bits) -> Optional[str]:
    """The range of 'g', a field without bounds, as the data model gives it
    where it has a formula of it, such as "bytes 33+KXLEN:34", or else None"""

    if g.kind != "formula" or not g.range:
        return None
    unit = "bit" if g.unit == "bits" else "byte"
    return f"{unit}s {g.range}" if ":" in g.range else f"{unit} {g.range}"


def entries_of(name: str) -> str:
    """The entries of an array of 'name', in a doc: "Host Entries" for an
    array named Host Entry, else "ANA Group Descriptor entries" """

    if name.endswith("Entry"):
        return f"{name[: -len('Entry')]}Entries"
    return f"{name} entries"


def counted(entries: Array) -> str:
    """How many 'entries' there are, as the doc of a member or of a struct
    says it: counted by the field that 'counted_by' gives, which the
    specification can give as a 0's based value, or as many as the data
    says"""

    if entries.counted_by:
        return f", counted by {entries.counted_by}"
    return ", as many as the data says"


def covers(tests: Sequence) -> bool:
    """Whether senfd's comparisons 'tests' cover every value: one of them
    tests a field against a value with == and another tests that field
    against that value with !="""

    known = {(t.field, t.comparison, t.value) for t in tests if t is not None}
    return any((f, "!=", v) in known for f, op, v in known if op == "==")


def masks_of(
    member: str,
    prefix: str,
    figures: List[Tuple],
    width: int,
    nested: bool = False,
    beyond: Optional[List[Tuple[Bits, str, Bits]]] = None,
    unnamed: Optional[List[str]] = None,
    acronyms: bool = False,
) -> List[Tuple[str, int, str]]:
    """The masks of the fields of 'figures': for each mask, the name, the value
    and the doc, whose field has its acronym where 'acronyms' is set, for a doc
    that names a field without a mask. Each item of 'figures' is the fields of
    a figure, the source of the figure and, optionally, the condition of the
    figure and senfd's comparison of that condition. A field has no mask when
    the field has no acronym, has a formula for its bits, is beyond 'width', is
    an alternative of a union other than the first, or has bits that an earlier
    field of the same condition covers. A field of the same acronym and bits
    under more than one condition has one mask, and its doc names the
    conditions, unless their comparisons cover every value. The fields of a
    case have masks at their bits in the field, named by the field and their
    acronym, and so have the fields within a field when 'nested' is set. yace
    reports two masks of one member of a struct whose bits overlap, so only the
    masks of the dwords of a command, whose docs name no member, are 'nested'.
    A field within a field whose bits the data model gives beyond the bits of
    that field, as NOCOMP within SO of Key Value 1.4 Figure 31, has no mask;
    'beyond' gets the field it is within, the member and the field. 'unnamed'
    gets the doc that a field without an acronym would have as a mask, of a
    field within a field as well when 'nested' is set, as Result Code within RC
    of Base 2.4 Figure 688."""

    # By name: the value, the doc before and after the conditions, and the
    # conditions with their comparisons
    found: Dict[str, Tuple[int, str, str, List[Tuple]]] = {}
    taken: Dict[Optional[str], int] = {}

    def add(name: str, value: int, head: str, tail: str, condition: Tuple) -> bool:
        """Adds the mask, or the condition to the mask of the name and value;
        False where the name has another value"""

        if name not in found:
            found[name] = (value, head, tail, [condition])
        elif found[name][0] != value:
            return False
        elif condition not in found[name][3]:
            found[name][3].append(condition)
        return True

    def within(
        name: str, field: Bits, at: int, cite: str, of: str, condition: Tuple
    ) -> None:
        """Adds the masks of the fields within 'field', whose bit 0 is the bit
        'at' of the member, named by 'name' and the acronym of each. A field
        of all the bits of 'field' is 'field' again, as UIDX of UIDX."""

        width = field.upper - field.lower + 1
        for sub in field.fields:
            if sub.unit != "bits" or sub.lower is None or sub.upper is None:
                continue
            if not sub.acronym:
                if sub.upper < width:
                    lower, upper = at + sub.lower, at + sub.upper
                    without(sub, f"{bits_text(lower, upper)} of {of}", cite)
                continue
            if sub.upper >= width:
                if beyond is not None:
                    beyond.append((field, of, sub))
                continue
            if (sub.lower, sub.upper) == (0, width - 1):
                continue
            lower, upper = at + sub.lower, at + sub.upper
            part = ((1 << (upper + 1)) - 1) ^ ((1 << lower) - 1)
            named = f"{name}_{part_of(sub.acronym)}"
            what = case_name(sub) if acronyms else sub.name or sub.acronym
            where = f"; {bits_text(lower, upper)} of {of}"
            if add(named, part, f"{what} ({cite})", where, condition):
                within(named, sub, lower, cite, of, condition)

    def without(field: Bits, where: str, cite: str) -> None:
        """Adds the doc of 'field', a field without an acronym, to 'unnamed'"""

        if unnamed is not None and field.role == "field" and field.name:
            doc = f"{field.name} ({cite}); {where}"
            if doc not in unnamed:
                unnamed.append(doc)

    for fields, source, *condition in figures:
        cond = condition[0] if condition else None
        test = condition[1] if len(condition) > 1 else None
        for field in fields:
            if field.lower is None or field.upper is None:
                continue
            if field.upper >= width or field.union is not None and field.alternative:
                continue
            of = (
                member
                if member.startswith(("Command Dword", "Completion Queue Entry"))
                else f"'{member}'"
            )
            if not field.acronym:
                where = bits_text(field.lower, field.upper)
                without(field, f"{where} of {of}", senfd.query.cite(source))
                continue
            value = ((1 << (field.upper + 1)) - 1) ^ ((1 << field.lower) - 1)
            name = f"{prefix}_{part_of(field.acronym)}"
            if name in found and found[name][0] != value:
                continue
            if value & taken.get(cond, 0):
                continue
            taken[cond] = taken.get(cond, 0) | value
            where = bits_text(field.lower, field.upper)
            what = case_name(field) if acronyms else field.name or field.acronym
            cite = senfd.query.cite(source)
            tail = f"; {where} of {of}"
            for case in field.cases:
                if case.role == "field" and (case.name or case.acronym):
                    tail += f"; {case_name(case)} when {condition_of(case)}"
            add(name, value, f"{what} ({cite})", tail, (cond, test))
            for case in field.cases:
                if case.role == "field" and case.acronym and not case.fields:
                    add(
                        f"{name}_{part_of(case.acronym)}",
                        value,
                        f"{case_name(case)} ({cite})",
                        f", when {condition_of(case)}; {where} of {of}",
                        (cond, test),
                    )
                for sub in case.fields:
                    if not sub.acronym or sub.lower is None or sub.upper is None:
                        continue
                    lower, upper = field.lower + sub.lower, field.lower + sub.upper
                    if upper > field.upper:
                        continue
                    part = ((1 << (upper + 1)) - 1) ^ ((1 << lower) - 1)
                    add(
                        f"{name}_{part_of(sub.acronym)}",
                        part,
                        f"{sub.name or sub.acronym} ({cite})",
                        f", when {condition_of(case)}; {bits_text(lower, upper)} of"
                        f" {of}",
                        (cond, test),
                    )
            if nested:
                within(
                    name,
                    field,
                    field.lower,
                    senfd.query.cite(field.structure or source),
                    of,
                    (cond, test),
                )

    masks: List[Tuple[str, int, str]] = []
    for name, (value, head, tail, conditions) in found.items():
        texts = [text for text, _ in conditions]
        when = ""
        if None not in texts and not covers([test for _, test in conditions]):
            when = f", when {' or '.join(texts)}"
        masks.append((name, value, f"{head}{when}{tail}"))
    return masks


def hidden_masks(
    member: str,
    prefix: str,
    figures: List[Tuple],
    width: int,
    masks: List[Tuple[str, int, str]],
    unnamed: Optional[List[str]] = None,
) -> List[Tuple[str, int, str]]:
    """The masks of the fields within the fields of 'figures' that masks_of()
    gives with 'nested' and not in 'masks', as the masks of a member of a
    struct leave them out, with the docs that name the fields with their
    acronyms. No mask of these is written; ValueEnums.of() names the enums of
    their fields by them. 'unnamed' is as masks_of() takes it."""

    names = {name for name, _, _ in masks}
    return [
        mask
        for mask in masks_of(
            member, prefix, figures, width, nested=True, unnamed=unnamed, acronyms=True
        )
        if mask[0] not in names
    ]


def without_masks(
    in_bytes: List[str], nested: List[str], unnamed: Sequence[str] = ()
) -> List[str]:
    """The lines of the doc of a struct, or of the comment of a command or a
    feature, that name the fields without masks: 'in_bytes' of the members
    that are arrays of bytes, 'nested' within a field that has a mask, and
    'unnamed' without an acronym, each as the doc of its mask would be"""

    lines: List[str] = []
    if in_bytes:
        lines += ["", "Without masks, as fields of an array of bytes:", *in_bytes]
    if nested:
        lines += [
            "",
            "Without masks, as fields within a field that has a mask:",
            *nested,
        ]
    if unnamed:
        lines += ["", "Without masks, as fields without an acronym:", *unnamed]
    return lines


def comment_notes(notes: List[str], lines: List[str]) -> List[str]:
    """'notes', lines of a comment, followed by 'lines' of a doc, which begin
    with an empty line, as without_masks() gives them. A comment has an empty
    line before its notes, so 'lines' lose their first when 'notes' is empty."""

    more = [f" * {line}" if line else " *" for line in lines]
    return notes + (more if notes else more[1:])


def case_name(case: Union[Case, Bits]) -> str:
    """The name of the field of 'case', a case or a field, with the acronym if
    there is one"""

    if case.name and case.acronym:
        return f"{case.name} ({case.acronym})"
    return case.name or case.acronym or ""


def condition_of(case: Case) -> str:
    """When 'case' applies, as "ZRA is Report Zones or Extended Report Zones" """

    named = [w.name for w in case.when]
    return f"{case.selector} is {' or '.join(named)}" if named else "otherwise"


def conditional(field: Bits) -> bool:
    """Whether the place of 'field' depends on a condition, as the PDU Header
    Digest of a TCP PDU is present only when HDGSTF is 1"""

    placements = field.placements
    return any(p.range is None for p in placements) or (
        len({(p.lower, p.upper) for p in placements}) > 1
    )


def layouts_of_bits(spec: SpecDocument) -> Dict[Tuple[str, int], Structure]:
    """The bit layouts of 'spec', the registers included, by document and
    figure number"""

    layouts: Dict[Tuple[str, int], Structure] = {}
    for structure in spec.structures:
        if structure.unit == "bits":
            layouts.setdefault(figure_of(structure.source), structure)
    return layouts


def shifted(fields: Sequence[Bits], by: int) -> List[Bits]:
    """'fields' with 'by' subtracted from their bounds, without the fields
    below 'by'; a negative 'by' moves the fields up"""

    return [
        f.model_copy(update={"lower": f.lower - by, "upper": f.upper - by})
        for f in fields
        if f.lower is not None and f.upper is not None and f.lower >= by
    ]


def units_of(fields: Sequence[Bits], nbytes: int) -> Optional[List[Word]]:
    """The units of senfd.layout.words() for the bit fields 'fields' of
    'nbytes' bytes, or None where words() cannot lay them out, as for a field
    of a formula"""

    try:
        return words(list(fields), nbytes)
    except ValueError:
        return None


@dataclass
class Member:
    """A member of the struct of a structure: 'count' is None for a scalar, 0
    for a flexible array, and otherwise the length of an array of 'ctype'"""

    name: str
    ctype: str
    count: Optional[int]
    lower: int
    doc: str
    fields: List[Bits] = default(default_factory=list)
    size: int = 0
    description: str = ""
    # The field that is the whole member, for the values it names
    whole: Optional[Bits] = None
    # The bytes of one element of an array
    stride: int = 0
    # The field of the member, or the first entry of its array
    field: Optional[Bits] = None
    # The figures of the masks of the member, where they are not 'fields'
    figures: Optional[List[Tuple[List[Bits], Source]]] = None
    # The bit fields of an array of bytes, which have no masks
    unmasked: List[Bits] = default(default_factory=list)
    # What follows the bytes in the doc, where 'doc' is empty
    after: str = ""


def halves_of(field: Bits, structure: Structure, lower: int) -> Optional[List[Member]]:
    """The members of the dword 'field' of a structure of HALVES at byte
    'lower': one for each unit of senfd.layout.words() for its fields, named by
    the widest field of the unit, with the masks of the fields that a field of
    the unit has before the masks of the unit's own fields. None for a dword
    without fields."""

    laid = [g for g in field.fields if g.unit == "bits"]
    units = units_of(laid, 4) if laid else None
    if not units:
        return None
    source = field.structure or structure.source
    members = []
    for unit in units:
        named = [g for g in unit.fields if g.acronym]
        if not named:
            raise ValueError(f"{structure.title}: {field.name} has a unit of no field")
        widest = max(named, key=lambda g: g.upper - g.lower)
        fills = len(named) == 1 and (widest.lower, widest.upper) == (
            0,
            8 * unit.size - 1,
        )
        figures: List[Tuple[List[Bits], Source]] = []
        for g in named:
            nested = [h for h in g.fields if h.unit == "bits"]
            if nested and not fills:
                figures.append((shifted(nested, -g.lower), g.structure or source))
        if not fills:
            figures.append((list(unit.fields), source))
        members.append(
            Member(
                part_of(widest.acronym).lower(),
                TYPES[unit.size],
                None,
                lower + unit.offset,
                " and ".join(
                    g.name or g.acronym for g in sorted(named, key=lambda g: -g.upper)
                ),
                size=unit.size,
                field=field,
                figures=figures,
            )
        )
    return members


def layout_of(
    structure: Structure,
    element_of,
    record_of=None,
    flexible: bool = False,
    see: str = "",
) -> Tuple[List[Member], List[str]]:
    """The members of the struct of 'structure', in the order of their offsets,
    and the fields that are not members, each with the reason

    'element_of' returns the struct of the element of an array, for the name,
    the size, the link and the rows of an entry. 'record_of' returns the
    struct of the layout that a field of bytes links to, for the field and its
    size. A field at an offset that depends on the data, such as "(NZ+1)*64",
    after the last field at a fixed offset, ends the struct with a flexible
    array; 'flexible' tells whether the struct has that member, which the
    fields that are not members name. Such a field before a field at a fixed
    offset, as the Stuff Bytes of the RPMB Data Frame, is in the bytes before
    that field, which are a member named by the fields they hold. Other bytes
    that no field covers are a member whose doc says whether the figure
    elides rows there, and cites 'see', the structure that 'structure'
    extends, or else says that the bytes are not in the figure. A field whose
    place depends on a condition ends the struct without a member. Of a
    union, the first alternative is the member. Raises ValueError for a field
    of bytes that a field before it has."""

    def bounded(f: Bits) -> bool:
        return f.lower is not None and f.upper is not None

    rows = [f for f in structure.fields if not (f.union is not None and f.alternative)]
    # A figure can give its rows from the highest offset down
    ordered = list(rows)
    lowers = [f.lower for f in rows if bounded(f)]
    if len(lowers) > 1 and lowers[0] > lowers[-1]:
        ordered.reverse()
    # The fields without bounds before each field with bounds that follows
    # them, by the id of that field
    inside: Dict[int, List[Bits]] = {}
    pending: List[Bits] = []
    last = max((i for i, f in enumerate(ordered) if bounded(f)), default=-1)
    for f in ordered[: last + 1]:
        if not bounded(f):
            pending.append(f)
        elif pending:
            inside[id(f)], pending = pending, []
    held = {id(g) for these in inside.values() for g in these}
    fields = sorted(
        (f for f in rows if id(f) not in held),
        key=lambda f: (f.lower is None, f.lower or 0),
    )
    halves = (doc_of(structure.source), structure.key) in HALVES
    members: List[Member] = []
    left: List[str] = []
    names: Dict[str, int] = {}
    offset = 0

    def unique(text: str, lower: int) -> str:
        return text if text not in names else f"{text}_{lower}"

    def named(text: str, lower: int) -> str:
        name = unique(text, lower)
        names[name] = lower
        return name

    def array(
        base: str,
        size: Optional[int],
        link: Optional[Source] = None,
        record: Sequence[Bits] = (),
    ) -> Tuple[str, Optional[int]]:
        """The element type of an array of 'base', and the number of elements
        of that type in each entry: the struct that element_of() returns, or
        else an integer of 'size' bytes, or else bytes"""

        element = element_of(base, size, link, record)
        if element:
            return f"struct {element}", 1
        if size in TYPES:
            return TYPES[size], 1
        return "uint8_t", size

    def place(g: Bits) -> str:
        """Where 'g', a field without bounds, lies: at its range as the data
        model gives it, where the data model has a formula of it, such as
        "bytes 33+KXLEN:34", or else at an offset the data gives, as for the
        range "Variable Sized" """

        return formula_range(g) or "at an offset the data gives"

    def bits_in(bits: List[Bits], g: Bits) -> List[str]:
        """The bit fields 'bits' within 'g', a field of bytes, as fields that
        are not members, by their bits in 'g'"""

        return [
            f"{case_name(h)}: {bits_text(h.lower, h.upper)} of {g.name or g.acronym}"
            for h in bits
            if h.role == "field"
            and (h.name or h.acronym)
            and h.lower is not None
            and h.upper is not None
        ]

    def rest(following: int, into: str, what: str, entries=None) -> List[str]:
        """The fields from 'following' on, as fields that are not members:
        in the flexible member 'into' of 'what', or else one of the 'entries'
        of 'what' or after them"""

        def within(g: Bits) -> str:
            if flexible:
                return f", in '{into}'"
            if entries is None:
                return f", after the {what}"
            if g.array and (g.array.name, g.array.lower) == (
                entries.name,
                entries.lower,
            ):
                return f", one of the {entries_of(what)}"
            return f", after the {entries_of(what)}"

        lines: List[str] = []
        for g in fields[following:]:
            if g.name:
                lines.append(f"{case_name(g)}: {place(g)}{within(g)}")
                lines += parts(g, g.name, member=False)[0]
        return lines

    def parts(f: Bits, into: str, member: bool = True) -> Tuple[List[str], List[Bits]]:
        """The fields of bytes within 'f', as fields that are not members, by
        their bytes in 'into', with the struct of a layout of their size that
        one links to; and the bit fields within those fields of bytes, at
        their bits in 'f', where those are not of such a struct. Where 'f' is
        no 'member', or a field of bytes has no bounds, the bit fields within
        it are fields that are not members as well, by their bits in it."""

        lines: List[str] = []
        laid: List[Bits] = []
        for g in f.fields:
            if g.unit != "bytes" or g.role != "field" or not (g.name or g.acronym):
                continue
            bits = [h for h in g.fields if h.unit == "bits"]
            if g.lower is None or g.upper is None:
                lines.append(f"{case_name(g)}: {place(g)}, in {into}")
                lines += bits_in(bits, g)
                continue
            size = g.upper - g.lower + 1
            plain = g.encoding or g.byte_order == "big-endian" or record_of is None
            record = None if plain else record_of(g, size)
            where = f"byte {g.lower}" if size == 1 else f"bytes {g.upper}:{g.lower}"
            of = f", a 'struct {record}'" if record else ""
            lines.append(f"{case_name(g)}: {where} of {into}{of}")
            if record:
                continue
            if member:
                laid += shifted(bits, -8 * g.lower)
            else:
                lines += bits_in(bits, g)
        return lines, laid

    index = 0
    while index < len(fields):
        f = fields[index]
        if conditional(f):
            left += [
                f"{g.name or g.range}: at an offset the data gives"
                for g in fields[index:]
                if g.name
            ]
            break
        entries = f.array
        if f.lower is None or f.upper is None:
            left += parts(f, f.name, member=False)[0]
            if entries:
                what = entries.name
                ctype, _ = array(what, entries.stride, f.structure, f.fields)
                name = (
                    part_of(entries.acronym).lower()
                    if entries.acronym
                    else member_name(what)
                )
            else:
                what = f.name or "data"
                ctype, name = "uint8_t", member_name(what)
            members.append(Member(named(name, offset), ctype, 0, offset, what, field=f))
            left += rest(index + 1, name, what, entries)
            break
        if f.lower < offset:
            raise ValueError(
                f"{structure.title}: '{f.name or f.range}' is of bytes that a field"
                " before it has"
            )
        within = inside.get(id(f), [])
        if f.lower > offset:
            gap = f.lower - offset
            if within:
                name = named(
                    "_".join(
                        part_of(g.acronym).lower() if g.acronym else member_name(g.name)
                        for g in within
                    ),
                    offset,
                )
                doc = " and ".join(g.name or g.acronym for g in within)
                left += [
                    f"{g.name or g.range}: bytes {g.range}, in '{name}'" for g in within
                ]
                members.append(
                    Member(
                        name,
                        "uint8_t",
                        None if gap == 1 else gap,
                        offset,
                        doc,
                        size=gap,
                    )
                )
            else:
                # The model marks the row after the rows that a figure elides
                if f.elided:
                    rows = entries_of(f.array.name) if f.array else "rows"
                    after = f", {rows} that the figure elides"
                else:
                    after = "" if see else ", not in the figure"
                if see:
                    after += f", see {see}"
                members.append(
                    Member(
                        named(f"rsvd{offset}", offset),
                        "uint8_t",
                        None if gap == 1 else gap,
                        offset,
                        "",
                        size=gap,
                        after=after,
                    )
                )
            offset = f.lower
        elif within:
            left += [f"{g.name or g.range}: bytes {g.range}" for g in within]
        size = f.upper - f.lower + 1
        # An array is one member only when the figure numbers its entries from
        # 0 or from 1; CDW10 to CDW15 of the Common Command Format stay
        # separate members
        if entries and entries.first in (0, 1) and entries.lower == f.lower:
            following = index + 1
            while (
                following < len(fields)
                and fields[following].array is not None
                and (fields[following].array.name, fields[following].array.lower)
                == (entries.name, entries.lower)
            ):
                following += 1
            name = (
                part_of(entries.acronym).lower()
                if entries.acronym
                else member_name(entries.name)
            )
            stride = entries.stride
            ctype, per = array(entries.name, stride, f.structure, f.fields)
            swapped = f.byte_order == "big-endian"
            if f.encoding or swapped:
                ctype, per = "uint8_t", stride
            count = entries.count or entries.maximum
            if stride is None or count is None:
                members.append(
                    Member(named(name, offset), ctype, 0, offset, entries.name, field=f)
                )
                left += rest(following, name, entries.name, entries)
                break
            numbered = entries.name
            if entries.first:
                numbered += f" {entries.first} to {entries.first + count - 1}"
            what = f"{numbered}, {count} of {stride} bytes"
            if entries.count is None:
                what = f"{numbered}, up to {count} of {stride} bytes{counted(entries)}"
            if swapped:
                what += ", big-endian"
            members.append(
                Member(
                    named(name, offset),
                    ctype,
                    count * (per or 1),
                    offset,
                    what,
                    size=count * stride,
                    description=f.description,
                    stride=stride,
                    field=f,
                )
            )
            offset += count * stride
            index = following
            continue
        if f.role == "reserved":
            name = f"rsvd{f.lower}"
        elif f.acronym:
            name = part_of(f.acronym).lower()
        else:
            name = member_name(f.name or f"bytes_{f.lower}")
        doc = f.name or "Reserved"
        # A string, such as the Firmware Revision, and a big-endian field are
        # arrays of bytes whatever their size
        swapped = f.byte_order == "big-endian"
        scalar = size in TYPES and not f.encoding and not swapped
        if swapped:
            doc += ", big-endian"
        laid = [g for g in f.fields if g.unit == "bits"]
        if halves and scalar and size == 4:
            these = halves_of(f, structure, f.lower)
            if these:
                for m in these:
                    members.append(replace(m, name=named(m.name, m.lower)))
                offset = f.upper + 1
                index += 1
                continue
        record = (
            None
            if scalar or swapped or f.encoding or record_of is None
            else record_of(f, size)
        )
        if not record:
            named_parts, inner = parts(f, f"'{unique(name, f.lower)}'")
            left += named_parts
            laid += inner
        if record:
            members.append(
                Member(
                    named(name, f.lower),
                    f"struct {record}",
                    None,
                    f.lower,
                    doc,
                    size=size,
                    description=f.description,
                    field=f,
                )
            )
            offset = f.upper + 1
            index += 1
            continue
        element = (
            None if scalar or swapped or f.encoding else element_of_bits(size, laid)
        )
        if element:
            members.append(
                Member(
                    named(name, f.lower),
                    TYPES[element],
                    size // element,
                    f.lower,
                    doc,
                    laid,
                    size,
                    f.description,
                    stride=element,
                    field=f,
                )
            )
            offset = f.upper + 1
            index += 1
            continue
        members.append(
            Member(
                named(name, f.lower),
                TYPES[size] if scalar else "uint8_t",
                None if scalar else size,
                f.lower,
                doc,
                laid if scalar else [],
                size,
                f.description,
                f if scalar else None,
                field=f,
                unmasked=[] if scalar else laid,
            )
        )
        offset = f.upper + 1
        index += 1
    return members, left


def integer_of(layout: Structure) -> int:
    """The size of the smallest integer that holds the fields of the bit
    layout 'layout' at the bits the data model gives, or 0. An entry of the
    Metadata Element Descriptor of Base 2.4 Figure 533 has its fixed fields in
    4 bytes, before an Element Value whose size the data gives."""

    uppers = [
        g.upper for g in layout.fields if g.lower is not None and g.upper is not None
    ]
    if not uppers:
        return 0
    return next((n for n in TYPES if 8 * n > max(uppers)), 0)


def element_of_bits(size: int, fields: List[Bits]) -> Optional[int]:
    """The size of the integers that a field of 'size' bytes is an array of,
    where senfd.layout.words() gives one unit for its bit fields, as 8 for the
    32 bytes of Directives Supported of the Identify Directive; otherwise None"""

    if not [g for g in fields if g.acronym]:
        return None
    units = units_of(fields, size)
    if units and len(units) == 1 and units[0].count > 1:
        return units[0].size
    return None


def alternatives_of(field: Bits) -> List[Tuple[Case, List[Bits]]]:
    """The cases of 'field' that give its bytes as other fields, as PSDT gives
    DPTR of the Common Command Format as PRP1 and PRP2 or as SGL1, each with
    the fields of the case in the order of their offsets in 'field'"""

    if field.unit != "bytes" or field.lower is None or field.upper is None:
        return []
    found = []
    for case in field.cases:
        fields = [
            g
            for g in case.fields
            if g.unit == "bytes"
            and g.lower is not None
            and g.upper is not None
            and g.role != "reserved"
        ]
        if case.role == "field" and fields:
            found.append((case, sorted(fields, key=lambda g: g.lower)))
    return found


def number_of(entry: Entry) -> Optional[int]:
    """The value of 'entry' as a number, or None for a range or a value that
    is no code"""

    typed = entry.typed.get(entry.value_column)
    if typed is None or typed.kind not in ("identifier", "number"):
        return None
    if typed.upper is not None and typed.upper != typed.lower:
        return None
    return typed.lower


def packed(kind: str, name: str, doc: List[str], body: List[str]) -> List[str]:
    """The lines of the packed 'kind', "struct" or "union", named 'name', with
    the doc lines 'doc' and the members 'body'"""

    return [
        "/**",
        *[f" * {line}".rstrip() for line in doc],
        " */",
        f"{kind} __attribute__((packed)) {name} {{",
        *body,
        "};",
        "",
    ]


def defines(masks: List[Tuple[str, int, str]]) -> List[str]:
    """The lines of 'masks', each a name, a value and a doc, and an empty line"""

    lines = [
        line
        for name, value, doc in masks
        for line in (f"/** {doc} */", f"#define {name} 0x{value:x}")
    ]
    return [*lines, ""] if lines else []


def value_name(entry: Entry) -> Optional[str]:
    """The name of the value 'entry', as senfd's value_names() returns it. For
    a value that refers to a value of another field, as a Reporting Option of
    ZRAS refers to a Zone State, the name is the acronym of that value, or
    None when the data model links none."""

    named = entry.name or next(iter(value_names(entry)), None)
    if not named and entry.refers_to and entry.refers_to.acronym:
        return entry.refers_to.acronym
    return named


def value_doc(entry: Entry) -> str:
    """The doc of the constant of 'entry': the name of the value, or the
    description of a value that value_name() names by an acronym"""

    if not entry.name and entry.refers_to and entry.description:
        return " ".join(entry.description.split())
    return value_name(entry) or ""


def constants_of(values: List[Entry]) -> Tuple[Tuple[int, str], ...]:
    """The constants of 'values': each value that is a number and has a name
    and a role other than "reserved", with that name"""

    found = []
    for entry in values:
        value = number_of(entry)
        named = value_name(entry)
        if value is None or not named or entry.role == "reserved":
            continue
        found.append((value, named))
    return tuple(found)


# The largest constant of an enum, which C before C23 makes an int
ENUM_MAX = 0x7FFFFFFF


def enum_lines(name: str, doc: str, constants: List[Tuple[str, int, str]]) -> List[str]:
    """The lines of the enum 'name' with 'constants', each a name, a value and
    a doc. A name that occurs again with the same value has no second
    constant, and with another value has its value as a suffix. A value above
    ENUM_MAX is a macro after the enum."""

    body: List[str] = []
    macros: List[str] = []
    seen: Dict[str, int] = {}
    for constant, value, what in constants:
        if seen.get(constant) == value:
            continue
        if constant in seen:
            constant = f"{constant}_{value:X}H"
            if constant in seen:
                continue
        seen[constant] = value
        if value > ENUM_MAX:
            macros += [f"/** {what} */", f"#define {constant} 0x{value:x}"]
        else:
            body.append(f"\t{constant} = 0x{value:x}, ///< {what}")
    if not body:
        return ["/*", f" * {doc}", " */", *macros, ""] if macros else []
    lines = ["/**", f" * {doc}"]
    if macros:
        lines += [
            " *",
            " * The values that an int cannot hold follow the enum as macros",
        ]
    lines += [" */", f"enum {name} {{", *body, "};", ""]
    return lines + ([*macros, ""] if macros else [])


def values_enum(mask: str, doc: str, values: List[Entry], cited: str) -> List[str]:
    """The lines of the enum 'mask', with the doc 'doc', of the 'values' of a
    field in the figure 'cited', one constant for each of constants_of()"""

    constants = []
    for entry in values:
        value = number_of(entry)
        named = value_name(entry)
        if value is None or not named or entry.role == "reserved":
            continue
        constants.append(
            (
                f"{mask}_{identifier(named)}",
                value,
                f"{value_doc(entry)} ({cited})",
            )
        )
    return enum_lines(mask.lower(), doc, constants)


def dwords_of_spec(spec: SpecDocument) -> List[Dwords]:
    """Every description of dwords of a command, of an operation and of a
    feature"""

    found: List[Dwords] = []
    for command_set in spec.command_sets.values():
        for command in command_set.commands.values():
            found += command.sqe + command.cqe
            for operation in command.operations:
                found += operation.sqe + operation.cqe
    for feature in spec.features.values():
        found += feature.sqe + feature.cqe
        for operation in feature.operations:
            found += operation.sqe + operation.cqe
    return found


def values_of(spec: SpecDocument, field: Bits) -> List[Entry]:
    """The values of 'field' that senfd.query.values_of_field() gives, but
    none where 'field' links to a table of status values; see the module
    docstring"""

    if not field.values and any(
        table.kind == "status_values"
        for table in senfd.query.tables_of_values(spec, field)
    ):
        return []
    return senfd.query.values_of_field(spec, field)


class ValueEnums:
    """Writes the enums of the values of fields. The values of an acronym in a
    document form one enum, named by the document and the acronym and written
    once, when every field of the document with that acronym has the same
    values, or else when more of its fields have one set of values than any
    other set, as PSDT of Figure 92 has in Base 2.4 against PSDT of the Fabrics
    command. The other fields keep enums of their own. An acronym that is the
    name of its field, as STATUS of Status is, has no shared enum. A field
    whose values are a table of status values has no enum; see values_of().

    Of the dwords of a command, the fields of the dwords and of their cases
    count, and not the fields inside a field: CDW0 of each Fabrics Command
    Type has the fields of Base 2.4 Figure 96 inside it, whose enums are named
    by their masks unless their values are those of the enum of the
    document. A field inside a field has no enum where the enum of a field
    with its acronym that is not inside a field, written before it in the
    document, has its values already, as PSDT of the Fabrics command has them
    for PSDT within CDW0 of each Fabrics Command Type."""

    def __init__(self, spec: SpecDocument, stem: str):
        self.spec = spec
        self.upper = identifier(stem)
        self.written: Set[Tuple[str, str]] = set()
        # The values of each acronym of a document that the enum of a field
        # not inside a field has
        self.seen: Set[Tuple[str, str, Tuple[Tuple[int, str], ...]]] = set()
        signatures: Dict[Tuple[str, str], Dict[Tuple[Tuple[int, str], ...], int]] = {}

        def add(doc: str, fields: Sequence[Bits], nested: bool = True) -> None:
            for f in all_fields(list(fields)) if nested else fields:
                for g in [f, *[s for c in f.cases for s in c.fields]]:
                    # An acronym that is the name of its field, as STATUS of
                    # Status, names no value of its own
                    if not g.acronym or identifier(g.acronym) == identifier(
                        g.name or ""
                    ):
                        continue
                    found = constants_of(values_of(spec, g))
                    if found:
                        counts = signatures.setdefault((doc, g.acronym), {})
                        counts[found] = counts.get(found, 0) + 1

        for structure in spec.structures:
            add(doc_of(structure.source), structure.fields)
        for dwords in dwords_of_spec(spec):
            add(doc_of(dwords.source), dwords.fields, nested=False)
        self.shared: Dict[Tuple[str, str], Tuple[Tuple[int, str], ...]] = {}
        for key, counts in signatures.items():
            ranked = sorted(counts.items(), key=lambda item: -item[1])
            if len(ranked) == 1 or ranked[0][1] > ranked[1][1]:
                self.shared[key] = ranked[0][0]

    def of(
        self,
        doc: str,
        prefix: str,
        figures: List[Tuple],
        masks: List[Tuple[str, int, str]],
        hidden: Sequence[Tuple[str, int, str]] = (),
    ) -> List[str]:
        """The enums of the values of the fields of 'figures', and of the
        fields within them, that have a mask among 'masks' or 'hidden', or that
        are the whole member that 'prefix' names. 'hidden' are the masks that
        hidden_masks() and masks_of() give for fields without a mask, as AESS
        within AESSI of an AE Occurrence of the Management Interface. The enum
        of a field is named by its mask, or by the member, or else by 'doc' and
        its acronym. The values of a field are those of the table that
        'values_in' links to, or else of the layout that the field around it
        links to, as QT within MOS of Base 2.4 Figure 167 has the values of
        Figure 171, or else of its figure; the enum cites that figure."""

        named_masks = {name for name, _, _ in [*masks, *hidden]}
        if not masks:
            named_masks.add(prefix)
        lines: List[str] = []

        def within(
            field: Bits, acronym: str, link: Optional[Source]
        ) -> List[Tuple[Bits, str, Optional[Source]]]:
            """The fields within 'field', and within those, each with the
            acronyms that name its mask after 'prefix', as masks_of() names
            them, and with the layout that the nearest field around it links
            to, or else 'link'; that layout holds its values"""

            found: List[Tuple[Bits, str, Optional[Source]]] = []
            around = field.structure or link
            for sub in field.fields:
                if sub.unit == "bits" and sub.acronym:
                    named = f"{acronym}_{part_of(sub.acronym)}"
                    found += [(sub, named, around), *within(sub, named, around)]
            return found

        for fields, source, *_ in figures:
            # A field of a case, such as DL of ZM, has the acronym of its mask:
            # the field and the field of the case
            parts = [
                (sub, f"{field.acronym}_{sub.acronym}", False, None, False)
                for field in fields
                if field.acronym
                for case in field.cases
                for sub in case.fields
                if sub.acronym
            ]
            # A field within a field has the enum of the document where its
            # values are those of that enum, as DTD within OPC
            parts += [
                (sub, named, True, link, True)
                for field in fields
                if field.acronym
                for sub, named, link in within(field, part_of(field.acronym), None)
            ]
            for field, acronym, shares, link, inner in [
                (f, f.acronym, True, None, False) for f in fields
            ] + parts:
                if not acronym:
                    continue
                values = values_of(self.spec, field)
                signature = constants_of(values)
                valued = [c for c in field.cases if c.values]
                what = field.name or field.acronym
                if len(valued) == 1 and not values:
                    # The values of the one case that names them are the field's,
                    # such as the Reporting Options of ZRAS when ZRA is Report Zones
                    values = valued[0].values
                    what = f"{case_name(valued[0])} when {condition_of(valued[0])}"
                if not values:
                    continue
                mask = f"{prefix}_{part_of(acronym)}"
                if mask not in named_masks:
                    if prefix not in named_masks or not prefix.endswith(
                        f"_{part_of(acronym)}"
                    ):
                        continue
                    mask = prefix
                named_masks.discard(mask)
                cited = senfd.query.cite(field.values_in or link or source)
                shared = self.shared.get((doc, field.acronym or ""))
                seen = (doc, field.acronym or "", constants_of(values))
                if signature and signature == shared and shares:
                    if (doc, field.acronym) in self.written:
                        continue
                    self.written.add((doc, field.acronym))
                    mask = f"{self.upper}_{doc.upper()}_{part_of(field.acronym)}"
                elif inner and seen in self.seen:
                    # A field within a field whose values the enum of a field
                    # not inside a field has, as PSDT within CDW0 of each
                    # Fabrics Command Type, has no enum of its own
                    continue
                if not inner:
                    self.seen.add(seen)
                lines += values_enum(
                    mask, f"The values of {what} ({cited})", values, cited
                )
        return lines


def operation_enums(command: Command, masks: List[Tuple[str, int, str]]) -> List[str]:
    """The enums of the operations of 'command' that two fields select, such
    as DTYPE and DOPER of Directive Send: one enum of the second field for
    each value of the first, in the order of the data model"""

    by_acronym = {name.rsplit("_", 1)[-1]: name for name, _, _ in masks}
    groups: Dict[Tuple[str, str, int], List] = {}
    for operation in command.operations:
        selectors = operation.selectors
        if len(selectors) != 2 or not all(k in by_acronym for k in selectors):
            continue
        outer, inner = list(selectors)
        groups.setdefault((outer, inner, selectors[outer]), []).append(operation)

    lines: List[str] = []
    for (outer, inner, at), operations in groups.items():
        mask = f"{by_acronym[inner]}_{outer}{at:X}H"
        constants = [
            (
                f"{mask}_{identifier(o.name)}",
                o.selectors[inner],
                f"{o.name} ({senfd.query.cite(o.source)})",
            )
            for o in operations
            if o.name
        ]
        lines += enum_lines(
            mask.lower(),
            f"The values of {inner} of {command.name} when {outer} is {at:X}h",
            constants,
        )
    return lines


def split_wide(dwords: List[Dwords]) -> List[Dwords]:
    """'dwords' with each description of more than two dwords, as the whole
    SQE of a Fabrics command, split into a description of the dword or the two
    dwords that each of its fields lies in, since a mask is at most 64 bits.
    A field across more than two dwords, as SGL1, has no description."""

    split: List[Dwords] = []
    for d in dwords:
        if d.upper - d.lower < 2:
            split.append(d)
            continue
        spans: Dict[Tuple[int, int], List[Bits]] = {}
        for f in d.fields:
            if f.lower is None or f.upper is None:
                continue
            first, last = d.lower + f.lower // 32, d.lower + f.upper // 32
            if last - first < 2:
                spans.setdefault((first, last), []).append(f)
        for (first, last), fields in sorted(spans.items()):
            by = 32 * (first - d.lower)
            split.append(
                d.model_copy(
                    update={
                        "lower": first,
                        "upper": last,
                        "fields": shifted(fields, by),
                    }
                )
            )
    return split


def no_masks(named: List[str], what: str) -> str:
    """The line of a comment that names the fields 'named' of 'what', a pair of
    dwords that starts at an odd dword, which have no masks"""

    verb = "has no mask" if len(named) == 1 else "have no masks"
    return (
        f" * {', '.join(named)} of {what} {verb}: a pair of dwords that starts at"
        " an odd dword is no 64-bit value"
    )


def beyond_notes(beyond: List[Tuple[Bits, str, Bits]]) -> List[str]:
    """The lines of a comment that name the fields 'beyond' that masks_of()
    gives, which have no masks, by the field they are within, with their bits
    as the data model gives them"""

    within: Dict[Tuple[str, str, int], List[str]] = {}
    for field, of, sub in beyond:
        width = field.upper - field.lower + 1
        named = f"{sub.acronym} ({bits_text(sub.lower, sub.upper)})"
        subs = within.setdefault((field.acronym, of, width), [])
        if named not in subs:
            subs.append(named)
    lines = []
    for (acronym, of, width), subs in within.items():
        if len(subs) == 1:
            listed, verb = subs[0], "has no mask: the data model gives it"
        else:
            listed = f"{', '.join(subs[:-1])} and {subs[-1]}"
            verb = "have no masks: the data model gives them"
        lines.append(
            f" * {listed} within {acronym} of {of} {verb} bits beyond the"
            f" {width} bits of {acronym}"
        )
    return lines


def dword_masks(
    enums: ValueEnums,
    doc: str,
    prefix: str,
    sqe: List[Dwords],
    cqe: List[Dwords],
    of: str = "",
    unnamed: Optional[List[str]] = None,
) -> Tuple[List[Tuple[str, int, str]], List[str], List[str]]:
    """The masks of the fields of the dwords 'sqe' of a command and 'cqe' of
    its completion queue entry, named by 'prefix' and the dword, such as CDW11
    or CQE_DW0, the enums of their values, and the lines of a comment that
    name the fields without masks: those of a pair of dwords that starts at an
    odd dword, and those that beyond_notes() names. 'of' follows the dword in
    the doc of a mask, such as " of Enable Directive". 'unnamed' is as
    masks_of() takes it."""

    masks: List[Tuple[str, int, str]] = []
    values: List[str] = []
    notes: List[str] = []
    for part, word, dwords in (
        ("CDW", "Command Dword", split_wide(sqe)),
        ("CQE_DW", "Completion Queue Entry Dword", split_wide(cqe)),
    ):
        for lower, last in sorted({(d.lower, d.upper) for d in dwords}):
            figures = described(dwords, lower, last)
            if last == lower + 1 and lower % 2:
                named = [g.acronym for f in figures for g in f.fields if g.acronym]
                if named:
                    what = f"{word}s {lower} and {last}{of}"
                    notes.append(no_masks(list(dict.fromkeys(named)), what))
                continue
            dword = f"{lower}" if lower == last else f"{lower}_{last}"
            member = f"{prefix}_{part}{dword}"
            fields = [
                (f.fields, f.source, f.condition, f.selected_when) for f in figures
            ]
            what = (
                f"{word} {lower}{of}"
                if lower == last
                else f"{word}s {lower} and {last}{of}, as one 64-bit value"
            )
            beyond: List[Tuple[Bits, str, Bits]] = []
            these = masks_of(
                what, member, fields, 32 * (last - lower + 1), True, beyond, unnamed
            )
            notes += beyond_notes(beyond)
            masks += these
            values += enums.of(doc, member, fields, these)
    return masks, values, notes


def operation_masks(
    enums: ValueEnums,
    doc: str,
    command,
    upper: str,
    unnamed: Optional[List[str]] = None,
) -> Tuple[List[Tuple[str, int, str]], List[str], List[str]]:
    """The masks of the fields of the dwords of the operations of 'command',
    a command or a feature, named by 'upper', the operation and the dword, the
    enums of their values, and the lines of a comment that name the fields
    without masks, as dword_masks() gives them with 'unnamed'"""

    masks: List[Tuple[str, int, str]] = []
    values: List[str] = []
    notes: List[str] = []
    for o in command.operations:
        if not o.name or not (o.sqe or o.cqe):
            continue
        name = f"{upper}_{identifier(o.name)}"
        these, more, odd = dword_masks(
            enums, doc, name, o.sqe, o.cqe, f" of {o.name}", unnamed
        )
        masks += these
        values += more
        notes += odd
    return masks, values, notes


def status_tables(spec: SpecDocument) -> List:
    """The tables of status values that are not of one command or operation:
    those of the status code types, and the tables that list the codes of a
    type. A table of a command is one that the command cites, or one whose
    codes another table of its document, type and scope lists as well, as the
    codes of the Grant Host Access operation of Base 2.4 Figure 403."""

    cited = {
        figure_of(source)
        for command_set in spec.command_sets.values()
        for command in command_set.commands.values()
        for source in command.status_values
    }
    tables = [
        values
        for values in spec.values
        if values.kind == "status_values"
        and values.qualifier
        and figure_of(values.source) not in cited
    ]

    def scope(values) -> Tuple:
        return (doc_of(values.source), values.status_code_type, values.status_scope)

    def codes(values) -> Set[Optional[int]]:
        return {number_of(entry) for entry in values.entries} - {None}

    return [
        values
        for values in tables
        if values.status_code_type is None
        or not any(
            other is not values
            and scope(other) == scope(values)
            and codes(values) < codes(other)
            for other in tables
        )
    ]


class SpecHeaders:
    """Writes the spec headers from the data model"""

    NAME = "xnvme_spec"

    def __init__(self, output: Path, options: Dict[str, str]):
        self.output = Path(output) / self.NAME
        self.output.mkdir(parents=True, exist_ok=True)
        self.options = dict(options)

    def commands(self, spec: SpecDocument) -> List[Tuple[str, Command]]:
        """The commands that the option 'commands' selects, each with the
        abbreviation of its command set"""

        wanted = [w.strip() for w in self.options.get("commands", "").split(",") if w]
        if wanted == ["none"]:
            return []
        chosen = []
        for set_key, command_set in spec.command_sets.items():
            for cmd_key, command in command_set.commands.items():
                if not wanted or any(
                    w in (set_key, f"{set_key}:{cmd_key}") for w in wanted
                ):
                    chosen.append((set_key_of(command_set), command))
        return chosen

    def structures(self, spec: SpecDocument) -> List[Structure]:
        """The byte layouts that the option 'structures' selects; by default
        those whose kind is not in SKIPPED"""

        wanted = [w.strip() for w in self.options.get("structures", "").split(",") if w]
        if wanted == ["none"]:
            return []
        chosen = []
        for structure in spec.structures:
            if structure.unit != "bytes":
                continue
            key = f"{doc_of(structure.source)}:{structure.key}"
            if (
                not wanted
                and structure.kind not in SKIPPED
                or structure.key in wanted
                or key in wanted
            ):
                chosen.append(structure)
        return chosen

    def write_structures(
        self,
        spec: SpecDocument,
        enums: ValueEnums,
        stem: str,
        taken: set,
        out: Dict[str, List[str]],
    ) -> None:
        """Writes the structs of structures() into 'out' by document. The
        struct of an array element or of a linked layout comes before the
        struct that uses it."""

        by_figure: Dict[Tuple[str, int], Structure] = {
            figure_of(s.source): s for s in spec.structures if s.unit == "bytes"
        }
        bit_layouts = layouts_of_bits(spec)
        titles: Dict[Tuple[str, int], str] = {}
        for layout in spec.structures:
            titles.setdefault(figure_of(layout.source), layout.title)
        flexible = self.options.get("flexible", "0") not in ("0", "", "no", "false")
        # The name of the struct of each structure, by its figure and key
        written: Dict[Tuple[str, int, str], str] = {}
        entries_written: set = set()

        def at(structure: Structure) -> Tuple[str, int, str]:
            return (*figure_of(structure.source), structure.key)

        def record_of(structure: Structure, field: Bits, size: int) -> Optional[str]:
            """Writes the struct of the layout that 'field' of 'structure' links
            to, such as the Generic SGL Descriptor Format of SGL1, and returns
            its name. Returns None when the layout is of another document, is
            not 'size' bytes, or has 'field' as its only field, as the Host
            Identifier of Base Figure 538 has only HOSTID."""

            linked = (
                by_figure.get(figure_of(field.structure)) if field.structure else None
            )
            if (
                linked is None
                or linked is structure
                or doc_of(linked.source) != doc_of(structure.source)
                or linked.size != size
                or [g.acronym for g in linked.fields] == [field.acronym]
            ):
                return None
            return write(linked) or None

        def write_bits(structure: Structure, nbytes: int) -> Optional[str]:
            """Writes the struct of an entry of 'nbytes' bytes that the bit
            layout 'structure' defines, such as the Power State Descriptor,
            with the integers that senfd.layout.words() gives"""

            if at(structure) in written:
                return written[at(structure)]
            doc = doc_of(structure.source)
            units = units_of(structure.fields, nbytes)
            name = f"{stem}_{doc}_{structure.key}"
            if not units or name in taken:
                written[at(structure)] = ""
                return None
            taken.add(name)
            written[at(structure)] = name
            upper = identifier(name)
            body: List[str] = []
            masks: List[Tuple[str, int, str]] = []
            values: List[str] = []
            nested: List[str] = []
            unnamed: List[str] = []
            for unit in units:
                count = "" if unit.count == 1 else f"[{unit.count}]"
                named = [g for g in unit.fields if g.acronym]
                where = (
                    f"byte {unit.offset}"
                    if unit.size * unit.count == 1
                    else f"bytes {unit.offset + unit.size * unit.count - 1}:{unit.offset}"
                )
                if not named:
                    count = "" if unit.size == 1 else f"[{unit.size}]"
                    body.append(
                        f"\tuint8_t rsvd{unit.offset}{count}; ///< Reserved, {where}"
                    )
                    continue
                field, *more = named
                whole = not more and (field.lower, field.upper) == (
                    0,
                    8 * unit.size - 1,
                )
                member = "_".join(part_of(g.acronym).lower() for g in named)
                prefix = f"{upper}_{identifier(member)}"
                # A field alone in its unit from bit 0 is the member. yace
                # prints the value of a field under the name VAL, so a field
                # with the acronym VAL has no mask
                bare = not more and (
                    field.lower == 0 or identifier(field.acronym) == "VAL"
                )
                these = (
                    []
                    if whole or bare
                    else masks_of(
                        member,
                        prefix,
                        [(named, structure.source)],
                        8 * unit.size,
                    )
                )
                if bare and not whole:
                    where = f"{bits_text(field.lower, field.upper)} of {where}"
                what = (
                    field.name
                    if whole
                    else "; ".join(g.name or g.acronym for g in named)
                )
                see = f", see {prefix}_*" if these else ""
                ctype = TYPES.get(unit.size, "uint8_t")
                body.append(f"\t{ctype} {member}{count}; ///< {what}, {where}{see}")
                hidden = (
                    hidden_masks(
                        member,
                        prefix,
                        [(named, structure.source)],
                        8 * unit.size,
                        these,
                        unnamed,
                    )
                    if these
                    else []
                )
                nested += [text for _, _, text in hidden]
                masks += these
                values += enums.of(
                    doc,
                    prefix,
                    (
                        [([field], structure.source)]
                        if bare or whole
                        else [(named, structure.source)]
                    ),
                    these,
                    hidden,
                )
            lines = out.setdefault(doc, [])
            what = f"{structure.title} ({senfd.query.cite(structure.source)}), one entry of {nbytes} bytes"
            doc_lines = [what, *without_masks([], nested, unnamed)]
            lines += packed("struct", name, doc_lines, body) + defines(masks) + values
            return name

        def write_cases(structure: Structure, struct: str, m: Member) -> Optional[str]:
            """Writes the union of the cases of the member 'm' of the struct
            'struct', as for DPTR of the Common Command Format. A case of one
            field of all the bytes is a member of the union. A case of other
            fields is a packed struct named by their acronyms. Raises
            ValueError when those fields do not fill the bytes of 'm'."""

            cases = alternatives_of(m.field) if m.field and m.size else []
            name = f"{struct}_{m.name}"
            if not cases or name in taken:
                return None
            taken.add(name)
            doc = doc_of(structure.source)
            cite = senfd.query.cite(structure.source)
            lines = out.setdefault(doc, [])

            def member_of(g: Bits) -> str:
                return part_of(g.acronym).lower() if g.acronym else member_name(g.name)

            def ctype_of(g: Bits) -> Tuple[str, str]:
                size = g.upper - g.lower + 1
                if g.byte_order == "big-endian" or g.encoding:
                    return "uint8_t", f"[{size}]"
                if size in TYPES:
                    return TYPES[size], ""
                record = record_of(structure, g, size)
                if record:
                    return f"struct {record}", ""
                return "uint8_t", f"[{size}]"

            body: List[str] = []
            for case, fields in cases:
                when = condition_of(case)
                whole = len(fields) == 1 and (fields[0].lower, fields[0].upper) == (
                    0,
                    m.size - 1,
                )
                member = "_".join(member_of(g) for g in fields)
                what = " and ".join(g.name or g.acronym for g in fields)
                if whole:
                    ctype, count = ctype_of(fields[0])
                    body.append(f"\t{ctype} {member}{count}; ///< {what}, when {when}")
                    continue
                record = f"{name}_{member}"
                taken.add(record)
                ends = [g.upper + 1 for g in fields]
                if [g.lower for g in fields] != [0, *ends[:-1]] or ends[-1] != m.size:
                    raise ValueError(
                        f"{structure.title}: the fields of {m.doc} when {when} do"
                        " not fill its bytes"
                    )
                inner: List[str] = []
                for g in fields:
                    ctype, count = ctype_of(g)
                    inner.append(
                        f"\t{ctype} {member_of(g)}{count}; ///< {g.name or g.acronym},"
                        f" bytes {g.upper}:{g.lower}"
                    )
                lines += packed(
                    "struct",
                    record,
                    [f"{what} of {m.doc} ({cite}), when {when}"],
                    inner,
                )
                body.append(f"\tstruct {record} {member}; ///< {what}, when {when}")
            selectors = ", ".join(dict.fromkeys(case.selector for case, _ in cases))
            what = f"{m.doc} ({cite}), of {m.size} bytes, as {selectors} selects"
            lines += packed("union", name, [what], body)
            return name

        def write(structure: Structure) -> Optional[str]:
            if at(structure) in written:
                return written[at(structure)]
            doc = doc_of(structure.source)

            def element_of(
                base: str,
                size: Optional[int],
                link: Optional[Source] = None,
                record: Sequence[Bits] = (),
            ) -> Optional[str]:
                linked = by_figure.get(figure_of(link)) if link else None
                # A layout of one field of all its bytes, as the Temperature
                # Sensor of SMART / Health Information, is no struct
                alone = linked is not None and [
                    (g.lower, g.upper) for g in linked.fields if g.role != "reserved"
                ] == [(0, (linked.size or 0) - 1)]
                if linked is not None and linked is not structure and not alone:
                    if size is None or linked.size == size:
                        return write(linked)
                # An entry of an array can give its fields as rows of bytes,
                # as a Source Range of Copy does
                if record and all(
                    g.unit == "bytes"
                    and g.upper is not None
                    and (size is None or g.upper < size)
                    for g in record
                ):
                    return write(
                        structure.model_copy(
                            update={
                                "title": base,
                                "name": base,
                                "key": alias(base),
                                "kind": "other",
                                "size": size,
                                "fields": list(record),
                            }
                        )
                    )
                return None

            extended = ""
            see = ""
            if structure.extends:
                cite = senfd.query.cite(structure.extends)
                title = titles.get(figure_of(structure.extends))
                extended = f"{title} ({cite})" if title else cite
                see = extended
                if structure.extends_field:
                    extended = f"{structure.extends_field} of {extended}"
            members, left = layout_of(
                structure,
                element_of,
                lambda field, size: record_of(structure, field, size),
                flexible,
                see,
            )
            if not [m for m in members if m.count != 0]:
                written[at(structure)] = ""
                return None
            kind = "log_" if structure.kind == "log_page" else ""
            name = f"{stem}_{doc}_{kind}{structure.key}"
            if name in taken:
                name = f"{name}_figure{structure.source.figure_nr}"
            if name in taken:
                written[at(structure)] = ""
                return None
            taken.add(name)
            written[at(structure)] = name
            upper = identifier(name)
            body: List[str] = []
            masks: List[Tuple[str, int, str]] = []
            values: List[str] = []
            follows: List[str] = []
            # The fields without masks, of an array of bytes and within a field
            # that has a mask, by the docs their masks would have
            in_bytes: List[str] = []
            nested: List[str] = []
            unnamed: List[str] = []

            def entry_of(layout: Structure, nbytes: int) -> Optional[str]:
                """The prefix of the masks of the fields of 'layout', the bit
                layout of an entry of 'nbytes' bytes, named by the layout, or
                None where it has none. The masks and the enums of their values
                go with the first struct that has such entries, as ACS and IOCS
                of the Commands Supported and Effects log page share them."""

                prefix = identifier(f"{stem}_{doc}_{layout.key}")
                entry = [(layout.fields, layout.source)]
                these = masks_of(layout.name, prefix, entry, 8 * nbytes)
                if prefix not in entries_written:
                    entries_written.add(prefix)
                    hidden = hidden_masks(
                        layout.name, prefix, entry, 8 * nbytes, these, unnamed
                    )
                    masks.extend(these)
                    values.extend(enums.of(doc, prefix, entry, these, hidden))
                    nested.extend(text for _, _, text in hidden)
                return prefix if these else None

            for m in members:
                if m.count == 0 and not flexible:
                    entries = m.field.array if m.field else None
                    # The range of the first field, as "n:16", defines the
                    # variables of the ranges of the fields after it
                    start = formula_range(m.field) if m.field else None
                    if entries is None:
                        size = (
                            f", at {start}"
                            if start
                            else ", of as many bytes as the data says"
                        )
                        follows.append(
                            f"Followed, from byte {m.lower}, by {m.doc}{size}"
                        )
                        continue
                    link = m.field.structure
                    layout = bit_layouts.get(figure_of(link)) if link else None
                    nbytes = integer_of(layout) if layout else 0
                    of = ""
                    if m.ctype.startswith("struct"):
                        of = f", each a '{m.ctype}'"
                    elif layout and nbytes and (prefix := entry_of(layout, nbytes)):
                        of = f", each with {prefix}_* in its bytes {nbytes - 1}:0"
                    elif entries.stride:
                        of = f", each of {entries.stride} byte"
                        of += "s" if entries.stride > 1 else ""
                    # A field with no index is the whole list, not its first
                    # entry
                    first = ""
                    if start:
                        first = f", the first at {start}"
                        if entries.index is None:
                            first = f", at {start}"
                    follows.append(
                        f"Followed, from byte {m.lower}, by {entries_of(m.doc)}"
                        f"{first}{counted(entries)}{of}"
                    )
                    continue
                union = write_cases(structure, name, m)
                if union:
                    m = replace(m, ctype=f"union {union}", count=None, fields=[])
                where = (
                    f"byte {m.lower}"
                    if m.size == 1
                    else (
                        f"bytes {m.lower + m.size - 1}:{m.lower}"
                        if m.size
                        else f"from byte {m.lower}"
                    )
                )
                link = m.field.structure if m.field else None
                described_by = bit_layouts.get(figure_of(link)) if link else None
                figures = (
                    m.figures
                    if m.figures is not None
                    else [
                        (
                            m.fields,
                            link if described_by and m.fields else structure.source,
                        )
                    ]
                )
                if m.stride and described_by and described_by.size == m.stride:
                    if m.stride in TYPES and m.ctype == TYPES[m.stride]:
                        # A scalar element keeps its type, and its masks are
                        # named by the entry, as the LBA Format of LBAF of
                        # Identify Namespace
                        prefix = entry_of(described_by, m.stride)
                        if prefix:
                            m = replace(m, doc=f"{m.doc} of {prefix}_*")
                        figures = []
                    else:
                        element = write_bits(described_by, m.stride)
                        if element:
                            m = replace(
                                m, ctype=f"struct {element}", count=m.size // m.stride
                            )
                # The masks of an array of integers apply to its first element
                bitwise = bool(m.count and m.fields)
                these = masks_of(
                    f"{m.name}[0]" if bitwise else m.name,
                    f"{upper}_{identifier(m.name)}",
                    figures,
                    8 * (m.stride if bitwise else m.size),
                )
                hidden = hidden_masks(
                    f"{m.name}[0]" if bitwise else m.name,
                    f"{upper}_{identifier(m.name)}",
                    figures,
                    8 * (m.stride if bitwise else m.size),
                    these,
                    unnamed,
                )
                nested += [text for _, _, text in hidden]
                unmasked = (
                    [(m.unmasked, link if described_by else structure.source)]
                    if m.unmasked
                    else []
                )
                if unmasked:
                    of_bytes = masks_of(
                        m.name,
                        f"{upper}_{identifier(m.name)}",
                        unmasked,
                        8 * m.size,
                        nested=True,
                        unnamed=unnamed,
                        acronyms=True,
                    )
                    in_bytes += [text for _, _, text in of_bytes]
                    hidden += of_bytes
                see = f", see {upper}_{identifier(m.name)}_*" if these else ""
                count = "" if m.count is None else f"[{m.count or ''}]"
                text = f"{m.doc}, {where}" if m.doc else where
                body.append(f"\t{m.ctype} {m.name}{count}; ///< {text}{m.after}{see}")
                masks += these
                whole = [([m.whole], structure.source)] if m.whole and not these else []
                values += enums.of(
                    doc,
                    f"{upper}_{identifier(m.name)}",
                    figures + whole + unmasked,
                    these,
                    hidden,
                )
            lines = out.setdefault(doc, [])
            what = [f"{structure.title} ({senfd.query.cite(structure.source)})"]
            if extended:
                what += ["", f"Extends {extended}"]
            if follows:
                what += ["", *follows]
            if left:
                what += ["", "Not members:", *left]
            what += without_masks(in_bytes, nested, unnamed)
            lines += packed("struct", name, what, body) + defines(masks) + values
            return name

        for structure in self.structures(spec):
            write(structure)

    def write_enums(
        self, spec: SpecDocument, stem: str, out: Dict[str, List[str]]
    ) -> None:
        """Writes into 'out', by document, the enums of the opcodes of each
        command set, of the feature identifiers, of the log page identifiers,
        and of the status codes, one enum for each type and scope"""

        upper = identifier(stem)
        opcodes: Dict[Tuple[str, str], List[Tuple[str, int, str]]] = {}
        titles = {set_key_of(s): s.title or s.name for s in spec.command_sets.values()}
        for set_key, command in self.commands(spec):
            name = identifier(f"{stem}_{set_key}_{alias(command.name)}")
            doc = doc_of(command.source)
            opcodes.setdefault((doc, set_key), []).append(
                (
                    f"{name}_OPCODE",
                    command.opcode,
                    f"{command.name} ({senfd.query.cite(command.source)})",
                )
            )
        for (doc, set_key), constants in opcodes.items():
            out.setdefault(doc, []).extend(
                enum_lines(
                    f"{stem}_{set_key}_opcode",
                    f"The opcodes of the commands of the {titles[set_key]}",
                    constants,
                )
            )
        identified: List[Tuple[str, List[Union[Feature, LogPage]]]] = [
            ("feature", list(spec.features.values())),
            ("log_page", list(spec.log_pages.values())),
        ]
        for what, items in identified:
            by_doc: Dict[str, List[Tuple[str, int, str]]] = {}
            for item in sorted(items, key=lambda i: i.identifier):
                doc = doc_of(item.source)
                by_doc.setdefault(doc, []).append(
                    (
                        f"{upper}_{doc.upper()}_{identifier(what)}_"
                        f"{identifier(item.name)}",
                        item.identifier,
                        f"{item.name} ({senfd.query.cite(item.source)})",
                    )
                )
            for doc, constants in by_doc.items():
                out.setdefault(doc, []).extend(
                    enum_lines(
                        f"{stem}_{doc}_{what}_identifier",
                        f"The identifiers of the {what.replace('_', ' ')}s",
                        constants,
                    )
                )
        statuses: Dict[Tuple[str, str], List[Tuple[str, int, str]]] = {}
        described: Dict[Tuple[str, str], str] = {}
        for values in status_tables(spec):
            doc = doc_of(values.source)
            # The status codes of the Fabrics commands give the values of the
            # other commands other meanings, so they have an enum of their own
            what = alias(values.qualifier)
            fabrics = values.status_scope == "fabrics"
            if values.status_code_type is not None:
                what += "_fabrics_status" if fabrics else "_status"
            elif what != "status_code_type":
                what += "_status"
            key = (doc, what)
            name = identifier(f"{stem}_{doc}_{what}")
            described[key] = (
                f"The status codes of the type {values.qualifier}"
                + (", of the Fabrics commands" if fabrics else "")
                if values.status_code_type is not None
                else f"The values of {values.qualifier}"
            )
            for entry in values.entries:
                value = number_of(entry)
                if value is None or not entry.name or entry.role == "reserved":
                    continue
                statuses.setdefault(key, []).append(
                    (
                        f"{name}_{identifier(entry.name)}",
                        value,
                        f"{entry.name} ({senfd.query.cite(values.source)})",
                    )
                )
        for (doc, what), constants in statuses.items():
            out.setdefault(doc, []).extend(
                enum_lines(f"{stem}_{doc}_{what}", described[(doc, what)], constants)
            )

    def write_features(
        self,
        spec: SpecDocument,
        enums: ValueEnums,
        stem: str,
        out: Dict[str, List[str]],
    ) -> None:
        """Writes into 'out', by document, the masks and value enums of the
        fields of each feature in the dwords of Set Features and in Dword 0 of
        the completion queue entry. The fields of an operation of a feature,
        as Lockdown Persistence Personality of Configurable Device Personality,
        are named by the operation as well."""

        upper = identifier(stem)
        for feature in sorted(spec.features.values(), key=lambda f: f.identifier):
            doc = doc_of(feature.source)
            prefix = f"{upper}_{doc.upper()}_FEATURE_{identifier(feature.name)}"
            unnamed: List[str] = []
            masks, values, notes = dword_masks(
                enums, doc, prefix, feature.sqe, feature.cqe, unnamed=unnamed
            )
            of_operations, more, odd = operation_masks(
                enums, doc, feature, prefix, unnamed
            )
            masks += of_operations
            values += more
            notes += odd
            notes = comment_notes(notes, without_masks([], [], unnamed))
            if not masks and not notes:
                continue
            lines = out.setdefault(doc, [])
            lines += [
                "/*",
                f" * The fields of the feature {feature.name}"
                f" ({senfd.query.cite(feature.source)})",
            ]
            if notes:
                lines += [" *", *notes]
            lines += [" */"]
            lines += (defines(masks) or [""]) + values

    def write_registers(
        self,
        spec: SpecDocument,
        enums: ValueEnums,
        stem: str,
        out: Dict[str, List[str]],
    ) -> None:
        """Writes into 'out', by document, one packed struct with each register
        at its offset, one for the registers of each PCI capability, the masks
        of the fields of each register, and the enums of their named values.
        Raises ValueError for a register whose bytes another register covers."""

        if self.options.get("registers", "") == "none":
            return
        groups: Dict[Tuple[str, Optional[str]], List[Structure]] = {}
        for structure in spec.structures:
            if structure.kind == "register" and structure.acronym:
                key = (doc_of(structure.source), structure.offset_base)
                groups.setdefault(key, []).append(structure)
        for (doc, base), registers in groups.items():
            within = f"{base.lower()}_" if base else ""
            struct = f"{stem}_{doc}_{within}registers"
            upper = identifier(struct)
            body: List[str] = []
            masks: List[Tuple[str, int, str]] = []
            values: List[str] = []
            notes: List[str] = []
            in_bytes: List[str] = []
            nested: List[str] = []
            unnamed: List[str] = []
            offset = 0
            placed = sorted(
                (r for r in registers if r.offset is not None), key=lambda r: r.offset
            )
            for register in placed + [r for r in registers if r.offset is None]:
                name = register.acronym.lower()
                size = register.size or 0
                cite = senfd.query.cite(register.source)
                scalar = size in TYPES
                # yace takes a mask to be of a member, so a register that is
                # no member has no masks
                these = (
                    masks_of(
                        name,
                        f"{upper}_{identifier(name)}",
                        [(register.fields, register.source)],
                        8 * size,
                    )
                    if scalar and register.offset is not None
                    else []
                )
                masks += these
                figures = [(register.fields, register.source)]
                prefix = f"{upper}_{identifier(name)}"
                if register.offset is None:
                    values += enums.of(doc, prefix, figures, these)
                    at = register.offset_formula or "an offset the data gives"
                    notes.append(f"{register.title} ({cite}): at {at}")
                    continue
                if scalar:
                    hidden = hidden_masks(
                        name, prefix, figures, 8 * size, these, unnamed
                    )
                    nested += [text for _, _, text in hidden]
                else:
                    hidden = masks_of(
                        name,
                        prefix,
                        figures,
                        8 * size,
                        nested=True,
                        unnamed=unnamed,
                        acronyms=True,
                    )
                    in_bytes += [text for _, _, text in hidden]
                values += enums.of(doc, prefix, figures, these, hidden)
                at = register.offset
                if at < offset:
                    raise ValueError(
                        f"{register.title}: of bytes that another register has"
                    )
                if at > offset:
                    gap = at - offset
                    count = "" if gap == 1 else f"[{gap}]"
                    where = (
                        f"{offset:X}h" if gap == 1 else f"{offset:X}h to {at - 1:X}h"
                    )
                    body.append(
                        f"\tuint8_t rsvd{offset:x}{count}; ///< Reserved, {where}"
                    )
                see = f", see {upper}_{identifier(name)}_*" if these else ""
                ctype = TYPES[size] if scalar else "uint8_t"
                count = "" if scalar else f"[{size}]"
                body.append(
                    f"\t{ctype} {name}{count}; ///< {register.title} ({cite}), "
                    f"at {at:X}h{see}"
                )
                offset = at + size
            if base:
                what = [f"The registers of {base}, at their offsets from {base}"]
            else:
                what = [f"The registers of the {doc} specification, at their offsets"]
            if notes:
                what += ["", "Not members:", *notes]
            what += without_masks(in_bytes, nested, unnamed)
            lines = out.setdefault(doc, [])
            if body:
                lines += packed("struct", struct, what, body)
            lines += (defines(masks) or [""]) + values

    def generate(self):
        spec = load(self.options.get("spec"))
        stem = alias(self.options.get("prefix", "xnvme_spec"))
        enums = ValueEnums(spec, stem)

        identifiers: Dict[str, List[str]] = {}
        structs: Dict[str, List[str]] = {}
        commands: Dict[str, List[str]] = {}
        taken = {
            f"{stem}_{set_key}_{alias(command.name)}"
            for set_key, command in self.commands(spec)
        }
        self.write_enums(spec, stem, identifiers)
        self.write_structures(spec, enums, stem, taken, structs)
        registers: Dict[str, List[str]] = {}
        self.write_registers(spec, enums, stem, registers)
        features: Dict[str, List[str]] = {}
        self.write_features(spec, enums, stem, features)

        for set_key, command in self.commands(spec):
            doc = doc_of(command.source)
            struct = f"{stem}_{set_key}_{alias(command.name)}"
            upper = identifier(struct)
            notes: List[str] = []
            masks: List[Tuple[str, int, str]] = []
            values: List[str] = []
            unnamed: List[str] = []
            for member, lower, last in members_of(command):
                width = 32 * (last - lower + 1)
                figures = described(command.sqe, lower, last)
                what = (
                    f"Command Dword {lower}"
                    if lower == last
                    else f"Command Dwords {lower} and {last}, as one 64-bit value"
                )
                fields = [
                    (f.fields, f.source, f.condition, f.selected_when) for f in figures
                ]
                beyond: List[Tuple[Bits, str, Bits]] = []
                these = masks_of(
                    what,
                    f"{upper}_{identifier(member)}",
                    fields,
                    width,
                    True,
                    beyond,
                    unnamed,
                )
                notes += beyond_notes(beyond)
                masks += these
                values += enums.of(doc, f"{upper}_{identifier(member)}", fields, these)
                conditions = list(dict.fromkeys(f.condition for f in figures))
                if len(conditions) > 1 and conditions[0]:
                    notes.append(
                        f" * {what} is described if "
                        + ", or if ".join(conditions)
                        + "; the doc of a mask names the conditions of the"
                        " descriptions that have its field, unless they cover"
                        " every value"
                    )
            for lower, last in sorted({(d.lower, d.upper) for d in command.sqe}):
                if last != lower + 1 or not lower % 2:
                    continue
                figures = described(command.sqe, lower, last)
                named = [g.acronym for f in figures for g in f.fields if g.acronym]
                if named:
                    what = f"Command Dwords {lower} and {last}"
                    notes.append(no_masks(list(dict.fromkeys(named)), what))
            completion, more, odd = dword_masks(
                enums, doc, upper, [], command.cqe, unnamed=unnamed
            )
            masks += completion
            values += more
            notes += odd
            operated, operated_values, odd = operation_masks(
                enums, doc, command, upper, unnamed
            )
            notes += odd
            notes = comment_notes(notes, without_masks([], [], unnamed))
            lines = commands.setdefault(doc, [])
            lines += [
                "/*",
                f" * The fields of {command.name}, by the dwords of the command",
            ]
            if notes:
                lines += [" *", *notes]
            lines += [" */"]
            lines += defines(masks + operated) or [""]
            lines += values
            lines += operation_enums(command, masks)
            lines += operated_values

        headers: List[Path] = []
        for doc in dict.fromkeys(
            [*identifiers, *structs, *registers, *commands, *features]
        ):
            name = f"{stem}_{doc}"
            guard = f"LIB{identifier(name)}_H"
            text = [
                "/**",
                f" * What the {doc} specification defines, by {stem}",
                " *",
                f" * Written from the data model of senfd {spec.meta.version}, by"
                " toolbox/xnvme_spec_headers.py;",
                " * each name and mask is as the figure it cites gives it. Do not",
                " * edit: see the contributor docs on producing the spec headers.",
                " *",
                f" * @file lib{name}.h",
                " */",
                f"#ifndef {guard}",
                f"#define {guard}",
                "",
                "#include <stdint.h>",
                "",
                *identifiers.get(doc, []),
                *structs.get(doc, []),
                *registers.get(doc, []),
                *commands.get(doc, []),
                *features.get(doc, []),
                f"#endif /* {guard} */",
                "",
            ]
            path = self.output / f"lib{name}.h"
            path.write_text("\n".join(text), encoding="utf-8")
            headers.append(path)
        return generated(headers, [])
