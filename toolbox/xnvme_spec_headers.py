#!/usr/bin/env python3
"""
The spec headers of xNVMe, from senfd's data model of the NVMe specifications

This file is a generator for yace. yace runs the generator and parses the
headers that the generator writes, as yace parses any header. The contributor
docs have a tutorial on producing the spec headers:

    yace --generate toolbox/xnvme_spec_headers.py:SpecHeaders \\
         --generator-option xnvme_spec.spec=merged.spec.document.json \\
         --output /tmp/spec --strict --emit capi

The build does not run the generator. A contributor runs the generator on a
machine with the specification documents and commits the files that the
generator writes.

The generator writes a header for each document of the data model. Each header
is named by the abbreviation of its document, such as libxnvme_spec_base.h,
libxnvme_spec_nvm.h, libxnvme_spec_kv.h, libxnvme_spec_zns.h and
libxnvme_spec_tcp.h. No header refers to another header.

For each command, the generator writes the masks of the fields of its
dwords, named by the command, the dword and the acronym of the field, and
documented with their figure. A pair of dwords that the data model describes
with fields across both, such as CDW10 and CDW11 of Read, is one 64-bit value,
and its masks are named by both dwords. The generator writes no struct for a
command: union xnvme_cmd_sqe of libxnvme_cmd.h holds the dwords of every
command. The opcode of the command is in the enum of the
opcodes of its command set. The command preps of toolbox/xnvme_cmd_prep.py
make the commands, so the generator writes no function.

The generator has these options for the commands. 'spec' is the merged
specification document that senfd writes; the default is the one in senfd's
cache, as for senfd lint. 'prefix' is the prefix of every name; the default is
"xnvme_spec". 'commands' is a comma separated list of command sets, such as
"nvm", or of commands, such as "nvm:write"; the default is all commands, and
"none" writes no commands.

The identifiers of the features and of the log pages are enums as well, and
so are the status codes of each document, with one enum for each type of
status code.

The registers of each document are one packed struct, with each register at
its offset, as a driver maps the registers. The generator also writes the
masks of the fields of each register, and an enum of the values of each field
whose values the specification names. The option 'registers', set to "none",
leaves the registers out.

The generator also writes the byte layouts of the data model: the data
structures, the log pages, and the layouts without a named kind, such as the
PDUs of the transports. The generator leaves out the examples and the buffers
of data. Each layout is a packed struct, named by its document and its name.
The struct has the fields at their offsets, reserved members for the bytes
that no field covers, and masks for the bits of the fields. The option
'structures' selects other layouts, by name or by document and name, such as
"base:identify_controller", or is "none". The option 'flexible', set to "1",
writes the entries that follow a struct as a flexible array member, which yace
reports; by default the doc of the struct names the entries.

Some fields have no bits in the data model, but a description that says that
another figure defines the field; Command Dword 0 of the Common Command Format
refers to Figure 92 in this way. Such a field gets the masks of the layout of
that figure. Some entries have no byte layout in the data model, only layouts
of their dwords and of the fields of the dwords; the completion queue entry of
Base is one. Such an entry is a packed struct as well. Each half of a dword is
a 16-bit member named by its widest field, and the masks of the layout of that
field come before the masks of the dword.

An array whose entries another figure defines, such as LBAF of Identify
Namespace, which refers to the LBA Format of Figure 125, takes the layout of
that figure when the figure is as wide as an entry. An entry of 1, 2, 4 or 8
bytes keeps its integer type, and the masks are named by the entry, such as
XNVME_SPEC_NVM_LBA_FORMAT_LBADS. A larger entry, such as the Power State
Descriptor, is a packed struct of its own: a field of 1, 2, 4 or 8 bytes at
a byte is a member, and the fields that share the smallest aligned unit that
holds them are a member with masks.

A field of a command, of a structure or of an entry, whose values the
specification names, gets an enum of the values, as a field of a register
does. A name is the value's name without what it adds in parentheses; a value
for which the data model has only a sentence gets no constant. The operations
of a command that two fields select, such as DTYPE and DOPER of Directive
Send, are an enum of the second field for each value of the first.

When the data model describes a dword differently under each of several
conditions, as it does for CDW13 of Write, the generator writes the masks of
the description that has no condition, or of the first description when every
description has a condition. The doc of the member names the other
descriptions. A field gets no mask when it has no acronym, or when an earlier
field covers its bits.

"I/O" is one word, "io", in names, such as XNVME_SPEC_IO_FLUSH_OPCODE.
"""

import re
from dataclasses import dataclass
from dataclasses import field as default
from dataclasses import replace
from pathlib import Path
from types import SimpleNamespace
from typing import Dict, List, Optional, Tuple, Union

import senfd.query
from senfd.documents.spec import (
    Bits,
    Command,
    Dwords,
    Entry,
    Feature,
    LogPage,
    Source,
    SpecDocument,
    Structure,
)
from senfd.documents.spec import alias as senfd_alias
from senfd.documents.spec import value_names

# The dwords of the data pointer, which the data model describes as one field
# across the four dwords
POINTERS = {(6, 9): "Data Pointer (DPTR)"}

# The C type of a field, by its size in bytes; a field of any other size is an
# array of bytes
TYPES = {1: "uint8_t", 2: "uint16_t", 4: "uint32_t", 8: "uint64_t"}

# The kinds of structure that are not written unless the option 'structures'
# names them. Every other byte layout is written, including the layouts without
# a named kind, such as the PDUs of the transports.
SKIPPED = ("example", "data_buffer", "status_values")

# Figures whose rows cite fields of other structures. A first column, before
# the bytes, names the other structure, so the offsets of such a figure do not
# form one layout.
CITATIONS = (
    "nvm:reference_identify_controller_or_namespace_data_structures",
    "zns:size_and_capacity_fields",
)

# A field named by its index among similar fields, such as "Power State 0
# Descriptor"
INDEXED = re.compile(r"^(?P<head>.+?)\s+(?P<index>\d[\d,]*)(?P<tail>\s+\D.*)?$")


def words(text: str) -> str:
    """'text' with I/O as one word, so that names say IO rather than I_O"""

    return re.sub(r"\bI/O\b", "IO", text, flags=re.IGNORECASE)


def alias(name: str) -> str:
    """The key that senfd makes of 'name', with I/O as one word"""

    return senfd_alias(words(name))


def identifier(text: str) -> str:
    """'text' as a part of a C identifier, in capitals"""

    return re.sub(r"[^A-Za-z0-9]+", "_", words(text)).strip("_").upper()


def senfd_version(spec: Optional[SpecDocument] = None) -> str:
    """The version of senfd that wrote the data model 'spec', as the model says,
    else the version of the senfd installed"""

    written = getattr(getattr(spec, "meta", None), "version", None)
    if written:
        return written

    from importlib.metadata import PackageNotFoundError, version

    try:
        return version("senfd")
    except PackageNotFoundError:
        return "unknown"


def generated(headers: List[Path], sources: List[Path]):
    """The headers and sources that a generator wrote, in the type that yace
    expects: yace's own type when yace is installed, and a stand-in otherwise,
    so that the generator can be tested without yace"""

    try:
        from yace.generators import Generated

        return Generated(headers=headers, sources=sources)
    except ImportError:
        return SimpleNamespace(headers=headers, sources=sources)


def members_of(command: Command) -> List[Tuple[str, int, int, str]]:
    """The members of the struct of 'command': for each member, the name, the
    first and the last dword, and the C type. A pair of dwords that the data
    model describes with fields across both is one 64-bit member, where the
    pair is aligned to 64 bits"""

    pairs = {
        (d.lower, d.upper)
        for d in command.sqe
        if d.upper == d.lower + 1 and d.lower % 2 == 0
    }
    members = []
    dword = 0
    while dword < 16:
        if (dword, dword + 3) in POINTERS:
            members.append((f"cdw{dword}_{dword + 1}", dword, dword + 1, "uint64_t"))
            members.append(
                (f"cdw{dword + 2}_{dword + 3}", dword + 2, dword + 3, "uint64_t")
            )
            dword += 4
        elif (dword, dword + 1) in pairs:
            members.append((f"cdw{dword}_{dword + 1}", dword, dword + 1, "uint64_t"))
            dword += 2
        else:
            members.append((f"cdw{dword}", dword, dword, "uint32_t"))
            dword += 1
    return members


def described(
    command: Command, lower: int, upper: int
) -> Tuple[List[Dwords], List[str]]:
    """The descriptions of the dwords 'lower' to 'upper' that get masks, and
    the conditions of the descriptions that get none. The descriptions without
    a condition get masks; when every description has a condition, the
    descriptions of the first condition do"""

    these = [d for d in command.sqe if (d.lower, d.upper) == (lower, upper)]
    plain = [d for d in these if d.condition is None]
    conditions = list(dict.fromkeys(d.condition for d in these if d.condition))
    if plain:
        return plain, conditions
    if conditions:
        return [d for d in these if d.condition == conditions[0]], conditions[1:]
    return [], []


def masks_of(
    member: str,
    prefix: str,
    figures: List[Tuple[List[Bits], Source]],
    width: int,
) -> List[Tuple[str, int, str]]:
    """The masks of the fields of 'figures': for each mask, the name, the
    value and the doc. Each item of 'figures' is the fields of a figure and the
    source of the figure. A field gets no mask when it has no acronym, when its
    bits are a formula, when it is beyond 'width', when it is an alternative of
    a union other than the first, or when an earlier field covers its bits"""

    masks: List[Tuple[str, int, str]] = []
    taken = 0
    names: Dict[str, int] = {}
    for fields, source in figures:
        for field in fields:
            if not field.acronym or field.lower is None or field.upper is None:
                continue
            if field.upper >= width or field.union is not None and field.alternative:
                continue
            value = ((1 << (field.upper + 1)) - 1) ^ ((1 << field.lower) - 1)
            name = f"{prefix}_{identifier(field.acronym)}"
            if names.get(name) == value:
                continue
            if name in names or value & taken:
                continue
            names[name] = value
            taken |= value
            where = (
                f"bit {field.lower}"
                if field.lower == field.upper
                else f"bits {field.upper}:{field.lower}"
            )
            what = field.name or field.acronym
            of = member if member.startswith("Command Dword") else f"'{member}'"
            doc = f"{what} ({senfd.query.cite(source)}); {where} of {of}"
            masks.append((name, value, doc))
    return masks


# The documents and command sets by the abbreviations the specifications use
ABBREVIATIONS = {
    "i_o": "io",
    "key_value": "kv",
    "zoned_namespace": "zns",
    "subsystem_local_memory": "slm",
    "computational_programs": "cp",
    "management_interface": "mi",
    "pci_express_transport": "pcie",
    "rdma_transport": "rdma",
    "tcp_transport": "tcp",
    "nvme_over_pcie_transport": "pcie",
    "nvme_over_rdma_transport": "rdma",
    "nvme_over_tcp_transport": "tcp",
    "pcie_commands_using_an_out_of_band_mechanism": "pcie_commands",
}


def document_key(document: str) -> str:
    """The key of 'document', such as "base" or "kv": the abbreviation of the
    document if it has one, and otherwise the alias of its short name. The
    header of the document is named by this key"""

    key = alias(re.sub(r"\s+[\d.]+$", "", senfd.query.document_name(document)))
    return ABBREVIATIONS.get(key, key)


def set_key_of(key: str) -> str:
    """The abbreviation of the command set 'key' if it has one, such as "kv" for
    "key_value", and otherwise 'key'"""

    return ABBREVIATIONS.get(key, key)


def structure_name(structure: Structure) -> str:
    """The name of 'structure' without the command it belongs to and without
    its abbreviation: "Identify Controller" for "Identify - Identify
    Controller", and "PDU Common Header" for "PDU Common Header (CH)" """

    return re.sub(r"\s*\([^)]*\)$", "", structure.name.rsplit(" - ", 1)[-1])


def layouts_of_bits(spec: SpecDocument) -> Dict[Tuple[str, int], Structure]:
    """The bit layouts of 'spec', except the registers, by document and
    figure number, such as Command Dword 0 in Figure 92 of Base"""

    layouts: Dict[Tuple[str, int], Structure] = {}
    for structure in spec.structures:
        if structure.unit == "bits" and structure.kind != "register":
            key = (document_key(structure.source.document), structure.source.figure_nr)
            layouts.setdefault(key, structure)
    return layouts


# How the description of a field says that another figure defines its layout,
# such as "is defined in Figure 92", or a description that is only "Refer to
# Figure 24". A description that merely mentions a figure does not match.
DEFINED_BY = re.compile(
    r"^\s*Refer to Figure\s+(\d+)\s*\.|\bdefined (?:in|by) Figure\s+(\d+)\b",
    re.IGNORECASE,
)


def width_of(structure: Structure) -> int:
    """The number of bits of the bit layout 'structure'"""

    return 1 + max(
        (f.upper for f in structure.fields if f.upper is not None), default=-1
    )


def figure_referred(description: str) -> Optional[int]:
    """The figure that 'description' says defines the layout of the field, if
    'description' names exactly one"""

    figures = {a or b for a, b in DEFINED_BY.findall(description)}
    return int(figures.pop()) if len(figures) == 1 else None


def size_of(structure: Structure) -> Optional[int]:
    """The size of 'structure' in bytes, if every field has a numeric offset"""

    fields = [
        f for f in structure.fields if not (f.union is not None and f.alternative)
    ]
    if not fields or any(f.upper is None for f in fields):
        return None
    return max(f.upper for f in fields if f.upper is not None) + 1


def entry_of(structure: Structure) -> Optional[Tuple[Structure, int, List[int]]]:
    """
    The first entry of 'structure', the size of an entry in bytes, and the
    offsets of the entries that the figure shows, if the figure of 'structure'
    lays out the entries of a list, such as the Source Range entries of Copy;
    otherwise None

    Such a figure shows the first entries and the last entry, and its rows name
    the fields of each entry again. The function recognizes a list when the
    same fields are at the same place in each entry-sized period of bytes; a
    structure that only repeats a name does not match. A row without a name,
    such as the rows of the last entry, is matched by its place alone.
    """

    def key_of(f: Bits) -> str:
        return identifier(f.acronym).lower() if f.acronym else alias(f.name or "")

    def reserved(f: Bits) -> bool:
        return (f.name or "").lower().startswith("reserved")

    fields = [
        f for f in structure.fields if not (f.union is not None and f.alternative)
    ]
    if any(f.lower is None or f.upper is None for f in fields):
        return None
    named = [f for f in fields if not reserved(f)]
    if len(named) < 2 or named[0].lower != 0:
        return None
    stride = next((f.lower for f in named[1:] if key_of(f) == key_of(named[0])), None)
    if not stride:
        return None

    entries: Dict[int, List[Tuple[str, int, int]]] = {}
    for f in named:
        index = f.lower // stride
        if f.upper // stride != index:
            return None
        at = index * stride
        entries.setdefault(index, []).append((key_of(f), f.lower - at, f.upper - at))
    for entry in entries.values():
        if [place[1:] for place in entry] != [place[1:] for place in entries[0]]:
            return None
        if any(a[0] and b[0] and a[0] != b[0] for a, b in zip(entry, entries[0])):
            return None

    first = [f for f in fields if f.upper < stride]
    if max(f.upper for f in first) + 1 != stride:
        return None

    return (
        structure.model_copy(update={"fields": first}),
        stride,
        [index * stride for index in sorted(entries)],
    )


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
    # The bytes of one element of an array of a run of fields
    stride: int = 0


# The name of the layout of one dword of an entry, such as "Completion Queue
# Entry: DW 3", and of the layout of a field of its dwords, such as "Completion
# Queue Entry: Status Field"
DWORD_OF_ENTRY = re.compile(r"^(?P<entry>.+): DW (?P<dword>\d+)$")
FIELD_OF_ENTRY = re.compile(r"^(?P<entry>.+): (?P<field>.+) Field$")


def entries_of_dwords(
    spec: SpecDocument,
) -> Dict[Tuple[str, str], Tuple[Dict[int, Structure], Dict[str, Structure]]]:
    """The entries for which the data model has layouts of the dwords but no
    byte layout, such as the Completion Queue Entry, by document and name. For
    each entry: the layout of each dword by its number, and the layouts of
    fields by their name"""

    of_bytes = {
        (document_key(s.source.document), alias(s.name))
        for s in spec.structures
        if s.unit == "bytes"
    }
    entries: Dict[
        Tuple[str, str], Tuple[Dict[int, Structure], Dict[str, Structure]]
    ] = {}
    for structure in spec.structures:
        if structure.unit != "bits":
            continue
        doc = document_key(structure.source.document)
        dword = DWORD_OF_ENTRY.match(structure.name)
        if dword and (doc, alias(dword["entry"])) not in of_bytes:
            dwords, _ = entries.setdefault((doc, dword["entry"]), ({}, {}))
            dwords.setdefault(int(dword["dword"]), structure)
    for structure in spec.structures:
        field = (
            FIELD_OF_ENTRY.match(structure.name) if structure.unit == "bits" else None
        )
        key = (
            (document_key(structure.source.document), field["entry"]) if field else None
        )
        if key in entries:
            entries[key][1].setdefault(alias(field["field"]), structure)
    return entries


def shifted(fields: List[Bits], by: int) -> List[Bits]:
    """'fields' with their bits lowered by 'by', so that they are relative to a half
    of their dword"""

    return [
        f.model_copy(update={"lower": f.lower - by, "upper": f.upper - by})
        for f in fields
        if f.lower is not None and f.upper is not None and f.lower >= by
    ]


def layout_of_dwords(
    dwords: Dict[int, Structure], fields: Dict[str, Structure]
) -> List[Tuple[Member, List[Tuple[List[Bits], Source]]]]:
    """The members of an entry, from the layouts of its 'dwords', each with
    the figures of its masks. A half of a dword that one field fills, or that
    its widest field names, is a 16-bit member, such as the SQ Head Pointer. A
    dword with a field across its halves, or without a layout in the data
    model, is a 32-bit member. When 'fields' has a layout for a field by its
    name, the masks of that layout come before the masks of the dword, as the
    masks of the Status Field come before those of the Status and Phase Tag"""

    members: List[Tuple[Member, List[Tuple[List[Bits], Source]]]] = []
    for number in range(max(dwords) + 1):
        lower = 4 * number
        dword = dwords.get(number)
        if dword is None:
            what = f"Dword {number}, of no layout in the data model"
            members.append(
                (Member(f"dw{number}", "uint32_t", None, lower, what, size=4), [])
            )
            continue
        laid = [f for f in dword.fields if f.lower is not None and f.upper is not None]
        if any(f.lower < 16 <= f.upper for f in laid):
            members.append(
                (
                    Member(
                        f"dw{number}",
                        "uint32_t",
                        None,
                        lower,
                        f"Dword {number}",
                        size=4,
                    ),
                    [(laid, dword.source)],
                )
            )
            continue
        for half in (0, 16):
            these = [f for f in laid if half <= f.lower and f.upper < half + 16]
            named = [f for f in these if f.acronym]
            if not named:
                members.append(
                    (
                        Member(
                            f"rsvd{lower + half // 8}",
                            "uint16_t",
                            None,
                            lower + half // 8,
                            "Reserved",
                            size=2,
                        ),
                        [],
                    )
                )
                continue
            widest = max(named, key=lambda f: f.upper - f.lower)
            fills = len(named) == 1 and widest.upper - widest.lower == 15
            doc = " and ".join(
                f.name or f.acronym for f in sorted(named, key=lambda f: -f.upper)
            )
            figures = []
            described_by = None if fills else fields.get(alias(widest.name or ""))
            if described_by:
                figures.append(
                    (shifted(described_by.fields, half), described_by.source)
                )
            if not fills:
                figures.append((shifted(these, half), dword.source))
            members.append(
                (
                    Member(
                        identifier(widest.acronym).lower(),
                        "uint16_t",
                        None,
                        lower + half // 8,
                        doc,
                        size=2,
                    ),
                    figures,
                )
            )
    return members


def index_of(field: Bits) -> Optional[int]:
    """The index in the name of 'field', such as 65535 for "Stream Identifier
    65,535" """

    match = INDEXED.match(field.name or "")
    return int(match["index"].replace(",", "")) if match else None


def base_of(field: Bits) -> Optional[str]:
    """The name of 'field' without its index, if it is named by one"""

    match = INDEXED.match(field.name or "")
    return f"{match['head']}{match['tail'] or ''}" if match else None


def layout_of(structure: Structure, element_of) -> Tuple[List[Member], List[str]]:
    """
    The members of the struct of 'structure', in the order of their offsets,
    and the fields that are left out of the struct, each with the reason

    The bytes that no field covers are reserved members. A run of fields of one
    size that are named by their index, such as "Zone Descriptor 0" and "Zone
    Descriptor 1", is an array. 'element_of' gives the element type for the
    name and the size of the fields: the struct of the structure of that name,
    or else bytes. A field at an offset that depends on the data, such as
    "(NZ+1)*64", ends the struct with a flexible array; when the field
    continues a run, the flexible array is of that run. Of a union, the first
    alternative is the member, and the doc of the member names the others.
    """

    fields = [
        f for f in structure.fields if not (f.union is not None and f.alternative)
    ]
    others: Dict[int, List[str]] = {}
    for f in structure.fields:
        if f.union is not None and f.alternative and f.name and f.acronym:
            others.setdefault(f.union, []).append(f"{f.name} ({f.acronym})")
    members: List[Member] = []
    left: List[str] = []
    names: Dict[str, int] = {}
    offset = 0

    def named(text: str, lower: int) -> str:
        name = text if text not in names else f"{text}_{lower}"
        names[name] = lower
        return name

    def array(base: str, size: Optional[int]) -> Tuple[str, Optional[int]]:
        """The element type of an array of 'base', and the number of bytes per
        element: the struct of the structure named 'base', or else bytes"""

        element = element_of(base, size)
        if element:
            return f"struct {element}", 1
        if size in TYPES:
            return TYPES[size], 1
        return "uint8_t", size

    index = 0
    while index < len(fields):
        f = fields[index]
        # A run is numbered from 0, or from 1 as the Result Data Structures are
        first = index_of(f)
        base = base_of(f) if first in (0, 1) else None
        if f.lower is None or f.upper is None:
            ctype, _ = array(base or "", None) if base else ("uint8_t", None)
            what = base or f.name or "data"
            members.append(Member(named(alias(what), offset), ctype, 0, offset, what))
            left += [
                f"{g.name or g.range}: at an offset the data gives, in '{alias(what)}'"
                for g in fields[index + 1 :]
                if g.name
            ]
            break
        if f.lower < offset:
            left.append(f"{f.name or f.range}: of bytes a field before it has")
            index += 1
            continue
        if f.lower > offset:
            gap = f.lower - offset
            members.append(
                Member(
                    named(f"rsvd{offset}", offset),
                    "uint8_t",
                    None if gap == 1 else gap,
                    offset,
                    "Reserved",
                    size=gap,
                )
            )
            offset = f.lower
        size = f.upper - f.lower + 1
        if base and first is not None:
            # The documents leave out the rows between the first entries and
            # the last entry, so the next entry of a run can be at the offset
            # of its index rather than directly after the previous entry
            length = 1
            following = index + 1
            while following < len(fields):
                g = fields[following]
                at = index_of(g)
                if (
                    at is None
                    or base_of(g) != base
                    or at - first < length
                    or g.lower != f.lower + (at - first) * size
                    or g.upper != g.lower + size - 1
                ):
                    break
                length = at - first + 1
                following += 1
            # The run is named by the acronym of its first field without the
            # index, such as PSD for PSD0, or else by the name without the index
            stripped = re.sub(rf"{first}$", "", f.acronym or "")
            run_name = identifier(stripped).lower() if stripped else alias(base)
            rest = fields[following] if following < len(fields) else None
            if rest is not None and rest.lower is None:
                # Entries that alternate with other entries, such as a Zone
                # Descriptor with its Extension, are not an array of one struct
                bases = list(dict.fromkeys(filter(None, map(base_of, fields[index:]))))
                ctype, _ = array(base, size) if bases == [base] else ("uint8_t", 1)
                what = " and ".join(bases)
                members.append(Member(named(run_name, offset), ctype, 0, offset, what))
                left += [
                    f"{g.name or g.range}: at an offset the data gives, in '{run_name}'"
                    for g in fields[following + 1 :]
                    if g.name
                ]
                break
            if length > 1:
                ctype, per = array(base, size)
                members.append(
                    Member(
                        named(run_name, offset),
                        ctype,
                        length * (per or 1),
                        offset,
                        f"{base}, {length} of {size} bytes",
                        size=length * size,
                        description=f.description,
                        stride=size,
                    )
                )
                offset += length * size
                index = following
                continue
        reserved = (f.name or "").lower().startswith("reserved") and not f.acronym
        if reserved:
            name = f"rsvd{f.lower}"
        elif f.acronym:
            name = identifier(f.acronym).lower()
        else:
            name = alias(f.name or f"bytes_{f.lower}")
        doc = f.name or "Reserved"
        if f.union is not None and others.get(f.union):
            doc += "; or, in the same bytes, " + ", or ".join(others[f.union])
        # A string is an array of bytes whatever its size, such as the Firmware
        # Revision
        scalar = size in TYPES and "ASCII" not in f.description
        ctype = TYPES[size] if scalar else "uint8_t"
        members.append(
            Member(
                named(name, f.lower),
                ctype,
                None if scalar else size,
                f.lower,
                doc,
                # Only bit fields get masks; byte ranges within a bigger
                # field do not
                [g for g in f.fields if g.unit == "bits"] if scalar else [],
                size,
                f.description,
                f if scalar else None,
            )
        )
        offset = f.upper + 1
        index += 1
    return members, left


@dataclass
class Group:
    """Bytes of an entry given by a bit layout: one field that is a whole
    member, or the fields that share the smallest aligned unit holding them"""

    lower: int
    size: int
    fields: List[Bits]


def unit_of(lower: int, upper: int) -> Optional[Tuple[int, int]]:
    """The smallest naturally aligned unit of 1, 2, 4 or 8 bytes holding the
    bits 'lower' to 'upper', as its first byte and its size"""

    for size in (1, 2, 4, 8):
        if lower // (8 * size) == upper // (8 * size):
            return lower // (8 * size) * size, size
    return None


def groups_of_bits(fields: List[Bits]) -> Optional[List[Group]]:
    """The members of an entry of the bit layout 'fields', in the order of
    their bytes, or None when a field fits no unit of up to 8 bytes. A field
    without an acronym is reserved and makes no member"""

    groups: List[Group] = []
    for field in sorted(fields, key=lambda f: f.lower or 0):
        if not field.acronym or field.lower is None or field.upper is None:
            continue
        unit = unit_of(field.lower, field.upper)
        if unit is None:
            return None
        groups.append(Group(unit[0], unit[1], [field]))
    merged = True
    while merged:
        merged = False
        for a, b in zip(groups, groups[1:]):
            if b.lower < a.lower + a.size:
                unit = unit_of(
                    8 * a.lower, 8 * max(a.lower + a.size, b.lower + b.size) - 1
                )
                if unit is None:
                    return None
                a.lower, a.size, a.fields = unit[0], unit[1], a.fields + b.fields
                groups.remove(b)
                merged = True
                break
    return groups


def number_of(entry: Entry) -> Optional[int]:
    """The value of 'entry' as a number, if the value is one number and not a
    range"""

    typed = entry.typed.get(entry.value_column)
    if typed is not None and typed.lower is not None:
        return typed.lower if typed.lower == typed.upper else None
    match = re.match(r"^\s*([0-9A-Fa-f]+)h\s*$", entry.value)
    return int(match[1], 16) if match else None


def value_name(entry: Entry) -> Optional[str]:
    """The name of the value 'entry' without what it adds in parentheses, as
    "Domain scope" of "Domain scope (i.e., ...)", unless the data model has
    only the sentence describing it, which makes no name for a constant"""

    named = entry.name or next(iter(value_names(entry)), None)
    while named and re.search(r"\([^()]*\)", named):
        named = re.sub(r"\s*\([^()]*\)", "", named).strip()
    # The documents leave a parenthesis open now and then
    if named and "(" in named:
        named = named[: named.index("(")].strip()
    if named and len(named.split()) > 8:
        return None
    return named


def operation_enums(command: Command, masks: List[Tuple[str, int, str]]) -> List[str]:
    """The enums of the operations of 'command' that two fields select, such
    as DOPER within a DTYPE of Directive Send: one enum of the inner field for
    each value of the outer one, the outer being the field that the data model
    names first. An operation that one field selects is in the values of that
    field"""

    by_acronym = {name.rsplit("_", 1)[-1]: name for name, _, _ in masks}
    groups: Dict[Tuple[str, str, int], List] = {}
    for operation in getattr(command, "operations", None) or []:
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


def enum_lines(name: str, doc: str, constants: List[Tuple[str, int, str]]) -> List[str]:
    """The lines of the enum 'name' with 'constants', each a name, a value and
    a doc. A name that occurs again is left out when its value is the same, and
    otherwise gets its value as a suffix"""

    body: List[str] = []
    seen: Dict[str, int] = {}
    for constant, value, what in constants:
        if seen.get(constant) == value:
            continue
        if constant in seen:
            constant = f"{constant}_{value:X}H"
            if constant in seen:
                continue
        seen[constant] = value
        body.append(f"\t{constant} = 0x{value:x}, ///< {what}")
    if not body:
        return []
    return ["/**", f" * {doc}", " */", f"enum {name} {{", *body, "};", ""]


def value_enums(
    prefix: str,
    figures: List[Tuple[List[Bits], Source]],
    masks: List[Tuple[str, int, str]],
) -> List[str]:
    """The enums of the values that the specification names for the fields of
    'figures' that have a mask among 'masks', or that are the whole member
    'prefix' names. The enum of a field is named by its mask, or by the member,
    and each constant by that and the name of the value. A value that is a
    range, or is named reserved, is left out, as for the registers"""

    named_masks = {name for name, _, _ in masks}
    if not masks:
        named_masks.add(prefix)
    lines: List[str] = []
    for fields, source in figures:
        for field in fields:
            if not getattr(field, "acronym", None) or not getattr(
                field, "values", None
            ):
                continue
            mask = f"{prefix}_{identifier(field.acronym)}"
            if mask not in named_masks:
                if prefix not in named_masks or not prefix.endswith(
                    f"_{identifier(field.acronym)}"
                ):
                    continue
                mask = prefix
            constants = []
            for entry in field.values:
                value = number_of(entry)
                named = value_name(entry)
                if value is None or not named or alias(named) == "reserved":
                    continue
                constants.append(
                    (
                        f"{mask}_{identifier(named)}",
                        value,
                        f"{named} ({senfd.query.cite(source)})",
                    )
                )
            lines += enum_lines(
                mask.lower(),
                f"The values of {field.name or field.acronym} ({senfd.query.cite(source)})",
                constants,
            )
            named_masks.discard(mask)
    return lines


class SpecHeaders:
    """Writes the spec headers from the data model, and the sources of the
    functions that the headers declare"""

    NAME = "xnvme_spec"

    def __init__(self, output: Path, options: Dict[str, str]):
        self.output = Path(output) / self.NAME
        self.output.mkdir(parents=True, exist_ok=True)
        self.options = dict(options)

    def commands(self, spec: SpecDocument) -> List[Tuple[str, Command]]:
        """The commands that the option 'commands' asks for, each with the alias
        of its command set; an empty list for the value "none" """

        wanted = [w.strip() for w in self.options.get("commands", "").split(",") if w]
        if wanted == ["none"]:
            return []
        chosen = []
        for set_key, command_set in spec.command_sets.items():
            for cmd_key, command in command_set.commands.items():
                if not wanted or any(
                    w in (set_key, f"{set_key}:{cmd_key}") for w in wanted
                ):
                    chosen.append((set_key_of(set_key), command))
        return chosen

    def structures(self, spec: SpecDocument) -> List[Structure]:
        """The structures to write. By default these are the byte layouts whose
        kind is not in SKIPPED. The option 'structures' names structures
        instead, by alias or by document and alias, such as
        "base:identify_controller", or is "none" """

        wanted = [w.strip() for w in self.options.get("structures", "").split(",") if w]
        if wanted == ["none"]:
            return []
        chosen = []
        for structure in spec.structures:
            if structure.unit != "bytes":
                continue
            key = alias(structure_name(structure))
            doc = document_key(structure.source.document)
            if (
                not wanted
                and structure.kind not in SKIPPED
                and f"{doc}:{key}" not in CITATIONS
                or key in wanted
                or f"{doc}:{key}" in wanted
            ):
                chosen.append(structure)
        return chosen

    def write_structures(
        self, spec: SpecDocument, stem: str, taken: set, out: Dict[str, List[str]]
    ) -> None:
        """Writes the structs of the structures to write, and of the structures
        that are elements of their arrays, into 'out' by document. The struct of
        an element comes before the structs that use it"""

        by_name: Dict[Tuple[str, str], List[Structure]] = {}
        bit_layouts = layouts_of_bits(spec)
        for structure in spec.structures:
            if structure.unit == "bytes":
                key = (
                    document_key(structure.source.document),
                    alias(structure_name(structure)),
                )
                by_name.setdefault(key, []).append(structure)
        flexible = self.options.get("flexible", "0") not in ("0", "", "no", "false")
        written: Dict[int, str] = {}
        layouts: Dict[Tuple[str, ...], str] = {}
        # The names of the structs, whatever the order they are written in
        planned = {
            f"{stem}_{document_key(s.source.document)}_"
            f"{'log_' if s.kind == 'log_page' else ''}{alias(structure_name(s))}"
            for s in spec.structures
            if s.unit == "bytes"
        }

        def write_bits(structure: Structure, nbytes: int) -> Optional[str]:
            """Writes the struct of an entry that the bit layout 'structure'
            defines, such as the Power State Descriptor, of 'nbytes' bytes"""

            if id(structure) in written:
                return written[id(structure)]
            doc = document_key(structure.source.document)
            groups = groups_of_bits(structure.fields)
            name = f"{stem}_{doc}_{alias(structure_name(structure))}"
            if not groups or name in taken:
                written[id(structure)] = ""
                return None
            taken.add(name)
            written[id(structure)] = name
            upper = identifier(name)
            cite = senfd.query.cite(structure.source)
            body: List[str] = []
            masks: List[Tuple[str, int, str]] = []
            values: List[str] = []
            offset = 0
            for group in groups:
                if group.lower > offset:
                    gap = group.lower - offset
                    count = "" if gap == 1 else f"[{gap}]"
                    body.append(f"\tuint8_t rsvd{offset}{count}; ///< Reserved")
                (field, *more) = group.fields
                whole = not more and (field.lower, field.upper) == (
                    8 * group.lower,
                    8 * (group.lower + group.size) - 1,
                )
                member = "_".join(identifier(f.acronym).lower() for f in group.fields)
                prefix = f"{upper}_{identifier(member)}"
                # A field alone in its unit from bit 0 is the member, the bits
                # above it reserved. A part named VAL would take the name that
                # yace prints the value of a field by, so it gets no mask
                bare = not more and (
                    field.lower == 8 * group.lower or identifier(field.acronym) == "VAL"
                )
                these = (
                    []
                    if whole or bare
                    else masks_of(
                        member,
                        prefix,
                        [(shifted(group.fields, 8 * group.lower), structure.source)],
                        8 * group.size,
                    )
                )
                where = (
                    f"byte {group.lower}"
                    if group.size == 1
                    else f"bytes {group.lower + group.size - 1}:{group.lower}"
                )
                if bare and not whole:
                    hi, lo = (
                        field.upper - 8 * group.lower,
                        field.lower - 8 * group.lower,
                    )
                    bits = f"bit {lo}" if hi == lo else f"bits {hi}:{lo}"
                    where = f"{bits} of {where}"
                what = (
                    field.name
                    if whole
                    else "; ".join(f.name or f.acronym for f in group.fields)
                )
                see = f", see {prefix}_*" if these else ""
                body.append(
                    f"\t{TYPES[group.size]} {member}; ///< {what}, {where}{see}"
                )
                masks += these
                fields = (
                    [([field], structure.source)]
                    if whole
                    else [(shifted(group.fields, 8 * group.lower), structure.source)]
                )
                values += value_enums(
                    prefix, [([field], structure.source)] if bare else fields, these
                )
                offset = group.lower + group.size
            if offset < nbytes:
                body.append(f"\tuint8_t rsvd{offset}[{nbytes - offset}]; ///< Reserved")
            lines = out.setdefault(doc, [])
            lines += [
                "/**",
                f" * {structure.name} ({cite}), one entry of {nbytes} bytes",
                " */",
                f"struct __attribute__((packed)) {name} {{",
                *body,
                "};",
                "",
            ]
            for mask, value, text in masks:
                lines += [f"/** {text} */", f"#define {mask} 0x{value:x}"]
            if masks:
                lines.append("")
            lines += values
            return name

        def write(structure: Structure) -> Optional[str]:
            if id(structure) in written:
                return written[id(structure)]
            doc = document_key(structure.source.document)

            def element_of(base: str, size: Optional[int]) -> Optional[str]:
                for key in (
                    alias(base),
                    alias(re.sub(r"\s+Data Structure$", "", base)),
                ):
                    for candidate in by_name.get((doc, key), []):
                        if (
                            candidate is not structure
                            and (size is None or size_of(candidate) == size)
                            and size_of(candidate)
                        ):
                            return write(candidate)
                return None

            entry = entry_of(structure)
            members, left = layout_of(entry[0] if entry else structure, element_of)
            if not [m for m in members if m.count != 0]:
                written[id(structure)] = ""
                return None
            shape = tuple(f"{m.name}:{m.ctype}:{m.count}:{m.lower}" for m in members)
            kind = "log_" if structure.kind == "log_page" else ""
            name = f"{stem}_{doc}_{kind}{alias(structure_name(structure))}"
            if shape in layouts and layouts[shape].startswith(name):
                written[id(structure)] = layouts[shape]
                return layouts[shape]
            if name in taken:
                name = f"{name}_figure{structure.source.figure_nr}"
            if name in taken:
                written[id(structure)] = ""
                return None
            taken.add(name)
            layouts[shape] = name
            written[id(structure)] = name
            upper = identifier(name)
            body: List[str] = []
            masks: List[Tuple[str, int, str]] = []
            values: List[str] = []
            follows: List[str] = []
            for m in members:
                if m.count == 0 and not flexible:
                    of = f", each a '{m.ctype}'" if m.ctype.startswith("struct") else ""
                    follows += [
                        f" * Followed, from byte {m.lower}, by as many {m.doc} as the"
                        f" data says{of}",
                    ]
                    continue
                where = (
                    f"byte {m.lower}"
                    if m.size == 1
                    else (
                        f"bytes {m.lower + m.size - 1}:{m.lower}"
                        if m.size
                        else f"from byte {m.lower}"
                    )
                )
                figures = [(m.fields, structure.source)]
                referred = figure_referred(m.description)
                described_by = bit_layouts.get((doc, referred)) if referred else None
                if not m.fields and m.count is None and described_by:
                    figures = [(described_by.fields, described_by.source)]
                if m.stride and described_by and width_of(described_by) == 8 * m.stride:
                    if m.stride in TYPES and m.ctype == TYPES[m.stride]:
                        # An element that is a scalar keeps its type, and the
                        # masks of the entry are named by the entry, as the LBA
                        # Format of LBAF of Identify Namespace
                        named = f"{stem}_{doc}_{alias(structure_name(described_by))}"
                        if named in taken or named in planned:
                            named = f"{named}_figure{described_by.source.figure_nr}"
                        entry_masks = masks_of(
                            structure_name(described_by),
                            identifier(named),
                            [(described_by.fields, described_by.source)],
                            8 * m.stride,
                        )
                        masks += entry_masks
                        prefix = identifier(named)
                        if entry_masks:
                            m = replace(m, doc=f"{m.doc} of {prefix}_*")
                        values += value_enums(
                            prefix,
                            [(described_by.fields, described_by.source)],
                            entry_masks,
                        )
                        figures = []
                    else:
                        element = write_bits(described_by, m.stride)
                        if element:
                            m = replace(
                                m, ctype=f"struct {element}", count=m.size // m.stride
                            )
                these = masks_of(
                    m.name,
                    f"{upper}_{identifier(m.name)}",
                    figures,
                    8 * m.size,
                )
                see = f", see {upper}_{identifier(m.name)}_*" if these else ""
                count = "" if m.count is None else f"[{m.count or ''}]"
                body.append(f"\t{m.ctype} {m.name}{count}; ///< {m.doc}, {where}{see}")
                masks += these
                whole = [([m.whole], structure.source)] if m.whole and not these else []
                values += value_enums(
                    f"{upper}_{identifier(m.name)}", figures + whole, these
                )
            lines = out.setdefault(doc, [])
            doc_lines = [
                "/**",
                f" * {structure.name} ({senfd.query.cite(structure.source)})",
            ]
            if entry:
                _, stride, offsets = entry
                at = ", ".join(map(str, offsets))
                doc_lines += [
                    " *",
                    f" * One entry of {stride} bytes; the figure shows them at bytes {at}",
                ]
            if follows:
                doc_lines += [" *", *follows]
            if left:
                doc_lines += [" *", " * Not members:"]
                doc_lines += [f" * {why}" for why in left]
            lines.extend(
                [
                    *doc_lines,
                    " */",
                    f"struct __attribute__((packed)) {name} {{",
                    *body,
                    "};",
                    "",
                ]
            )
            for mask, value, doc_text in masks:
                lines.extend([f"/** {doc_text} */", f"#define {mask} 0x{value:x}"])
            if masks:
                lines.append("")
            lines.extend(values)
            return name

        for structure in self.structures(spec):
            write(structure)

    def write_entries(
        self, spec: SpecDocument, stem: str, taken: set, out: Dict[str, List[str]]
    ) -> None:
        """Writes the structs of the entries for which the data model has only
        layouts of the dwords, such as the Completion Queue Entry, into 'out' by
        document, unless the option 'structures' leaves them out"""

        wanted = [w.strip() for w in self.options.get("structures", "").split(",") if w]
        for (doc, entry), (dwords, fields) in entries_of_dwords(spec).items():
            key = alias(entry)
            if wanted and key not in wanted and f"{doc}:{key}" not in wanted:
                continue
            name = f"{stem}_{doc}_{key}"
            if name in taken:
                continue
            taken.add(name)
            upper = identifier(name)
            cited = "; ".join(
                senfd.query.cite(dwords[n].source) for n in sorted(dwords)
            )
            body: List[str] = []
            masks: List[Tuple[str, int, str]] = []
            values: List[str] = []
            for m, figures in layout_of_dwords(dwords, fields):
                these = masks_of(
                    m.name, f"{upper}_{identifier(m.name)}", figures, 8 * m.size
                )
                where = f"bytes {m.lower + m.size - 1}:{m.lower}"
                see = f", see {upper}_{identifier(m.name)}_*" if these else ""
                body.append(f"\t{m.ctype} {m.name}; ///< {m.doc}, {where}{see}")
                masks += these
                values += value_enums(f"{upper}_{identifier(m.name)}", figures, these)
            lines = out.setdefault(doc, [])
            lines += [
                "/**",
                f" * {entry}, of the layouts of its dwords ({cited})",
                " */",
                f"struct __attribute__((packed)) {name} {{",
                *body,
                "};",
                "",
            ]
            for mask, value, text in masks:
                lines += [f"/** {text} */", f"#define {mask} 0x{value:x}"]
            if masks:
                lines.append("")
            lines += values

    def write_enums(
        self, spec: SpecDocument, stem: str, out: Dict[str, List[str]]
    ) -> None:
        """Writes the enums into 'out' by document: the opcodes of each command
        set, the identifiers of the features and of the log pages, and the
        status codes of each document, one enum for each type of status code"""

        upper = identifier(stem)
        opcodes: Dict[Tuple[str, str], List[Tuple[str, int, str]]] = {}
        names = {set_key_of(k): s.name for k, s in spec.command_sets.items()}
        for set_key, command in self.commands(spec):
            name = identifier(f"{stem}_{set_key}_{alias(command.name)}")
            doc = document_key(command.source.document)
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
                    f"The opcodes of the {names[set_key]} commands",
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
                doc = document_key(item.source.document)
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
        for values in spec.values:
            if (
                values.kind != "status_values"
                or (values.name or "Status Code") != "Status Code"
                or not values.qualifier
            ):
                continue
            doc = document_key(values.source.document)
            # The table of the status code types has no type itself. Two
            # tables of one type with different scopes, such as the I/O commands
            # and the Fabrics commands, give one value different meanings, so
            # each scope gets its own enum. A scope that names a command set
            # means the command set of the document.
            scope = values.scope
            if scope and re.search(r"Command Set( Specific)?$", scope):
                scope = None
            what = alias(values.qualifier)
            if what != "status_code_type":
                what += f"_{alias(scope)}_status" if scope else "_status"
            key = (doc, what)
            name = identifier(f"{stem}_{doc}_{what}")
            described[key] = (
                "The types of status"
                if what == "status_code_type"
                else f"The status codes of the type {values.qualifier}"
                + (f", of {scope}" if scope else "")
            )
            for entry in values.entries:
                value = number_of(entry)
                if value is None or not entry.name or alias(entry.name) == "reserved":
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

    def write_registers(
        self, spec: SpecDocument, stem: str, out: Dict[str, List[str]]
    ) -> None:
        """
        Writes the registers of each document into 'out' by document: one
        packed struct with each register at its offset, as a driver maps the
        registers, the masks of the fields of each register, and an enum of the
        values of each field whose values the specification names

        A register whose offset is not a hexadecimal number, or whose bytes
        another register also covers, is not a member of the struct; the doc of
        the struct names it instead.
        """

        if self.options.get("registers", "") == "none":
            return
        by_doc: Dict[str, List[Tuple[int, int, str, Structure]]] = {}
        unplaced: Dict[str, List[str]] = {}
        for structure in spec.structures:
            if structure.kind != "register":
                continue
            doc = document_key(structure.source.document)
            head = re.split(r"\s+-\s+", structure.name, maxsplit=1)[0]
            name = identifier(re.sub(r"\s*\([^)]*\)", "", head)).lower()
            ups = [f.upper for f in structure.fields if f.upper is not None]
            match = re.fullmatch(r"([0-9A-Fa-f]+)h", (structure.offset or "").strip())
            if not match or not ups or not name:
                unplaced.setdefault(doc, []).append(
                    f"{structure.name}: at no offset of a number"
                )
                continue
            size = (max(ups) + 8) // 8
            by_doc.setdefault(doc, []).append(
                (int(match[1], 16), size, name, structure)
            )
        for doc, registers in by_doc.items():
            struct = f"{stem}_{doc}_registers"
            upper = identifier(struct)
            body: List[str] = []
            masks: List[Tuple[str, int, str]] = []
            enums: List[str] = []
            left = list(unplaced.get(doc, []))
            offset = 0
            names: set = set()
            for at, size, name, register in sorted(registers, key=lambda r: r[0]):
                if at < offset or name in names:
                    left.append(f"{register.name}: of bytes another register has")
                    continue
                if at > offset:
                    gap = at - offset
                    count = "" if gap == 1 else f"[{gap}]"
                    where = (
                        f"{offset:X}h" if gap == 1 else f"{offset:X}h to {at - 1:X}h"
                    )
                    body.append(
                        f"\tuint8_t rsvd{offset:x}{count}; ///< Reserved, {where}"
                    )
                names.add(name)
                scalar = size in TYPES
                cite = senfd.query.cite(register.source)
                these = (
                    masks_of(
                        name,
                        f"{upper}_{identifier(name)}",
                        [(register.fields, register.source)],
                        8 * size,
                    )
                    if scalar
                    else []
                )
                see = f", see {upper}_{identifier(name)}_*" if these else ""
                ctype = TYPES[size] if scalar else "uint8_t"
                count = "" if scalar else f"[{size}]"
                body.append(
                    f"\t{ctype} {name}{count}; ///< {register.name} ({cite}), "
                    f"at {at:X}h{see}"
                )
                masks += these
                for field in register.fields:
                    constants = []
                    for entry in field.values:
                        value = number_of(entry)
                        named = value_name(entry)
                        if value is None or not named or alias(named) == "reserved":
                            continue
                        constants.append(
                            (
                                f"{upper}_{identifier(name)}_"
                                f"{identifier(field.acronym or '')}_{identifier(named)}",
                                value,
                                f"{named} ({cite})",
                            )
                        )
                    if field.acronym and constants:
                        enums += enum_lines(
                            f"{struct}_{name}_{identifier(field.acronym).lower()}",
                            f"The values of {field.name or field.acronym} of"
                            f" {register.name}",
                            constants,
                        )
                offset = at + size
            lines = out.setdefault(doc, [])
            lines += [
                "/**",
                f" * The registers of the {doc} specification, at their" " offsets",
            ]
            if left:
                lines += [" *", " * Not members:", *[f" * {why}" for why in left]]
            lines += [
                " */",
                f"struct __attribute__((packed)) {struct} {{",
                *body,
                "};",
                "",
            ]
            for mask, value, text in masks:
                lines += [f"/** {text} */", f"#define {mask} 0x{value:x}"]
            lines += ["", *enums]

    def generate(self):
        path = self.options.get("spec")
        spec = senfd.query.load(Path(path) if path else None)
        stem = alias(self.options.get("prefix", "xnvme_spec"))

        enums: Dict[str, List[str]] = {}
        structs: Dict[str, List[str]] = {}
        commands: Dict[str, List[str]] = {}
        taken = {
            f"{stem}_{set_key}_{alias(command.name)}"
            for set_key, command in self.commands(spec)
        }
        self.write_enums(spec, stem, enums)
        self.write_structures(spec, stem, taken, structs)
        if self.options.get("structures", "").strip() != "none":
            self.write_entries(spec, stem, taken, structs)
        registers: Dict[str, List[str]] = {}
        self.write_registers(spec, stem, registers)

        for set_key, command in self.commands(spec):
            doc = document_key(command.source.document)
            struct = f"{stem}_{set_key}_{alias(command.name)}"
            upper = identifier(struct)
            notes: List[str] = []
            masks: List[Tuple[str, int, str]] = []
            values: List[str] = []
            for member, lower, last, ctype in members_of(command):
                width = 32 * (last - lower + 1)
                figures, others = described(command, lower, last)
                if (lower, last + 2) in POINTERS or (lower - 2, last) in POINTERS:
                    figures, others = [], []
                what = (
                    f"Command Dword {lower}"
                    if lower == last
                    else f"Command Dwords {lower} and {last}, as one 64-bit value"
                )
                these = masks_of(
                    what,
                    f"{upper}_{identifier(member)}",
                    [(f.fields, f.source) for f in figures],
                    width,
                )
                masks += these
                values += value_enums(
                    f"{upper}_{identifier(member)}",
                    [(f.fields, f.source) for f in figures],
                    these,
                )
                if others:
                    notes.append(
                        f" * {what} is described otherwise if "
                        + ", or if ".join(others)
                    )
            lines = commands.setdefault(doc, [])
            lines += [
                "/*",
                f" * The fields of {command.name}, by the dwords of the command",
            ]
            if notes:
                lines += [" *", *notes]
            lines += [" */"]
            for name, value, text in masks:
                lines += [f"/** {text} */", f"#define {name} 0x{value:x}"]
            lines.append("")
            lines += values
            lines += operation_enums(command, masks)

        headers: List[Path] = []
        for doc in dict.fromkeys([*enums, *structs, *registers, *commands]):
            name = f"{stem}_{doc}"
            guard = f"LIB{identifier(name)}_H"
            text = [
                "/**",
                f" * What the {doc} specification defines, by {stem}",
                " *",
                f" * Written from the data model of senfd {senfd_version(spec)}, by"
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
                *enums.get(doc, []),
                *structs.get(doc, []),
                *registers.get(doc, []),
                *commands.get(doc, []),
                f"#endif /* {guard} */",
                "",
            ]
            path = self.output / f"lib{name}.h"
            path.write_text("\n".join(text), encoding="utf-8")
            headers.append(path)
        return generated(headers, [])
