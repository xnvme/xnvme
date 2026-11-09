#!/usr/bin/env python3
"""
The spec headers of xNVMe, from senfd's data model of the NVMe specifications

A generator for yace, which runs it and parses what it writes as any header;
see the tutorial on producing the spec headers in the contributor docs:

    yace --generate toolbox/xnvme_spec_headers.py:SpecHeaders \\
         --generator-option xnvme_spec.spec=merged.spec.document.json \\
         --output /tmp/spec --strict --emit capi

It is no part of the build: a contributor runs it, on a machine with the
specification documents, and commits what it writes.

It writes a header for each document of the data model, named by the document's
abbreviation, as xnvme_spec_base.h, xnvme_spec_nvm.h, xnvme_spec_kv.h,
xnvme_spec_zns.h and xnvme_spec_tcp.h, and a source of the functions it
declares. Each stands on its own: nothing in one refers to another.

For each command it writes a struct of its 16 submission queue dwords, an
integer each, or one of 64 bits for the pairs the data model has fields across, and
the masks of its fields, named by the specification's acronyms and documented
with their figure, in the form yace links masks to fields by; its opcode, in
the enum of those of its command set; and a function making a command of it.

The options are 'spec', the merged specification document senfd writes (by
default the one in senfd's cache, as for senfd lint), 'prefix' of every
name, by default "xnvme_spec", and 'commands', a comma separated list of
command sets, as "nvm", or commands of them, as "nvm:write", by default all,
or "none".

The identifiers of the features and of the log pages, and the status codes of
each document by their type, are enums too.

The registers of each document are a packed struct of them at their offsets,
as a driver maps them, with the masks of their fields and an enum of the
values of each field whose values the specification names. The option
'registers', if "none", leaves them out.

It writes the layouts of bytes of the data model as well, but those of examples and
buffers of data: the data structures, the log pages, and those of no named
kind, as the PDUs of the transports. Each is a packed struct, named by its
document and its name, of its fields at their offsets, the bytes no field has
reserved, with the masks of the fields' bits. The option 'structures' names
others, as for 'commands', by their name or that of their document and
theirs, as "base:identify_controller", or is "none"; 'flexible', if "1", makes
the entries following a struct a flexible array, which yace reports, rather
than a line of its doc.

A dword described otherwise for each of conditions, as CDW13 of Write is, is
given the masks of its description of no condition, else those of its first;
the others are named in the doc of the field. A field whose bits a field
before it has, or that has no acronym, gets no mask.
"""

import re
from dataclasses import dataclass
from dataclasses import field as default
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
    alias,
    value_names,
)

# The dwords of the pointers, which the data model has as one field across them
POINTERS = {(6, 9): "Data Pointer (DPTR)"}

# The C type of a field of as many bytes; one of any other size is of bytes
TYPES = {1: "uint8_t", 2: "uint16_t", 4: "uint32_t", 8: "uint64_t"}

# The kinds of structure written unless the option 'structures' says otherwise:
# those of bytes but an example or a buffer of data, so the layouts of no named
# kind too, as the PDUs of the transports
SKIPPED = ("example", "data_buffer", "status_values")

# Figures whose rows cite fields of other structures, which a first column
# before the bytes names, so their offsets are not of one layout
CITATIONS = (
    "nvm:reference_identify_controller_or_namespace_data_structures",
    "zns:size_and_capacity_fields",
)

# A field named by its index among fields alike, as "Power State 0 Descriptor"
INDEXED = re.compile(r"^(?P<head>.+?)\s+(?P<index>\d[\d,]*)(?P<tail>\s+\D.*)?$")


def identifier(text: str) -> str:
    """'text' as the part of a C identifier, in capitals"""

    return re.sub(r"[^A-Za-z0-9]+", "_", text).strip("_").upper()


def senfd_version() -> str:
    """The version of senfd, which wrote the data model"""

    from importlib.metadata import PackageNotFoundError, version

    try:
        return version("senfd")
    except PackageNotFoundError:
        return "unknown"


def generated(headers: List[Path], sources: List[Path]):
    """What a generator wrote, as yace takes it; yace's own type when it is
    installed, so the generator is tested without yace"""

    try:
        from yace.generators import Generated

        return Generated(headers=headers, sources=sources)
    except ImportError:
        return SimpleNamespace(headers=headers, sources=sources)


def members_of(command: Command) -> List[Tuple[str, int, int, str]]:
    """The members of the struct of 'command': the name, the first and last
    dword, and the C type of each, the pairs the data model has fields across as
    one of 64 bits where they are aligned to it"""

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
    """The descriptions of the dwords 'lower' to 'upper' whose masks are given,
    those of no condition else those of the first, and the conditions of the
    descriptions left out"""

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
    """The masks of the fields of 'figures', each the fields of a figure and
    its source, as the name, the value and the doc of each; a field without an
    acronym, of a formula, beyond 'width', of an alternative but the first of a
    union, or of bits a field before it has is left out"""

    masks: List[Tuple[str, int, str]] = []
    taken = 0
    names: Dict[str, int] = {}
    for fields, source in figures:
        for field in fields:
            if not field.acronym or field.kind != "span":
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
            doc = f"{what} ({senfd.query.cite(source)}); {where} of '{member}'"
            masks.append((name, value, doc))
    return masks


# The documents and command sets by the abbreviations the specifications use
ABBREVIATIONS = {
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
    """The document by the alias of its short name, or its abbreviation, as
    "base" or "kv"; the key of the header of what it describes"""

    key = alias(re.sub(r"\s+[\d.]+$", "", senfd.query.document_name(document)))
    return ABBREVIATIONS.get(key, key)


def set_key_of(key: str) -> str:
    """A command set by its abbreviation, as "kv" of "key_value", if it has one"""

    return ABBREVIATIONS.get(key, key)


def structure_name(structure: Structure) -> str:
    """The name of 'structure' without what it is of nor its abbreviation,
    "Identify Controller" of "Identify - Identify Controller", "PDU Common
    Header" of "PDU Common Header (CH)" """

    return re.sub(r"\s*\([^)]*\)$", "", structure.name.rsplit(" - ", 1)[-1])


def size_of(structure: Structure) -> Optional[int]:
    """The bytes of 'structure', if every field of it is at an offset given"""

    fields = [
        f for f in structure.fields if not (f.union is not None and f.alternative)
    ]
    if not fields or any(f.kind != "span" for f in fields):
        return None
    return max(f.upper for f in fields if f.upper is not None) + 1


def entry_of(structure: Structure) -> Optional[Tuple[Structure, int, List[int]]]:
    """
    The first entry of 'structure', if its figure lays out the entries of a
    list, as the Source Range entries of Copy, with the bytes of an entry and
    the offsets of those the figure shows; else None

    The figures show the first entries and the last, in rows naming the fields
    of each again, so it is by the same fields at the same place in each period
    of bytes that they are told from a structure repeating a name. A row of no
    name, as the last entries have, goes by its place alone.
    """

    def key_of(f: Bits) -> str:
        return identifier(f.acronym).lower() if f.acronym else alias(f.name or "")

    fields = [
        f for f in structure.fields if not (f.union is not None and f.alternative)
    ]
    if any(f.kind != "span" for f in fields):
        return None
    named = [f for f in fields if f.role == "field"]
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
    """A member of the struct of a structure: 'count' is None of a scalar, 0 of
    a flexible array, else the length of an array of 'ctype'"""

    name: str
    ctype: str
    count: Optional[int]
    lower: int
    doc: str
    fields: List[Bits] = default(default_factory=list)
    size: int = 0


def index_of(field: Bits) -> Optional[int]:
    """The index 'field' is named by, as 65535 of "Stream Identifier 65,535" """

    match = INDEXED.match(field.name or "")
    return int(match["index"].replace(",", "")) if match else None


def base_of(field: Bits) -> Optional[str]:
    """The name of 'field' without its index, if it is named by one"""

    match = INDEXED.match(field.name or "")
    return f"{match['head']}{match['tail'] or ''}" if match else None


def layout_of(structure: Structure, element_of) -> Tuple[List[Member], List[str]]:
    """
    The members of the struct of 'structure', in the order of their offsets,
    and the fields left out of it, each as why

    The bytes no field has are reserved members. A run of fields named by their
    index, as "Zone Descriptor 0", "Zone Descriptor 1", of one size, is an array
    of what 'element_of' gives for their name and size: a struct of the
    structure of the name, else the bytes. A field at an offset the data gives,
    as "(NZ+1)*64", ends the struct with a flexible array, of the run it
    continues if there is one. Of a union, the first alternative is the member;
    the others are named in its doc.
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
        """The element type of an array of 'base' and the bytes per element of
        it, as the struct of its structure or else bytes"""

        element = element_of(base, size)
        if element:
            return f"struct {element}", 1
        if size in TYPES:
            return TYPES[size], 1
        return "uint8_t", size

    index = 0
    while index < len(fields):
        f = fields[index]
        # A run is numbered from 0 or from 1, as the Result Data Structures
        first = index_of(f)
        base = base_of(f) if first in (0, 1) else None
        if f.kind != "span":
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
            # the last, so an entry may follow at the offset of its index
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
            # Named as the acronym of the first is without its index, as PSD
            # of PSD0, else by the name without it
            stripped = re.sub(rf"{first}$", "", f.acronym or "")
            run_name = identifier(stripped).lower() if stripped else alias(base)
            rest = fields[following] if following < len(fields) else None
            if rest is not None and rest.kind != "span":
                # Entries alternating with others, as a Zone Descriptor with
                # its Extension, are of no one struct
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
                    )
                )
                offset += length * size
                index = following
                continue
        if f.role == "reserved":
            name = f"rsvd{f.lower}"
        elif f.acronym:
            name = identifier(f.acronym).lower()
        else:
            name = alias(f.name or f"bytes_{f.lower}")
        doc = f.name or "Reserved"
        if f.union is not None and others.get(f.union):
            doc += "; or, in the same bytes, " + ", or ".join(others[f.union])
        # A string is of bytes whatever its size, as the Firmware Revision
        scalar = size in TYPES and "ASCII" not in f.description
        ctype = TYPES[size] if scalar else "uint8_t"
        members.append(
            Member(
                named(name, f.lower),
                ctype,
                None if scalar else size,
                f.lower,
                doc,
                # Bytes within it, as of a bigger field, are no masks
                [g for g in f.fields if g.unit == "bits"] if scalar else [],
                size,
            )
        )
        offset = f.upper + 1
        index += 1
    return members, left


def number_of(entry: Entry) -> Optional[int]:
    """The number 'entry' is the value of, if it is one and not a range"""

    typed = entry.typed.get(entry.value_column)
    if typed is not None and typed.lower is not None:
        return typed.lower if typed.lower == typed.upper else None
    match = re.match(r"^\s*([0-9A-Fa-f]+)h\s*$", entry.value)
    return int(match[1], 16) if match else None


def enum_lines(name: str, doc: str, constants: List[Tuple[str, int, str]]) -> List[str]:
    """The enum 'name' of 'constants', each the name, the value and the doc;
    a name given again is left out if of the same value, else suffixed by its
    value"""

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


class SpecHeaders:
    """Writes the spec header of the commands of the data model, and the source of
    the functions making them"""

    NAME = "xnvme_spec"

    def __init__(self, output: Path, options: Dict[str, str]):
        self.output = Path(output) / self.NAME
        self.output.mkdir(parents=True, exist_ok=True)
        self.options = dict(options)

    def commands(self, spec: SpecDocument) -> List[Tuple[str, Command]]:
        """The commands asked for, by the alias of their command set, or none"""

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
        """The structures asked for: of bytes and of no kind of SKIPPED, unless
        'structures' names them, by their alias or that of their document and theirs, as
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
        """The structs of the structures asked for, and of those their arrays
        are of, each before the structs having it, into 'out' by document"""

        by_name: Dict[Tuple[str, str], List[Structure]] = {}
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
                these = masks_of(
                    m.name,
                    f"{upper}_{identifier(m.name)}",
                    [(m.fields, structure.source)],
                    8 * m.size,
                )
                see = f", see {upper}_{identifier(m.name)}_*" if these else ""
                count = "" if m.count is None else f"[{m.count or ''}]"
                body.append(f"\t{m.ctype} {m.name}{count}; ///< {m.doc}, {where}{see}")
                masks += these
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
            return name

        for structure in self.structures(spec):
            write(structure)

    def write_enums(
        self, spec: SpecDocument, stem: str, out: Dict[str, List[str]]
    ) -> None:
        """The enums of the opcodes of each command set, of the identifiers of
        the features and of the log pages, and of the status codes of each
        document by their type, into 'out' by the document of each"""

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
            # A table of the types of status is of no type, and tables of one
            # type for each of scopes, as the I/O and the Fabrics commands, give
            # one value different meanings
            # A scope naming a command set names that of the document
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
        The registers of each document, into 'out' by document: a packed struct
        of them at their offsets, as a driver maps them, the masks of their
        fields, and an enum of the values of each field whose values are named

        A register at an offset of no hexadecimal number, or of bytes another
        has, is named in the doc of the struct rather than a member of it.
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
                        named = entry.name or next(iter(value_names(entry)), None)
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
        sources: Dict[str, List[str]] = {}
        taken = {
            f"{stem}_{set_key}_{alias(command.name)}"
            for set_key, command in self.commands(spec)
        }
        self.write_enums(spec, stem, enums)
        self.write_structures(spec, stem, taken, structs)
        registers: Dict[str, List[str]] = {}
        self.write_registers(spec, stem, registers)

        for set_key, command in self.commands(spec):
            doc = document_key(command.source.document)
            struct = f"{stem}_{set_key}_{alias(command.name)}"
            upper = identifier(struct)
            fields: List[str] = []
            masks: List[Tuple[str, int, str]] = []
            for member, lower, last, ctype in members_of(command):
                width = 32 * (last - lower + 1)
                figures, others = described(command, lower, last)
                if (lower, last + 2) in POINTERS or (lower - 2, last) in POINTERS:
                    figures, others = [], []
                these = masks_of(
                    member,
                    f"{upper}_{identifier(member)}",
                    [(f.fields, f.source) for f in figures],
                    width,
                )
                what = (
                    f"Command Dword {lower}"
                    if lower == last
                    else f"Command Dwords {lower} and {last}"
                )
                if others:
                    what += "; described otherwise if " + ", or if ".join(others)
                see = f", see {upper}_{identifier(member)}_*" if these else ""
                fields.append(f"\t{ctype} {member}; ///< {what}{see}")
                masks += these
            lines = commands.setdefault(doc, [])
            lines += [
                f"/** The submission queue entry of {command.name} */",
                f"struct {struct} {{",
                *fields,
                "};",
                "",
            ]
            for name, value, text in masks:
                lines += [f"/** {text} */", f"#define {name} 0x{value:x}"]
            lines += [
                "",
                "/**",
                f" * Make 'cmd' the command {command.name}: its opcode in Command",
                " * Dword 0, every other bit cleared",
                " *",
                " * @param cmd The command to make",
                " */",
                "void",
                f"{struct}_init(struct {struct} *cmd);",
                "",
            ]
            sources.setdefault(doc, []).extend(
                [
                    "void",
                    f"{struct}_init(struct {struct} *cmd)",
                    "{",
                    "\tmemset(cmd, 0, sizeof(*cmd));",
                    f"\tcmd->cdw0 = {upper}_OPCODE;",
                    "}",
                    "",
                ]
            )

        headers: List[Path] = []
        written: List[Path] = []
        for doc in dict.fromkeys([*enums, *structs, *registers, *commands]):
            name = f"{stem}_{doc}"
            guard = f"{identifier(name)}_H"
            text = [
                "/**",
                f" * What the {doc} specification defines, by {stem}",
                " *",
                f" * Written from the data model of senfd {senfd_version()}, by"
                " toolbox/xnvme_spec_headers.py;",
                " * each name and mask is as the figure it cites gives it. Do not",
                " * edit: see the contributor docs on producing the spec headers.",
                " *",
                f" * @file {name}.h",
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
            path = self.output / f"{name}.h"
            path.write_text("\n".join(text), encoding="utf-8")
            headers.append(path)
            if doc in sources:
                path = self.output / f"{name}.c"
                source = [f'#include "{name}.h"', "", "#include <string.h>", ""]
                path.write_text("\n".join(source + sources[doc]), encoding="utf-8")
                written.append(path)
        return generated(headers, written)
